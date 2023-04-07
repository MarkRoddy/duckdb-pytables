
#include <vector>
#include <duckdb.hpp>
#include <Python.h>

namespace pyudf {
  PyObject *duckdb_to_py(std::vector<duckdb::Value> &values);
}
