#define DUCKDB_EXTENSION_MAIN

#include "python_udf_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"


#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

namespace duckdb {

inline void Python_udfScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) { 
			return StringVector::AddString(result, "Python_udf "+name.GetString()+" 🐥");;
        });
}

static void LoadInternal(DatabaseInstance &instance) {
	Connection con(instance);
    con.BeginTransaction();

    auto &catalog = Catalog::GetSystemCatalog(*con.context);

    CreateScalarFunctionInfo python_udf_fun_info(
            ScalarFunction("python_udf", {LogicalType::VARCHAR}, LogicalType::VARCHAR, Python_udfScalarFun));
    python_udf_fun_info.on_conflict = OnCreateConflict::ALTER_ON_CONFLICT;
    catalog.CreateFunction(*con.context, &python_udf_fun_info);
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
