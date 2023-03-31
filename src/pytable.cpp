
#include <duckdb.hpp>
#include <duckdb/parser/expression/constant_expression.hpp>
#include <duckdb/parser/expression/function_expression.hpp>
#include <pytable.hpp>

namespace pyudf {

struct PyScanBindData : public duckdb::TableFunctionData {
	std::string file_path;
	// klibpp::SeqStreamIn *stream;
};

struct PyScanLocalState : public duckdb::LocalTableFunctionState {
	bool done = false;
};

void PyScan(duckdb::ClientContext &context, duckdb::TableFunctionInput &data, duckdb::DataChunk &output) {
	auto bind_data = (const PyScanBindData *)data.bind_data;
	auto local_state = (PyScanLocalState *)data.local_state;

	if (local_state->done) {
		return;
	}

	return;
	// auto stream = bind_data->stream;
	// auto records = stream->read(STANDARD_VECTOR_SIZE);

	// auto read_records = 0;

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
	auto file_name = input.inputs[0].GetValue<std::string>();

	auto &fs = duckdb::FileSystem::GetFileSystem(context);

	if (!fs.FileExists(file_name)) {
		throw duckdb::IOException("File does not exist: " + file_name);
	}

	result->file_path = file_name;
	// result->stream = new klibpp::SeqStreamIn(result->file_path.c_str());
	// result->stream = NULL;

	return_types.push_back(duckdb::LogicalType::VARCHAR);
	return_types.push_back(duckdb::LogicalType::VARCHAR);
	return_types.push_back(duckdb::LogicalType::VARCHAR);

	names.push_back("id");
	names.push_back("description");
	names.push_back("sequence");

	return std::move(result);
}

struct PyScanGlobalState : public duckdb::GlobalTableFunctionState {
	PyScanGlobalState() : duckdb::GlobalTableFunctionState() {
	}
};

duckdb::unique_ptr<duckdb::GlobalTableFunctionState> PyInitGlobalState(duckdb::ClientContext &context,
                                                                       duckdb::TableFunctionInitInput &input) {
	auto result = duckdb::make_unique<PyScanGlobalState>();
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
	    "python_table", {duckdb::LogicalType::VARCHAR, duckdb::LogicalType::VARCHAR, duckdb::LogicalType::VARCHAR},
	    PyScan, PyBind, PyInitGlobalState, PyInitLocalState);

	duckdb::CreateTableFunctionInfo py_table_function_info(py_table_function);
	return duckdb::make_unique<duckdb::CreateTableFunctionInfo>(py_table_function_info);
}

} // namespace pyudf
