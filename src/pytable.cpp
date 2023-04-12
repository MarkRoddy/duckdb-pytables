
#include <Python.h>
#include <iostream>
#include <stdexcept>
#include <duckdb.hpp>
#include <duckdb/parser/expression/constant_expression.hpp>
#include <duckdb/parser/expression/function_expression.hpp>
#include <pytable.hpp>
#include "python_function.hpp"

using namespace duckdb;

namespace pyudf {

struct PyScanBindData : public TableFunctionData {
	PythonFunction *function;

	// Function arguments coerced to a tuple used in Python calling semantics,
	// todo: free after function execution is complete
	PyObject *arguments;
};

struct PyScanLocalState : public LocalTableFunctionState {
	bool done = false;
};

struct PyScanGlobalState : public GlobalTableFunctionState {
	PyScanGlobalState() : GlobalTableFunctionState() {
	}
};

PyObject *duckdb_to_py(std::vector<Value> &values) {
	PyObject *py_tuple = PyTuple_New(values.size());

	for (size_t i = 0; i < values.size(); i++) {
		PyObject *py_value = nullptr;

		switch (values[i].type().id()) {
		case LogicalTypeId::BOOLEAN:
			py_value = PyBool_FromLong(values[i].GetValue<bool>());
			break;
		case LogicalTypeId::TINYINT:
			py_value = PyLong_FromLong(values[i].GetValue<int8_t>());
			break;
		case LogicalTypeId::SMALLINT:
			py_value = PyLong_FromLong(values[i].GetValue<int16_t>());
			break;
		case LogicalTypeId::INTEGER:
			py_value = PyLong_FromLong(values[i].GetValue<int32_t>());
			break;
		case LogicalTypeId::BIGINT:
			py_value = PyLong_FromLongLong(values[i].GetValue<int64_t>());
			break;
		case LogicalTypeId::FLOAT:
			py_value = PyFloat_FromDouble(values[i].GetValue<float>());
			break;
		case LogicalTypeId::DOUBLE:
			py_value = PyFloat_FromDouble(values[i].GetValue<double>());
			break;
		case LogicalTypeId::VARCHAR:
			py_value = PyUnicode_FromString(values[i].GetValue<std::string>().c_str());
			break;
		default:
			Py_INCREF(Py_None);
			py_value = Py_None;
		}

		PyTuple_SetItem(py_tuple, i, py_value);
	}

	return py_tuple;
}

void PyScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto bind_data = (const PyScanBindData *)data.bind_data;
	auto local_state = (PyScanLocalState *)data.local_state;

	if (local_state->done) {
		return;
	}

	PyObject *result;
	PythonException *error;
	std::tie(result, error) = bind_data->function->call(bind_data->arguments);
	if (!result) {
		Py_XDECREF(result);
		std::string err = error->message;
		error->~PythonException();
		throw std::runtime_error(err);
	} else if (!PyIter_Check(result)) {
		Py_XDECREF(result);
		throw std::runtime_error("Error: function '" + bind_data->function->function_name() +
		                         "' did not return an iterator\n");
	}

	PyObject *row;
	PyObject *item;
	while ((row = PyIter_Next(result))) {
		if (!PyList_Check(row)) {
			std::cerr << "Error: Row record not List as expected\n";
			output.SetValue(0, output.size(), Value(""));
		} else if (PyList_Size(row) != 1) {
			std::cerr << "Error: Row record did not contain exactly 1 column as expected\n";
			output.SetValue(0, output.size(), Value(""));
		} else {
			item = PyList_GetItem(row, 0);
			if (!PyUnicode_Check(item)) {
				std::cerr << "Error: item in row record is not a string\n";
				output.SetValue(0, output.size(), Value(""));
			} else {
				auto value = PyUnicode_AsUTF8(item);
				output.SetValue(0, output.size(), Value(value));
			}
			Py_DECREF(item);
		}
		Py_DECREF(row);
		output.SetCardinality(output.size() + 1);
	}

	// PyIter_Next will return null if the iterator is exhausted or if an
	// exception has occurred during resumption of the underlying function,
	// so at this point we need to check which of these is the case.
	if (PyErr_Occurred()) {
		error = new PythonException();
		std::string err = error->message;
		error->~PythonException();
		Py_DECREF(result);
		throw std::runtime_error(err);
	}
	Py_DECREF(result);
	local_state->done = true;
}

unique_ptr<FunctionData> PyBind(ClientContext &context, TableFunctionBindInput &input,
                                                std::vector<LogicalType> &return_types,
                                                std::vector<std::string> &names) {
	auto result = make_unique<PyScanBindData>();
	auto module_name = input.inputs[0].GetValue<std::string>();
	auto function_name = input.inputs[1].GetValue<std::string>();
        auto names_and_types = input.inputs[2];
        auto arguments = ListValue::GetChildren(input.inputs[3]);

        auto &child_type = names_and_types.type();
        if (child_type.id() != LogicalTypeId::STRUCT) {
          throw BinderException("columns requires a struct as input");
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

        
	result->function = new PythonFunction(module_name, function_name);
	result->arguments = duckdb_to_py(arguments);
	if (NULL == result->arguments) {
		throw IOException("Failed coerce function arguments");
	}

	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> PyInitGlobalState(ClientContext &context,
                                                                       TableFunctionInitInput &input) {
	auto result = make_unique<PyScanGlobalState>();
	// result.function = get_python_function()
	return std::move(result);
}

unique_ptr<LocalTableFunctionState> PyInitLocalState(ExecutionContext &context,
                                                                     TableFunctionInitInput &input,
                                                                     GlobalTableFunctionState *global_state) {
	auto bind_data = (const PyScanBindData *)input.bind_data;
	auto &gstate = (PyScanGlobalState &)*global_state;

	auto local_state = make_unique<PyScanLocalState>();

	return std::move(local_state);
}

unique_ptr<CreateTableFunctionInfo> GetPythonTableFunction() {
	auto py_table_function = TableFunction("python_table",
	                                               {LogicalType::VARCHAR, LogicalType::VARCHAR,
                                                        /* Return Column Names + Types*/
                                                        // LogicalType::STRUCT({}),
                                                        LogicalType::ANY,
                                                        //LogicalType::VARCHAR, LogicalType::VARCHAR),
	                                                /* Python Function Arguments */
	                                                LogicalType::LIST(LogicalType::ANY),
},
	                                               PyScan, PyBind, PyInitGlobalState, PyInitLocalState);

	CreateTableFunctionInfo py_table_function_info(py_table_function);
	return make_unique<CreateTableFunctionInfo>(py_table_function_info);
}

} // namespace pyudf
