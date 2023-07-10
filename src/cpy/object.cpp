
#include <Python.h>
#include <stdexcept>
#include <cpy/object.hpp>

namespace cpy {

  PyObject* Object::getpy() {
    Py_INCREF(obj);
    return obj;
  }
  
  Object::Object(PyObject *obj) : obj(obj) {
    Py_INCREF(obj);
  }

  Object::Object(PyObject* obj, bool assumeOwnership) : obj(obj) {
    if(!assumeOwnership) {
      Py_INCREF(obj);
    }
  }
  
  Object::~Object() {
    if(obj) {
      Py_DECREF(obj);
    }
  }

  // copy
  Object::Object(const Object &other) : obj(other.obj) {
    Py_INCREF(obj);
  }

  // Move
  Object::Object(Object &&other) : obj(other.obj) {
    other.obj = nullptr;
  }

  bool Object::isinstance(cpy::Object cls) {
    PyObject* clsptr = cls.getpy();
    bool is_cls = isinstance(clsptr);
    Py_DECREF(clsptr);
    return is_cls;
  }

  bool Object::isinstance(PyObject* cls) {
    int is_cls = PyObject_IsInstance(obj, cls);
    return is_cls;
  }
  
  cpy::Object Object::iter() {
    PyObject *py_iter = PyObject_GetIter(obj);
    if(!py_iter) {
      // todo: include python error
      throw std::runtime_error("Failed to convert to an iterator");
    } else {
      cpy::Object itrobj(py_iter);
      Py_DECREF(py_iter);
      return itrobj;
    }
  }
} // namespace cpy
