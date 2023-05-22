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

	/*
	 Somewhat of  a hack to add CWD to the module search path. This is going to be a
	 behavior most people expect (or at least I expected). Note the docs that explain
	 module search path setup give some light on why they shouldn't be expected when
	 embedding the interpreter:
	 The first entry in the module search path is the directory that contains
	 the input script, if there is one. Otherwise, the first entry is the current
	 directory, which is the case when executing the interactive shell, a -c
	 command, or -m module.
	*/

	// PyRun_SimpleString("import sys; sys.path.insert(0, '')\n");

	// todo: auto populate this against the version of libpython we're building against
	void *libpython = dlopen(PYTHON_LIB_NAME, RTLD_NOW | RTLD_GLOBAL);
	if (!libpython) {
		// todo: user is in a bad shape if something goes wrong here. Give a useful message.
		std::cerr << "Failed to dyanmically load your libpython shared library: PYTHON_LIB_NAME. You may see errors "
		             "about missing symbols."
		          << std::endl;
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
