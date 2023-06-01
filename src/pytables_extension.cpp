#define DUCKDB_EXTENSION_MAIN

#include <dlfcn.h>
#include <iostream>
#include "config.h"
#include <Python.h>
#include "pyscalar.hpp"
#include "pytable.hpp"
#include "pytables_extension.hpp"
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
	// pytables_fun_info.on_conflict = OnCreateConflict::ALTER_ON_CONFLICT;
	catalog.CreateFunction(*con.context, python_scalar);

	// pyudf::GetPythonTableFunction();
	auto python_table = pyudf::GetPythonTableFunction();
	catalog.CreateTableFunction(context, python_table.get());

	// Initialize the Python interpreter
	Py_Initialize();

	// Python C Extensions will encounter errors about missing symbols unless
	// we eplicitly load the entire contents of the shared library. We do this
	// with the dlopen() function which takes the path to the shared library. This
	// can create some issues! Most notably where is the library and/or which one
	// should we load. Our strategy at this time is to check if the user has
	// supplied us a path to the file via an environment variable, look up the path
	// via reflection on one of the preloaded symbols, and finally to "guess" the
	// file name via some heuristics.
	const char *libpath;
	libpath = std::getenv("LIBPYTHONSO_PATH");
	if (!libpath) {
		// No env variable. Try examining a preloaded symbol.
		Dl_info info;
		if ((dladdr((void *)Py_Initialize, &info)) && (info.dli_fname)) {
			libpath = info.dli_fname;
		} else {
			// Issue doing symbol lookup, fallback to our "guess"
			libpath = PYTHON_LIB_NAME;
		}
	}
	void *libpython = dlopen(libpath, RTLD_NOW | RTLD_GLOBAL);
	if (!libpython) {
		std::cerr << "Failed to dyanmically load your libpython shared library: " << PYTHON_LIB_NAME
		          << ". You may see errors about missing symbols." << std::endl;
		auto errMsg = dlerror();
		std::cerr << "Error Details: " << errMsg << std::endl;
	}
	con.Commit();
}

void PytablesExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string PytablesExtension::Name() {
	return "pytables";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void pytables_init(duckdb::DatabaseInstance &db) {
	LoadInternal(db);
}

DUCKDB_EXTENSION_API const char *pytables_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
