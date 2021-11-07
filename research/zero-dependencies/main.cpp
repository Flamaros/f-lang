// https://hero.handmade.network/forums/code-discussion/t/129-howto_-_building_without_import_libraries



#include <intrin.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>
#include <stdint.h>
#include <stdbool.h>

#define PE_GET_OFFSET(module, offset) ((uint8_t*)(module) + offset)

typedef struct win32_teb
{
    char Padding1[0x60];
    PPEB peb;
} win32_teb;

typedef struct win32_msdos
{
    uint8_t Padding1[0x3C];
    uint32_t PEOffset;
} win32_msdos;

typedef struct win32_pe_image_data
{
    uint32_t VirtualAddress;
    uint32_t Size;
} win32_pe_image_data;

typedef struct win32_pe
{
    // COFF
    uint8_t Signature[4];
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;

    // Assuming PE32+ Optional Header since this is 64bit only
    // standard fields
    uint16_t Magic; 
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;

    // windows specific fields
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;

    // data directories
    win32_pe_image_data ExportTable;
    win32_pe_image_data ImportTable;
    win32_pe_image_data ResourceTable;
    win32_pe_image_data ExceptionTable;
    win32_pe_image_data CertificateTable;
    win32_pe_image_data BaseRelocationTable;
    win32_pe_image_data Debug;
    win32_pe_image_data Architecture;
    win32_pe_image_data GlobalPtr;
    win32_pe_image_data TLSTable;
    win32_pe_image_data LoadConfigTable;
    win32_pe_image_data BoundImport;
    win32_pe_image_data IAT;
    win32_pe_image_data DelayImportDescriptor;
    win32_pe_image_data CLRRuntimeHeader;
    win32_pe_image_data ReservedTable;
} win32_pe;

typedef struct win32_pe_export_table
{
    uint32_t ExportFlags;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t NameRVA;
    uint32_t OrdinalBase;
    uint32_t AddressTableEntries;
    uint32_t NumberofNamePointers;
    uint32_t ExportAddressTableRVA;
    uint32_t NamePointerRVA;
    uint32_t OrdinalTableRVA;
} win32_pe_export_table;

typedef struct win32_ldr_data_entry
{
    LIST_ENTRY LinkedList;
    LIST_ENTRY UnusedList;
    PVOID BaseAddress;
    PVOID Reserved2[1];
    PVOID DllBase;
    PVOID EntryPoint;
    PVOID Reserved3;
    USHORT DllNameLength;
    USHORT DllNameMaximumLength;
    PWSTR  DllNameBuffer;
} win32_ldr_data_entry;

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow) {
	win32_teb *teb = (win32_teb*)__readgsqword(0x30);
	PLIST_ENTRY head = &teb->peb->Ldr->InMemoryOrderModuleList;
	void *kernel32_module = NULL;
	
	const wchar_t kernel32_name[] = L"KERNEL32.DLL";
	    
	PLIST_ENTRY entry = head->Flink;
	while (entry != head) {
		win32_ldr_data_entry *ldr_entry = (win32_ldr_data_entry*)entry;
		
		bool are_the_same = true;
		const int size_krn32 = sizeof(kernel32_name);
		const int size_buf = ldr_entry->DllNameLength;
		const int size = (size_krn32 < size_buf? size_krn32 : size_buf) / sizeof(wchar_t);
		for(int i = 0; i < size; ++i) {
			if(kernel32_name[i] != ldr_entry->DllNameBuffer[i]) {
				are_the_same = false;
				break;
			}
		}
		if(are_the_same) {
			kernel32_module = ldr_entry->BaseAddress;
			break;
		}
		
		entry = entry->Flink;
	}
	
	win32_msdos *dos_header = (win32_msdos*)PE_GET_OFFSET(kernel32_module, 0);
	win32_pe *pe_header= (win32_pe*)PE_GET_OFFSET(kernel32_module, dos_header->PEOffset);
	win32_pe_export_table *export_table = (win32_pe_export_table*)PE_GET_OFFSET(kernel32_module, pe_header->ExportTable.VirtualAddress);
	uint32_t* name_ptr_table = (uint32_t*)PE_GET_OFFSET(kernel32_module, export_table->NamePointerRVA);
	
	int index_into_ordinal_table = 0;
	for(int i = 0; i < export_table->NumberofNamePointers ; ++i) {
		const char get_proc_addr_name[] = "GetProcAddress";
		bool are_the_same = true;
		const char *proc_name = (const char*)PE_GET_OFFSET(kernel32_module, name_ptr_table[i]);
		for(int j = 0; proc_name[j] && get_proc_addr_name[j]; ++j) {
			if(proc_name[j] != get_proc_addr_name[j]) {
				are_the_same = false;
				break;
			}
		}
		if(are_the_same) {
			index_into_ordinal_table = i;
			break;
		}
	}
	
	uint16_t* ordinal_table = (uint16_t*)PE_GET_OFFSET(kernel32_module, export_table->OrdinalTableRVA);
	uint16_t gpa_ordinal = ordinal_table[index_into_ordinal_table];
	uint32_t* export_adress_table = (uint32_t*)PE_GET_OFFSET(kernel32_module, export_table->ExportAddressTableRVA);
	uint32_t gpa_rva = export_adress_table[gpa_ordinal];
	
	typedef FARPROC(*GetProcAddress_Type)(
		HMODULE hModule,
		LPCSTR  lpProcName
	);
	
	GetProcAddress_Type My_GetProcAddress = (GetProcAddress_Type)PE_GET_OFFSET(kernel32_module, gpa_rva);
	
	typedef HMODULE(*LoadLibraryA_Type)(
		LPCSTR lpLibFileName
	);
	LoadLibraryA_Type LoadLibraryA_ = (LoadLibraryA_Type)My_GetProcAddress(kernel32_module,"LoadLibraryA");
	
	typedef BOOL (* AllocConsole_Type)(void);
	
	AllocConsole_Type AllocConsole_ = (AllocConsole_Type)My_GetProcAddress(kernel32_module, "AllocConsole");
	
	AllocConsole_();
	
	typedef BOOL(*WriteConsoleA_Type)(
		HANDLE  hConsoleOutput,
		const VOID    *lpBuffer,
		DWORD   nNumberOfCharsToWrite,
		LPDWORD lpNumberOfCharsWritten
	);
	
	typedef HANDLE(*GetStdHandle_Type)(
		DWORD nStdHandle
	);
	
	WriteConsoleA_Type WriteConsoleA_ = (WriteConsoleA_Type)My_GetProcAddress(kernel32_module, "WriteConsoleA");
	GetStdHandle_Type GetStdHandle_ = (GetStdHandle_Type)My_GetProcAddress(kernel32_module, "GetStdHandle");
	
	const char hello_world[] = "Hello World!"; 
	DWORD written;
	
	WriteConsoleA_(GetStdHandle_(STD_OUTPUT_HANDLE),hello_world, sizeof(hello_world) - written, &written);
	
	while(true);
	
	return 69420;
}