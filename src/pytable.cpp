
#include <Python.h>
#include <iostream>
#include <duckdb.hpp>
#include <duckdb/parser/expression/constant_expression.hpp>
#include <duckdb/parser/expression/function_expression.hpp>
#include <pytable.hpp>

namespace pyudf {

struct PyScanBindData : public duckdb::TableFunctionData {
	std::string module_name;
	std::string function_name;
	// A python function object
	PyObject *function;

	// Function arguments coerced to a tuple used in Python calling semantics
	PyObject *arguments;
};

struct PyScanLocalState : public duckdb::LocalTableFunctionState {
	bool done = false;
};

struct PyScanGlobalState : public duckdb::GlobalTableFunctionState {
	PyScanGlobalState() : duckdb::GlobalTableFunctionState() {
	}
};

PyObject *duckdb_to_py(std::vector<duckdb::Value> &values) {
	PyObject *py_tuple = PyTuple_New(values.size());

	for (size_t i = 0; i < values.size(); i++) {
		PyObject *py_value = nullptr;

		switch (values[i].type().id()) {
		case duckdb::LogicalTypeId::BOOLEAN:
			py_value = PyBool_FromLong(values[i].GetValue<bool>());
			break;
		case duckdb::LogicalTypeId::TINYINT:
			py_value = PyLong_FromLong(values[i].GetValue<int8_t>());
			break;
		case duckdb::LogicalTypeId::SMALLINT:
			py_value = PyLong_FromLong(values[i].GetValue<int16_t>());
			break;
		case duckdb::LogicalTypeId::INTEGER:
			py_value = PyLong_FromLong(values[i].GetValue<int32_t>());
			break;
		case duckdb::LogicalTypeId::BIGINT:
			py_value = PyLong_FromLongLong(values[i].GetValue<int64_t>());
			break;
		case duckdb::LogicalTypeId::FLOAT:
			py_value = PyFloat_FromDouble(values[i].GetValue<float>());
			break;
		case duckdb::LogicalTypeId::DOUBLE:
			py_value = PyFloat_FromDouble(values[i].GetValue<double>());
			break;
		case duckdb::LogicalTypeId::VARCHAR:
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

PyObject *get_python_function(std::string module_name, std::string function_name) {
	// Import the specified module
	PyObject *module = PyImport_ImportModule(module_name.c_str());
	if (!module) {
		PyErr_Print();
		std::cerr << "Error: could not import module '" << module_name << "'\n";
		return NULL;
	}

	// Get the specified function from the module
	PyObject *function = PyObject_GetAttrString(module, function_name.c_str());
	if (!function) {
		std::cerr << "Error: could not find function '" << function_name << "' in module '" << module_name << "'\n";
		return NULL;
	} else if (!PyCallable_Check(function)) {
		std::cerr << "Error: Function'" << function_name << "' in module '" << module_name
		          << " is not a callable object'\n";
		return NULL;
	} else {
		return function;
	}
}

void PyScan(duckdb::ClientContext &context, duckdb::TableFunctionInput &data, duckdb::DataChunk &output) {
	auto bind_data = (const PyScanBindData *)data.bind_data;
	auto local_state = (PyScanLocalState *)data.local_state;

	if (local_state->done) {
		return;
	}

	PyObject *result = PyObject_CallObject(bind_data->function, bind_data->arguments);
	if (!result) {
		PyErr_Print();
		throw duckdb::IOException("Error: function '" + bind_data->function_name + "' did not return a value\n");
	} else if (!PyIter_Check(result)) {
		throw duckdb::IOException("Error: function '" + bind_data->function_name + "' did not return an iterator\n");
	}

	PyObject *row;
	PyObject *item;
	while ((row = PyIter_Next(result))) {
		if (!PyList_Check(row)) {
			std::cerr << "Error: Row record not List as expected\n";
			output.SetValue(0, output.size(), duckdb::Value(""));
		} else if (PyList_Size(row) != 1) {
			std::cerr << "Error: Row record did not contain exactly 1 column as expected\n";
			output.SetValue(0, output.size(), duckdb::Value(""));
		} else {
			item = PyList_GetItem(row, 0);
			if (!PyUnicode_Check(item)) {
				std::cerr << "Error: item in row record is not a string\n";
				output.SetValue(0, output.size(), duckdb::Value(""));
			} else {
				auto value = PyUnicode_AsUTF8(item);
				output.SetValue(0, output.size(), duckdb::Value(value));
				// output.SetCardinality(output.size() + 1);
			}
			Py_DECREF(item);
		}
		Py_DECREF(row);
		output.SetCardinality(output.size() + 1);
	}
	Py_DECREF(result);
	local_state->done = true;
}

duckdb::unique_ptr<duckdb::FunctionData> PyBind(duckdb::ClientContext &context, duckdb::TableFunctionBindInput &input,
                                                std::vector<duckdb::LogicalType> &return_types,
                                                std::vector<std::string> &names) {
	auto result = duckdb::make_unique<PyScanBindData>();
	auto module_name = input.inputs[0].GetValue<std::string>();
	auto function_name = input.inputs[1].GetValue<std::string>();
	auto arguments = duckdb::ListValue::GetChildren(input.inputs[2]);
	auto column_names = duckdb::ListValue::GetChildren(input.inputs[3]);

	result->module_name = module_name;
	result->function_name = function_name;
	result->function = get_python_function(module_name, function_name);
	if (NULL == result->function) {
		throw duckdb::IOException("Failed to load function");
	}
	result->arguments = duckdb_to_py(arguments);
	if (NULL == result->arguments) {
		throw duckdb::IOException("Failed coerce function arguments");
	}

	auto iter = column_names.begin();
	for (iter; iter < column_names.end(); iter++) {
		// todo: (optionally?) source the schema from the function, maybe the column names too?
		return_types.push_back(duckdb::LogicalType::VARCHAR);
	}

	iter = column_names.begin();
	for (iter; iter < column_names.end(); iter++) {
		names.push_back(duckdb::StringValue::Get(*iter));
	}

	return std::move(result);
}

duckdb::unique_ptr<duckdb::GlobalTableFunctionState> PyInitGlobalState(duckdb::ClientContext &context,
                                                                       duckdb::TableFunctionInitInput &input) {
	auto result = duckdb::make_unique<PyScanGlobalState>();
	// result.function = get_python_function()
	return std::move(result);
}

duckdb::unique_ptr<duckdb::LocalTableFunctionState> PyInitLocalState(duckdb::ExecutionContext &context,
                                                                     duckdb::TableFunctionInitInput &input,
                                                                     duckdb::GlobalTableFunctionState *global_state) {
	auto bind_data = (const PyScanBindData *)input.bind_data;
	auto &gstate = (PyScanGlobalState &)*global_state;

	auto local_state = duckdb::make_unique<PyScanLocalState>();

	return std::move(local_state);
}

duckdb::unique_ptr<duckdb::CreateTableFunctionInfo> GetPythonTableFunction() {
	auto py_table_function = duckdb::TableFunction("python_table",
	                                               {duckdb::LogicalType::VARCHAR, duckdb::LogicalType::VARCHAR,
	                                                /* Python Function Arguments */
	                                                duckdb::LogicalType::LIST(duckdb::LogicalType::ANY),
	                                                /* Return Column Names */
	                                                duckdb::LogicalType::LIST(duckdb::LogicalType::VARCHAR)},
	                                               PyScan, PyBind, PyInitGlobalState, PyInitLocalState);

	duckdb::CreateTableFunctionInfo py_table_function_info(py_table_function);
	return duckdb::make_unique<duckdb::CreateTableFunctionInfo>(py_table_function_info);
}

} // namespace pyudf
