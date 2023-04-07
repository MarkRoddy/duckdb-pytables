#include <python_function.hpp>
#include <python_exception.hpp>
#include <stdexcept>

PythonFunction::PythonFunction(const std::string &module_name, const std::string &function_name)
    : module_name_(module_name), function_name_(function_name), module(nullptr), function(nullptr) {
	PyObject *module_obj = PyImport_ImportModule(module_name.c_str());
	if (!module_obj) {
		PyErr_Print();
		throw std::runtime_error("Failed to import module: " + module_name);
	}

	module = module_obj;

	PyObject *function_obj = PyObject_GetAttrString(module, function_name.c_str());
	if (!function_obj) {
		PyErr_Print();
		throw std::runtime_error("Failed to find function: " + function_name);
	}

	if (!PyCallable_Check(function_obj)) {
		Py_DECREF(function_obj);
		throw std::runtime_error("Function is not callable: " + function_name);
	}

	function = function_obj;
}

PythonFunction::~PythonFunction() {
	Py_DECREF(function);
	Py_DECREF(module);
}

std::pair<PyObject *, PythonException *> PythonFunction::call(PyObject *args) const {
	PyObject *result = PyObject_CallObject(function, args);

	if (result == nullptr) {
		PythonException *error = new PythonException();
		return std::make_pair(nullptr, error);
	} else {
		return std::make_pair(result, nullptr);
	}
}
