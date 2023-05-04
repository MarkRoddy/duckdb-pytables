#define DUCKDB_EXTENSION_MAIN

#include <Python.h>
#include "pyscalar.hpp"
#include "pytable.hpp"
#include "python_udf_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"

#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

namespace duckdb {

inline void Python_udfScalarFun(duckdb::DataChunk &args, duckdb::ExpressionState &state, duckdb::Vector &result) {
	auto &module_vector = args.data[0];
	auto &func_vector = args.data[1];
	auto &arg_vector = args.data[2];

	duckdb::TernaryExecutor::Execute<duckdb::string_t, duckdb::string_t, duckdb::string_t, duckdb::string_t>(
	    module_vector, func_vector, arg_vector, result, args.size(),
	    [&](duckdb::string_t module_name, duckdb::string_t func_name, duckdb::string_t argument) {
		    return duckdb::StringVector::AddString(
		        result,
		        pyudf::executePythonFunction(module_name.GetString(), func_name.GetString(), argument.GetString()));
	    });
}

static void LoadInternal(DatabaseInstance &instance) {
	Connection con(instance);
	con.BeginTransaction();

	auto &catalog = Catalog::GetSystemCatalog(*con.context);
	auto &context = *con.context;

	CreateScalarFunctionInfo python_udf_fun_info(
	    ScalarFunction("python_udf", {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR},
	                   LogicalType::VARCHAR, Python_udfScalarFun));

	python_udf_fun_info.on_conflict = OnCreateConflict::ALTER_ON_CONFLICT;
	catalog.CreateFunction(*con.context, python_udf_fun_info);

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

	PyRun_SimpleString("import sys; sys.path.insert(0, '')\n");
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
