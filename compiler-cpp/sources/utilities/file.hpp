#pragma once

#include <string>
#include <vector>

#include <filesystem>

namespace utilities
{
    bool    read_all_file(const std::filesystem::path& file_path, std::vector<uint8_t>& data);
	bool    read_all_file(const std::filesystem::path& file_path, std::string& data);
}
