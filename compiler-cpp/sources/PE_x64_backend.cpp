#include "PE_x64_backend.hpp"

#include "globals.hpp" // report_error

#include <fstd/system/file.hpp>

#include <fstd/core/assert.hpp>
#include <fstd/core/string_builder.hpp>

#include <fstd/stream/array_read_stream.hpp>

#include <fstd/language/defer.hpp>

#include <third-party/SpookyV2.h>

#include <Windows.h>

#include <time.h>

#include <tracy/Tracy.hpp>

using namespace fstd;
using namespace fstd::core;
using namespace fstd::system;
using namespace fstd::stream;
using namespace fstd::container;
using namespace fstd::language;

using namespace f;

#define DLL_MODE 0

constexpr DWORD	page_size = 4096;	// 4096 on x86
#if DLL_MODE == 1
constexpr DWORD	image_base = 0x10000000;
#else
constexpr ULONGLONG	image_base = 0x00400000;
#endif
constexpr DWORD	section_alignment = page_size;	// 4;	// @Warning should be greater or equal to file_alignment
constexpr DWORD	file_alignment = 512;		// 4;		// @TODO check that because the default value according to the official documentation is 512
constexpr WORD	major_image_version = 1;
constexpr WORD	minor_image_version = 0;
constexpr ULONGLONG size_of_stack_reserve = 0x100000;	// @Warning same flags as Visual Studio 2019
constexpr ULONGLONG size_of_stack_commit = 0x1000;
constexpr ULONGLONG size_of_heap_reserve = 0x100000;
constexpr ULONGLONG size_of_heap_commit = 0x1000;

// My values (not serialized)
constexpr size_t PE_header_start_address = 0xD0; // @Warning This value is actually hard coded because I don't have any DOS stub, otherwise I should be able to compute it from the DOS stub size.

enum class Section_Type
{
	IDATA,
	RELOC,
	TEXT,
	RDATA,
	DATA,
	BSS,

	COUNT,
};

struct Section_Data
{
	Section_Type			type = Section_Type::COUNT;
	uint8_t					name[8] = {};
	ASM::Section*			asm_section = nullptr;

	IMAGE_SECTION_HEADER	image_section_header = {};
	DWORD					image_section_virtual_address = 0;
	DWORD					image_section_pointer_to_raw_data = 0;
	DWORD					image_section_header_address = 0;
	DWORD					section_address = 0;
};

Section_Data	sections_data[(size_t)Section_Type::COUNT] = {
	{Section_Type::IDATA,	".idata"},
	{Section_Type::RELOC,	".reloc"},
	{Section_Type::TEXT,	".text"},
	{Section_Type::RDATA,	".rdata"},
	{Section_Type::DATA,	".data"},
	{Section_Type::BSS,		".bss"}
};

// @TODO variables that have to be computed at run time
DWORD	size_of_code = 0;			// The size of the code (text) section, or the sum of all code sections if there are multiple sections.
DWORD	size_of_initialized_data = 0;	// The size of the initialized data section, or the sum of all such sections if there are multiple data sections.
DWORD	size_of_uninitialized_data = 0;	// The size of the uninitialized data section (BSS), or the sum of all such sections if there are multiple BSS sections.
DWORD	address_of_entry_point = 0;	// Relative to image_base
DWORD	base_of_code = 0;			// Relative to image_base
DWORD	size_of_image = 0;			// @Warning must be a multiple of section_alignment, it's the sum of header and section with each size aligned on section_alignement (size that the process will take in memory once loaded)
DWORD	size_of_headers = 0;		// IMAGE_DOS_HEADER.e_lfanew + 4 byte signature + size of IMAGE_FILE_HEADER + size of optional header + size of all section headers : and rounded at a multiple of file_alignment
DWORD	image_check_sum = 0;		// Only for drivers, DLL loaded at boot time, DLL loaded in critical system process
WORD	subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;	// @Warning IMAGE_SUBSYSTEM_WINDOWS_CUI == console application (with a main) - IMAGE_SUBSYSTEM_WINDOWS_GUI == GUI application (with a WinMain)

// Addresses where there is a value to patch, or necessary to compute other ones
DWORD	image_base_address = 0;
DWORD	image_dos_header_address;
DWORD	image_nt_header_address;


// Maximum instruction size:
// https://stackoverflow.com/questions/14698350/x86-64-asm-maximum-bytes-for-an-instruction#:~:text=The%20x86%20instruction%20set%20(16,0x67%20prefixes%2C%20for%20example).
// Intel say 15, but Quantitative Approach has said 17 and now in the 6th edition says 18 bytes

// For explanation of "mov ebp, esp" (thing in 32bits calling convention)
// https://stackoverflow.com/questions/21718397/what-are-the-esp-and-the-ebp-registers
// Ease the compiler debugging
// @TODO I may want implement option "omit frame pointers" to increase performances by using ebp as general purpose register


// @TODO get memory page size dynamically:
// https://stackoverflow.com/questions/3351940/detecting-the-memory-page-size
// Page size under Windows (depending on CPU arch)
// https://devblogs.microsoft.com/oldnewthing/20210510-00/?p=105200


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
	if (raw_size % alignement == 0) {
		return raw_size;
	}
    return raw_size + (alignement - (raw_size % alignement));
}

static DWORD	align_address(DWORD address, DWORD alignement)
{
    return address + (address % alignement);
}

void f::PE_x64_backend::initialize_backend()
{
    //ZoneScopedN("f::PE_x64_backend::initialize_backend");
}

void f::PE_x64_backend::compile(const ASM::ASM& asm_result, const fstd::system::Path& output_file_path)
{
    ZoneScopedN("f::PE_x64_backend::compile");

    uint32_t	bytes_written;

    File    output_file;
    bool    open;

    open = open_file(output_file, output_file_path, (File::Opening_Flag)(
        (uint32_t)File::Opening_Flag:: WRITE |
        (uint32_t)File::Opening_Flag::CREATE));

    if (open == false) {
        String_Builder		string_builder;
        language::string	message;

        defer {
            free_buffers(string_builder);
            release(message);
        };

        print_to_builder(string_builder, "Failed to open file: \"%v\"\n", to_string(output_file_path));

        message = to_string(string_builder);
        report_error(Compiler_Error::error, (char*)to_utf8(message));
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
    IMAGE_NT_HEADERS64		image_nt_header;	// @Warning should be aligned on 8 byte boundary

    RtlSecureZeroMemory(&image_dos_header, sizeof(image_dos_header));

    // @TODO I think that I should copy a complete dos header with dosstub (it can be completely hard-coded)

    image_dos_header.e_magic = (WORD)'M' | ((WORD)'Z' << 8);	    // 'MZ' @Warning take care of the endianness / 0x5A4D
    image_dos_header.e_lfanew = PE_header_start_address;	// Offset to the image_nt_header
    // @TODO complete the MS-DOS stub program, we should print an ERROR message ("This program cannot be run in DOS mode") and exit with code: 1

    image_dos_header_address = (DWORD)get_file_position(output_file);;
    write_file(output_file, (uint8_t*)&image_dos_header, sizeof(image_dos_header), &bytes_written);
    write_zeros(output_file, PE_header_start_address - sizeof(image_dos_header)); // Move to current position (jumping implementation of the DOS_STUB) to PE_header_start_address

    image_nt_header.Signature = (WORD)'P' | ((WORD)'E' << 8);	// 'PE\0\0' @Warning take care of the endianness / 0x50450000
    image_nt_header.FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
#if DLL_MODE == 1
    image_nt_header.FileHeader.NumberOfSections = (WORD)memory::get_array_size(asm_result.sections) + 2; // +2 for idata and reloc implicitely added
#else
    image_nt_header.FileHeader.NumberOfSections = (WORD)container::get_array_size(asm_result.sections) + 1; // +1 for idata implicitely added
#endif
    image_nt_header.FileHeader.TimeDateStamp = (DWORD)(current_time_in_seconds_since_1970);	// @Warning UTC time
    image_nt_header.FileHeader.PointerToSymbolTable = 0;	// This value should be zero for an image because COFF debugging information is deprecated.
    image_nt_header.FileHeader.NumberOfSymbols = 0;			// This value should be zero for an image because COFF debugging information is deprecated.
    image_nt_header.FileHeader.SizeOfOptionalHeader = sizeof(image_nt_header.OptionalHeader);	// @TODO the size of the OptionalHeader is not fixed, so fixe the sizeof
    image_nt_header.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE /*| IMAGE_FILE_DLL*/;

    image_nt_header.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    image_nt_header.OptionalHeader.MajorLinkerVersion = 0;
    image_nt_header.OptionalHeader.MinorLinkerVersion = 1;
    image_nt_header.OptionalHeader.SizeOfCode = size_of_code;
    image_nt_header.OptionalHeader.SizeOfInitializedData = size_of_initialized_data;
    image_nt_header.OptionalHeader.SizeOfUninitializedData = size_of_uninitialized_data;
    image_nt_header.OptionalHeader.AddressOfEntryPoint = address_of_entry_point;
    image_nt_header.OptionalHeader.BaseOfCode = base_of_code;
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
        ZoneScopedN("Data Directories");

        RtlSecureZeroMemory(image_nt_header.OptionalHeader.DataDirectory, sizeof(image_nt_header.OptionalHeader.DataDirectory));	// @TODO replace it by the corresponding intrasect while translating this code in f-lang

        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = 0; // It's patched later with the Virtual Address of IDATA section
        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = ((DWORD)hash_table_get_size(asm_result.imported_libraries) + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR); // + 1 for the null entry

#if DLL_MODE == 1
        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0x3000; // @TODO need to be computed and patched
        image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = sizeof(IMAGE_BASE_RELOCATION) + sizeof(text_relocation_offsets)
            + (sizeof(IMAGE_BASE_RELOCATION) + sizeof(text_relocation_offsets)) % 4 // padding of previous block
            + sizeof(IMAGE_BASE_RELOCATION) + sizeof(rdata_relocation_offsets); // relocation of first memory page of .text section
#endif
    }

    image_nt_header_address = (DWORD)get_file_position(output_file);
    write_file(output_file, (uint8_t*)&image_nt_header, sizeof(image_nt_header), & bytes_written);


    core::Assert(image_nt_header.FileHeader.NumberOfSections <= 96);

	language::string_view	section_name;

	language::assign(section_name, (uint8_t*)".text");
	sections_data[(size_t)Section_Type::TEXT].asm_section = ASM::get_section(asm_result, section_name);

	language::assign(section_name, (uint8_t*)".rdata");
	sections_data[(size_t)Section_Type::RDATA].asm_section = ASM::get_section(asm_result, section_name);

	language::assign(section_name, (uint8_t*)".data");
	sections_data[(size_t)Section_Type::DATA].asm_section = ASM::get_section(asm_result, section_name);

	language::assign(section_name, (uint8_t*)".bss");
	sections_data[(size_t)Section_Type::BSS].asm_section = ASM::get_section(asm_result, section_name);

	for (size_t i = 0; i < (size_t)Section_Type::COUNT; i++)
	{
#if DLL_MODE == 0
		if (i == (size_t)Section_Type::RELOC) {
			continue;
		}
#endif

		ZoneScoped;

		// Initialize struct memory, string like name should be null terminated,...
		zero_memory(&sections_data[i].image_section_header, sizeof(sections_data[i].image_section_header));	// @TODO replace it by the corresponding intrasect while translating this code in f-lang

		if (sections_data[i].asm_section)
		{
			// @Warning I take the name from the user code instead of the hardcoded version
			// This will be necessary for custom section created by user (but for the moment there is no way for the user to specify sections caracteristics)
			memory_copy(sections_data[i].image_section_header.Name, to_utf8(sections_data[i].asm_section->name), get_string_size(sections_data[i].asm_section->name));
			sections_data[i].image_section_header.Misc.VirtualSize = (DWORD)stream::get_size(sections_data[i].asm_section->stream_data);
		}
		else
		{
			memory_copy(sections_data[i].image_section_header.Name, sections_data[i].name, length_of_null_terminated_string(sections_data[i].name));
			sections_data[i].image_section_header.Misc.VirtualSize = 0;
		}
		sections_data[i].image_section_header.VirtualAddress = sections_data[i].image_section_virtual_address;
		sections_data[i].image_section_header.SizeOfRawData = compute_aligned_size(sections_data[i].image_section_header.Misc.VirtualSize, file_alignment);
		sections_data[i].image_section_header.PointerToRawData = 0;
		sections_data[i].image_section_header.PointerToRelocations = 0x00;
		sections_data[i].image_section_header.PointerToLinenumbers = 0x00;
		sections_data[i].image_section_header.NumberOfRelocations = 0;
		sections_data[i].image_section_header.NumberOfLinenumbers = 0;

		if (i == (size_t)Section_Type::IDATA) {
			sections_data[i].image_section_header.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ;
		}
		else if (i == (size_t)Section_Type::RELOC) {
			sections_data[i].image_section_header.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ;
		}
		else if (i == (size_t)Section_Type::TEXT) {
			sections_data[i].image_section_header.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
		}
		else if (i == (size_t)Section_Type::RDATA) {
			sections_data[i].image_section_header.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
		}
		else if (i == (size_t)Section_Type::DATA) {
			sections_data[i].image_section_header.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
		}

		sections_data[i].image_section_header_address = (DWORD)get_file_position(output_file);
		write_file(output_file, (uint8_t*)&sections_data[i].image_section_header, sizeof(sections_data[i].image_section_header), &bytes_written);
	}

	// Move to next aligned position before writing code (text_section)
	size_of_headers = (DWORD)get_file_position(output_file);
	set_file_position(output_file, compute_aligned_size(size_of_headers, file_alignment));

	size_of_image = (DWORD)get_file_position(output_file);
	size_of_image = compute_aligned_size(size_of_image, section_alignment);


	Section_Data	initial_section_data;
	Section_Data*	previous_section_data = &initial_section_data;

	initial_section_data.type = Section_Type::COUNT;
	initial_section_data.image_section_pointer_to_raw_data = compute_aligned_size(size_of_headers, file_alignment);
	initial_section_data.section_address = compute_aligned_size(size_of_image, section_alignment);

	// @TODO I need a starting previous stub
	// Write sections
	for (size_t i = 0; i < (size_t)Section_Type::COUNT; i++)
	{
#if DLL_MODE == 0
		if (i == (size_t)Section_Type::RELOC) {
			continue;
		}
#endif

		sections_data[i].image_section_pointer_to_raw_data = compute_aligned_size(previous_section_data->image_section_pointer_to_raw_data, file_alignment)
															+ previous_section_data->image_section_header.SizeOfRawData;
		sections_data[i].section_address = compute_aligned_size(previous_section_data->section_address, section_alignment) + compute_aligned_size(previous_section_data->image_section_header.SizeOfRawData, section_alignment);

		if (i == (size_t)Section_Type::RELOC)
		{
			// @TODO Fix the reloc code before removing this assert
			// This code is old and was refactored without any test
			core::Assert(false);

			ZoneScopedN("Write relocation data (.reloc section data)");

			IMAGE_BASE_RELOCATION image_base_relocation;

			DWORD reloc_section_size = 0;

			// @Warning IMAGE_BASE_RELOCATION headers have to be aligned on 32bits, but because I start to the begining of the .reloc section the first is aligned correctly.

			// for .text section
			{
				image_base_relocation.VirtualAddress = 0x1000; // @TODO compute it, RVA of first memory page of .text section
				image_base_relocation.SizeOfBlock = sizeof(IMAGE_BASE_RELOCATION) + 3 * sizeof(WORD); // @TODO compute it: (3 external function call)

				write_file(output_file, (uint8_t*)&image_base_relocation, sizeof(image_base_relocation), &bytes_written);
				reloc_section_size += bytes_written;

				write_file(output_file, (uint8_t*)&text_relocation_offsets, sizeof(text_relocation_offsets), &bytes_written);
				reloc_section_size += bytes_written;
			}

			// @Warning @WTF PE-bear don't see correct size of blocks after the padding
			DWORD padding = reloc_section_size % 4;
			if (padding) {
				write_zeros(output_file, padding);
				reloc_section_size += padding;
			}

			// for .rdata section
			{
				image_base_relocation.VirtualAddress = 0x2000; // @TODO compute it, RVA of first memory page of .rdata section
				image_base_relocation.SizeOfBlock = sizeof(IMAGE_BASE_RELOCATION) + 1 * sizeof(WORD); // @TODO compute it: (3 external function call)

				write_file(output_file, (uint8_t*)&image_base_relocation, sizeof(image_base_relocation), &bytes_written);
				reloc_section_size += bytes_written;

				write_file(output_file, (uint8_t*)&rdata_relocation_offsets, sizeof(rdata_relocation_offsets), &bytes_written);
				reloc_section_size += bytes_written;
			}

			write_zeros(output_file, sections_data[(size_t)Section_Type::RELOC].image_section_header.SizeOfRawData - reloc_section_size);
			size_of_image += compute_aligned_size(sections_data[(size_t)Section_Type::RDATA].image_section_header.SizeOfRawData, section_alignment); // @TODO why Section_Type::RDATA instead of Section_Type::RELOC?
		}
		else if (i == (size_t)Section_Type::IDATA)
		{
			ZoneScopedN("Write import data (.idata section data)");

			// Struct of idata section :
			// array of IMAGE_IMPORT_DESCRIPTOR
			// 0
			// For each imported libraries
			//   library name (variable length)
			//   ILT (fixed size depending on number of imported functions)
			//   0
			//   IAT (fixed size depending on number of imported functions (same as ILT))
			//   0
			//   HNT (variable size (depends of names of imported functions))
			//   0

			struct Library_Import_Computations
			{
				DWORD		library_name_RVA;
				DWORD		ILT_RVA;
				DWORD		IAT_RVA;
				DWORD		HNT_start_RVA;
				DWORD		HNT_end_RVA;
			};

			ssize_t								nb_imported_dlls = hash_table_get_size(asm_result.imported_libraries);
			DWORD								image_import_descriptors_size = (DWORD)(nb_imported_dlls + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
			DWORD								idata_section_size = 0;
			Array<Library_Import_Computations>	libraries_import_computations;

			// Write libraries import header and fill libraries_import_computations
			{
				IMAGE_IMPORT_DESCRIPTOR	import_descriptor;

				Library_Import_Computations		empty_lib_imp_computations = { };
				Library_Import_Computations*	previous_lib_imp_computations = &empty_lib_imp_computations;
				reserve_array(libraries_import_computations, nb_imported_dlls);

				empty_lib_imp_computations.HNT_end_RVA = image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
					+ image_import_descriptors_size;

				auto it_library = hash_table_begin(asm_result.imported_libraries);
				auto it_library_end = hash_table_end(asm_result.imported_libraries);
				for (ssize_t library_index = 0; !equals<uint16_t, string_view, ASM::Imported_Library*, 32>(it_library, it_library_end); hash_table_next<uint16_t, string_view, ASM::Imported_Library*, 32>(it_library), library_index++)
				{
					ASM::Imported_Library* imported_library = *hash_table_get<uint16_t, string_view, ASM::Imported_Library*, 32>(it_library);
					ssize_t						nb_imported_functions = hash_table_get_size(imported_library->functions);
					Library_Import_Computations	current_lib_imp_computations;

					// Compute few statistics
					{
						current_lib_imp_computations.library_name_RVA = previous_lib_imp_computations->HNT_end_RVA;
						current_lib_imp_computations.ILT_RVA = current_lib_imp_computations.library_name_RVA + (DWORD)get_string_size(imported_library->name) + 1;
						current_lib_imp_computations.IAT_RVA = current_lib_imp_computations.ILT_RVA + (DWORD)(nb_imported_functions + 1) * sizeof(uint64_t);	// @TODO fix it for 32bits
						current_lib_imp_computations.HNT_start_RVA = current_lib_imp_computations.IAT_RVA + (DWORD)(nb_imported_functions + 1) * sizeof(uint64_t);	// @TODO fix it for 32bits
						current_lib_imp_computations.HNT_end_RVA = current_lib_imp_computations.HNT_start_RVA;

						auto it_function = hash_table_begin(imported_library->functions);
						auto it_function_end = hash_table_end(imported_library->functions);
						for (ssize_t function_index = 0; !equals<uint16_t, string_view, ASM::Imported_Function*, 32>(it_function, it_function_end); hash_table_next<uint16_t, string_view, ASM::Imported_Function*, 32>(it_function), function_index++)
						{
							ASM::Imported_Function* imported_function = *hash_table_get<uint16_t, string_view, ASM::Imported_Function*, 32>(it_function);

							current_lib_imp_computations.HNT_end_RVA += sizeof(WORD)	// Hint
								+ (DWORD)get_string_size(imported_function->name) + 1;	// Name

							// Fix RVA of imported functions
							imported_function->RVA_of_IAT_entry = current_lib_imp_computations.IAT_RVA + (uint32_t)function_index * sizeof(uint64_t);	// @TODO change it in 32bits
						}

						array_push_back(libraries_import_computations, current_lib_imp_computations);
						previous_lib_imp_computations = get_array_last_element(libraries_import_computations);
					}

					// RVA to original unbound IAT (PIMAGE_THUNK_DATA) -> mostly rva to ILT
					import_descriptor.OriginalFirstThunk = sections_data[i].section_address + current_lib_imp_computations.ILT_RVA;

					import_descriptor.TimeDateStamp = 0;
					import_descriptor.ForwarderChain = 0; // -1 if no forwarders

					// RVA to DLL name
					import_descriptor.Name = sections_data[i].section_address + current_lib_imp_computations.library_name_RVA;

					// RVA to IAT (if bound this IAT has actual addresses)
					import_descriptor.FirstThunk = sections_data[i].section_address + current_lib_imp_computations.IAT_RVA;

					write_file(output_file, (uint8_t*)&import_descriptor, sizeof(import_descriptor), &bytes_written);
					idata_section_size += bytes_written;
				}
			}
			// Write zeros to terminate import_descriptor table
			write_zeros(output_file, sizeof(IMAGE_IMPORT_DESCRIPTOR));
			idata_section_size += sizeof(IMAGE_IMPORT_DESCRIPTOR);

			// Write libraries import data
			{
				auto	it_library = hash_table_begin(asm_result.imported_libraries);
				auto	it_library_end = hash_table_end(asm_result.imported_libraries);
				for (ssize_t library_index = 0; !equals<uint16_t, string_view, ASM::Imported_Library*, 32>(it_library, it_library_end); hash_table_next<uint16_t, string_view, ASM::Imported_Library*, 32>(it_library), library_index++)
				{
					ASM::Imported_Library* imported_library = *hash_table_get<uint16_t, string_view, ASM::Imported_Library*, 32>(it_library);
					Library_Import_Computations* current_lib_imp_computations = get_array_element(libraries_import_computations, library_index);

					// Write library name
					write_file(output_file, to_utf8(imported_library->name), (uint32_t)get_string_size(imported_library->name), &bytes_written);
					write_zeros(output_file, 1); // add '\0'
					idata_section_size += bytes_written + 1;
					// --


					// ILT and IAT are exactly the same in file, but the loader will resolve addresses for the IAT when loading the binary
					// @TODO do a copy for the IAT after the transition of this code to the memory stream which should allow a copy
					for (size_t j = 0; j < 2; j++)
					{
						LONGLONG	RVA;
						DWORD		current_offset = 0;

						auto it_function = hash_table_begin(imported_library->functions);
						auto it_function_end = hash_table_end(imported_library->functions);
						for (; !equals<uint16_t, string_view, ASM::Imported_Function*, 32>(it_function, it_function_end); hash_table_next<uint16_t, string_view, ASM::Imported_Function*, 32>(it_function))
						{
							ASM::Imported_Function* imported_function = *hash_table_get<uint16_t, string_view, ASM::Imported_Function*, 32>(it_function);

							RVA = sections_data[i].section_address
								+ current_lib_imp_computations->HNT_start_RVA
								+ current_offset;
							write_file(output_file, (uint8_t*)&RVA, sizeof(RVA), &bytes_written);

							current_offset += sizeof(WORD)	// Hint
								+ (DWORD)get_string_size(imported_function->name) + 1;	// Name
							idata_section_size += bytes_written;
						}

						// Write a 0 at the end of each ILT and IAT tables (the loader count the entries of each dll by using the null terminated pattern)
						write_zeros(output_file, sizeof(LONGLONG));
						idata_section_size += sizeof(LONGLONG);
					}

					// HNT (Hint/Name Table)
					{
						IMAGE_IMPORT_BY_NAME name;

						name.Hint = 0; // @TODO @Optimize a proper linker will certainly try to get a good hint to help the loader.

						auto it_function = hash_table_begin(imported_library->functions);
						auto it_function_end = hash_table_end(imported_library->functions);
						for (; !equals<uint16_t, string_view, ASM::Imported_Function*, 32>(it_function, it_function_end); hash_table_next<uint16_t, string_view, ASM::Imported_Function*, 32>(it_function))
						{
							ASM::Imported_Function* imported_function = *hash_table_get<uint16_t, string_view, ASM::Imported_Function*, 32>(it_function);

							write_file(output_file, (uint8_t*)&name.Hint, sizeof(name.Hint), &bytes_written);
							idata_section_size += bytes_written;

							write_file(output_file, (uint8_t*)language::to_utf8(imported_function->name), (DWORD)get_string_size(imported_function->name), &bytes_written); // +1 to write the ending '\0'
							write_zeros(output_file, 1);
							idata_section_size += bytes_written + 1;
						}
					}
				}
			}

			// Size of this section is computed during the generation
			sections_data[i].image_section_header.Misc.VirtualSize = idata_section_size;
			sections_data[i].image_section_header.SizeOfRawData = compute_aligned_size(sections_data[i].image_section_header.Misc.VirtualSize, file_alignment);

			write_zeros(output_file, sections_data[i].image_section_header.SizeOfRawData - idata_section_size);
			size_of_image += compute_aligned_size(sections_data[i].image_section_header.SizeOfRawData, section_alignment);
		}
		else if (sections_data[i].asm_section)
		{
			ZoneScopedN("Write user section");

			write_file(output_file, sections_data[i].asm_section->stream_data, &bytes_written);
			write_zeros(output_file, sections_data[i].image_section_header.SizeOfRawData - bytes_written);
			size_of_image += compute_aligned_size(sections_data[i].image_section_header.SizeOfRawData, section_alignment);
		}

		if (sections_data[i].image_section_header.SizeOfRawData) {	// Basically if the section have no size it was skipped
			previous_section_data = &sections_data[i];
		}
	}



    // Size_Of_Image as it is the size of the image + headers, it means that it is the full size of the file
    // and here we are at the end of file

    // Patching address and sizes

	// @TODO do that in a loop, I should have something similar to value patching in the code section
	// I just need the address, size of value and the value to apply a patch
	// I certainly may remove some duplicated variables that are already in section headers and take values from that

    // size_of_code
    {
        ZoneScopedN("size_of_code");

        DWORD size_of_code_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfCode);
        size_of_code = sections_data[(size_t)Section_Type::TEXT].image_section_header.SizeOfRawData;	 // @Warning size of the sum of all .text sections

        set_file_position(output_file, size_of_code_address);
        write_file(output_file, (uint8_t*)&size_of_code, sizeof(size_of_code), &bytes_written);
    }

    // size_of_initialized_data
    {
        ZoneScopedN("size_of_initialized_data");

        DWORD size_of_initialized_data_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfInitializedData);
        size_of_initialized_data = sections_data[(size_t)Section_Type::TEXT].image_section_header.SizeOfRawData;	 // @Warning size of the sum of all .text sections
        // 1024
        set_file_position(output_file, size_of_initialized_data_address);
        write_file(output_file, (uint8_t*)&size_of_initialized_data, sizeof(size_of_initialized_data), &bytes_written);
    }

    // size_of_uninitialized_data
    {
        ZoneScopedN("size_of_uninitialized_data");

        DWORD size_of_uninitialized_data_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfUninitializedData);
        size_of_uninitialized_data = 0;	 // @Warning size unitialized data in all .text sections

        set_file_position(output_file, size_of_uninitialized_data_address);
        write_file(output_file, (uint8_t*)&size_of_uninitialized_data, sizeof(size_of_uninitialized_data), &bytes_written);
    }

    // address_of_entry_point
    {
        ZoneScopedN("address_of_entry_point");

        DWORD address_of_entry_point_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.AddressOfEntryPoint);
        address_of_entry_point = sections_data[(size_t)Section_Type::TEXT].section_address;
        // 4096
        set_file_position(output_file, address_of_entry_point_address);
        write_file(output_file, (uint8_t*)&address_of_entry_point, sizeof(address_of_entry_point), &bytes_written);
    }

    // base_of_code
    {
        ZoneScopedN("base_of_code");

        DWORD base_of_code_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.BaseOfCode);
        base_of_code = sections_data[(size_t)Section_Type::TEXT].section_address;
        // 4096
        set_file_position(output_file, base_of_code_address);
        write_file(output_file, (uint8_t*)&base_of_code, sizeof(base_of_code), &bytes_written);
    }

#if 0 // Inexistant in x64 only in x86
    // base_of_data
    {
        ZoneScopedN("base_of_data");

        DWORD base_of_data_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.BaseOfData);

        // 8192
        set_file_position(output_file, base_of_data_address);
        write_file(output_file, (uint8_t*)&base_of_data, sizeof(base_of_data), &bytes_written);
    }
#endif

    // size_of_image
    // Gets the size (in bytes) of the image, including all headers, as the image is loaded in memory.
    // The size (in bytes) of the image, which is a multiple of SectionAlignment.
    {
        ZoneScopedN("size_of_image");

        DWORD size_of_image_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfImage);
        //        size_of_image = compute_aligned_size(size_of_image, section_alignment);
                // 
        set_file_position(output_file, size_of_image_address);
        write_file(output_file, (uint8_t*)&size_of_image, sizeof(size_of_image), &bytes_written);
    }

    // size_of_headers (The combined size of an MS DOS stub, PE header, and section headers rounded up to a multiple of FileAlignment.)
    {
        ZoneScopedN("size_of_headers");

        DWORD size_of_header_address = image_nt_header_address + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfHeaders);
        size_of_headers = compute_aligned_size(size_of_headers, file_alignment);
        // 
        set_file_position(output_file, size_of_header_address);
        write_file(output_file, (uint8_t*)&size_of_headers, sizeof(size_of_headers), &bytes_written);
    }

    // text_image_section_virtual_address
	if (sections_data[(size_t)Section_Type::TEXT].asm_section)
    {
        ZoneScopedN("text_image_section_virtual_address");

        DWORD text_image_section_virtual_address_address = sections_data[(size_t)Section_Type::TEXT].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
        sections_data[(size_t)Section_Type::TEXT].image_section_virtual_address = sections_data[(size_t)Section_Type::TEXT].section_address;
		sections_data[(size_t)Section_Type::TEXT].asm_section->position_in_file = sections_data[(size_t)Section_Type::TEXT].image_section_pointer_to_raw_data;
		sections_data[(size_t)Section_Type::TEXT].asm_section->RVA = sections_data[(size_t)Section_Type::TEXT].image_section_virtual_address;

        set_file_position(output_file, text_image_section_virtual_address_address);
        write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::TEXT].image_section_virtual_address, sizeof(sections_data[(size_t)Section_Type::TEXT].image_section_virtual_address), &bytes_written);
    }

    // text_image_section_pointer_to_raw_data
	if (sections_data[(size_t)Section_Type::TEXT].asm_section)
    {
        ZoneScopedN("text_image_section_pointer_to_raw_data");

        DWORD text_image_section_pointer_to_raw_data_address = sections_data[(size_t)Section_Type::TEXT].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
        // text_image_section_pointer_to_raw_data is already computed

        set_file_position(output_file, text_image_section_pointer_to_raw_data_address);
        write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::TEXT].image_section_pointer_to_raw_data, sizeof(sections_data[(size_t)Section_Type::TEXT].image_section_pointer_to_raw_data), &bytes_written);
    }

    // rdata_image_section_virtual_address
	if (sections_data[(size_t)Section_Type::RDATA].asm_section)
    {
        ZoneScopedN("rdata_image_section_virtual_address");

        DWORD rdata_image_section_virtual_address_address = sections_data[(size_t)Section_Type::RDATA].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
        sections_data[(size_t)Section_Type::RDATA].image_section_virtual_address = sections_data[(size_t)Section_Type::RDATA].section_address;
		sections_data[(size_t)Section_Type::RDATA].asm_section->position_in_file = sections_data[(size_t)Section_Type::RDATA].image_section_pointer_to_raw_data;
		sections_data[(size_t)Section_Type::RDATA].asm_section->RVA = sections_data[(size_t)Section_Type::RDATA].image_section_virtual_address;

        set_file_position(output_file, rdata_image_section_virtual_address_address);
        write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::RDATA].image_section_virtual_address, sizeof(sections_data[(size_t)Section_Type::RDATA].image_section_virtual_address), &bytes_written);
    }

    // rdata_image_section_pointer_to_raw_data
	if (sections_data[(size_t)Section_Type::RDATA].asm_section)
	{
        ZoneScopedN("rdata_image_section_pointer_to_raw_data");

        DWORD rdata_image_section_pointer_to_raw_data_address = sections_data[(size_t)Section_Type::RDATA].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
        // rdata_image_section_pointer_to_raw_data is already computed

        set_file_position(output_file, rdata_image_section_pointer_to_raw_data_address);
        write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::RDATA].image_section_pointer_to_raw_data, sizeof(sections_data[(size_t)Section_Type::RDATA].image_section_pointer_to_raw_data), &bytes_written);
    }

	// data_image_section_virtual_address
	if (sections_data[(size_t)Section_Type::DATA].asm_section)
	{
		ZoneScopedN("data_image_section_virtual_address");

		DWORD data_image_section_virtual_address_address = sections_data[(size_t)Section_Type::DATA].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
		sections_data[(size_t)Section_Type::DATA].image_section_virtual_address = sections_data[(size_t)Section_Type::DATA].section_address;
		sections_data[(size_t)Section_Type::DATA].asm_section->position_in_file = sections_data[(size_t)Section_Type::DATA].image_section_pointer_to_raw_data;
		sections_data[(size_t)Section_Type::DATA].asm_section->RVA = sections_data[(size_t)Section_Type::DATA].image_section_virtual_address;

		set_file_position(output_file, data_image_section_virtual_address_address);
		write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::DATA].image_section_virtual_address, sizeof(sections_data[(size_t)Section_Type::DATA].image_section_virtual_address), &bytes_written);
	}

	// data_image_section_pointer_to_raw_data
	if (sections_data[(size_t)Section_Type::DATA].asm_section)
	{
		ZoneScopedN("data_image_section_pointer_to_raw_data");

		DWORD data_image_section_pointer_to_raw_data_address = sections_data[(size_t)Section_Type::DATA].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
		// data_image_section_pointer_to_raw_data is already computed

		set_file_position(output_file, data_image_section_pointer_to_raw_data_address);
		write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::DATA].image_section_pointer_to_raw_data, sizeof(sections_data[(size_t)Section_Type::DATA].image_section_pointer_to_raw_data), &bytes_written);
	}

#if DLL_MODE == 1
    // reloc_image_section_virtual_address
    {
        ZoneScopedN("reloc_image_section_virtual_address");

        DWORD reloc_image_section_virtual_address_address = reloc_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
        reloc_image_section_virtual_address = reloc_section_address;

        set_file_position(output_file, reloc_image_section_virtual_address_address);
        write_file(output_file, (uint8_t*)&reloc_image_section_virtual_address, sizeof(reloc_image_section_virtual_address), &bytes_written);
    }

    // reloc_image_section_pointer_to_raw_data
    {
        ZoneScopedN("reloc_image_section_pointer_to_raw_data");

        DWORD reloc_image_section_pointer_to_raw_data_address = reloc_image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
        // reloc_image_section_pointer_to_raw_data is already computed

        set_file_position(output_file, reloc_image_section_pointer_to_raw_data_address);
        write_file(output_file, (uint8_t*)&reloc_image_section_pointer_to_raw_data, sizeof(reloc_image_section_pointer_to_raw_data), &bytes_written);
    }
#endif

	// IDATA header (some values were computed during the generation/write of this section)
	{
		ZoneScopedN("IDATA header patching");

		DWORD idata_image_section_virtual_size_address = sections_data[(size_t)Section_Type::IDATA].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, Misc.VirtualSize);
		set_file_position(output_file, idata_image_section_virtual_size_address);
		write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::IDATA].image_section_header.Misc.VirtualSize, sizeof(sections_data[(size_t)Section_Type::IDATA].image_section_header.Misc.VirtualSize), &bytes_written);

		DWORD idata_image_section_size_of_raw_data_address = sections_data[(size_t)Section_Type::IDATA].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, SizeOfRawData);
		set_file_position(output_file, idata_image_section_size_of_raw_data_address);
		write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::IDATA].image_section_header.SizeOfRawData, sizeof(sections_data[(size_t)Section_Type::IDATA].image_section_header.SizeOfRawData), &bytes_written);
	}

    // idata_image_section_virtual_address
    {
        ZoneScopedN("idata_image_section_virtual_address");

        DWORD idata_image_section_virtual_address_address = sections_data[(size_t)Section_Type::IDATA].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, VirtualAddress);
		sections_data[(size_t)Section_Type::IDATA].image_section_virtual_address = sections_data[(size_t)Section_Type::IDATA].section_address;

        set_file_position(output_file, idata_image_section_virtual_address_address);
        write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::IDATA].image_section_virtual_address, sizeof(sections_data[(size_t)Section_Type::IDATA].image_section_virtual_address), &bytes_written);
    }

    // idata_image_section_pointer_to_raw_data
    {
        ZoneScopedN("idata_image_section_pointer_to_raw_data");

        DWORD idata_image_section_pointer_to_raw_data_address = sections_data[(size_t)Section_Type::IDATA].image_section_header_address + offsetof(IMAGE_SECTION_HEADER, PointerToRawData);
        // idata_image_section_pointer_to_raw_data is already computed

        set_file_position(output_file, idata_image_section_pointer_to_raw_data_address);
        write_file(output_file, (uint8_t*)&sections_data[(size_t)Section_Type::IDATA].image_section_pointer_to_raw_data, sizeof(sections_data[(size_t)Section_Type::IDATA].image_section_pointer_to_raw_data), &bytes_written);
    }

	// NT HEADER (some values were computed during the generation/write of some sections)
	// @Warning done at end because some computation also happens during the patching process
	{
		{
			DWORD nt_header_data_directory_entry_import_virtual_address_address = image_nt_header_address
				+ offsetof(IMAGE_NT_HEADERS64, OptionalHeader)
				+ offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT])
				+ offsetof(IMAGE_DATA_DIRECTORY, VirtualAddress);
			image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = sections_data[(size_t)Section_Type::IDATA].image_section_virtual_address;

			set_file_position(output_file, nt_header_data_directory_entry_import_virtual_address_address);
			write_file(output_file, (uint8_t*)&image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress, sizeof(image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress), &bytes_written);
		}

		{
			// @TODO check if it is valid and/or optimal to put here address and size of the entire idata section because IAT is only a part in the idata section

			DWORD nt_header_data_directory_entry_IAT_virtual_address_address = image_nt_header_address
				+ offsetof(IMAGE_NT_HEADERS64, OptionalHeader)
				+ offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT])
				+ offsetof(IMAGE_DATA_DIRECTORY, VirtualAddress);
			image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = sections_data[(size_t)Section_Type::IDATA].image_section_virtual_address;

			set_file_position(output_file, nt_header_data_directory_entry_IAT_virtual_address_address);
			write_file(output_file, (uint8_t*)&image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress, sizeof(image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress), &bytes_written);

			// @TODO Optimize (should I compute the address to patch and do the file_position?)
			DWORD nt_header_data_directory_entry_IAT_size_address = image_nt_header_address
				+ offsetof(IMAGE_NT_HEADERS64, OptionalHeader)
				+ offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT])
				+ offsetof(IMAGE_DATA_DIRECTORY, Size);
			image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = sections_data[(size_t)Section_Type::IDATA].image_section_header.SizeOfRawData;

			set_file_position(output_file, nt_header_data_directory_entry_IAT_size_address);
			write_file(output_file, (uint8_t*)&image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size, sizeof(image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size), &bytes_written);
		}
	}

	// Patching addresses
	{
		ZoneScopedN("Patching addresses");

		for (ssize_t i_section = 0; i_section < container::get_array_size(asm_result.sections); i_section++)
		{
			ASM::Section* section = container::get_array_element(asm_result.sections, i_section);
			for (ssize_t j_add_to_patch = 0; j_add_to_patch < container::get_array_size(section->addr_to_patch); j_add_to_patch++)
			{
				ASM::ADDR_TO_PATCH* addr_to_patch = container::get_array_element(section->addr_to_patch, j_add_to_patch);
				ASM::Label**		found_label;

				uint64_t label_hash = SpookyHash::Hash64((const void*)to_utf8(addr_to_patch->label), get_string_size(addr_to_patch->label), 0);
				uint16_t label_short_hash = label_hash & 0xffff;

				found_label = fstd::container::hash_table_get(asm_result.labels, label_short_hash, addr_to_patch->label);
				if (!found_label) {
					report_error(Compiler_Error::internal_error, "Unable to find a label to patch an address in generated machine code.");
				}

				ASM::Label*	label = *found_label;
				uint32_t	addr;	// @Warning even in 64bits we do only 32bits relative to RIP (Re-extended Instruction Pointer) register displacement

				// @TODO handle function that are inside the text section, instead of in idata section
				// May work directly as for labels pointing to data
				if (label->imported_function) {
					addr = label->imported_function->RVA_of_IAT_entry - sections_data[(size_t)Section_Type::IDATA].section_address - addr_to_patch->addr_of_addr - sizeof(addr);
				}
				else if (label->section) {
					addr = label->section->RVA + label->RVA - section->RVA - addr_to_patch->addr_of_addr - sizeof(addr);	// @Warning the sizeof(addr) is the size of the addr we patch
				}

				set_file_position(output_file, section->position_in_file + addr_to_patch->addr_of_addr);
				write_file(output_file, (uint8_t*)&addr, sizeof(addr), &bytes_written);
			}
		}
	}

    close_file(output_file);
}
