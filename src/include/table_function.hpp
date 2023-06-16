
#include "python_function.hpp"
#include <Python.h>
#include <string>
#include <vector>
#include <utility>
#include <duckdb.hpp>

namespace pyudf {
class TableFunction : public PythonFunction {
public:
	TableFunction(const std::string &function_specifier);
	TableFunction(const std::string &module_name, const std::string &function_name);
	std::vector<std::string> column_names(PyObject *args, PyObject *kwargs);
	std::vector<duckdb::LogicalType> column_types(PyObject *args, PyObject *kwargs);

private:
	std::vector<PyObject *> pycolumn_types(PyObject *args, PyObject *kwargs);
	PyObject *wrap_function(PyObject *function);
	PyObject *import_decorator();
};

} // namespace pyudf
