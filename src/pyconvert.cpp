
#include <pyconvert.hpp>
#include <duckdb.hpp>
#include <Python.h>
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/eval.h>

namespace py = pybind11;


namespace pyudf {

PyObject *duckdb_to_py(duckdb::Value &value) {
	PyObject *py_value = nullptr;

	switch (value.type().id()) {
	case duckdb::LogicalTypeId::BOOLEAN:
		py_value = PyBool_FromLong(value.GetValue<bool>());
		break;
	case duckdb::LogicalTypeId::TINYINT:
		py_value = PyLong_FromLong(value.GetValue<int8_t>());
		break;
	case duckdb::LogicalTypeId::SMALLINT:
		py_value = PyLong_FromLong(value.GetValue<int16_t>());
		break;
	case duckdb::LogicalTypeId::INTEGER:
		py_value = PyLong_FromLong(value.GetValue<int32_t>());
		break;
	case duckdb::LogicalTypeId::BIGINT:
		py_value = PyLong_FromLongLong(value.GetValue<int64_t>());
		break;
	case duckdb::LogicalTypeId::FLOAT:
		py_value = PyFloat_FromDouble(value.GetValue<float>());
		break;
	case duckdb::LogicalTypeId::DOUBLE:
		py_value = PyFloat_FromDouble(value.GetValue<double>());
		break;
	case duckdb::LogicalTypeId::VARCHAR:
		py_value = PyUnicode_FromString(value.GetValue<std::string>().c_str());
		break;
	case duckdb::LogicalTypeId::STRUCT:
		py_value = StructToDict(value);
		break;
	default:
		std::cerr << "Unhandled Logical Type: " + value.type().ToString() << std::endl;
		Py_INCREF(Py_None);
		py_value = Py_None;
	}
	return py_value;
}

PyObject *duckdbs_to_pys(std::vector<duckdb::Value> &values) {
	PyObject *py_tuple = PyTuple_New(values.size());

	for (size_t i = 0; i < values.size(); i++) {
		PyObject *py_value = nullptr;
		py_value = duckdb_to_py(values[i]);
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
	case duckdb::LogicalTypeId::STRUCT:
		py_value = StructToDict(value);
		break;
	default:
		conversion_failed = true;
	}

	if (conversion_failed) {
		// DUCKDB_API Value(std::nullptr_t val); // NOLINT: Allow implicit conversion from `nullptr_t`
		value = duckdb::Value((std::nullptr_t)NULL);
	}
	return value;
}

void ConvertPyObjectsToDuckDBValues(PyObject *py_iterator, std::vector<duckdb::LogicalType> logical_types,
                                    std::vector<duckdb::Value> &result) {

	if (!PyIter_Check(py_iterator)) {
		throw duckdb::InvalidInputException("First argument must be an iterator");
	}

	PyObject *py_item;
	size_t index = 0;
	std::string error_message;
	while ((py_item = PyIter_Next(py_iterator))) {
		if (index >= logical_types.size()) {
			Py_DECREF(py_item);
			error_message = "A row with " + std::to_string(index + 1) + " values was detected though " +
			                std::to_string(logical_types.size()) + " columns were expected",
			throw duckdb::InvalidInputException(error_message);
		}
		duckdb::LogicalType logical_type = logical_types[index];

		duckdb::Value value = ConvertPyObjectToDuckDBValue(py_item, logical_type);
		result.push_back(value);
		Py_DECREF(py_item);
		index++;
	}

	if (PyErr_Occurred()) {
		// todo: use our Python exception wrapper
		error_message = "Python runtime error occurred during iteration";
		PyErr_Clear();
		throw std::runtime_error(error_message);
	}

	if (index != logical_types.size()) {
		error_message = "A row with " + std::to_string(index) + " values was detected though " +
		                std::to_string(logical_types.size()) + " columns were expected";
		throw duckdb::InvalidInputException(error_message);
	}
}

PyObject *pyObjectToIterable(PyObject *py_object) {
	auto iterable_class = py::module_::import("collections.abc").attr("Iterable");
        int is_iterable = py::isinstance(py_object, iterable_class);

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

bool isIterable(pybind11::handle pyObject) {
  auto Iterable = py::module_::import("collections.abc").attr("Iterable");
  return py::isinstance(pyObject, Iterable);
}


PyObject *StructToDict(duckdb::Value value) {
	// Build the keyword argument dictionary
	PyObject *py_value = PyDict_New();
	auto &child_type = value.type();
	auto &struct_children = duckdb::StructValue::GetChildren(value);
	D_ASSERT(duckdb::StructType::GetChildCount(child_type) == struct_children.size());
	for (idx_t i = 0; i < struct_children.size(); i++) {
		duckdb::Value name = duckdb::StructType::GetChildName(child_type, i);
		duckdb::Value val = struct_children[i];

		auto pyName = duckdb_to_py(name);
		auto pyValue = duckdb_to_py(val);
		PyDict_SetItem(py_value, pyName, pyValue);
	}
	return py_value;
}

py::dict StructToBindDict(duckdb::Value value) {
  py::dict pymap;

	auto &child_type = value.type();
	auto &struct_children = duckdb::StructValue::GetChildren(value);
	D_ASSERT(duckdb::StructType::GetChildCount(child_type) == struct_children.size());
	for (idx_t i = 0; i < struct_children.size(); i++) {
		duckdb::Value name = duckdb::StructType::GetChildName(child_type, i);
		duckdb::Value val = struct_children[i];

		auto pyName = duckdb_to_py(name);
		auto pyValue = duckdb_to_py(val);
                pymap[pyName] = pyValue;
	}
	return pymap;
}

} // namespace pyudf
