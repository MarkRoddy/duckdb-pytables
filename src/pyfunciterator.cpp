#include <pybind11/pybind11.h>
#include <queue>

namespace py = pybind11;




class PyFuncIterator {
  py::handle py_module;
  py::handle py_function;
  py::handle py_args;
  py::handle py_kwargs;

  
  py::iterator py_iterator;
  std::queue<py::handle> peak_queue;
  bool py_iterator_exhausted = false;

public:
  PyFuncIterator(std::string module_name, std::string func_name,
                 py::tuple args, py::dict kwargs)
    : py_args(args.ptr()), py_kwargs(kwargs.ptr())
  {
    py_module = py::module::import(module_name.c_str());
    py_function = py_module.attr(func_name.c_str());
  }

  std::vector<py::handle> peak(int num) {
    ensure_func_called();
    std::vector<py::handle> peaks;
    for (int i = 0; i < num; ++i) {
      if (iterator_exhausted) break;
      if (!py_iterator.check()) {
        iterator_exhausted = true;
        break;
      }
      peak_queue.push(*py_iterator);
      ++py_iterator;
    }
    while (!peak_queue.empty()) {
      peaks.push_back(peak_queue.front());
      peak_queue.pop();
    }
    return peaks;
  }

  PyFuncIterator& operator++() {
    ensure_func_called();
    if (!iterator_exhausted && py_iterator.check()) {
      ++py_iterator;
    } else {
      iterator_exhausted = true;
    }
    return *this;
  }

  py::handle operator*() {
    ensure_func_called();
    if (iterator_exhausted) {
      throw std::out_of_range("Iterator exhausted");
    }
    return *py_iterator;
  }

  // /home/mark/development/duckdb-python-udf/src/pytable.cpp:77:74: error: no match for ‘operator!=’ (operand types are ‘pyudf::PyFuncIterator’ and ‘pybind11::iterator’)
  // boolean operator!=(py::iterator it) {
  //   return it == 
  // }
  friend bool operator!=(const PyFuncIterator& a, const PyFuncIterator& b) {
    return a != b;
    // TODO: Implement
    // return false;
  }

  PyFuncIterator
private:
  void ensure_func_called() {
    if (!py_iterator) {
      py::object result = py_function(*py::reinterpret_borrow<py::tuple>(py_args),
                                      **py::reinterpret_borrow<py::dict>(py_kwargs));
      py_iterator = py::make_iterator(result);
    }
  }
};
