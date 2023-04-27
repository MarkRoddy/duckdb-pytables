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
#include <typeinfo>

namespace duckdb {

inline void Python_udfScalarFun(duckdb::DataChunk &args, duckdb::ExpressionState &state, duckdb::Vector &result) {
  // Each of these variables is of type duckdb::Vector I believe. The DataChunk class defines its 'data' field as
  // type: vector<Vector>
	auto &module_vector = args.data[0];
	auto &func_vector = args.data[1];
	auto &arg_vector = args.data[2];

        auto lambda = 	    [&](duckdb::string_t module_name, duckdb::string_t func_name, duckdb::Vector &list_vector) {
                              auto lists_size = ListVector::GetListSize(list_vector);
                              // auto &child_vector = ListVector::GetEntry(child_vector);
                              // child_vector.Flatten(lists_size);

                              // auto arguments = duckdb::ListValue::GetChildren(list_vector);
                              // std::cerr << "I'm gonna begin...";
                              // auto iter = list_vector.begin();
                              // std::cerr << "Did I get to begin?";
                              // for (iter; iter < list_vector.end(); iter++) {
                                // std::cerr << duckdb::StringValue::Get(*iter);
                                // std::cerr << "I got a value";
                              // }


                              // const std::type_info& originalType = typeid(list_vector);

                              // Print the name of the original type
                              // std::cout << "Original type: " << originalType.name() << std::endl;
                              
                              //SELECT python_udf('udfs', 'reverse', ['Sam']);
                              // Value Vector::GetValueInternal(const Vector &v_p, idx_t index_p) {
                              // for (idx_t i = 0; i < ListVector::GetListSize(list_vector); i++) {
                                // auto &val = Vector::GetValue(list_vector, i);
                                // std::cerr << std::to_string(val);
                              // }
                              // std::cerr << list_vector.ToString();
                              // for (size_t i = 0; i < list_vector.child_count(); i++) {
                              // for (size_t i = 0; i < list_vector.size(); i++) {
                              // for (size_t i = 0; i < list_vector.GetListSize(); i++) {
                              // for (size_t i = 0; i < list_vector.GetChildCount(); i++) {
                              // for (size_t i = 0; i < list_vector.Size(); i++) {
                              //   auto child_vector = list_vector.get_child_vector(i);
                              //   for (size_t j = 0; j < child_vector->size(); j++) {
                              //     std::cout << "child[" << i << "][" << j << "]: " << child_vector->GetString(j) << std::endl;
                              //   }
                              // }
                              // auto child_size = ListVector::GetListSize(input);
                              // auto args = duckdb::ListValue::GetChildren(list_vector);
                              // 	auto iter = args.begin();
                              //   for (iter; iter < args.end(); iter++) {
                              //     // names.push_back(duckdb::StringValue::Get(*iter));
                              //     std::cerr << duckdb::StringValue::Get(*iter);
                              //   }
                              // std::cerr << ((Vector&)list_vector).ToString();
                              // auto lv = ((Vector&)list_vector);
                              // for (idx_t i = 0; i < ListVector::GetListSize(lv); i++) {
                                // auto &val = Vector::GetValue(lv, i);
                                // auto val = lv.GetValue(i);
                                // std::cerr << val.ToString();
                              // }
                              
		    return duckdb::StringVector::AddString(
		        result,
		        pyudf::executePythonFunction(module_name.GetString(), func_name.GetString(), ""));
                            };
	duckdb::TernaryExecutor::Execute<duckdb::string_t, duckdb::string_t, duckdb::Vector, duckdb::string_t>(
	    module_vector, func_vector, arg_vector, result, args.size(), lambda);
}

static void LoadInternal(DatabaseInstance &instance) {
	Connection con(instance);
	con.BeginTransaction();

	auto &catalog = Catalog::GetSystemCatalog(*con.context);
	auto &context = *con.context;

	CreateScalarFunctionInfo python_udf_fun_info(
	    ScalarFunction("python_udf", {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::LIST(LogicalType::VARCHAR)},
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
