#include "f-lang.hpp"

#include "utilities.hpp"

#include <iostream>
#include <filesystem>

#include <fstream>

#include <Windows.h>

#include <time.h>
#include <assert.h>

namespace fs = std::filesystem;

constexpr DWORD	image_base = 0x00400000;
constexpr DWORD	section_alignment = 4;	// @Warning should be greater or equal to file_alignment
constexpr DWORD	file_alignment = 4;		// @TODO check that because the default value according to the official documentation is 512
constexpr WORD	major_image_version = 1;
constexpr WORD	minor_image_version = 0;
constexpr DWORD size_of_stack_reserve = 0x100000;	// @Warning same flags as Visual Studio 2019
constexpr DWORD size_of_stack_commit = 0x1000;
constexpr DWORD size_of_heap_reserve = 0x100000;
constexpr DWORD size_of_heap_commit = 0x1000;

// @TODO variables that have to be computed at run time
DWORD	size_of_code = 0;
DWORD	size_of_initialized_data = 0;
DWORD	size_of_uninitialized_data = 0;
DWORD	address_of_entry_point = 0;	// Relative to image_base
DWORD	base_of_code = 0;			// Relative to image_base
DWORD	base_of_data = 0;			// Relative to image_base
DWORD	size_of_image = 0;			// @Warning must be a multiple of section_alignment
DWORD	size_of_headers = 0;		// IMAGE_DOS_HEADER.e_lfanew + 4 byte signature + size of IMAGE_FILE_HEADER + size of optional header + size of all section headers : and rounded at a multiple of file_alignment
DWORD	image_check_sum = 0;		// Only for drivers, DLL loaded at boot time, DLL loaded in critical system process
WORD	subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;	// @Warning IMAGE_SUBSYSTEM_WINDOWS_CUI == console application (with a main) - IMAGE_SUBSYSTEM_WINDOWS_GUI == GUI application (with a WinMain)

int main(int ac, char** av)
{

	fs::path		binary_path = "hello_world.exe";
	std::ofstream	binary;

	binary.open(binary_path, std::ofstream::binary);
	if (binary.is_open() == false) {
		std::cout << "Failed to open " << binary_path << std::endl;
	}


	// https://en.wikipedia.org/wiki/Portable_Executable
	// https://fr.wikipedia.org/wiki/Portable_Executable

	// @Notice @Warning @TODO
	// Variables that start with PointerTo are offsets in the file
	// Variables that start with Virtual are relative offsets to ImageBase, in this case address in the reallocation table should be updated
	//
	// .text section contains the code (ASM)
	// .data section contains memory values
	// import address table (IAT) is used to link against symbols into a different module (dll), but call functions in that way cost an extra indirection made by a jump
	// 

	IMAGE_DOS_HEADER	image_dos_header = {};

	image_dos_header.e_magic = (WORD)'M' | ((WORD)'Z' << 8);	// 'MZ' @Warning take care of the endianness / 0x5A4D
	image_dos_header.e_lfanew = sizeof(image_dos_header);		// Offset to the image_nt_header

	// @TODO complete the MS-DOS stub program, we should print an error message ("This program cannot be run in DOS mode") and exit with code: 1

	IMAGE_NT_HEADERS32	image_nt_header;	// @Warning should be aligned on 8 byte boundary

	time_t	current_time_in_seconds_since_1970;
	current_time_in_seconds_since_1970 = time(nullptr);

	image_nt_header.Signature = (WORD)'P' | ((WORD)'E' << 8);	// 'PE\0\0' @Warning take care of the endianness / 0x50450000
	image_nt_header.FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
	image_nt_header.FileHeader.NumberOfSections = 0;
	image_nt_header.FileHeader.TimeDateStamp = (DWORD)(current_time_in_seconds_since_1970);	// @Warning UTC time
	image_nt_header.FileHeader.PointerToSymbolTable = 0;	// This value should be zero for an image because COFF debugging information is deprecated. 
	image_nt_header.FileHeader.NumberOfSymbols = 0;			// This value should be zero for an image because COFF debugging information is deprecated. 
	image_nt_header.FileHeader.SizeOfOptionalHeader = sizeof(image_nt_header.OptionalHeader);	// @TODO the size of the OptionalHeader is not fixed, so fixe the sizeof
	image_nt_header.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE /*| IMAGE_FILE_DEBUG_STRIPPED*/ /*| IMAGE_FILE_DLL*/;
	
	// Standard COFF fields.
	image_nt_header.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
	image_nt_header.OptionalHeader.MajorLinkerVersion = 0;
	image_nt_header.OptionalHeader.MinorLinkerVersion = 1;
	image_nt_header.OptionalHeader.SizeOfCode = size_of_code;
	image_nt_header.OptionalHeader.SizeOfInitializedData = size_of_initialized_data;
	image_nt_header.OptionalHeader.SizeOfUninitializedData = size_of_uninitialized_data;
	image_nt_header.OptionalHeader.AddressOfEntryPoint = address_of_entry_point;
	image_nt_header.OptionalHeader.BaseOfCode = base_of_code;
	image_nt_header.OptionalHeader.BaseOfData = base_of_data;

	// NT additional fields.
	image_nt_header.OptionalHeader.ImageBase = image_base;					// Binary will be loaded at this adress if available else on an other one that will overwrite this value
	image_nt_header.OptionalHeader.SectionAlignment = section_alignment;
	image_nt_header.OptionalHeader.FileAlignment = file_alignment;
	image_nt_header.OptionalHeader.MajorOperatingSystemVersion = (BYTE)(_WIN32_WINNT_WIN7 >> 8);	// @Warning _WIN32_WINNT_***** macros are encoded as 0xMMmm where MM is the major version number and mm the minor version number
	image_nt_header.OptionalHeader.MinorOperatingSystemVersion = (BYTE)_WIN32_WINNT_WIN7;			// @Warning _WIN32_WINNT_***** macros are encoded as 0xMMmm where MM is the major version number and mm the minor version number
	image_nt_header.OptionalHeader.MajorImageVersion = major_image_version;
	image_nt_header.OptionalHeader.MinorImageVersion = minor_image_version;
	image_nt_header.OptionalHeader.MajorSubsystemVersion = (BYTE)(_WIN32_WINNT_WIN7 >> 8);			// @Warning same as Visual Studio 2019
	image_nt_header.OptionalHeader.MinorSubsystemVersion = (BYTE)_WIN32_WINNT_WIN7;					// @Warning same as Visual Studio 2019
	image_nt_header.OptionalHeader.Win32VersionValue = 0;				// @Warning This member is reservedand must be 0.
	image_nt_header.OptionalHeader.SizeOfImage = size_of_image;
	image_nt_header.OptionalHeader.SizeOfHeaders = size_of_headers;
	image_nt_header.OptionalHeader.CheckSum = image_check_sum;
	image_nt_header.OptionalHeader.Subsystem = subsystem;
	image_nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE;	// @Warning same flags as Visual Studio 2019
	image_nt_header.OptionalHeader.SizeOfStackReserve = size_of_stack_reserve;
	image_nt_header.OptionalHeader.SizeOfStackCommit = size_of_stack_commit;
	image_nt_header.OptionalHeader.SizeOfHeapReserve = size_of_heap_reserve;
	image_nt_header.OptionalHeader.SizeOfHeapCommit = size_of_heap_commit;
	image_nt_header.OptionalHeader.LoaderFlags = 0;	// @Deprecated
	image_nt_header.OptionalHeader.NumberOfRvaAndSizes = 16;

	// Data Directories
	for (int i = 0; i < image_nt_header.OptionalHeader.NumberOfRvaAndSizes; i++) {
		image_nt_header.OptionalHeader.DataDirectory[i];
	}

	binary.write((const char*)&image_dos_header, sizeof(image_dos_header));
	binary.write((const char*)&image_nt_header, sizeof(image_nt_header));

	assert(image_nt_header.FileHeader.NumberOfSections <= 96);
	for (int i = 0; i < image_nt_header.FileHeader.NumberOfSections; i++) {
		IMAGE_SECTION_HEADER	image_section;

		binary.write((const char*)&image_section, sizeof(image_section));
	}

	return 0;
}
