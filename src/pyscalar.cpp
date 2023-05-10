#include "duckdb.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include <Python.h>
#include <string>
#include <iostream>
#include "python_function.hpp"
#include "pyconvert.hpp"

using namespace duckdb;
namespace pyudf {

static void PyScalarFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	for (idx_t row = 0; row < args.size(); row++) {
		// Grab the FunctionSpecifier argument. In practice this is almost always going
		// to be constants, but in theory they could be column values.
		auto &funcspec_column = args.data[0];
		auto funcspec_value = funcspec_column.GetValue(row).GetValue<std::string>();
		auto func = PythonFunction(funcspec_value);

		std::vector<duckdb::Value> duck_args;

		for (idx_t i = 1; i < args.ColumnCount(); i++) {
			// Fill the tuple with the arguments for this row
			auto &column = args.data[i];
			auto value = column.GetValue(row);
			duck_args.emplace_back(value);
		}
		auto pyargs = duckdb_to_py(duck_args);
		PyObject *pyresult;
		PythonException *error;
		std::tie(pyresult, error) = func.call(pyargs);
		if (!pyresult) {
			Py_XDECREF(pyresult);
			Py_XDECREF(pyargs);
			std::string err = error->message;
			error->~PythonException();
			throw std::runtime_error(err);
		} else {
			auto ddb_result = ConvertPyObjectToDuckDBValue(pyresult, duckdb::LogicalTypeId::VARCHAR);
			result.SetValue(row, ddb_result);
			Py_XDECREF(pyargs);
			Py_XDECREF(pyresult);
		}
	}
}

CreateScalarFunctionInfo GetPythonScalarFunction() {
	auto scalar_func = ScalarFunction("python_udf", {LogicalType::VARCHAR, LogicalType::VARCHAR}, LogicalType::VARCHAR,
	                                  PyScalarFunction);
	scalar_func.varargs = LogicalType::ANY;
	CreateScalarFunctionInfo py_scalar_function_info(scalar_func);
	return CreateScalarFunctionInfo(py_scalar_function_info);
}
} // namespace pyudf
