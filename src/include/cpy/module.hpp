
#include <string>
#include <Python.h>
#include <cpy/object.hpp>

namespace cpy {
  class Module : public cpy::Object {

  public:
    Module(const std::string &module_name);
    cpy::Object getattr(const std::string &attribute_name);

  private:
    std::string module_name_;
    static PyObject* doImport(const std::string &module_name);
  };

} // namespace cpy
