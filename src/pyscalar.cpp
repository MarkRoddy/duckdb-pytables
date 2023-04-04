#include "duckdb.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include <Python.h>
#include <string>
#include <iostream>

namespace pyudf {
std::string executePythonFunction(const std::string &module_name, const std::string &function_name,
                                  const std::string &argument) {

	// Import the module and retrieve the function object
	PyObject *module = PyImport_ImportModule(module_name.c_str());
	if (NULL == module) {
		throw std::invalid_argument("No such module: " + module_name);
	}

	PyObject *py_function_name = PyUnicode_FromString(function_name.c_str());
	int has_attr = PyObject_HasAttr(module, py_function_name);
	if (0 == has_attr) {
		throw std::invalid_argument("No such function: " + function_name);
	}
	PyObject *function = PyObject_GetAttrString(module, function_name.c_str());

	/*
	  todo: can we (should we) cache the above function lookup so it doesn't need to happen
	  on a per invocation basis.
	*/

	// Create the argument tuple
	PyObject *arg = Py_BuildValue("(s)", argument.c_str());

	// Call the function with the argument and retrieve the result
	PyObject *result = PyObject_CallObject(function, arg);

	// char* value_c = PyStr_AsString(result);
	const char *value_c = PyUnicode_AsUTF8(result);

	std::string value(value_c);
	// Convert the result to a double
	// double value = PyFloat_AsDouble(result);

	// Clean up and return the result
	Py_XDECREF(result);
	Py_XDECREF(arg);
	Py_XDECREF(function);
	Py_XDECREF(module);

	// Should be done at shutdown time. When exactly does that happen?
	// Py_Finalize();

	return value;
}

} // namespace pyudf
