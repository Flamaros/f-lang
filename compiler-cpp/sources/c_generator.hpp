#pragma once

#include "f_parser.hpp"

#include <filesystem>

namespace c_generator
{
    bool generate(const std::filesystem::path& output_filepath, const f::Parsing_Result& parsing_result);
}
