// Wrapper for defered call to a function that returns an interator


namespace pyudf {
  class DeferredFunctionCall {
    PyIterableFunction(py::handle, py::tuple args, py::dict kwargs);
    PyIterableFunction(std::string module_name, std::string func_name, py::tuple args, py::dict kwargs);
    py::iterator begin();
    py::iterator end();
  }
}
