
#include "duckdb.hpp"

namespace pyudf {
std::string executePythonFunction(const std::string &module_name, const std::string &function_name,
                                  const std::string &argument);

}
