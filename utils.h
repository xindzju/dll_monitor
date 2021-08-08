#include "cpp_utils.h"
#include "fmt/format.h"
#include <set>

struct Options {
	std::string processName;
	std::string dllName;
	bool showRunningProcess;
	bool removeDuplicateName;
	bool killAll;
};