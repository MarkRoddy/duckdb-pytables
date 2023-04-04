
#include <Python.h>
#include <iostream>
#include <duckdb.hpp>
#include <duckdb/parser/expression/constant_expression.hpp>
#include <duckdb/parser/expression/function_expression.hpp>
#include <pytable.hpp>

namespace pyudf {

struct PyScanBindData : public duckdb::TableFunctionData {
  std::string module_name;
  std::string function_name;
  // A python function object
  PyObject* function;

	// klibpp::SeqStreamIn *stream;
};

struct PyScanLocalState : public duckdb::LocalTableFunctionState {
	bool done = false;
};

struct PyScanGlobalState : public duckdb::GlobalTableFunctionState {
	PyScanGlobalState() : duckdb::GlobalTableFunctionState() {
	}  
};

PyObject* get_python_function(std::string module_name, std::string function_name) {
    // Import the specified module
  PyObject* module = PyImport_ImportModule(module_name.c_str());
  if (!module) {
    std::cerr << "Error: could not import module '" << module_name << "'\n";
    return NULL;
  }

  // Get the specified function from the module
  PyObject* function = PyObject_GetAttrString(module, function_name.c_str());
  if (!function) {
    std::cerr << "Error: could not find function '" << function_name << "' in module '" << module_name << "'\n";
    return NULL;
  } else if (!PyCallable_Check(function)) {
    std::cerr << "Error: Function'" << function_name << "' in module '" << module_name << " is not a callable object'\n";
    return NULL;
  } else {
    return function;
  }
    
}
  
void PyScan(duckdb::ClientContext &context, duckdb::TableFunctionInput &data, duckdb::DataChunk &output) {
	auto bind_data = (const PyScanBindData *)data.bind_data;
	auto local_state = (PyScanLocalState *)data.local_state;

	if (local_state->done) {
		return;
	}

	// return;
	// auto stream = bind_data->stream;
	// auto records = stream->read(STANDARD_VECTOR_SIZE);

	// auto read_records = 0;
        std::string phrase = "a very long string";
        for (int i = 0; i < phrase.length(); i++) {
          std::string phrase_record = phrase.substr(i, 1);
          output.SetValue(0, output.size(), duckdb::Value(phrase_record));
          output.SetCardinality(output.size() + 1);
        }
        local_state->done = true;


	// for (auto &record : records)
	//   {
	//     output.SetValue(0, output.size(), duckdb::Value(record.name));

	//     if (record.comment.empty())
	//       {
	//         output.SetValue(1, output.size(), duckdb::Value());
	//       }
	//     else
	//       {
	//         output.SetValue(1, output.size(), duckdb::Value(record.comment));
	//       }

	//     output.SetValue(2, output.size(), duckdb::Value(record.seq));

	//     output.SetCardinality(output.size() + 1);

	//     read_records++;
	//   }

	// if (read_records < STANDARD_VECTOR_SIZE)
	//   {
	//     local_state->done = true;
	//   }
}

duckdb::unique_ptr<duckdb::FunctionData> PyBind(duckdb::ClientContext &context, duckdb::TableFunctionBindInput &input,
                                                std::vector<duckdb::LogicalType> &return_types,
                                                std::vector<std::string> &names) {
	auto result = duckdb::make_unique<PyScanBindData>();
	auto module_name = input.inputs[0].GetValue<std::string>();
        auto function_name = input.inputs[1].GetValue<std::string>();
        auto column_names = duckdb::ListValue::GetChildren(input.inputs[2]);

        result->module_name = module_name;
        result->function_name = function_name;
        result->function = get_python_function(module_name, function_name);
        if (NULL == result->function) {
          throw duckdb::IOException("Failed to load function");
        }
        
        auto iter = column_names.begin();
        for(iter; iter < column_names.end(); iter++) {
          // todo: (optionally?) source the schema from the function, maybe the column names too?
          return_types.push_back(duckdb::LogicalType::VARCHAR);
        }

        iter = column_names.begin();
        for(iter; iter < column_names.end(); iter++) {
          names.push_back(duckdb::StringValue::Get(*iter));
        }

	return std::move(result);
}

duckdb::unique_ptr<duckdb::GlobalTableFunctionState> PyInitGlobalState(duckdb::ClientContext &context,
                                                                       duckdb::TableFunctionInitInput &input) {
	auto result = duckdb::make_unique<PyScanGlobalState>();
        // result.function = get_python_function()
	return std::move(result);
}

duckdb::unique_ptr<duckdb::LocalTableFunctionState> PyInitLocalState(duckdb::ExecutionContext &context,
                                                                     duckdb::TableFunctionInitInput &input,
                                                                     duckdb::GlobalTableFunctionState *global_state) {
	auto bind_data = (const PyScanBindData *)input.bind_data;
	auto &gstate = (PyScanGlobalState &)*global_state;

	auto local_state = duckdb::make_unique<PyScanLocalState>();

	return std::move(local_state);
}

duckdb::unique_ptr<duckdb::CreateTableFunctionInfo> GetPythonTableFunction() {
	// auto fasta_table_function = duckdb::TableFunction("read_fasta", {duckdb::LogicalType::VARCHAR}, FastaScan,
	// FastaBind, FastaInitGlobalState, FastaInitLocalState); duckdb::CreateTableFunctionInfo
	// fasta_table_function_info(fasta_table_function); return
	// duckdb::make_unique<duckdb::CreateTableFunctionInfo>(fasta_table_function_info);
	auto py_table_function = duckdb::TableFunction(
                                                       "python_table",
                                                       {duckdb::LogicalType::VARCHAR, duckdb::LogicalType::VARCHAR, duckdb::LogicalType::LIST(duckdb::LogicalType::VARCHAR)},
	    PyScan, PyBind, PyInitGlobalState, PyInitLocalState);

	duckdb::CreateTableFunctionInfo py_table_function_info(py_table_function);
	return duckdb::make_unique<duckdb::CreateTableFunctionInfo>(py_table_function_info);
}

} // namespace pyudf
