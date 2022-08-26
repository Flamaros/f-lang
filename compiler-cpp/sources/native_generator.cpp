#include "native_generator.hpp"

#include <fstd/system/file.hpp>
#include <fstd/core/assert.hpp>
#include <fstd/core/string_builder.hpp>

#include <Windows.h>

#include <time.h>
#include <assert.h>

using namespace fstd;
using namespace fstd::core;

using namespace f;

#define DLL_MODE 0

constexpr DWORD	page_size = 4096;	// 4096 on x86
constexpr DWORD	image_base = 0x00400000;
constexpr DWORD	section_alignment = page_size;	// 4;	// @Warning should be greater or equal to file_alignment
constexpr DWORD	file_alignment = 512;		// 4;		// @TODO check that because the default value according to the official documentation is 512
constexpr WORD	major_image_version = 1;
constexpr WORD	minor_image_version = 0;
constexpr DWORD size_of_stack_reserve = 0x100000;	// @Warning same flags as Visual Studio 2019
constexpr DWORD size_of_stack_commit = 0x1000;
constexpr DWORD size_of_heap_reserve = 0x100000;
constexpr DWORD size_of_heap_commit = 0x1000;

// My values (not serialized)
constexpr size_t PE_header_start_address = 0xD0; // @Warning This value is actually hard coded because I don't have any DOS stub, otherwise I should be able to compute it from the DOS stub size.

// @TODO variables that have to be computed at run time
DWORD	size_of_code = 0;			// The size of the code (text) section, or the sum of all code sections if there are multiple sections.
DWORD	size_of_initialized_data = 0;	// The size of the initialized data section, or the sum of all such sections if there are multiple data sections.
DWORD	size_of_uninitialized_data = 0;	// The size of the uninitialized data section (BSS), or the sum of all such sections if there are multiple BSS sections.
DWORD	address_of_entry_point = 0;	// Relative to image_base
DWORD	base_of_code = 0;			// Relative to image_base
DWORD	base_of_data = 0;			// Relative to image_base
DWORD	size_of_image = 0;			// @Warning must be a multiple of section_alignment, it's the sum of header and section with each size aligned on section_alignement (size that the process will take in memory once loaded)
DWORD	size_of_headers = 0;		// IMAGE_DOS_HEADER.e_lfanew + 4 byte signature + size of IMAGE_FILE_HEADER + size of optional header + size of all section headers : and rounded at a multiple of file_alignment
DWORD	image_check_sum = 0;		// Only for drivers, DLL loaded at boot time, DLL loaded in critical system process
WORD	subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;	// @Warning IMAGE_SUBSYSTEM_WINDOWS_CUI == console application (with a main) - IMAGE_SUBSYSTEM_WINDOWS_GUI == GUI application (with a WinMain)
DWORD	text_image_section_virtual_address = 0;
DWORD	text_image_section_pointer_to_raw_data = 0;
DWORD	rdata_image_section_virtual_address = 0;
DWORD	rdata_image_section_pointer_to_raw_data = 0;
DWORD	reloc_image_section_virtual_address = 0;
DWORD	reloc_image_section_pointer_to_raw_data = 0;
DWORD	idata_image_section_virtual_address = 0;
DWORD	idata_image_section_pointer_to_raw_data = 0;

// Addresses where there is a value to patch, or necessary to compute other ones
DWORD	image_base_address = 0;
DWORD	image_dos_header_address;
DWORD	image_nt_header_address;
DWORD	text_image_section_header_address;
DWORD	text_section_address;
DWORD	rdata_image_section_header_address;
DWORD	rdata_section_address;
#if DLL_MODE == 1
DWORD	reloc_image_section_header_address;
DWORD	reloc_section_address;
#endif
DWORD	idata_image_section_header_address;
DWORD	idata_section_address;

// @TODO should be generated not hard-coded
uint8_t	hello_world_instructions[] = {
    0x89, 0xE5,										   // mov    ebp, esp
    0x83, 0xEC, 0x04,								   // sub    esp, 0x4
    0x6A, -11,										   // push   0xfffffff5         // STD_OUTPUT_HANDLE == (DWORD)-11 => 0xfffffff5 en hexa
#if DLL_MODE == 1
    0xFF, 0x15, 0x38, 0x40, 0x40, 0x00,				   // call   GetStdHandle       // ImageBase 0x400000 + .idata virtual addr 0x4000 + 0x38 (offset in IAT)
#else
    0xFF, 0x15, 0x38, 0x30, 0x40, 0x00,				   // call   GetStdHandle       // ImageBase 0x400000 + .idata virtual addr 0x4000 + 0x38 (offset in IAT)
#endif
    0x89, 0xC3,										   // mov    ebx, eax
    0x6A, 0x00,										   // push   0x0
    0x8D, 0x45, 0xFC,								   // lea    eax, [ebp-0x4]
    0x50,											   // push   eax
    0x6A, 0x0B,										   // push   0xb				@Warning nNumberOfBytesToWrite
    0x68, 0x00, 0x20, 0x40, 0x00,				       // push   DWORD PTR ds:0x0	@Warning address of message string (first value in .rdata section)
    0x53,											   // push   ebx
#if DLL_MODE == 1
    0xFF, 0x15, 0x3C, 0x40, 0x40, 0x00,				   // call   WriteFile          // ImageBase 0x400000 + .idata virtual addr 0x4000 + 0x3C (offset in IAT)
#else
    0xFF, 0x15, 0x3C, 0x30, 0x40, 0x00,				   // call   WriteFile          // ImageBase 0x400000 + .idata virtual addr 0x4000 + 0x3C (offset in IAT)
#endif
    0x6A, 0x00,										   // push   0x0
#if DLL_MODE == 1
    0xFF, 0x15, 0x40, 0x40, 0x40, 0x00,				   // call   ExitProcess        // ImageBase 0x400000 + .idata virtual addr 0x4000 + 0x40 (offset in IAT)
#else
    0xFF, 0x15, 0x40, 0x30, 0x40, 0x00,				   // call   ExitProcess        // ImageBase 0x400000 + .idata virtual addr 0x4000 + 0x40 (offset in IAT)
#endif
    0xF4											   // hlt
};

// @TODO should be generated not hard-coded
uint8_t* dll_names[] = {
    (uint8_t*)"kernel32.dll",
};

uint8_t* message[] = {
    (uint8_t*)"Hello World",
};

uint8_t* kernel32_function_names[] = {
    (uint8_t*)"GetStdHandle",
    (uint8_t*)"WriteFile",
    (uint8_t*)"ExitProcess",
};

// @Warning 4 first bits are a flag (https://docs.microsoft.com/fr-fr/windows/win32/debug/pe-format#base-relocation-types):
// IMAGE_REL_BASED_DIR64 for x64 code?
WORD text_relocation_offsets[] = {
    7, // call GetStdHandle - text_image_section_header.PointerToRawData + instruction offset
    29, // call WriteFile - text_image_section_header.PointerToRawData + instruction offset
    37, // call ExitProcess - text_image_section_header.PointerToRawData + instruction offset
};

WORD rdata_relocation_offsets[] = {
    0, // variable message ("Hello World") - rdata_image_section_header.PointerToRawData + variable offset
};

// @TODO check if align_address isn't enough to compute aligned values
static DWORD	compute_aligned_size(DWORD raw_size, DWORD alignement)
{
    return raw_size + (alignement - (raw_size % alignement));
}

static DWORD	align_address(DWORD address, DWORD alignement)
{
    return address + (address % alignement);
}

static void	write_zeros(HANDLE file, uint32_t count)
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

bool f::PE_x64_backend::generate_hello_world()
{
    const char*	binary_path = "hello_world.exe";
    HANDLE		BINARY;
    DWORD		bytes_written;

    BINARY = CreateFileA(binary_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (BINARY == INVALID_HANDLE_VALUE) {
        String_Builder		string_builder;

        print_to_builder(string_builder, "Failed to open file: %Cs", binary_path);
        system::print(to_string(string_builder));
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
    IMAGE_SECTION_HEADER	idata_image_section_header;

    RtlSecureZeroMemory(&image_dos_header, sizeof(image_dos_header));

    // @TODO I think that I should copy a complete dos header with dosstub (it can be completely hard-coded)

    image_dos_header.e_magic = (WORD)'M' | ((WORD)'Z' << 8);	    // 'MZ' @Warning take care of the endianness / 0x5A4D
    image_dos_header.e_lfanew = PE_header_start_address;	// Offset to the image_nt_header
    // @TODO complete the MS-DOS stub program, we should print an ERROR message ("This program cannot be run in DOS mode") and exit with code: 1

    image_dos_header_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
    WriteFile(BINARY, (const void*)&image_dos_header, sizeof(image_dos_header), &bytes_written, NULL);
    write_zeros(BINARY, PE_header_start_address - sizeof(image_dos_header)); // Move to current position (jumping implementation of the DOS_STUB) to PE_header_start_address

    image_nt_header.Signature = (WORD)'P' | ((WORD)'E' << 8);	// 'PE\0\0' @Warning take care of the endianness / 0x50450000
    image_nt_header.FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
#if DLL_MODE == 1
    image_nt_header.FileHeader.NumberOfSections = 4;
#else
    image_nt_header.FileHeader.NumberOfSections = 3;
#endif
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
#if DLL_MODE == 1
    image_nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE | IMAGE_DLLCHARACTERISTICS_NO_SEH;	// @Warning same flags as Visual Studio 2019
#else
    image_nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE | IMAGE_DLLCHARACTERISTICS_NO_SEH;	// @Warning same flags as Visual Studio 2019
#endif
    image_nt_header.OptionalHeader.SizeOfStackReserve = size_of_stack_reserve;
    image_nt_header.OptionalHeader.SizeOfStackCommit = size_of_stack_commit;
    image_nt_header.OptionalHeader.SizeOfHeapReserve = size_of_heap_reserve;
    image_nt_header.OptionalHeader.SizeOfHeapCommit = size_of_heap_commit;
    image_nt_header.OptionalHeader.LoaderFlags = 0;	// @Deprecated
    image_nt_header.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

    // Data Directories
    {
        RtlSecureZeroMemory(image_nt_header.OptionalHeader.DataDirectory, sizeof(image_nt_header.OptionalHeader.DataDirectory));	// @TODO replace it by the corresponding intrasect while translating this code in f-lang

#if DLL_MODE == 1
        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = 0x4000; // @TODO need to be computed and patched
#else
        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = 0x3000; // @TODO need to be computed and patched
#endif
        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = 2 * sizeof(IMAGE_IMPORT_DESCRIPTOR); // kernel32.dll + null entry

#if DLL_MODE == 1
        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0x3000; // @TODO need to be computed and patched
        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = sizeof(IMAGE_BASE_RELOCATION) + sizeof(text_relocation_offsets)
                                                                                           + (sizeof(IMAGE_BASE_RELOCATION) + sizeof(text_relocation_offsets)) % 4 // padding of previous block
                                                                                           + sizeof(IMAGE_BASE_RELOCATION) + sizeof(rdata_relocation_offsets); // relocation of first memory page of .text section
#endif

        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
                                                                                               + 2 * sizeof(IMAGE_IMPORT_DESCRIPTOR) // Import table + null terminated entry
                                                                                               + sizeof(DWORD) * (3 + 1); // ILT (3 entries + 1 null)
        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = (3 + 1) * sizeof(DWORD); // @TODO take care of 64bit binaries // null entry count
    }

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
        rdata_image_section_header.Misc.VirtualSize = fstd::language::string_literal_size(message[0]) + 1;
        rdata_image_section_header.VirtualAddress = rdata_image_section_virtual_address;
        rdata_image_section_header.SizeOfRawData = compute_aligned_size(rdata_image_section_header.Misc.VirtualSize, file_alignment); // @TODO should be computed
        rdata_image_section_header.PointerToRawData = rdata_image_section_pointer_to_raw_data;
        rdata_image_section_header.PointerToRelocations = 0x00;
        rdata_image_section_header.PointerToLinenumbers = 0x00;
        rdata_image_section_header.NumberOfRelocations = 0;
        rdata_image_section_header.NumberOfLinenumbers = 0;
        rdata_image_section_header.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;

        rdata_image_section_header_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
        WriteFile(BINARY, (const void*)& rdata_image_section_header, sizeof(rdata_image_section_header), &bytes_written, NULL);
    }

#if DLL_MODE == 1
    // .reloc section
    {
        RtlSecureZeroMemory(&reloc_image_section_header, sizeof(reloc_image_section_header));	// @TODO replace it by the corresponding intrasect while translating this code in f-lang

        RtlCopyMemory(reloc_image_section_header.Name, ".reloc", 7);	// @Warning there is a '\0' ending character as it doesn't fill the 8 characters
        reloc_image_section_header.Misc.VirtualSize = 0; // @TODO fix it // @TODO compute it
        reloc_image_section_header.VirtualAddress = reloc_image_section_virtual_address;
        reloc_image_section_header.SizeOfRawData = compute_aligned_size(reloc_image_section_header.Misc.VirtualSize, file_alignment);
        reloc_image_section_header.PointerToRawData = reloc_image_section_pointer_to_raw_data;
        reloc_image_section_header.PointerToRelocations = 0x00;
        reloc_image_section_header.PointerToLinenumbers = 0x00;
        reloc_image_section_header.NumberOfRelocations = 0;
        reloc_image_section_header.NumberOfLinenumbers = 0;
        reloc_image_section_header.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ;

        reloc_image_section_header_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
        WriteFile(BINARY, (const void*)&reloc_image_section_header, sizeof(reloc_image_section_header), &bytes_written, NULL);
    }
#endif

    // .idata section
    {
        RtlSecureZeroMemory(&idata_image_section_header, sizeof(idata_image_section_header));	// @TODO replace it by the corresponding intrasect while translating this code in f-lang

        RtlCopyMemory(idata_image_section_header.Name, ".idata", 7);	// @Warning there is a '\0' ending character as it doesn't fill the 8 characters
        idata_image_section_header.Misc.VirtualSize = 126; // @TODO compute it
        idata_image_section_header.VirtualAddress = idata_image_section_virtual_address;
        idata_image_section_header.SizeOfRawData = compute_aligned_size(idata_image_section_header.Misc.VirtualSize, file_alignment); // @TODO should be computed
        idata_image_section_header.PointerToRawData = idata_image_section_pointer_to_raw_data;
        idata_image_section_header.PointerToRelocations = 0x00;
        idata_image_section_header.PointerToLinenumbers = 0x00;
        idata_image_section_header.NumberOfRelocations = 0;
        idata_image_section_header.NumberOfLinenumbers = 0;
        idata_image_section_header.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ;

        idata_image_section_header_address = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
        WriteFile(BINARY, (const void*)&idata_image_section_header, sizeof(idata_image_section_header), &bytes_written, NULL);
    }

    // Move to next aligned position before writing code (text_section)
    size_of_headers = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
    text_section_address = compute_aligned_size(size_of_headers, section_alignment);
    text_image_section_pointer_to_raw_data = compute_aligned_size(size_of_headers, file_alignment);
    SetFilePointer(BINARY, text_image_section_pointer_to_raw_data, NULL, FILE_BEGIN);

    size_of_image = SetFilePointer(BINARY, 0, NULL, FILE_CURRENT);
    size_of_image = compute_aligned_size(size_of_image, section_alignment);

    // Write code (.text section data)
    {
        WriteFile(BINARY, (const void*)hello_world_instructions, sizeof(hello_world_instructions), &bytes_written, NULL);
        write_zeros(BINARY, text_image_section_header.SizeOfRawData - sizeof(hello_world_instructions));
        size_of_image += compute_aligned_size(text_image_section_header.SizeOfRawData, section_alignment);
    }

    // Write read only data (.rdata section data)
    {
        DWORD rdata_section_size = 0;

        WriteFile(BINARY, (const void*)message[0], fstd::language::string_literal_size(message[0]) + 1, &bytes_written, NULL); // +1 to write the ending '\0'
        rdata_section_size += bytes_written;

        write_zeros(BINARY, rdata_image_section_header.SizeOfRawData - rdata_section_size);
        size_of_image += compute_aligned_size(rdata_image_section_header.SizeOfRawData, section_alignment);
    }

#if DLL_MODE == 1
    // Write relocation data (.reloc section data)
    {
        IMAGE_BASE_RELOCATION image_base_relocation;

        DWORD reloc_section_size = 0;

        // @Warning IMAGE_BASE_RELOCATION headers have to be aligned on 32bits, but because I start to the begining of the .reloc section the first is aligned correctly.

        // for .text section
        {
            image_base_relocation.VirtualAddress = 0x1000; // @TODO compute it, RVA of first memory page of .text section
            image_base_relocation.SizeOfBlock = sizeof(IMAGE_BASE_RELOCATION) + 3 * sizeof(WORD); // @TODO compute it: (3 external function call)

            WriteFile(BINARY, (const void*)&image_base_relocation, sizeof(image_base_relocation), &bytes_written, NULL);
            reloc_section_size += bytes_written;

            WriteFile(BINARY, (const void*)&text_relocation_offsets, sizeof(text_relocation_offsets), &bytes_written, NULL);
            reloc_section_size += bytes_written;
        }

        // @Warning @WTF PE-bear don't see correct size of blocks after the padding
        size_t padding = reloc_section_size % 4;
        if (padding) {
            write_zeros(BINARY, padding);
            reloc_section_size += padding;
        }

        // for .rdata section
        {
            image_base_relocation.VirtualAddress = 0x2000; // @TODO compute it, RVA of first memory page of .rdata section
            image_base_relocation.SizeOfBlock = sizeof(IMAGE_BASE_RELOCATION) + 1 * sizeof(WORD); // @TODO compute it: (3 external function call)

            WriteFile(BINARY, (const void*)&image_base_relocation, sizeof(image_base_relocation), &bytes_written, NULL);
            reloc_section_size += bytes_written;

            WriteFile(BINARY, (const void*)&rdata_relocation_offsets, sizeof(rdata_relocation_offsets), &bytes_written, NULL);
            reloc_section_size += bytes_written;
        }

        write_zeros(BINARY, reloc_image_section_header.SizeOfRawData - reloc_section_size);
        size_of_image += compute_aligned_size(rdata_image_section_header.SizeOfRawData, section_alignment);
    }
#endif

    // Write import data (.idata section data)
    {
        // @TODO Make it generic (actually hard coded)
        // @TODO check the section size in the header (idata_image_section_header.SizeOfRawData)

        DWORD HNT_size = 0;
        for (DWORD i = 0; i < 3; i++) {
            HNT_size += sizeof(WORD);
            HNT_size += fstd::language::string_literal_size(kernel32_function_names[i]) + 1;
        }

        DWORD idata_section_size = 0;
        {
            IMAGE_IMPORT_DESCRIPTOR import_descriptor;

            // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
            import_descriptor.OriginalFirstThunk = image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
                                                 + sizeof(IMAGE_IMPORT_DESCRIPTOR) * 2;
            import_descriptor.TimeDateStamp = 0;
            import_descriptor.ForwarderChain = 0; // -1 if no forwarders

            // RVA to DLL name
            import_descriptor.Name = image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
                                   + sizeof(IMAGE_IMPORT_DESCRIPTOR) * 2
                                   + sizeof(DWORD) * (3 + 1) * 2 // IAT + ILT
                                   + HNT_size; // HNT

            // RVA to IAT (if bound this IAT has actual addresses)
            import_descriptor.FirstThunk = image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
                                         + sizeof(IMAGE_IMPORT_DESCRIPTOR) * 2
                                         + sizeof(DWORD) * (3 + 1);

            WriteFile(BINARY, (const void*)&import_descriptor, sizeof(import_descriptor), &bytes_written, NULL);
            idata_section_size += bytes_written;
        }
        // Write zeros to terminate import_descriptor table
        write_zeros(BINARY, sizeof(IMAGE_IMPORT_DESCRIPTOR));
        idata_section_size += sizeof(IMAGE_IMPORT_DESCRIPTOR);

        // ILT (Import Lookup Table). A simple DWORD per entry that contains RVA to function names
        // IAT (Import Address Table). A simple DWORD per entry that contains RVA to function address.
        //
        // ILT and IAT are exactly the same in file, but the loader will resolve addresses for the IAT when loading the binary

        for (size_t i = 0; i < 2; i++)
        {
            {
                DWORD RVA;

                // GetStdHandle
                RVA = image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
                    + sizeof(IMAGE_IMPORT_DESCRIPTOR) * 2
                    + sizeof(DWORD) * (3 + 1) * 2 // ILT + IAT
                    + 0;
                WriteFile(BINARY, (const void*)&RVA, sizeof(RVA), &bytes_written, NULL);
                idata_section_size += bytes_written;

                // WriteFile
                RVA = image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
                    + sizeof(IMAGE_IMPORT_DESCRIPTOR) * 2
                    + sizeof(DWORD) * (3 + 1) * 2 // ILT + IAT;
                    + fstd::language::string_literal_size(kernel32_function_names[0]) + 1 + sizeof(WORD);
                WriteFile(BINARY, (const void*)&RVA, sizeof(RVA), &bytes_written, NULL);
                idata_section_size += bytes_written;

                // ExitProcess
                RVA = image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
                    + sizeof(IMAGE_IMPORT_DESCRIPTOR) * 2
                    + sizeof(DWORD) * (3 + 1) * 2 // ILT + IAT;
                    + fstd::language::string_literal_size(kernel32_function_names[0]) + 1 + fstd::language::string_literal_size(kernel32_function_names[1]) + 1 + sizeof(WORD) * 2;
                WriteFile(BINARY, (const void*)&RVA, sizeof(RVA), &bytes_written, NULL);
                idata_section_size += bytes_written;
            }
            write_zeros(BINARY, sizeof(DWORD));
            idata_section_size += bytes_written;
        }

        // HNT (Hint/Name Table)
        {
            IMAGE_IMPORT_BY_NAME name;

            name.Hint = 0; // @TODO @Optimize a proper linker will certainly try to get a good hint to help the loader.

            for (size_t i = 0; i < 3; i++)
            {
                WriteFile(BINARY, (const void*)&name.Hint, sizeof(name.Hint), &bytes_written, NULL);
                idata_section_size += bytes_written;

                WriteFile(BINARY, (const void*)kernel32_function_names[i], fstd::language::string_literal_size(kernel32_function_names[i]) + 1, &bytes_written, NULL); // +1 to write the ending '\0'
                idata_section_size += bytes_written;
            }
        }

        // Dll names
        for (size_t i = 0; i < 1; i++)
        {
            WriteFile(BINARY, (const void*)dll_names[i], fstd::language::string_literal_size(dll_names[i]) + 1, &bytes_written, NULL); // +1 to write the ending '\0'
            idata_section_size += bytes_written;
        }

        // @TODO write zeros??? to terminate the import_descriptors table

        write_zeros(BINARY, idata_image_section_header.SizeOfRawData - idata_section_size);
        size_of_image += compute_aligned_size(idata_image_section_header.SizeOfRawData, section_alignment);
    }

    // Size_Of_Image as it is the size of the image + headers, it means that it is the full size of the file
    // and here we are at the end of file

    // Patching address and sizes

    // size_of_code
    {
        DWORD size_of_code_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfCode);
        size_of_code = text_image_section_header.SizeOfRawData;	 // @Warning size of the sum of all .text sections

        SetFilePointer(BINARY, size_of_code_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&size_of_code, sizeof(size_of_code), &bytes_written, NULL);
    }

    // size_of_initialized_data
    {
        DWORD size_of_initialized_data_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfInitializedData);
        size_of_initialized_data = text_image_section_header.SizeOfRawData;	 // @Warning size of the sum of all .text sections
        // 1024
        SetFilePointer(BINARY, size_of_initialized_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&size_of_initialized_data, sizeof(size_of_initialized_data), &bytes_written, NULL);
    }

    // size_of_uninitialized_data
    {
        DWORD size_of_uninitialized_data_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfUninitializedData);
        size_of_uninitialized_data = 0;	 // @Warning size unitialized data in all .text sections

        SetFilePointer(BINARY, size_of_uninitialized_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&size_of_uninitialized_data, sizeof(size_of_uninitialized_data), &bytes_written, NULL);
    }

    // address_of_entry_point
    {
        DWORD address_of_entry_point_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.AddressOfEntryPoint);
        address_of_entry_point = text_section_address;
        // 4096
        SetFilePointer(BINARY, address_of_entry_point_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&address_of_entry_point, sizeof(address_of_entry_point), &bytes_written, NULL);
    }

    // base_of_code
    {
        DWORD base_of_code_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.BaseOfCode);
        base_of_code = text_section_address;
        // 4096
        SetFilePointer(BINARY, base_of_code_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&base_of_code, sizeof(base_of_code), &bytes_written, NULL);
    }

    // base_of_data
    {
        DWORD base_of_data_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.BaseOfData);
        rdata_section_address = compute_aligned_size(text_section_address + text_image_section_header.SizeOfRawData, section_alignment);
        rdata_image_section_pointer_to_raw_data = align_address(text_image_section_pointer_to_raw_data + text_image_section_header.SizeOfRawData, file_alignment);
#if DLL_MODE == 1
        reloc_section_address = compute_aligned_size(rdata_section_address + rdata_image_section_header.SizeOfRawData, section_alignment);
        reloc_image_section_pointer_to_raw_data = align_address(rdata_image_section_pointer_to_raw_data + rdata_image_section_header.SizeOfRawData, file_alignment);
#endif

#if DLL_MODE == 1
        idata_section_address = compute_aligned_size(reloc_section_address + reloc_image_section_header.SizeOfRawData, section_alignment);
        idata_image_section_pointer_to_raw_data = align_address(reloc_image_section_pointer_to_raw_data + reloc_image_section_header.SizeOfRawData, file_alignment);
#else
        idata_section_address = compute_aligned_size(rdata_section_address + rdata_image_section_header.SizeOfRawData, section_alignment);
        idata_image_section_pointer_to_raw_data = align_address(rdata_image_section_pointer_to_raw_data + rdata_image_section_header.SizeOfRawData, file_alignment);
#endif

        base_of_data = rdata_section_address;
        // 8192
        SetFilePointer(BINARY, base_of_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&base_of_data, sizeof(base_of_data), &bytes_written, NULL);
    }

    // size_of_image
    // Gets the size (in bytes) of the image, including all headers, as the image is loaded in memory.
    // The size (in bytes) of the image, which is a multiple of SectionAlignment.
    {
        DWORD size_of_image_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfImage);
//        size_of_image = compute_aligned_size(size_of_image, section_alignment);
        // 
        SetFilePointer(BINARY, size_of_image_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&size_of_image, sizeof(size_of_image), &bytes_written, NULL);
    }

    // size_of_headers (The combined size of an MS DOS stub, PE header, and section headers rounded up to a multiple of FileAlignment.)
    {
        DWORD size_of_header_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfHeaders);
        size_of_headers = compute_aligned_size(size_of_headers, file_alignment);
        // 
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
        // text_image_section_pointer_to_raw_data is already computed

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
        // rdata_image_section_pointer_to_raw_data is already computed

        SetFilePointer(BINARY, rdata_image_section_pointer_to_raw_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&rdata_image_section_pointer_to_raw_data, sizeof(rdata_image_section_pointer_to_raw_data), &bytes_written, NULL);
    }

#if DLL_MODE == 1
    // reloc_image_section_virtual_address
    {
        DWORD reloc_image_section_virtual_address_address = reloc_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
        reloc_image_section_virtual_address = reloc_section_address;

        SetFilePointer(BINARY, reloc_image_section_virtual_address_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&reloc_image_section_virtual_address, sizeof(reloc_image_section_virtual_address), &bytes_written, NULL);
    }

    // reloc_image_section_pointer_to_raw_data
    {
        DWORD reloc_image_section_pointer_to_raw_data_address = reloc_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
        // reloc_image_section_pointer_to_raw_data is already computed

        SetFilePointer(BINARY, reloc_image_section_pointer_to_raw_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&reloc_image_section_pointer_to_raw_data, sizeof(reloc_image_section_pointer_to_raw_data), &bytes_written, NULL);
    }
#endif

    // idata_image_section_virtual_address
    {
        DWORD idata_image_section_virtual_address_address = idata_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
        idata_image_section_virtual_address = idata_section_address;

        SetFilePointer(BINARY, idata_image_section_virtual_address_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&idata_image_section_virtual_address, sizeof(idata_image_section_virtual_address), &bytes_written, NULL);
    }

    // idata_image_section_pointer_to_raw_data
    {
        DWORD idata_image_section_pointer_to_raw_data_address = idata_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
        // idata_image_section_pointer_to_raw_data is already computed

        SetFilePointer(BINARY, idata_image_section_pointer_to_raw_data_address, NULL, FILE_BEGIN);
        WriteFile(BINARY, (const void*)&idata_image_section_pointer_to_raw_data, sizeof(idata_image_section_pointer_to_raw_data), &bytes_written, NULL);
    }

    CloseHandle(BINARY);

    return true;
}
