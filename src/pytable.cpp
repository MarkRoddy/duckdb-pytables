
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
	std::vector<LogicalType> return_types;

	// todo: free after consumed + rename to something better, py_row_iterator?
	PyObject *function_result_iterable;
};

struct PyScanLocalState : public LocalTableFunctionState {
	bool done = false;
};

struct PyScanGlobalState : public GlobalTableFunctionState {
	PyScanGlobalState() : GlobalTableFunctionState() {
	}
};

std::pair<std::vector<duckdb::Value> *, std::string>
ConvertPyObjectsToDuckDBValues(PyObject *py_iterator, std::vector<duckdb::LogicalType> logical_types) {
	std::vector<duckdb::Value> *result = new std::vector<duckdb::Value>();
	std::string error_message;

	if (!PyIter_Check(py_iterator)) {
		error_message = "First argument must be an iterator";
		return {nullptr, error_message};
	}

	PyObject *py_item;
	size_t index = 0;

	while ((py_item = PyIter_Next(py_iterator))) {
		if (index >= logical_types.size()) {
			Py_DECREF(py_item);
			error_message = "A row with " + std::to_string(index + 1) + " values was detected though " +
			                std::to_string(logical_types.size()) + " columns were expected",
			delete result;
			return {nullptr, error_message};
		}

		duckdb::Value value;
		PyObject *py_value;
		duckdb::LogicalType logical_type = logical_types[index];
		bool conversion_failed = false;

		switch (logical_type.id()) {
		case duckdb::LogicalTypeId::BOOLEAN:
			if (!PyBool_Check(py_item)) {
				conversion_failed = true;
			} else {
				value = duckdb::Value(Py_True == py_item);
			}
			break;
		case duckdb::LogicalTypeId::TINYINT:
		case duckdb::LogicalTypeId::SMALLINT:
		case duckdb::LogicalTypeId::INTEGER:
			if (!PyLong_Check(py_item)) {
				conversion_failed = true;
			} else {
				value = duckdb::Value((int32_t)PyLong_AsLong(py_item));
			}
			break;
		// case duckdb::LogicalTypeId::BIGINT:
		//   if (!PyLong_Check(py_item)) {
		//     conversion_failed = true;
		//   } else {
		//     value = duckdb::Value(PyLong_AsLongLong(py_item));
		//   }
		//   break;
		case duckdb::LogicalTypeId::FLOAT:
		case duckdb::LogicalTypeId::DOUBLE:
			if (!PyFloat_Check(py_item)) {
				conversion_failed = true;
			} else {
				value = duckdb::Value(PyFloat_AsDouble(py_item));
			}
			break;
		case duckdb::LogicalTypeId::VARCHAR:
			if (!PyUnicode_Check(py_item)) {
				conversion_failed = true;
			} else {
				py_value = PyUnicode_AsUTF8String(py_item);
				value = duckdb::Value(PyBytes_AsString(py_value));
				Py_DECREF(py_value);
			}
			break;
			// Add more cases for other LogicalTypes here
		default:
			conversion_failed = true;
		}

		if (conversion_failed) {
			// DUCKDB_API Value(std::nullptr_t val); // NOLINT: Allow implicit conversion from `nullptr_t`
			value = duckdb::Value((std::nullptr_t)NULL);
		}

		result->push_back(value);
		Py_DECREF(py_item);
		index++;
	}

	if (PyErr_Occurred()) {
		// todo: use our Python exception wrapper
		error_message = "Python runtime error occurred during iteration";
		PyErr_Clear();
		delete result;
		return {nullptr, error_message};
	}

	if (index != logical_types.size()) {
		error_message = "A row with " + std::to_string(index) + " values was detected though " +
		                std::to_string(logical_types.size()) + " columns were expected";
		delete result;
		return {nullptr, error_message};
	}

	return {result, error_message};
}

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

PyObject *pyObjectToIterable(PyObject *py_object) {
	PyObject *collections_module = PyImport_ImportModule("collections");
	if (!collections_module) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to import collections module");
		return nullptr;
	}

	PyObject *iterable_class = PyObject_GetAttrString(collections_module, "Iterable");
	if (!iterable_class) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to get Iterable class from collections module");
		Py_DECREF(collections_module);
		return nullptr;
	}

	int is_iterable = PyObject_IsInstance(py_object, iterable_class);
	Py_DECREF(iterable_class);
	Py_DECREF(collections_module);

	if (!is_iterable) {
		PyErr_SetString(PyExc_TypeError, "Input must be an iterable or an object that can be iterated upon");
		return nullptr;
	}

	PyObject *py_iter = PyObject_GetIter(py_object);
	if (!py_iter) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to get iterator from the input object");
	}

	return py_iter;
}

void PyScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto bind_data = (const PyScanBindData *)data.bind_data;
	auto local_state = (PyScanLocalState *)data.local_state;

	if (local_state->done) {
		return;
	}

	PyObject *result = bind_data->function_result_iterable;

	PyObject *row;
	int read_records = 0;
	while ((read_records < STANDARD_VECTOR_SIZE) && (row = PyIter_Next(result))) {
		auto iter_row = pyObjectToIterable(row);
		if (!iter_row) {
			// todo: cleanup?
			throw std::runtime_error("Error: Row record not iterable as expected");
		} else {
			std::string errmsg;
			std::vector<duckdb::Value> *duck_row;
			std::tie(duck_row, errmsg) = ConvertPyObjectsToDuckDBValues(iter_row, bind_data->return_types);
			if (!duck_row) {
				// todo: cleanup
				throw std::runtime_error(errmsg);
			} else {
				for (long unsigned int i = 0; i < duck_row->size(); i++) {
					auto v = duck_row->at(i);
					// todo: Am I doing this correctly? I have no idea.
					output.SetValue(i, output.size(), v);
				}
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
		PythonException *error;
		error = new PythonException();
		std::string err = error->message;
		error->~PythonException();
		Py_DECREF(result);
		throw std::runtime_error(err);
	}
	if (!row) {
		// We've exhausted our iterator
		local_state->done = true;
		Py_DECREF(result);
	}
}

unique_ptr<FunctionData> PyBind(ClientContext &context, TableFunctionBindInput &input,
                                std::vector<LogicalType> &return_types, std::vector<std::string> &names) {
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

	result->return_types = std::vector<LogicalType>(return_types);
	result->function = new PythonFunction(module_name, function_name);
	result->arguments = duckdb_to_py(arguments);
	if (NULL == result->arguments) {
		throw IOException("Failed coerce function arguments");
	}

	// Invoke the function and grab a copy of the iterable it returns.
	PyObject *iter;
	PythonException *error;
	std::tie(iter, error) = result->function->call(result->arguments);
	if (!iter) {
		Py_XDECREF(iter);
		std::string err = error->message;
		error->~PythonException();
		throw std::runtime_error(err);
	} else if (!PyIter_Check(iter)) {
		Py_XDECREF(iter);
		throw std::runtime_error("Error: function '" + result->function->function_name() +
		                         "' did not return an iterator\n");
	}
	result->function_result_iterable = iter;
	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> PyInitGlobalState(ClientContext &context, TableFunctionInitInput &input) {
	auto result = make_unique<PyScanGlobalState>();
	// result.function = get_python_function()
	return std::move(result);
}

unique_ptr<LocalTableFunctionState> PyInitLocalState(ExecutionContext &context, TableFunctionInitInput &input,
                                                     GlobalTableFunctionState *global_state) {
	// Leaving tehese commented out in case we need them in the future.
	// auto bind_data = (const PyScanBindData *)input.bind_data;
	// auto &gstate = (PyScanGlobalState &)*global_state;

	auto local_state = make_unique<PyScanLocalState>();

	return std::move(local_state);
}

unique_ptr<CreateTableFunctionInfo> GetPythonTableFunction() {
	auto py_table_function = TableFunction("python_table",
	                                       {
	                                           LogicalType::VARCHAR,
	                                           LogicalType::VARCHAR,
	                                           /* Return Column Names + Types*/
	                                           // LogicalType::STRUCT({}),
	                                           LogicalType::ANY,
	                                           // LogicalType::VARCHAR, LogicalType::VARCHAR),
	                                           /* Python Function Arguments */
	                                           LogicalType::LIST(LogicalType::ANY),
	                                       },
	                                       PyScan, PyBind, PyInitGlobalState, PyInitLocalState);

	CreateTableFunctionInfo py_table_function_info(py_table_function);
	return make_unique<CreateTableFunctionInfo>(py_table_function_info);
}

} // namespace pyudf
