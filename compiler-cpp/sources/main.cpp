#include "f-lang.hpp"

#include "utilities.hpp"

#include <iostream>
#include <filesystem>

#include <fstream>

#include <Windows.h>

namespace fs = std::filesystem;

int main(int ac, char** av)
{

	fs::path		binary_path = "hello_world.exe";
	std::ofstream	binary;

	binary.open(binary_path, std::ofstream::binary);
	if (binary.is_open() == false) {
		std::cout << "Failed to open " << binary_path << std::endl;
	}

	IMAGE_DOS_HEADER	image_dos_header = {};

	image_dos_header.e_magic = (WORD)'M' | ((WORD)'Z' << 8);	// 'MZ' @Warning take care of the endianness
	image_dos_header.e_lfanew = sizeof(image_dos_header);		// Offset to the image_nt_header

	IMAGE_NT_HEADERS	image_nt_header = {};

	image_nt_header.Signature = (WORD)'P' | ((WORD)'E' << 8);	// 'PE\0\0' @Warning take care of the endianness
//	image_nt_header.FileHeader.Machine = ;
//	image_nt_header.OptionalHeader = nullptr;

	binary.write((const char*)&image_dos_header, sizeof(image_dos_header));
	binary.write((const char*)&image_nt_header, sizeof(image_nt_header));

	return 0;
}
