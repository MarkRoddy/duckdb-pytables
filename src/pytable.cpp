
#include <Python.h>
#include <iostream>
#include <stdexcept>
#include <duckdb.hpp>
#include <duckdb/parser/expression/constant_expression.hpp>
#include <duckdb/parser/expression/function_expression.hpp>
#include <pytable.hpp>
#include "python_function.hpp"
#include <pyconvert.hpp>
// #include <pyfunciterator.hpp>
#include <peekableiterator.hpp>
#include <pydeferred_function_call.hpp>

#include <typeinfo>

namespace py = pybind11;
using namespace duckdb;
namespace pyudf {

struct PyScanBindData : public TableFunctionData {
  std::string module_name;
  std::string function_name;

  std::vector<Value> sql_arguments;
  std::vector<LogicalType> return_types;
  
  // Function arguments coerced to a tuple used in Python calling semantics,
  PyObject *py_args;

  // Keyword arguments coerced to a dict to be used in **kwarg calling semantics
  PyObject *py_kwargs;

  // Return value of the function specified
  // PyObject *function_result_iterable;

  // todo: add a wrapper around the function that will lazy import the function and
  // has hte option to peak at hte first value (values?) to be used if no `columns
  // argument is specified.

  // todo: move to 'PyScanGlobalState'
  // PyFuncIterator iterable_function;
  // pybind11::iterator function_result;

  // Iterable returned by our function call. The function call may or may not
  // have already happened depending on whether or not we needed to peak at
  // the return values to figure out our column types.
  PeekableIterator<PyDeferredFunctionCall::ResultIterator<py::handle>> function_result;
};

struct PyScanLocalState : public LocalTableFunctionState {
	bool done = false;
};

struct PyScanGlobalState : public GlobalTableFunctionState {

	PyScanGlobalState() : GlobalTableFunctionState() {
	}
};

void FinalizePyTable(PyScanBindData &bind_data) {
	// Free the iterable returned by our python function call
	// Py_DECREF(bind_data.function_result_iterable);
	// bind_data.function_result_iterable = nullptr;

	// Free each entry of the arguments tuple and the tuple itself
	// Py_ssize_t size = PyTuple_Size(bind_data.arguments);
	// for (Py_ssize_t i = 0; i < size; i++) {
	// 	PyObject *arg = PyTuple_GetItem(bind_data.arguments, i);
	// 	Py_XDECREF(arg);
	// }
}

void PyScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &bind_data = (PyScanBindData &)*data.bind_data;
	auto &local_state = (PyScanLocalState &)*data.local_state;
	if (local_state.done) {
		return;
	}
        int read_records = 0;
	auto it = bind_data.function_result;
        auto end = it.end();
        for (read_records = 0; read_records < STANDARD_VECTOR_SIZE && it != end; ++read_records) {
                auto row = *it;
                if(!isIterable(row)) {
                  // todo: cleanup?
                  throw std::runtime_error("Error: Row record not iterable as expected");
                }
                // todo: wrap in try/catch
                auto iter_row = py::iter(row);

                std::vector<duckdb::Value> duck_row = {};
                ConvertPyObjectsToDuckDBValues(iter_row.ptr(), bind_data.return_types, duck_row);
                for (long unsigned int i = 0; i < duck_row.size(); i++) {
                  auto v = duck_row.at(i);
                  output.SetValue(i, output.size(), v);
                }
                output.SetCardinality(output.size() + 1);
                try {
                  ++it;
                } catch(py::error_already_set &e) {
                  throw std::runtime_error(e.what());
                }
	}

        if (it == end) {
		// We've exhausted our iterator
		local_state.done = true;
		FinalizePyTable(bind_data);
		return;
	}
}


void ParseFunctionAndArguments(TableFunctionBindInput &input, unique_ptr<PyScanBindData> &state) {
	auto params = input.named_parameters;
	if ((0 < params.count("module")) && (0 < params.count("func"))) {
		state->module_name = input.named_parameters["module"].GetValue<std::string>();
		state->function_name = input.named_parameters["func"].GetValue<std::string>();
		state->sql_arguments = input.inputs;
	} else if ((0 == params.count("module")) && (0 == params.count("func"))) {
		// Neither module nor func specified, infer this from arg list.
		auto func_spec_value = input.inputs[0];
		if (func_spec_value.type().id() != LogicalTypeId::VARCHAR) {
			throw InvalidInputException(
			    "First argument must be string specifying 'module:func' if name parameters not supplied");
		}
		auto func_specifier = func_spec_value.GetValue<std::string>();
		std::tie(state->module_name, state->function_name) = parse_func_specifier(func_specifier);

		/* Since we had to infer our functions specifier from the argument list, we treat
		   everything after the first argument as input to the python function. */
		state->sql_arguments = std::vector<Value>(input.inputs.begin() + 1, input.inputs.end());
	} else if (0 < params.count("module")) {
		throw InvalidInputException("Module specified w/o a corresponding function");
	} else if (0 < params.count("func")) {
		throw InvalidInputException("Function specified w/o a corresponding module");
	} else {
		throw InvalidInputException("I don't know how logic works");
	}

        // Now that we have our module name and function name, pull it out into our wrapper in case
        // return type parsing needs to examine the function output
        state->py_args = py::tuple(result->sql_arguments.size());
        for (size_t i = 0; i < result->sql_arguments.size(); i++) {
          pyargs[i] = duckdb_to_py(result->sql_arguments[i]);
        }

	if (0 < params.count("kwargs")) {
		auto input_kwargs = params["kwargs"];
		auto ik_type = input_kwargs.type().id();
		if (ik_type != LogicalTypeId::STRUCT) {
			throw InvalidInputException("kwargs must be a struct mapping argument names to values");
		}
                state->py_kwargs = StructToBindDict(input_kwargs);
	}
          // PeekableIterator<PyDeferredFunctionCall::ResultIterator<py::handle>> function_result;
        // PyDeferredFunctionCall(std::string module_name, std::string func_name, py::tuple args, py::dict kwargs) {
        PyDeferredFunctionCall defered_func = new PyDeferredFunctionCall(state->module_name, state->function_name, state->py_args, state->py_kwargs);
        result->function_result = PeekableIterator<PyDeferredFunctionCall::ResultIterator<py::handle>>(defered_func.begin());
        //                        auto defered_func 
        // result->iterable_function = new PyFuncIterator(state->module_name, state->function_name, state->py_args, state->py_kwargs);

}

void ParseNamesAndTypesFromArgument(ClientContext &context, Value names_and_types, std::vector<LogicalType> &return_types, std::vector<std::string> &names) {
	
	auto &child_type = names_and_types.type();
	if (child_type.id() != LogicalTypeId::STRUCT) {
		throw InvalidInputException("columns requires a struct mapping column names to data types");
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
}  
unique_ptr<FunctionData> PyBind(ClientContext &context, TableFunctionBindInput &input,
                                std::vector<LogicalType> &return_types, std::vector<std::string> &names) {
        auto result = make_uniq<PyScanBindData>();
        ParseFunctionAndArguments(input, result);


        auto names_and_types = input.named_parameters["columns"];
        ParseNamesAndTypesFromArgument(context, names_and_types, return_types, names);                               
	if (names.empty()) {
		throw BinderException("require at least a single column as input!");
	}
	result->return_types = std::vector<LogicalType>(return_types);

        
        // Call the Python function with the arguments
        // py::object result = my_func(arg1, arg2, **kwargs);
        py::object funcresult = func(*pyargs, **pykwargs);
        
	// Invoke the function and grab a copy of the iterable it returns.
	// PyObject *iter;
	// PythonException *error;
	// std::tie(iter, error) = func.call(result->arguments, result->kwargs);
	// if (!iter) {
	// 	Py_XDECREF(iter);
	// 	std::string err = error->message;
	// 	error->~PythonException();
	// 	throw std::runtime_error(err);
	// } else if (!PyIter_Check(iter)) {
	// 	Py_XDECREF(iter);
	// 	throw std::runtime_error("Error: function '" + func.function_name() + "' did not return an iterator\n");
	// }
	// result->function_result = py::iter(funcresult);
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
	    TableFunction("pytable", {}, PyScan, (table_function_bind_t)PyBind, PyInitGlobalState, PyInitLocalState);

	// todo: don't configure this for older versions of duckdb
	py_table_function.varargs = LogicalType::ANY;

	py_table_function.named_parameters["module"] = LogicalType::VARCHAR;
	py_table_function.named_parameters["func"] = LogicalType::VARCHAR;
	py_table_function.named_parameters["columns"] = LogicalType::ANY;
	py_table_function.named_parameters["kwargs"] = LogicalType::ANY;

	CreateTableFunctionInfo py_table_function_info(py_table_function);
	return make_uniq<CreateTableFunctionInfo>(py_table_function_info);
}

} // namespace pyudf
