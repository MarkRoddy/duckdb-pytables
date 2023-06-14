#include <config.h>
#include <duckdb.hpp>
#include <python_function.hpp>
#include <python_exception.hpp>
#include <stdexcept>
#include <typeinfo>

namespace pyudf {

PythonFunction::PythonFunction(const std::string &function_specifier) {
	std::string module_name;
	std::string function_name;
	std::tie(module_name, function_name) = parse_func_specifier(function_specifier);
	init(module_name, function_name);
}

PythonFunction::PythonFunction(const std::string &module_name, const std::string &function_name) {
	init(module_name, function_name);
}

void PythonFunction::init(const std::string &module_name, const std::string &function_name) {
	module_name_ = module_name;
	function_name_ = function_name;
	module = nullptr;
	function = nullptr;
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
        wrapped_function = wrap_function(function);
}

PythonFunction::~PythonFunction() {
        Py_DECREF(wrapped_function);
	Py_DECREF(function);
	Py_DECREF(module);
}

PyObject * PythonFunction::wrap_function(PyObject *function) {
  PyObject *decorator = import_decorator();
  if(!decorator) {
    decorator = load_internal_decorator();
  }
  if(!decorator) {
    // todo: raise error?
    return nullptr;
  }

  // First, check if the function is already wrapped using isinstance(), return it if so

  // Otherwise, go ahead and wrappe it
  PyObject *args = PyTuple_New(1);
  PyTuple_SetItem(args, 1, function);
  PyObject *wrapped_function = PyObject_CallObject(function, args);
  // todo: handle exception from wrapping
  return wrapped_function;
}

PyObject * PythonFunction::import_decorator() {
  PyObject *module_obj = PyImport_ImportModule("ducktables");
  if (!module_obj) {
    // todo: Maybe we want to report this error if it isn't just import error?
    // PyErr_Print();
    return nullptr;
  }
  
  PyObject *function_obj = PyObject_GetAttrString(module, "ducktable");
  if (!function_obj) {
    // todo: Maybe we want to report this error if it isn't just import error?
    // PyErr_Print();
    return nullptr;
  }
  return function_obj;

}

PyObject * PythonFunction::load_internal_decorator() {
// #include <Python.h>

//   PyObject* getPythonFunction(const std::string& code, const std::string& func_name) {
//     Py_InitializeEx(0);

    std::string code = PYTHON_SCHEMA_DECORATOR;
    std::string func_name = "ducktable";
    PyObject* main_module = PyImport_AddModule("__main__");  // borrowed reference
    PyObject* global_dict = PyModule_GetDict(main_module);  // borrowed reference

    PyObject* pCodeObj = Py_CompileString(code.c_str(), "", Py_file_input);
    if (!pCodeObj) {
      PyErr_Print();
      return nullptr;
    }

    PyObject* result = PyEval_EvalCode(pCodeObj, global_dict, global_dict);
    Py_DECREF(pCodeObj);
    Py_XDECREF(result);  // no longer need the result, decrement its reference count

    if (PyErr_Occurred()) {
      PyErr_Print();
      return nullptr;
    }

    PyObject* func = PyDict_GetItemString(global_dict, func_name.c_str());  // borrowed reference

    if (!func || !PyCallable_Check(func)) {
      PyErr_Format(PyExc_RuntimeError, "%s is not a function", func_name.c_str());
      PyErr_Print();
      return nullptr;
    }

    Py_INCREF(func);  // caller is responsible for decrementing

    // Py_FinalizeEx();

    return func;
}
  
std::pair<PyObject *, PythonException *> PythonFunction::call(PyObject *args, PyObject *kwargs) const {
	PyObject *result = PyObject_Call(wrapped_function, args, kwargs);

	if (result == nullptr) {
		PythonException *error = new PythonException();
		return std::make_pair(nullptr, error);
	} else {
		return std::make_pair(result, nullptr);
	}
}

std::pair<PyObject *, PythonException *> PythonFunction::call(PyObject *args) const {
	PyObject *result = PyObject_CallObject(wrapped_function, args);

	if (result == nullptr) {
		PythonException *error = new PythonException();
		return std::make_pair(nullptr, error);
	} else {
		return std::make_pair(result, nullptr);
	}
}

std::pair<std::string, std::string> parse_func_specifier(std::string specifier) {
	auto delim_location = specifier.find(":");
	if (delim_location == std::string::npos) {
		throw duckdb::InvalidInputException("Function specifier lacks a ':' to delineate module and function");
	} else {
		auto module = specifier.substr(0, delim_location);
		auto function = specifier.substr(delim_location + 1, (specifier.length() - delim_location));
		return {module, function};
	}
}

} // namespace pyudf
