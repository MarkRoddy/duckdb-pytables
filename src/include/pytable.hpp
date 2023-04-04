
#include <duckdb.hpp>
#include <duckdb/parser/parsed_data/create_table_function_info.hpp>
#include <duckdb/parser/tableref/table_function_ref.hpp>

namespace pyudf {
duckdb::unique_ptr<duckdb::CreateTableFunctionInfo> GetPythonTableFunction();
}
