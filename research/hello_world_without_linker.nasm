; How to build (you'll need nasm compiler)

; Be sure that nasm compiler path is in your PATH env
; nasm hello_world_without_linker.nasm -o hello_world_without_linker.exe
; 
; Here we use some specific nasm instructions to generate the binary
; we can directly tell to nasm what to write in the output file, so we do it
; this will help us to be closer of what we want to be able to do later in cpp
; with our compiler.


IMAGEBASE equ 400000h			; Image Base address

; Few Win32 defines I use
IMAGE_FILE_EXECUTABLE_IMAGE		equ 0002h
IMAGE_FILE_32BIT_MACHINE		equ 0100h
IMAGE_NT_OPTIONAL_HDR32_MAGIC	equ 010bh
IMAGE_SUBSYSTEM_WINDOWS_CUI		equ 2


BITS 32							; Target processor mode (32 bits here)
ORG IMAGEBASE					; Set origin of the executable in RAM when Windows will load it



; IMAGE_DOS_HEADER
image_start:
	dw "MZ"						; e_magic;                     // Magic number
    dw 0						; e_cblp;                      // Bytes on last page of file
    dw 0						; e_cp;                        // Pages in file
    dw 0						; e_crlc;                      // Relocations
    dw 0						; e_cparhdr;                   // Size of header in paragraphs
    dw 0						; e_minalloc;                  // Minimum extra paragraphs needed
    dw 0						; e_maxalloc;                  // Maximum extra paragraphs needed
    dw 0						; e_ss;                        // Initial (relative) SS value
    dw 0						; e_sp;                        // Initial SP value
    dw 0						; e_csum;                      // Checksum
    dw 0						; e_ip;                        // Initial IP value
    dw 0						; e_cs;                        // Initial (relative) CS value
    dw 0						; e_lfarlc;                    // File address of relocation table
    dw 0						; e_ovno;                      // Overlay number
    dw 0,0,0,0					; e_res[4];                    // Reserved words
    dw 0						; e_oemid;                     // OEM identifier (for e_oeminfo)
    dw 0						; e_oeminfo;                   // OEM information; e_oemid specific
    dw 0,0,0,0,0,0,0,0,0,0		; e_res2[10];                  // Reserved words
    dd 0x40						; e_lfanew;                    // File address of new exe header (sizeof(IMAGE_DOS_HEADER) because we put it just after)

; IMAGE_NT_HEADERS32 - lowest possible start is at 0x4
signature:
	dw 'PE',0														; Signature

; IMAGE_NT_HEADERS32::IMAGE_FILE_HEADER
		dw 0x14c													; Machine = IMAGE_FILE_MACHINE_I386
		dw 0														; NumberOfSections
		dd 0														; TimeDateStamp @TODO find the Win32 equivalent to time(NULL) that is a libc function
		dd 0														; PointerToSymbolTable
		dd 0														; NumberOfSymbols
		dw 0														; SizeOfOptionalHeader
		dw IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE	; Characteristics

; IMAGE_NT_HEADERS32::IMAGE_OPTIONAL_HEADER32
		dw IMAGE_NT_OPTIONAL_HDR32_MAGIC							; Magic
		db 0														; MajorLinkerVersion
		db 1														; MinorLinkerVersion
		dd 0														; SizeOfCode - The size of the code (text) section, or the sum of all code sections if there are multiple sections. 
		dd 0														; SizeOfInitializedData
		dd 0														; SizeOfUninitializedData
		dd _main - IMAGEBASE										; AddressOfEntryPoint
		dd 0														; BaseOfCode
		dd 0														; BaseOfData
		dd IMAGEBASE												; ImageBase
		dd 4														; SectionAlignment - overlapping address with IMAGE_DOS_HEADER.e_lfanew
		dd 4														; FileAlignment
		dw 0														; MajorOperatingSystemVersion
		dw 0														; MinorOperatingSystemVersion
		dw 0														; MajorImageVersion
		dw 0														; MinorImageVersion
		dw 4														; MajorSubsystemVersion
		dw 0														; MinorSubsystemVersion
		dd 0														; Win32VersionValue
		dd image_end - image_start									; SizeOfImage
		dd 0														; SizeOfHeaders
		dd 0														; CheckSum
		dw IMAGE_SUBSYSTEM_WINDOWS_CUI								; Subsystem
		dw 0														; DllCharacteristics
		dd 0														; SizeOfStackReserve
		dd 0														; SizeOfStackCommit
		dd 0														; SizeOfHeapReserve
		dd 0														; SizeOfHeapCommit
		dd 0														; LoaderFlags
		dd 3														; NumberOfRvaAndSizes (@Warning we have 3 directory_entry_import)

; IMAGE_DIRECTORY_ENTRY_EXPORT
			dd 0													; VirtualAddress
			dd 0													; Size

; IMAGE_DIRECTORY_ENTRY_IMPORT
			dd IMAGE_IMPORT_DESCRIPTOR - IMAGEBASE					; VirtualAddress
			dd 0													; Size

kernel32.dll_iat:
GetStdHandle:
  dd import_by_name_GetStdHandle - IMAGEBASE
  dd 0
WriteFile:
  dd import_by_name_WriteFile - IMAGEBASE
  dd 0
ExitProcess:
  dd import_by_name_ExitProcess - IMAGEBASE
  dd 0
kernel32.dll_hintnames:
  dd import_by_name_GetStdHandle - IMAGEBASE
  dw 0


import_by_name_GetStdHandle:										; IMAGE_IMPORT_BY_NAME
  dw 0																; Hint, terminate list before
  db 'GetStdHandle'													; Name

import_by_name_WriteFile:											; IMAGE_IMPORT_BY_NAME
  dw 0																; Hint, terminate list before
  db 'WriteFile'													; Name

import_by_name_ExitProcess:											; IMAGE_IMPORT_BY_NAME
  dw 0																; Hint, terminate list before
  db 'ExitProcess'													; Name

IMAGE_IMPORT_DESCRIPTOR:
; IMAGE_IMPORT_DESCRIPTOR for kernel32.dll
  dd kernel32.dll_hintnames - IMAGEBASE								; OriginalFirstThunk / Characteristics













   global _main
;    extern  _GetStdHandle@4
;    extern  _WriteFile@20
;    extern  _ExitProcess@4

section .text

_main:
    ; DWORD  bytes;    
    mov     ebp, esp
    sub     esp, 4

    ; hStdOut = GetstdHandle(STD_OUTPUT_HANDLE)
    push    -11
    call    [GetStdHandle]
    mov     ebx, eax    

    ; WriteFile( hstdOut, message, length(message), &bytes, 0);
    push    0
    lea     eax, [ebp - 4]
    push    eax
    push    (message_end - message)
    push    message
    push    ebx
    call    [WriteFile]

    ; ExitProcess(0)
    push    0
    call    [ExitProcess]

    ; never here
    hlt

message:
    db      'Hello World', 9

message_end:
image_end:
