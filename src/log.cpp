
#include <cstdlib> // for getenv
#include <iostream>
#include <log.hpp>

namespace pyudf {
bool _debugEnabled = std::getenv("PYTABLES_DEBUG") != nullptr;
void debug(const std::string &msg) {
	if (_debugEnabled) {
		std::cerr << msg << std::endl;
	}
}
} // namespace pyudf
