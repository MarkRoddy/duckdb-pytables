#include "duckdb.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include <Python.h>
#include <string>
#include <iostream>
#include "python_function.hpp"

namespace pyudf {
std::string executePythonFunction(const std::string &module_name, const std::string &function_name,
                                  const std::string &argument) {
	PythonFunction func(module_name, function_name);
	PyObject *arguments = Py_BuildValue("(s)", argument.c_str());

	PyObject *retvalue;
	PythonException *error;
	std::tie(retvalue, error) = func.call(arguments);

	std::string value;
	if (!error) {
		const char *value_c = PyUnicode_AsUTF8(retvalue);
		value = std::string(value_c);
	} else {
		error->print_error();
		error->~PythonException();
	}
	Py_XDECREF(retvalue);
	Py_XDECREF(arguments);
	return value;
}

} // namespace pyudf
