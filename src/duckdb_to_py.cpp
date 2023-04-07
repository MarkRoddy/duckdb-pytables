
#include <duckdb_to_py.hpp>
#include <duckdb.hpp>
#include <Python.h>

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
			Py_INCREF(Py_None);
			py_value = Py_None;
		}

		PyTuple_SetItem(py_tuple, i, py_value);
	}

	return py_tuple;
}
}
