#ifndef PYTHONFUNCTION_H
#define PYTHONFUNCTION_H

#include <Python.h>
#include <string>
#include <utility>
#include <iostream>

class PythonFunctionError {
public:
  std::string message;
  std::string traceback;
  void print_error() {
    // PyErr_Print();
    std::cerr << message;
  }
};

class PythonFunction {
public:
  PythonFunction(const std::string& module_name, const std::string& function_name);

  ~PythonFunction();

  std::pair<PyObject*, PythonFunctionError*> call(PyObject* args) const;

private:
  std::string module_name;
  std::string function_name;
  PyObject* module;
  PyObject* function;
};

#endif // PYTHONFUNCTION_H
