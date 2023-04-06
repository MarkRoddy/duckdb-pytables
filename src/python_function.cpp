#include <python_function.hpp>
#include <stdexcept>

PythonFunction::PythonFunction(const std::string& module_name, const std::string& function_name)
  : module_name(module_name), function_name(function_name), module(nullptr), function(nullptr)
{
  PyObject* module_obj = PyImport_ImportModule(module_name.c_str());
  if (!module_obj) {
    PyErr_Print();
    throw std::runtime_error("Failed to import module: " + module_name);
  }

  module = module_obj;

  PyObject* function_obj = PyObject_GetAttrString(module, function_name.c_str());
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

PythonFunction::~PythonFunction()
{
  Py_DECREF(function);
  Py_DECREF(module);
}

std::pair<PyObject*, PythonFunctionError*> PythonFunction::call(PyObject* args) const
{
  PyObject* result = PyObject_CallObject(function, args);

  if (result == nullptr) {
    PythonFunctionError* error = new PythonFunctionError();
    error->message = "";
    error->traceback = "";

    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);

    if (pvalue != nullptr) {
      PyObject* py_type_str = PyObject_Str(ptype);
      PyObject* py_value_str = PyObject_Str(pvalue);
      PyObject* py_traceback_str = PyObject_Str(ptraceback);

      error->message = PyUnicode_AsUTF8(py_value_str);
      error->traceback = PyUnicode_AsUTF8(py_traceback_str);

      Py_DECREF(py_type_str);
      Py_DECREF(py_value_str);
      Py_DECREF(py_traceback_str);

      Py_DECREF(ptype);
      Py_DECREF(pvalue);
      Py_DECREF(ptraceback);
    }

    return std::make_pair(nullptr, error);
  }

  return std::make_pair(result, nullptr);
}
