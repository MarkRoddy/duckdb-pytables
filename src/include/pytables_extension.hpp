#pragma once

#include "pyscalar.hpp"
#include "duckdb.hpp"

namespace duckdb {

class PytablesExtension : public Extension {
public:
	void Load(DuckDB &db) override;
	std::string Name() override;
};

} // namespace duckdb
