
#include <Python.h>
#include <python_exception.hpp>

void PythonException::print_error() {
	// PyErr_Print();
	std::cerr << message;
}

PythonException::PythonException(std::string message, std::string traceback) : message(message), traceback(traceback) {
}

PythonException::PythonException() {
	PyObject *ptype, *pvalue, *ptraceback;
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);

	if (pvalue != nullptr) {
		PyObject *py_type_str = PyObject_Str(ptype);
		PyObject *py_value_str = PyObject_Str(pvalue);
		PyObject *py_traceback_str = PyObject_Str(ptraceback);

		// message = PyUnicode_AsUTF8AndSize(py_value_str, nullptr);
		// py_value = PyUnicode_AsUTF8String(py_item);
		// value = duckdb::Value(PyBytes_AsString(py_value));
		auto py_value_unicode = PyUnicode_AsUTF8String(py_value_str);
		message = PyBytes_AsString(py_value_unicode);
		Py_DECREF(py_value_unicode);

		auto py_traceback_unicode = PyUnicode_AsUTF8String(py_traceback_str);
		traceback = PyBytes_AsString(py_traceback_unicode);
		Py_DECREF(py_traceback_unicode);
		// traceback = PyUnicode_AsUTF8AndSize(py_traceback_str, nullptr);

		Py_DECREF(py_type_str);
		Py_DECREF(py_value_str);
		Py_DECREF(py_traceback_str);

		/* I've only observed ptraceback coming back null, but to be safe we're checking
		   each of these values. Otherwise, we have a segfault that masks a helpful
		   Python error message.
		*/
		if (ptype) {
			Py_DECREF(ptype);
		}
		if (pvalue) {
			Py_DECREF(pvalue);
		}
		if (ptraceback) {
			Py_DECREF(ptraceback);
		}
	}
}
