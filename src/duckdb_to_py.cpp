
#include <duckdb_to_py.hpp>
#include <duckdb.hpp>
#include <Python.h>
#include <iostream>

namespace pyudf {
  
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
                  std::cerr << "Unhandled Logical Type: " + values[i].type().ToString() << std::endl;
			Py_INCREF(Py_None);
			py_value = Py_None;
		}

		PyTuple_SetItem(py_tuple, i, py_value);
	}

	return py_tuple;
}

duckdb::Value ConvertPyObjectToDuckDBValue(PyObject *py_item, duckdb::LogicalType logical_type) {
  duckdb::Value value;
  PyObject *py_value;
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
                return value;
}
}
