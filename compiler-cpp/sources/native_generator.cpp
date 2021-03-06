#include "native_generator.hpp"

#include <fstd/system/file.hpp>

#include <iostream>

#include <Windows.h>

#include <time.h>
#include <assert.h>

#include <fstd/core/assert.hpp>

using namespace fstd;

constexpr DWORD	image_base = 0x00400000;
constexpr DWORD	section_alignment = 4096;	// 4;	// @Warning should be greater or equal to file_alignment
constexpr DWORD	file_alignment = 512;		// 4;		// @TODO check that because the default value according to the official documentation is 512
constexpr WORD	major_image_version = 1;
constexpr WORD	minor_image_version = 0;
constexpr DWORD size_of_stack_reserve = 0x100000;	// @Warning same flags as Visual Studio 2019
constexpr DWORD size_of_stack_commit = 0x1000;
constexpr DWORD size_of_heap_reserve = 0x100000;
constexpr DWORD size_of_heap_commit = 0x1000;

// @TODO variables that have to be computed at run time
DWORD	size_of_code = 0;			// The size of the code (text) section, or the sum of all code sections if there are multiple sections.
DWORD	size_of_initialized_data = 0;	// The size of the initialized data section, or the sum of all such sections if there are multiple data sections.
DWORD	size_of_uninitialized_data = 0;	// The size of the uninitialized data section (BSS), or the sum of all such sections if there are multiple BSS sections.
DWORD	address_of_entry_point = 0;	// Relative to image_base
DWORD	base_of_code = 0;			// Relative to image_base
DWORD	base_of_data = 0;			// Relative to image_base
DWORD	size_of_image = 0;			// @Warning must be a multiple of section_alignment
DWORD	size_of_headers = 0;		// IMAGE_DOS_HEADER.e_lfanew + 4 byte signature + size of IMAGE_FILE_HEADER + size of optional header + size of all section headers : and rounded at a multiple of file_alignment
DWORD	image_check_sum = 0;		// Only for drivers, DLL loaded at boot time, DLL loaded in critical system process
WORD	subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;	// @Warning IMAGE_SUBSYSTEM_WINDOWS_CUI == console application (with a main) - IMAGE_SUBSYSTEM_WINDOWS_GUI == GUI application (with a WinMain)
DWORD	text_image_section_virtual_address = 0;
DWORD	text_image_section_pointer_to_raw_data = 0;
DWORD	rdata_image_section_virtual_address = 0;
DWORD	rdata_image_section_pointer_to_raw_data = 0;
DWORD	reloc_image_section_virtual_address = 0;
DWORD	reloc_image_section_pointer_to_raw_data = 0;

// Addresses where there is a value to patch, or necessary to compute other ones
DWORD	image_base_address = 0;
DWORD	image_dos_header_address;
DWORD	image_nt_header_address;
DWORD	text_image_section_header_address;
DWORD	text_section_address;
DWORD	rdata_image_section_header_address;
DWORD	rdata_section_address;
DWORD	reloc_image_section_header_address;
DWORD	reloc_section_address;

std::uint8_t	hello_world_instructions[] = {
    0x89, 0xE5,										   // mov    ebp, esp
    0x83, 0xEC, 0x04,								   // sub    esp, 0x4
    0x6A, 0xF5,										   // push   0xfffffff5         @TODO correct F5 value
    0xE8, 0xFC, 0x00, 0x00, 0x00,					   // call   8 <_main+0x8>		GetStdHandle @TODO fixe the adress
    0x89, 0xC3,										   // mov    ebx, eax
    0x6A, 0x00,										   // push   0x0
    0x8D, 0x45, 0xFC,								   // lea    eax, [ebp-0x4]
    0x50,											   // push   eax
    0x6A, 0x0B,										   // push   0xb				@Warning nNumberOfBytesToWrite
    0xFF, 0x35, 0x00, 0x00, 0x00, 0x00,				   // push   DWORD PTR ds:0x0	@Warning address of lpNumberOfBytesWritten
    0x53,											   // push   ebx
    0xE8, 0xFC, 0xFF, 0xFF, 0xFF,					   // call   1e <_main+0x1e>	WriteFile @TODO fixe the adress
    0x6A, 0x00,										   // push   0x0
    0xE8, 0xFC, 0xFF, 0xFF, 0xFF,					   // call   25 <_main+0x25>	ExitProcess @TODO fixe the adress
    0xF4											   // hlt
};

DWORD	compute_aligned_size(DWORD raw_size, DWORD alignement)
{
    return raw_size + (alignement - (raw_size % alignement));
}

void	write_zeros(HANDLE file, uint32_t count)
{
    constexpr uint32_t	buffer_size = 512;
    static bool			initialized = false;
    static char			zeros_buffer[buffer_size];
    uint32_t			nb_iterations = count / buffer_size;
    uint32_t			modulo = count % buffer_size;
    DWORD				bytes_written;

    if (initialized == false) {
        ZeroMemory(zeros_buffer, buffer_size);
        initialized = true;
    }

    for (uint32_t i = 0; i < nb_iterations; i++) {
        WriteFile(file, (const void*)zeros_buffer, buffer_size, &bytes_written, NULL);
    }
    WriteFile(file, (const void*)zeros_buffer, modulo, &bytes_written, NULL);
}

bool generate_hello_world()
{
    const char*	binary_path = "hello_world.exe";
    HANDLE		BINARY;
    DWORD		bytes_written;

    BINARY = CreateFileA(binary_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (BINARY == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to open " << binary_path << std::endl;
        return false;
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


    // Information about Windows system APIs and libraries
    // https://en.wikipedia.org/wiki/Microsoft_Windows_library_files

    time_t	current_time_in_seconds_since_1970;
    current_time_in_seconds_since_1970 = time(nullptr);

    IMAGE_DOS_HEADER		image_dos_header;
    IMAGE_NT_HEADERS32		image_nt_header;	// @Warning should be aligned on 8 byte boundary
    IMAGE_SECTION_HEADER	text_image_section_header;
    IMAGE_SECTION_HEADER	rdata_image_section_header;
    IMAGE_SECTION_HEADER	reloc_image_section_header;

    RtlSecureZeroMemory(&image_dos_header, sizeof(image_dos_header));

    image_dos_header.e_magic = (WORD)'M' | ((WORD)'Z' << 8);	// 'MZ' @Warning take care of the endianness / 0x5A4D
    image_dos_header.e_lfanew = sizeof(image_dos_header);		// Offset to the image_nt_header
    // @TODO complete the MS-DOS stub program, we should print an ERROR message ("This program cannot be run in DOS mode") and exit with code: 1

    image_dos_header_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
    WriteFile(BINARY, (const void*)&image_dos_header, sizeof(image_dos_header), &bytes_written, NULL);



    image_nt_header.Signature = (WORD)'P' | ((WORD)'E' << 8);	// 'PE\0\0' @Warning take care of the endianness / 0x50450000
    image_nt_header.FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
    image_nt_header.FileHeader.NumberOfSections = 3;
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
    image_nt_header.OptionalHeader.MajorOperatingSystemVersion = (BYTE)(_WIN32_WINNT_WIN7 >> 8);	// @Warning _WIN32_WINNT_***** macros are encoded as 0xMMmm where MM is the major version NUMBER and mm the minor version number
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
    RtlSecureZeroMemory(image_nt_header.OptionalHeader.DataDirectory, sizeof(image_nt_header.OptionalHeader.DataDirectory));	// @TODO replace it by the corresponding intrasect while translating this code in f-lang

    image_nt_header_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
    WriteFile(BINARY, (const void*)&image_nt_header, sizeof(image_nt_header), &bytes_written, NULL);



    core::Assert(image_nt_header.FileHeader.NumberOfSections <= 96);

    // .text section
    {
        RtlSecureZeroMemory(&text_image_section_header, sizeof(text_image_section_header));	// @TODO replace it by the corresponding intrasect while translating this code in f-lang

        RtlCopyMemory(text_image_section_header.Name, ".text", 6);	// @Warning there is a '\0' ending character as it doesn't fill the 8 characters
        text_image_section_header.Misc.VirtualSize = sizeof(hello_world_instructions);
        text_image_section_header.VirtualAddress = text_image_section_virtual_address;
        text_image_section_header.SizeOfRawData = compute_aligned_size(text_image_section_header.Misc.VirtualSize, file_alignment);
        text_image_section_header.PointerToRawData = text_image_section_pointer_to_raw_data;
        text_image_section_header.PointerToRelocations = 0x00;
        text_image_section_header.PointerToLinenumbers = 0x00;
        text_image_section_header.NumberOfRelocations = 0;
        text_image_section_header.NumberOfLinenumbers = 0;
        text_image_section_header.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

        text_image_section_header_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
        WriteFile(BINARY, (const void*)&text_image_section_header, sizeof(text_image_section_header), &bytes_written, NULL);
    }

    // .rdata section
    {
        RtlSecureZeroMemory(&rdata_image_section_header, sizeof(rdata_image_section_header));	// @TODO replace it by the corresponding intrasect while translating this code in f-lang

        RtlCopyMemory(rdata_image_section_header.Name, ".rdata", 7);	// @Warning there is a '\0' ending character as it doesn't fill the 8 characters
        rdata_image_section_header.Misc.VirtualSize = 0;
        rdata_image_section_header.VirtualAddress = rdata_image_section_virtual_address;
        rdata_image_section_header.SizeOfRawData = compute_aligned_size(rdata_image_section_header.Misc.VirtualSize, file_alignment);
        rdata_image_section_header.PointerToRawData = rdata_image_section_pointer_to_raw_data;
        rdata_image_section_header.PointerToRelocations = 0x00;
        rdata_image_section_header.PointerToLinenumbers = 0x00;
        rdata_image_section_header.NumberOfRelocations = 0;
        rdata_image_section_header.NumberOfLinenumbers = 0;
        rdata_image_section_header.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

        rdata_image_section_header_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
        WriteFile(BINARY, (const void*)& rdata_image_section_header, sizeof(rdata_image_section_header), &bytes_written, NULL);
    }

    // .reloc section
    {
        RtlSecureZeroMemory(&reloc_image_section_header, sizeof(reloc_image_section_header));	// @TODO replace it by the corresponding intrasect while translating this code in f-lang

        RtlCopyMemory(reloc_image_section_header.Name, ".reloc", 7);	// @Warning there is a '\0' ending character as it doesn't fill the 8 characters
        reloc_image_section_header.Misc.VirtualSize = 0;
        reloc_image_section_header.VirtualAddress = reloc_image_section_virtual_address;
        reloc_image_section_header.SizeOfRawData = compute_aligned_size(reloc_image_section_header.Misc.VirtualSize, file_alignment);
        reloc_image_section_header.PointerToRawData = reloc_image_section_pointer_to_raw_data;
        reloc_image_section_header.PointerToRelocations = 0x00;
        reloc_image_section_header.PointerToLinenumbers = 0x00;
        reloc_image_section_header.NumberOfRelocations = 0;
        reloc_image_section_header.NumberOfLinenumbers = 0;
        reloc_image_section_header.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

        reloc_image_section_header_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
        WriteFile(BINARY, (const void*)&reloc_image_section_header, sizeof(reloc_image_section_header), &bytes_written, NULL);
    }

    text_section_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
    WriteFile(BINARY, (const void*)hello_world_instructions, sizeof(hello_world_instructions), &bytes_written, NULL);
    write_zeros(BINARY, text_image_section_header.SizeOfRawData - sizeof(hello_world_instructions));

    // Patching address and sizes
//	- Done - DWORD	size_of_code = 0;			// The size of the code (text) section, or the sum of all code sections if there are multiple sections.
//	DWORD	size_of_initialized_data = 0;	// The size of the initialized data section, or the sum of all such sections if there are multiple data sections.
//	DWORD	size_of_uninitialized_data = 0;	// The size of the uninitialized data section (BSS), or the sum of all such sections if there are multiple BSS sections.
//	DWORD	address_of_entry_point = 0;	// Relative to image_base
//	DWORD	base_of_code = 0;			// Relative to image_base
//	DWORD	base_of_data = 0;			// Relative to image_base
//	DWORD	size_of_image = 0;			// @Warning must be a multiple of file_alignment instead of section_alignement, it seems a bit weird to me
//	- Done - DWORD	size_of_headers = 0;		// IMAGE_DOS_HEADER.e_lfanew + 4 byte signature + size of IMAGE_FILE_HEADER + size of optional header + size of all section headers : and rounded at a multiple of file_alignment
//	DWORD	image_check_sum = 0;		// Only for drivers, DLL loaded at boot time, DLL loaded in critical system process
//	WORD	subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;	// @Warning IMAGE_SUBSYSTEM_WINDOWS_CUI == console application (with a main) - IMAGE_SUBSYSTEM_WINDOWS_GUI == GUI application (with a WinMain)
//	- Done - DWORD	text_image_section_virtual_address = 0;
//	- Done - DWORD	text_image_section_pointer_to_raw_data = 0;
//	- Done - DWORD	rdata_image_section_virtual_address = 0;
//	- Done - DWORD	rdata_image_section_pointer_to_raw_data = 0;
//	- Done - DWORD	reloc_image_section_virtual_address = 0;
//	- Done - DWORD	reloc_image_section_pointer_to_raw_data = 0;

    // size_of_code
    {
        DWORD size_of_code_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfCode);
        size_of_code = text_image_section_header.SizeOfRawData;	 // @Warning size of the sum of all .text sections

        SetFilePointer(BINARY, size_of_code_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&size_of_code, sizeof(size_of_code), &bytes_written, NULL);
    }

    // size_of_initialized_data
    {
        DWORD size_of_initialized_data_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfCode);
        size_of_initialized_data = text_image_section_header.SizeOfRawData;	 // @Warning size of the sum of all .text sections

        SetFilePointer(BINARY, size_of_initialized_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&size_of_initialized_data, sizeof(size_of_initialized_data), &bytes_written, NULL);
    }

    // size_of_headers
    {
        DWORD size_of_header_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfHeaders);
        size_of_headers = text_section_address;

        SetFilePointer(BINARY, size_of_header_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&size_of_headers, sizeof(size_of_headers), &bytes_written, NULL);
    }

    // text_image_section_virtual_address
    {
        DWORD text_image_section_virtual_address_address = text_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
        text_image_section_virtual_address = text_section_address;

        SetFilePointer(BINARY, text_image_section_virtual_address_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&text_image_section_virtual_address, sizeof(text_image_section_virtual_address), &bytes_written, NULL);
    }

    // text_image_section_pointer_to_raw_data
    {
        DWORD text_image_section_pointer_to_raw_data_address = text_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
        text_image_section_pointer_to_raw_data = text_section_address;

        SetFilePointer(BINARY, text_image_section_pointer_to_raw_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&text_image_section_pointer_to_raw_data, sizeof(text_image_section_pointer_to_raw_data), &bytes_written, NULL);
    }

    // rdata_image_section_virtual_address
    {
        DWORD rdata_image_section_virtual_address_address = rdata_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
        rdata_image_section_virtual_address = rdata_section_address;

        SetFilePointer(BINARY, rdata_image_section_virtual_address_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&rdata_image_section_virtual_address, sizeof(rdata_image_section_virtual_address), &bytes_written, NULL);
    }

    // rdata_image_section_pointer_to_raw_data
    {
        DWORD rdata_image_section_pointer_to_raw_data_address = rdata_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
        rdata_image_section_pointer_to_raw_data = rdata_section_address;

        SetFilePointer(BINARY, rdata_image_section_pointer_to_raw_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&rdata_image_section_pointer_to_raw_data, sizeof(rdata_image_section_pointer_to_raw_data), &bytes_written, NULL);
    }

    // reloc_image_section_virtual_address
    {
        DWORD reloc_image_section_virtual_address_address = reloc_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
        reloc_image_section_virtual_address = reloc_section_address;

        SetFilePointer(BINARY, reloc_image_section_virtual_address_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&reloc_image_section_virtual_address, sizeof(reloc_image_section_virtual_address), &bytes_written, NULL);
    }

    // rdata_image_section_pointer_to_raw_data
    {
        DWORD reloc_image_section_pointer_to_raw_data_address = reloc_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
        reloc_image_section_pointer_to_raw_data = reloc_section_address;

        SetFilePointer(BINARY, reloc_image_section_pointer_to_raw_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)& reloc_image_section_pointer_to_raw_data, sizeof(reloc_image_section_pointer_to_raw_data), &bytes_written, NULL);
    }

    CloseHandle(BINARY);

    return true;
}
