// Wrapper for defered call to a function that returns an interator

#include <pydeferred_function_call.hpp>
#include <pybind11/pybind11.h>

namespace py = pybind11;
namespace pyudf {

  void PyDeferredFunctionCall::init(py::handle function, py::tuple args, py::dict kwargs) {
    function_ = function;
    args_ = args;
    kwargs_ = kwargs;
  }

  PyDeferredFunctionCall::PyDeferredFunctionCall(py::handle function, py::tuple args, py::dict kwargs) {
    init(function, args, kwargs);
  }

  PyDeferredFunctionCall::PyDeferredFunctionCall(std::string module_name, std::string func_name, py::tuple args, py::dict kwargs) {
    auto module = py::module::import(module_name.c_str());
    auto function = module.attr(func_name.c_str());
    init(function, args, kwargs);
  }
    
    class ResultIterator {
      ResultIterator(PyDeferredFunctionCall function) {
        func = function
      }
      ResultIterator(PyDeferredFunctionCall function, py::iterator result_) {
        func = function
        result = result_;
      }
      py::handle operator*() {
        return *result;
      }
      ResultIterator& operator++() {
        auto r = result++;
        return ResultIterator(r);
      }

      friend bool operator==(const ResultIterator& a, const ResultIterator& b) {
        if (!(a.isSentinal() || b.isSentinal())) {
          // neither are the end sentinal, do a normal comparison
          return ((a.func == b.func) && (a.result == b.result));
        } else {
          // One of them is a sentinal, logical equivalency to being equal to the
          // end state sentinal is to check if the non-sentinal is exhausted.
          py::iterator end = py::iterator::sentinel();
          if (a.isSentinal()) {
            return (b.result == end);
          } else {
            return (a.result == end);
          }
      }
      
      friend bool operator!=(const ResultIterator& a, const ResultIterator& b) {
        return (!(a == b));
      }

      private:
      py::iterator result;
      PyDeferredFunctionCall func;
      void ensure_called() {
        if(!result) {
          result = func.call();
        }
      }
      bool isSentinal() {
        return (!func)
      }
    }
      ResultIterator PyDeferredFunctionCall::begin() {
      return ResultIterator(this);
    }
      ResultIterator PyDeferredFunctionCall::end() {
      return ResultIterator(nullptr);
    }

    // private:
      // py::handle function_;
      // py::tuple args_;
      // py::dict kwargs_;
      py::iterator PyDeferredFunctionCall::call() {
        py::handle result = function_(*args_, **kwargs);
        if (!py::isinstance(result, py::iterable)) {
        // todo: a better exception type?
          throw std::runtime_error("Input must be an iterable or an object that can be iterated upon");
        } else {
          return result;
        }
      }
    }; // class ResultIterator
      
  }; // class PyDeferredFunctionCall
} // namespace pyudf
