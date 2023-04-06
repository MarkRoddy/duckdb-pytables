#ifndef PYTHONFUNCTION_H
#define PYTHONFUNCTION_H

#include <Python.h>
#include <string>
#include <utility>
#include <python_exception.hpp>

class PythonFunction {
public:
	PythonFunction(const std::string &module_name, const std::string &function_name);

	~PythonFunction();

	std::pair<PyObject *, PythonException*> call(PyObject *args) const;

	std::string function_name() { return function_name_; }
	std::string module_name() { return module_name_; }

private:
	std::string module_name_;
	std::string function_name_;
	PyObject *module;
	PyObject *function;
};

#endif // PYTHONFUNCTION_H
