// standard headers
#include <stdint.h>
#include <stdbool.h>

// windows headers
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#define IMAGE_BASE 0x00400000
#define STRING_LIT(string) ((struct String){(char*)(string), (uint32_t)sizeof(string) - 1})
#define STACK_MAX 512
#define TO_STRING_2(x) #x
#define TO_STRING(x) TO_STRING_2(x)

#if _MSC_VER && !__clang__
#define REAL_MSVC 1
#endif

#if REAL_MSVC
#define NO_RTC __declspec(safebuffers)
#else
#define NO_RTC
#endif

struct String
{
    char *data;
    uint32_t length;
};

NO_RTC static void print_error_message(struct String const *const string)
{
    WriteFile((HANDLE)(uintptr_t)STD_ERROR_HANDLE,
              string->data, string->length,
              &(DWORD){0}, NULL);
}

// TODO: actually convert utf-16 to utf-8
NO_RTC static void print_error_message_wide(wchar_t const *string)
{
    char buffer[255];
    for (uint32_t i = 0;; ++i, ++string)
    {
        if (i == sizeof buffer || *string == L'\0')
        {
            WriteFile((HANDLE)(uintptr_t)STD_ERROR_HANDLE,
                      buffer, i, &(DWORD){0}, NULL);
        }
                
        buffer[i] = (char)*string;
    }
}

NO_RTC static size_t size_t_min(size_t const a, size_t const b)
{
    return a < b ? a : b;
}

NO_RTC static int64_t i64_min(int64_t const a, int64_t const b)
{
    return a < b ? a : b;
}

NO_RTC static void memory_copy(void *const dest,
                               void const *const source,
                               size_t count)
{
    char *dest8 = dest;
    char const *source8 = source;
    
    while(count-- != 0) *dest8++ = *source8++;
}

#if REAL_MSVC
#pragma function(memset)
#endif
void *memset(void *dest, int c, size_t count)
{
    char *bytes = (char *)dest;
    while (count--)
    {
            *bytes++ = (char)c;
    }
    
    return dest;
}

struct PeBuffer
{
    uint8_t data[1024];
    uint32_t offset;
} PeBuffer;

NO_RTC static void pe_buffer_write(struct PeBuffer *const pe_buffer,
                                   void const *const data,
                                   uint32_t const length)
{
    memory_copy(pe_buffer->data + pe_buffer->offset, data, length);
    pe_buffer->offset += length;
}

struct OffsetEntry
{
    uint8_t value;
    bool in_use;
};

struct OffsetTable
{
    struct OffsetEntry data[256];
    int32_t offset;
};

enum IRType
{
    IR_ADD_PTR,
    IR_ADD_VAL,
    IR_WHILE_START,
    IR_WHILE_END,
    IR_NOP,
    IR_PRINT,
    IR_INPUT,
};

struct IROp
{
    enum IRType type;
    uint32_t value;
};

NO_RTC static struct IROp next_ir(char const **code)
{
    struct IROp result = { .type = IR_NOP };

    for (;;)
    {
        switch (**code)
        {
            case '+':
            {
                if (result.type == IR_ADD_PTR)
                {
                    return result;
                }

                result.type = IR_ADD_VAL;
                ++result.value;

                ++*code;
                break;
            }

            case '-':
            {
                if (result.type == IR_ADD_PTR)
                {
                    return result;
                }

                result.type = IR_ADD_VAL;
                --result.value;

                ++*code;
                break;
            }
        
            case '>':
            {
                if (result.type == IR_ADD_VAL)
                {
                    return result;
                }

                result.type = IR_ADD_PTR;
                ++result.value;

                ++*code;
                break;
            }

            case '<':
            {
                if (result.type == IR_ADD_VAL)
                {
                    return result;
                }

                result.type = IR_ADD_PTR;
                --result.value;

                ++*code;
                break;
            }

            case '[':
            {
                if (result.type != IR_NOP) return result;

                result.type = IR_WHILE_START;
            
                ++*code;
                return result;
            }

            case ']':
            {
                if (result.type != IR_NOP) return result;

                result.type = IR_WHILE_END;
            
                ++*code;
                return result;
            }
            
            case '\0':
            {
                return result;
            }

            case '.':
            {            
                if (result.type != IR_NOP) return result;

                result.type = IR_PRINT;

                ++*code;
                return result;
            }

            case ',':
            {            
                if (result.type != IR_NOP) return result;

                result.type = IR_INPUT;
            
                ++*code;
                return result;
            }

            default: ++*code; break;
        }
    }
}

struct InsVec
{
    uint8_t *data;
    size_t size;
    size_t entry_offset;
};

NO_RTC static void ins_vec_init(struct InsVec *const self)
{
    self->data = VirtualAlloc(NULL,
                              0x80000000,
                              MEM_RESERVE,
                              PAGE_READWRITE);

    VirtualAlloc(self->data,
                 0x100000,
                 MEM_COMMIT,
                 PAGE_READWRITE);
}

NO_RTC static void ins_vec_append(struct InsVec *const self,
                                  void const *data,
                                  size_t const size)
{
    if ((self->size & (size_t)-0x100000) !=
        ((self->size + size) & (size_t)-0x100000))
    {
        VirtualAlloc(self->data + self->size + size,
                     0x100000,
                     MEM_COMMIT,
                     PAGE_READWRITE);
    }
    
    memory_copy(self->data + self->size, data, size);
    self->size += size;
}

NO_RTC static void add_entry_instruction(struct InsVec *const ins_vec,
                                         struct OffsetEntry const *const entry)
{
    switch ((int8_t)entry->value)
    {
        case 1:
        {
            // inc byte [rdx]
            ins_vec_append(ins_vec, "\xFE\x02", 2);
            break;
        }

        case -1:
        {
            // dec byte [rdx]
            ins_vec_append(ins_vec, "\xFE\x0A", 2);
            break;
        }

        default:
        {
            // add byte [rdx], entry.value
            ins_vec_append(ins_vec, "\x80\x02", 2);
            ins_vec_append(ins_vec, &entry->value, 1);
                        
            break;
        }
    }
}

NO_RTC static void flush_offset_table(struct InsVec *const ins_vec,
                                      struct OffsetTable *const table)
{
    struct OffsetEntry matching_offset_entry = {0};
    for(int8_t offset = INT8_MIN; offset != INT8_MAX; ++offset)
    {
        struct OffsetEntry *const entry = &table->data[(uint8_t)offset];
        if (!entry->in_use) continue;

        entry->in_use = false;        
        if (entry->value == 0) continue;
            
        if (offset == 0)
        {
            add_entry_instruction(ins_vec, entry);
        }
        else
        {
            if (!matching_offset_entry.in_use &&
                offset == table->offset)
            {
                matching_offset_entry = *entry;
                matching_offset_entry.in_use = true;
                
                goto matching_offset;
            }
            
            switch ((int8_t)entry->value)
            {
                case 1:
                {
                    // inc byte [rdx + offset]
                    ins_vec_append(ins_vec, "\xFE\x42", 2);
                    ins_vec_append(ins_vec, &offset, 1);
                    
                    break;
                }

                case -1:
                {
                    // dec byte [rdx + entry.value]
                    ins_vec_append(ins_vec, "\xFE\x4A", 2);
                    ins_vec_append(ins_vec, &offset, 1);
                    
                    break;
                }

                default:
                {
                    // add byte [rdx + offset], entry.value
                    ins_vec_append(ins_vec, "\x80\x42", 2);
                    ins_vec_append(ins_vec, &offset, 1);
                    ins_vec_append(ins_vec, &entry->value, 1);
                    
                    break;
                }
            }
        }

matching_offset:
        entry->value = 0;
    }

    if (table->offset != 0)
    {
        if (table->offset < INT8_MIN || table->offset > INT8_MAX)
        {
            ins_vec_append(ins_vec, "\x48\x81\xC2", 3);
            ins_vec_append(ins_vec, &table->offset, 4);
        }
        else
        {
            ins_vec_append(ins_vec, "\x48\x83\xC2", 3);
            ins_vec_append(ins_vec, &(uint8_t){table->offset}, 1);
        }
    }

    if (matching_offset_entry.in_use)
    {
        add_entry_instruction(ins_vec, &matching_offset_entry);
    }

    table->offset = 0;
}

NO_RTC static struct InsVec compile_bf(char const *source,
                                       int32_t const iat_table_offset)
{
    struct Stack
    {
        size_t data[STACK_MAX];
        size_t ptr;
    } stack;
    stack.ptr = 0;
    
    struct InsVec result = {0};
    ins_vec_init(&result);
    
    struct OffsetTable offset_table = {0};

    // sub rsp, 48
    // mov [rsp + 40], rdx
    ins_vec_append(&result,
                   "\x48\x83\xEC\x30"
                   "\x48\x89\x54\x24\x28", 5 + 4);

    static uint8_t const WriteFile_call[] = {
        0xB9, 0xF5, 0xFF, 0xFF, 0xFF, // mov ecx, -11
        0x41, 0xB8, 0x01, 0x00, 0x00, 0x00, // mov r8d, 1
        0x45, 0x31, 0xC9, // xor r9d, r9d
        0x4C, 0x89, 0x4C, 0x24, 0x20 //  mov qword [rsp + 32], r9
    };
                
    ins_vec_append(&result, WriteFile_call, sizeof WriteFile_call);
                
    // call [rip + __imp_WriteFile]
    ins_vec_append(&result, "\xFF\x15", 2);
    ins_vec_append(&result,
                   &(int32_t)
                   {
                       (iat_table_offset + 8) -
                       (int32_t)(result.size + 4)
                   },
                   sizeof(int32_t));
                
    // mov rdx, [rsp + 40]
    // add rsp, 48
    // ret
    ins_vec_append(&result,
                   "\x48\x8B\x54\x24\x28"
                   "\x48\x83\xC4\x30\xC3", 5 + 5);

    int32_t input_char_location = (int32_t)result.size;

    // sub rsp, 48
    // mov [rsp + 40], rdx
    ins_vec_append(&result,
                   "\x48\x83\xEC\x30"
                   "\x48\x89\x54\x24\x28", 5 + 4);

    static uint8_t const ReadFile_call[] = {
        0xB9, 0xF6, 0xFF, 0xFF, 0xFF, // mov ecx, -10
        0x41, 0xB8, 0x01, 0x00, 0x00, 0x00, // mov r8d, 1
        0x45, 0x31, 0xC9, // xor r9d, r9d
        0x4C, 0x89, 0x4C, 0x24, 0x20 //  mov qword [rsp + 32], r9
    };
                
    ins_vec_append(&result, ReadFile_call, sizeof ReadFile_call);
                
    // call [rip + __imp_ReadFile]
    ins_vec_append(&result, "\xFF\x15", 2);
    ins_vec_append(&result,
                   &(int32_t)
                   {
                       (iat_table_offset + 0) -
                       (int32_t)(result.size + 4)
                   },
                   sizeof(int32_t));                
    
    // mov rdx, [rsp + 40]
    // add rsp, 48
    // ret
    ins_vec_append(&result,
                   "\x48\x8B\x54\x24\x28"
                   "\x48\x83\xC4\x30\xC3", 5 + 5);

    result.entry_offset = result.size;
        
    /*
      push rdi
      sub rsp, (30000 + 32 + 8 + 8)
      lea rdi, [rsp + 48]
      mov rdx, rdi
      mov ecx, 30000
      xor eax, eax
      rep stosb
    */

    static uint8_t const jit_prolog[] = {
        0x57,
        0x48, 0x81, 0xEC, 0x60, 0x75, 0x00, 0x00,
        0x48, 0x8D, 0x7C, 0x24, 0x30,
        0x48, 0x89, 0xFA,
        0xB9, 0x30, 0x75, 0x00, 0x00,
        0x31, 0xC0,
        0xF3, 0x48, 0xAA,
    };

    ins_vec_append(&result, jit_prolog, sizeof(jit_prolog));

    while(*source != '\0')
    {
        struct IROp const ir = next_ir(&source);
        switch (ir.type)
        {
            case IR_ADD_VAL:
            {
                // add to the current offset entry
                offset_table.data[(uint8_t)offset_table.offset].in_use = true;
                offset_table.data[(uint8_t)offset_table.offset].value += ir.value;

                break;
            }
            
            case IR_ADD_PTR:
            {
                // add to the current offset entry
                offset_table.offset += ir.value;

                if (offset_table.offset < INT8_MIN ||
                    offset_table.offset > INT8_MAX)
                {
                    flush_offset_table(&result, &offset_table);
                }
                
                break;
            }
            
            case IR_WHILE_START:
            {
                flush_offset_table(&result, &offset_table);
                
                char const *peek_source = source;
                struct IROp const peek_ir = next_ir(&peek_source); 
                if (peek_ir.type == IR_ADD_VAL &&
                    (peek_ir.value == 1 ||
                     peek_ir.value == (uint32_t)-1) &&
                    next_ir(&peek_source).type == IR_WHILE_END)
                {
                    source = peek_source;
                    
                    // mov byte [rdx], 0
                    ins_vec_append(&result, "\xC6\x02\x00", 3);
                    break;
                }
                
                if (stack.ptr >= STACK_MAX)
                {
                    print_error_message(&STRING_LIT("error: stack size greater than "
                                                    TO_STRING(STACK_MAX) "\n"));
                    return result;
                }

                stack.data[stack.ptr++] = result.size + 5;
                
                static uint8_t const branch_top[] = {
                    0x80, 0x3A, 0x00, // cmp byte [rdx], 0
                    0x0F, 0x84, 0x00, 0x00, 0x00, 0x00, // je xxxxxxxx
                };
                
                ins_vec_append(&result, branch_top, sizeof(branch_top));
                break;
            }
            
            case IR_WHILE_END:
            {
                if (stack.ptr == 0)
                {
                    print_error_message(&STRING_LIT("error: unexpected ']'\n"));
                    return result;
                }

                flush_offset_table(&result, &offset_table);
                
                int64_t const stack_top = stack.data[--stack.ptr];
                
                int32_t const jmp_offset = -((result.size + 5) - (stack_top - 5));
                ins_vec_append(&result, "\xE9", 1);
                ins_vec_append(&result, &jmp_offset, 4);
                
                *(int32_t*)&result.data[stack_top] = result.size - (stack_top + 4);
                break;
            }
            
            case IR_PRINT:
            {
                flush_offset_table(&result, &offset_table);
                ins_vec_append(&result, "\xE8", 1);
                ins_vec_append(&result,
                               &(int32_t)
                               {
                                   -((int32_t)result.size + 4)
                               }, sizeof(int32_t));
                
                break;
            }
            
            case IR_INPUT:
            {
                flush_offset_table(&result, &offset_table);
                ins_vec_append(&result, "\xE8", 1);
                ins_vec_append(&result,
                               &(int32_t)
                               {
                                   input_char_location
                                   -((int32_t)result.size + 4)
                               }, sizeof(int32_t));
                break;
            }

            case IR_NOP: break;
        }
    }
    
    // add rsp, (30000 + 32 + 8 + 8)
    // pop rdi
    // ret
    ins_vec_append(&result, "\x48\x81\xC4\x60\x75\x00\x00\x5F\xC3", 9);
    
    return result;
}

NO_RTC static char const *read_entire_file(wchar_t const *const filepath)
{
    HANDLE file_handle;
    if (filepath != NULL)
    {
        file_handle = CreateFileW(filepath,
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        
        if (file_handle == INVALID_HANDLE_VALUE)
        {
            print_error_message(&STRING_LIT("error: could not open "));
            print_error_message_wide(filepath);
            print_error_message(&STRING_LIT("\n"));
            return NULL;
        }
    }
    else
    {
        file_handle = GetStdHandle(STD_INPUT_HANDLE);
    }
    
    switch(GetFileType(file_handle))
    {
        case FILE_TYPE_DISK:
        {
            LARGE_INTEGER file_size;
            GetFileSizeEx(file_handle, &file_size);
                          
            char *const result = VirtualAlloc(NULL,
                                              file_size.QuadPart,
                                              MEM_RESERVE | MEM_COMMIT,
                                              PAGE_READWRITE);

            for(;file_size.QuadPart != 0;
                file_size.QuadPart -= i64_min(file_size.QuadPart, 0xFFFFFFFF))
            {
                ReadFile(file_handle, result,
                         file_size.LowPart,
                         &(DWORD){0}, NULL);
            }
            
            return result;
        }

        case FILE_TYPE_CHAR:
        case FILE_TYPE_PIPE:
        {
#define TOTAL_RESERVE_SIZE ((size_t)1 << 40)
            size_t total_bytes_read = 0;
            char *const result = VirtualAlloc(NULL,
                                              TOTAL_RESERVE_SIZE,
                                              MEM_RESERVE,
                                              PAGE_READWRITE);
            
            VirtualAlloc(result,
                         0x100000,
                         MEM_COMMIT,
                         PAGE_READWRITE);
            
            for(;;)
            {
                DWORD const bytes_to_read =
                    (DWORD)size_t_min(0x100000,
                                      (TOTAL_RESERVE_SIZE - 1) - total_bytes_read);
                
                DWORD bytes_read;
                if (ReadFile(file_handle,
                             result + total_bytes_read,
                             bytes_to_read,
                             &bytes_read, NULL) == FALSE)
                {
                    break;
                }
                
                if ((total_bytes_read & (size_t)-0x100000) !=
                    ((total_bytes_read + bytes_read) & (size_t)-0x100000))
                {
                    VirtualAlloc(result + total_bytes_read + bytes_read,
                                 0x100000, MEM_COMMIT, PAGE_READWRITE);
                }

                total_bytes_read += bytes_read;
            }
            
            return result;            
#undef TOTAL_RESERVE_SIZE
        }
        
        default: return NULL;
    }
}


#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "kernel32.lib")

#pragma comment(linker, "-subsystem:console")
#pragma comment(linker, "-stack:0x100000,0x100000")

// compile as cl -Gs9999999 -O2 -Oi bf.c
NO_RTC __declspec(noreturn) void mainCRTStartup(void)
{
    int argc;
    wchar_t **const argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    wchar_t *input_file_path = NULL;
    wchar_t *output_file_path = L"a.exe";
    for (wchar_t **first = argv + 1; first != argv + argc; ++first)
    {
        wchar_t *argument = *first;
        if ((argument[0] == L'/' || argument[0] == L'-')    &&
            (argument[1] == L'o' || argument[1] == L'O'))
        {
            argument += 2;
            if (*argument == L'\0')
            {
                argument = *++first;
            }

            output_file_path = argument;
        }
        else
        {
            input_file_path = argument;
        }
    }

    if (output_file_path == NULL) 
    {
        ExitProcess(0);
    }

    char const *const input_data =
        read_entire_file(input_file_path);
    
    uint32_t file_offset = 0;
    HANDLE const file_handle =
        CreateFileW(output_file_path,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    
    struct PeBuffer pe_buffer = {.offset = 0};
    
    // mz header
    pe_buffer_write(&pe_buffer, "MZ", 2);
    pe_buffer.offset = 0x3C;
    pe_buffer_write(&pe_buffer, &(uint32_t){0x3C + 4}, 4);

    // pe header
    pe_buffer_write(&pe_buffer, "PE\0", 4);                  // PE MAGIC
    pe_buffer_write(&pe_buffer, &(uint16_t){0x8664}, 2);     // amd64
    pe_buffer_write(&pe_buffer, &(uint16_t){1}, 2);          // number of sections
    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] time stamp
    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] symbol table pointer
    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] symbol count
    pe_buffer_write(&pe_buffer, &(uint16_t){0x80}, 2);       // optional header size
    pe_buffer_write(&pe_buffer, &(uint16_t){0x0022}, 2);     // characteristics: PE32+, executable
    
    // opt header
    pe_buffer_write(&pe_buffer, &(uint16_t){0x020b}, 2);     // magic header PE32+
    pe_buffer_write(&pe_buffer, &(uint16_t){0x2313}, 2);     // [unused] linker version
    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] code size
    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] size of initialized data
    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] size of uninitialized data

    // fill out rva of entry point later
    uint32_t entry_location_offset = pe_buffer.offset;
    pe_buffer.offset += 4;

    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] base of code
    pe_buffer_write(&pe_buffer, &(uint64_t){IMAGE_BASE}, 8); // image base
    pe_buffer_write(&pe_buffer, &(uint32_t){0x200}, 4);      // sector alignment
    pe_buffer_write(&pe_buffer, &(uint32_t){0x200}, 4);      // file alignment
    pe_buffer_write(&pe_buffer, &(uint32_t){4}, 4);          // [unused] os version
    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] image version
    pe_buffer_write(&pe_buffer, &(uint32_t){4}, 4);          // subsystem version
    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] win32 version

    // fill out rva of end location later
    uint32_t end_location_offset = pe_buffer.offset;
    pe_buffer.offset += 4;

    pe_buffer_write(&pe_buffer, &(uint32_t){0x200}, 4);      // size of headers
    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] checksum
    pe_buffer_write(&pe_buffer, &(uint16_t){3}, 2);          // subsystem = console
    pe_buffer_write(&pe_buffer, &(uint16_t){0}, 2);          // [unused] dll characteristics
    pe_buffer_write(&pe_buffer,
                    (uint64_t[])
                    {
                        0x00100000,                          // maximum stack size
                        0x00010000,                          // initial stack size
                        0x00100000,                          // maximum heap size
                        0x00001000,                          // initial heap size
                    }, 4 * 8);

    pe_buffer_write(&pe_buffer, &(uint32_t){0}, 4);          // [unused] loader flags
    pe_buffer_write(&pe_buffer, &(uint32_t){2}, 4);          // number of rvas and sizes
    pe_buffer_write(&pe_buffer, (uint32_t[]){0, 0}, 4 * 2);  // [unused] export table
    pe_buffer_write(&pe_buffer, (uint32_t[]){0x200, 40}, 8); // import table

    // .text section header
    pe_buffer_write(&pe_buffer, ".text\0\0", 8);             // section name

    // fill in .text size later
    uint32_t text_size_offset = pe_buffer.offset;
    pe_buffer.offset += 4;

    pe_buffer_write(&pe_buffer, &(uint32_t){0x200}, 4);      // rva of .text

    // fill in .text size later
    pe_buffer.offset += 4;

    pe_buffer_write(&pe_buffer, &(uint32_t){0x200}, 4);      // file offset of .text
    pe_buffer_write(&pe_buffer, (uint32_t[]){0, 0, 0}, 32);  // [unused] debug info
    pe_buffer_write(&pe_buffer, &(uint32_t){0xE0000040}, 4); // flags, executable, readable, writeable
    
    // skip to start of .text section
    pe_buffer.offset = 0x200;

    // .text section
    pe_buffer.offset += 3 * 4;
    uint32_t kernel32_name_offset = pe_buffer.offset;
    pe_buffer.offset += (2 + 5) * 4;
    
    *(uint32_t*)(pe_buffer.data + kernel32_name_offset + 4) = pe_buffer.offset;
    *(uint32_t*)(pe_buffer.data + kernel32_name_offset) = pe_buffer.offset + 3 * 8;

    uint32_t iat_table_offset = pe_buffer.offset;
    pe_buffer.offset += 3 * 8;

    pe_buffer_write(&pe_buffer, "kernel32.dll", sizeof "kernel32.dll");

    *(uint64_t*)(pe_buffer.data + iat_table_offset + 0 * 8) = pe_buffer.offset;
    pe_buffer_write(&pe_buffer, "\0\0ReadFile", sizeof "\0\0ReadFile");
    *(uint64_t*)(pe_buffer.data + iat_table_offset + 1 * 8) = pe_buffer.offset;
    pe_buffer_write(&pe_buffer, "\0\0WriteFile", sizeof "\0\0WriteFile");
    *(uint32_t*)(pe_buffer.data + entry_location_offset) = pe_buffer.offset;

    SetFilePointer(file_handle, pe_buffer.offset, NULL, FILE_BEGIN);
    file_offset = pe_buffer.offset;

    struct InsVec const instruction_vector =
        compile_bf(input_data,
                   (int32_t)iat_table_offset -
                   (int32_t)pe_buffer.offset);
    
    *(uint32_t*)(pe_buffer.data + entry_location_offset) =
        pe_buffer.offset + instruction_vector.entry_offset;
    
    WriteFile(file_handle,
              instruction_vector.data,
              (DWORD)instruction_vector.size,
              &(DWORD){0}, NULL);

    uint32_t const total_length =
        instruction_vector.size + pe_buffer.offset;

    *(uint32_t*)(pe_buffer.data + end_location_offset) = total_length;

    *(uint32_t*)(pe_buffer.data + text_size_offset) = total_length - 0x200;
    *(uint32_t*)(pe_buffer.data + text_size_offset + 8) = total_length - 0x200;
    
    SetFilePointer(file_handle, 0, NULL, FILE_BEGIN);
    WriteFile(file_handle,
              pe_buffer.data,
              pe_buffer.offset,
              &(DWORD){0}, NULL);

    ExitProcess(0);
}