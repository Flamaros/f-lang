#pragma once

#include "f_parser.hpp"

#include <filesystem>

namespace c_generator
{
    bool generate(const std::filesystem::path& output_directory_path, const std::filesystem::path& output_file_name, const f::Parsing_Result& parsing_result);
}
