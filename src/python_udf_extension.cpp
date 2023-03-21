#define DUCKDB_EXTENSION_MAIN

#include "python_udf_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"


#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

#include <Python.h>
#include <string>
#include <iostream>

std::string executePythonFunction(const std::string& module_name, const std::string& function_name, const std::string& argument) {

  std::cerr << "Intializing the interpreter\n";
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

  /*
    todo: all above needs to be run at extention initialization time. Kind of rediculus to be
    creating an interpret for *every* invocation.
  */

  
  
  // Import the module and retrieve the function object
  std::cerr << "Grabbing the module\n";
  PyObject* module = PyImport_ImportModule(module_name.c_str());
  if ( NULL == module) {
    throw std::invalid_argument("No such module: " + module_name);
  } else {
    std::cerr << "I found a module...\n";
  }
  std::cerr << "Conberting the func name to a python value";
  PyObject* py_function_name = PyUnicode_FromString(function_name.c_str());
  std::cerr << "Checking if the module as the function";
  int has_attr = PyObject_HasAttr(module, py_function_name);
  if (0 == has_attr) {
    std::cerr << "Function not in module";
    throw std::invalid_argument("No such function: " + function_name);
  } else {
    std::cerr << "Function is on the module";
  }  
  PyObject* function = PyObject_GetAttrString(module, function_name.c_str());

  /*
    todo: can we (should we) cache the above function lookup so it doesn't need to happen
    on a per invocation basis.
  */


  // Create the argument tuple
  PyObject* arg = Py_BuildValue("(s)", argument.c_str());
  
  // Call the function with the argument and retrieve the result
  PyObject* result = PyObject_CallObject(function, arg);
  
  // char* value_c = PyStr_AsString(result);
  const char* value_c = PyUnicode_AsUTF8(result);

  std::string value(value_c);
  // Convert the result to a double
  // double value = PyFloat_AsDouble(result);

  // Clean up and return the result
  Py_XDECREF(result);
  Py_XDECREF(arg);
  Py_XDECREF(function);
  Py_XDECREF(module);
  Py_Finalize();

  return value;
}


namespace duckdb {

inline void Python_udfScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
  auto &name_vector = args.data[0];
  UnaryExecutor::Execute<string_t, string_t>(
                                             name_vector, result, args.size(),
                                             [&](string_t name) {
                                               return StringVector::AddString(result, executePythonFunction("udfs", "reverse", name.GetString()));
                                               // return executePythonFunction(
                                               // return StringVector::AddString(result, "Python_udf "+name.GetString()+" 🐥");;
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
