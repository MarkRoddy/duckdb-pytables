
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
        std::cerr << "Fetching the python error";
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        std::cerr << "Fetched the python error";

	if (pvalue != nullptr) {
                std::cerr << "Grabbing some values?";
		PyObject *py_type_str = PyObject_Str(ptype);
		PyObject *py_value_str = PyObject_Str(pvalue);
		PyObject *py_traceback_str = PyObject_Str(ptraceback);
                std::cerr << "Values grabbed";

                std::cerr << "Converting message";
		message = PyUnicode_AsUTF8(py_value_str);
                std::cerr << "Converting traceback";
		traceback = PyUnicode_AsUTF8(py_traceback_str);
                std::cerr << "Conversion complete, now some decrefs";

                std::cerr << "decrefs: py_type_str";
		Py_DECREF(py_type_str);
                std::cerr << "decrefs: py_value_str";
		Py_DECREF(py_value_str);
                std::cerr << "decrefs: py_traceback_str";
		Py_DECREF(py_traceback_str);

                std::cerr << "decrefs: ptype";
		Py_DECREF(ptype);
                std::cerr << "decrefs: pvalue";
		Py_DECREF(pvalue);
                std::cerr << "decrefs: ptraceback";
                if (ptraceback) {
                  // Apparently this can come back null!
                  Py_DECREF(ptraceback);
                }
                std::cerr << "some decrefs complete";
	}
}
