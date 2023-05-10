
#include <Python.h>
#include <iostream>
#include <stdexcept>
#include <duckdb.hpp>
#include <duckdb/parser/expression/constant_expression.hpp>
#include <duckdb/parser/expression/function_expression.hpp>
#include <pytable.hpp>
#include "python_function.hpp"
#include <pyconvert.hpp>

#include <typeinfo>

using namespace duckdb;
namespace pyudf {

struct PyScanBindData : public TableFunctionData {
	// Function arguments coerced to a tuple used in Python calling semantics,
	PyObject *arguments;

	std::vector<LogicalType> return_types;

	// Return value of the function specified
	PyObject *function_result_iterable;
};

struct PyScanLocalState : public LocalTableFunctionState {
	bool done = false;
};

struct PyScanGlobalState : public GlobalTableFunctionState {
	PyScanGlobalState() : GlobalTableFunctionState() {
	}
};

std::pair<std::string, std::string> parse_func_specifier(std::string specifier) {
	auto delim_location = specifier.find(":");
	if (delim_location == std::string::npos) {
		throw InvalidInputException("Function specifier lacks a ':' to delineate module and function");
	} else {
		auto module = specifier.substr(0, delim_location);
		auto function = specifier.substr(delim_location + 1, (specifier.length() - delim_location));
		return {module, function};
	}
}

void FinalizePyTable(PyScanBindData &bind_data) {
	// Free the iterable returned by our python function call
	Py_DECREF(bind_data.function_result_iterable);
	bind_data.function_result_iterable = nullptr;

	// Free each entry of the arguments tuple and the tuple itself
	Py_ssize_t size = PyTuple_Size(bind_data.arguments);
	for (Py_ssize_t i = 0; i < size; i++) {
		PyObject *arg = PyTuple_GetItem(bind_data.arguments, i);
		Py_XDECREF(arg);
	}
	// todo: for some reason this causes a segfault?
	// Py_XDECREF(bind_data.arguments);
}

void PyScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &bind_data = (PyScanBindData &)*data.bind_data;

	auto &local_state = (PyScanLocalState &)*data.local_state;

	if (local_state.done) {
		return;
	}

	PyObject *result = bind_data.function_result_iterable;
	if (nullptr == result) {
		throw std::runtime_error("Where did our iterator go?");
	}

	PyObject *row;
	int read_records = 0;
	while ((read_records < STANDARD_VECTOR_SIZE) && (row = PyIter_Next(result))) {
		auto iter_row = pyObjectToIterable(row);
		if (!iter_row) {
			// todo: cleanup?
			throw std::runtime_error("Error: Row record not iterable as expected");
		} else {
			std::vector<duckdb::Value> duck_row = {};
			ConvertPyObjectsToDuckDBValues(iter_row, bind_data.return_types, duck_row);
			for (long unsigned int i = 0; i < duck_row.size(); i++) {
				auto v = duck_row.at(i);
				// todo: Am I doing this correctly? I have no idea.
				output.SetValue(i, output.size(), v);
			}
			Py_DECREF(row);
			output.SetCardinality(output.size() + 1);
			read_records++;
		}
	}

	// PyIter_Next will return null if the iterator is exhausted or if an
	// exception has occurred during resumption of the underlying function,
	// so at this point we need to check which of these is the case.
	if (PyErr_Occurred()) {
		PythonException error = PythonException();

		// Shouldn't be necessary, but mark our scan as complete for good measure.
		local_state.done = true;

		// Clean everything up
		FinalizePyTable(bind_data);
		throw std::runtime_error(error.message);
	}
	if (!row) {
		// We've exhausted our iterator
		local_state.done = true;
		FinalizePyTable(bind_data);
		return;
	}
}

unique_ptr<FunctionData> PyBind(ClientContext &context, TableFunctionBindInput &input,
                                std::vector<LogicalType> &return_types, std::vector<std::string> &names) {
	auto result = make_uniq<PyScanBindData>();
	auto names_and_types = input.named_parameters["columns"];

	auto params = input.named_parameters;
	std::string module_name;
	std::string function_name;
	std::vector<Value> arguments;
	if ((0 < params.count("module")) && (0 < params.count("func"))) {
		module_name = input.named_parameters["module"].GetValue<std::string>();
		function_name = input.named_parameters["func"].GetValue<std::string>();
		arguments = input.inputs;
	} else if ((0 == params.count("module")) && (0 == params.count("func"))) {
		// Neither module nor func specified, infer this from arg list.
		auto func_spec_value = input.inputs[0];
		if (func_spec_value.type().id() != LogicalTypeId::VARCHAR) {
			throw InvalidInputException(
			    "First argument must be string specifying 'module:func' if name parameters not supplied");
		}
		auto func_specifier = func_spec_value.GetValue<std::string>();
		std::tie(module_name, function_name) = parse_func_specifier(func_specifier);

		/* Since we had to infer our functions specifier from the argument list, we treat
		   everything after the first argument as input to the python function. */
		arguments = std::vector<Value>(input.inputs.begin() + 1, input.inputs.end());
	} else if (0 < params.count("module")) {
		throw InvalidInputException("Module specified w/o a corresponding function");
	} else if (0 < params.count("func")) {
		throw InvalidInputException("Function specified w/o a corresponding module");
	} else {
		throw InvalidInputException("I don't know how logic works");
	}

	auto &child_type = names_and_types.type();
	if (child_type.id() != LogicalTypeId::STRUCT) {
		throw InvalidInputException("columns requires a struct as input");
	}
	auto &struct_children = StructValue::GetChildren(names_and_types);
	D_ASSERT(StructType::GetChildCount(child_type) == struct_children.size());
	for (idx_t i = 0; i < struct_children.size(); i++) {
		auto &name = StructType::GetChildName(child_type, i);
		auto &val = struct_children[i];
		names.push_back(name);
		if (val.type().id() != LogicalTypeId::VARCHAR) {
			throw BinderException("we require a type specification as string");
		}
		return_types.emplace_back(TransformStringToLogicalType(StringValue::Get(val), context));
	}
	if (names.empty()) {
		throw BinderException("require at least a single column as input!");
	}

	result->return_types = std::vector<LogicalType>(return_types);

	PythonFunction func = PythonFunction(module_name, function_name);
	result->arguments = duckdb_to_py(arguments);
	if (NULL == result->arguments) {
		throw IOException("Failed coerce function arguments");
	}

	// Invoke the function and grab a copy of the iterable it returns.
	PyObject *iter;
	PythonException *error;
	std::tie(iter, error) = func.call(result->arguments);
	if (!iter) {
		Py_XDECREF(iter);
		std::string err = error->message;
		error->~PythonException();
		throw std::runtime_error(err);
	} else if (!PyIter_Check(iter)) {
		Py_XDECREF(iter);
		throw std::runtime_error("Error: function '" + func.function_name() + "' did not return an iterator\n");
	}
	result->function_result_iterable = iter;
	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> PyInitGlobalState(ClientContext &context, TableFunctionInitInput &input) {
	auto result = make_uniq<PyScanGlobalState>();
	// result.function = get_python_function()
	return std::move(result);
}

unique_ptr<LocalTableFunctionState> PyInitLocalState(ExecutionContext &context, TableFunctionInitInput &input,
                                                     GlobalTableFunctionState *global_state) {
	// Leaving tehese commented out in case we need them in the future.
	// auto bind_data = (const PyScanBindData *)input.bind_data;
	// auto &gstate = (PyScanGlobalState &)*global_state;
	auto local_state = make_uniq<PyScanLocalState>();

	return std::move(local_state);
}

unique_ptr<CreateTableFunctionInfo> GetPythonTableFunction() {
	auto py_table_function =
	    TableFunction("python_table", {}, PyScan, (table_function_bind_t)PyBind, PyInitGlobalState, PyInitLocalState);

	py_table_function.varargs = LogicalType::ANY;
	py_table_function.named_parameters["module"] = LogicalType::VARCHAR;
	py_table_function.named_parameters["func"] = LogicalType::VARCHAR;
	py_table_function.named_parameters["columns"] = LogicalType::ANY;
	CreateTableFunctionInfo py_table_function_info(py_table_function);
	return make_uniq<CreateTableFunctionInfo>(py_table_function_info);
}

} // namespace pyudf
