#include <pybind11/pybind11.h>
#include <queue>

namespace py = pybind11;

namespace pyudf {
class PyFuncIterator {
  py::handle py_module;
  py::handle py_function;
  py::handle py_args;
  py::handle py_kwargs;
  py::iterator py_iterator;
  std::queue<py::handle> peak_queue;
public:
  PyFuncIterator(std::string module_name, std::string func_name, py::tuple args, py::dict kwargs);
  py::handle peak(int num);
  py::handle begin();
  py::handle end();
  py::handle get_next();
};
} // namespace pyudf
