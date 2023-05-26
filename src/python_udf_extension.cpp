#define DUCKDB_EXTENSION_MAIN

#include <dlfcn.h>
#include <iostream>
#include "config.h"
#include <Python.h>
#include "pyscalar.hpp"
#include "pytable.hpp"
#include "python_udf_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"

#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include <typeinfo>

namespace duckdb {

static void LoadInternal(DatabaseInstance &instance) {
	Connection con(instance);
	con.BeginTransaction();

	auto &catalog = Catalog::GetSystemCatalog(*con.context);
	auto &context = *con.context;

	auto python_scalar = pyudf::GetPythonScalarFunction();
	// python_udf_fun_info.on_conflict = OnCreateConflict::ALTER_ON_CONFLICT;
	catalog.CreateFunction(*con.context, python_scalar);

	// pyudf::GetPythonTableFunction();
	auto python_table = pyudf::GetPythonTableFunction();
	catalog.CreateTableFunction(context, python_table.get());

	// Initialize the Python interpreter
	Py_Initialize();

	const char *clibpath = std::getenv("LIBPYTHONSO_PATH");
	if (!clibpath) {
		clibpath = PYTHON_LIB_NAME;
	}
	void *libpython = dlopen(clibpath, RTLD_NOW | RTLD_GLOBAL);
	if (!libpython) {
		std::cerr << "Failed to dyanmically load your libpython shared library: " << PYTHON_LIB_NAME
		          << ". You may see errors about missing symbols." << std::endl;
		auto errMsg = dlerror();
		std::cerr << "Error Details: " << errMsg << std::endl;
	}
	con.Commit();
}

void Python_udfExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string Python_udfExtension::Name() {
	return "python_udf";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void python_udf_init(duckdb::DatabaseInstance &db) {
	LoadInternal(db);
}

DUCKDB_EXTENSION_API const char *python_udf_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
