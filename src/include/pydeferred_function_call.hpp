// Wrapper for defered call to a function that returns an interator

#include <pybind11/pybind11.h>

namespace py = pybind11;
namespace pyudf {
  class PyDeferredFunctionCall {
    PyDeferredFunctionCall(py::handle function, py::tuple args, py::dict kwargs);
    PyDeferredFunctionCall(std::string module_name, std::string func_name, py::tuple args, py::dict kwargs);
    class ResultIterator {
      py::handle operator*();
      ResultIterator& operator++();
      friend bool operator!=(const ResultIterator& a, const ResultIterator& b);
    };
    ResultIterator begin();
    ResultIterator end();
  private:
    py::handle function_;
    py::tuple args_;
    py::dict kwargs_;
    py::iterator call();

  };
}
