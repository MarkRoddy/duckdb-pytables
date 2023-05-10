
#include <vector>
#include <duckdb.hpp>
#include <Python.h>

namespace pyudf {
  PyObject *duckdb_to_py(std::vector<duckdb::Value> &values);

  void Py_DecRefTuple(PyObject* tpl);
  duckdb::Value ConvertPyObjectToDuckDBValue(PyObject *py_item, duckdb::LogicalType logical_type);
}
