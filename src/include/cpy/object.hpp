#include <Python.h>

namespace cpy {
  class Object {
  public:
    Object(PyObject *obj);
    Object(PyObject *obj, bool assumeOwnership);
    ~Object();
    Object(const Object&); // copy
    Object(Object&&); // move
    PyObject* getpy();
    bool isinstance(cpy::Object cls);
    bool isinstance(PyObject* cls);
    cpy::Object iter();
  protected:
    PyObject *obj;
  };
} // namespace cpy
