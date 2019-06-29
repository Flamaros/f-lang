#include "f-lang.hpp"

#include "macro_tokenizer.hpp"
#include "macro_parser.hpp"

#include "utilities.hpp"

#include <algorithm>	// std::transform std::to_lower
#include <fstream>
#include <iostream>		// std::cout
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

