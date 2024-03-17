#include "ASM_lexer.hpp"

#include "globals.hpp"

#include <fstd/language/defer.hpp>
#include <fstd/language/flags.hpp>
#include <fstd/core/logger.hpp>
#include <fstd/core/string_builder.hpp>
#include <fstd/memory/hash_table.hpp>
#include <fstd/system/file.hpp>

#include <third-party/SpookyV2.h>

using namespace fstd;
using namespace fstd::memory;
using namespace fstd::language;
using namespace fstd::core;

namespace f::ASM
{
    static const size_t    tokens_length_heuristic = 3;

	// @SpeedUp Ideally I want to store this Hash_Table directly in the binary to avoid cost of allocating and initiliazing it.
	// Or at least I might be able to open and get file content at the same time of the initialization of the lexer.
	// Getting file content cost about 100µs and lexer initialization (Hash_Table init + fill) about 400µs.
	// Notice that even the close_file cost a little and should be done at the same time as the lexing.
	// But threading might not be ideal since it will require some time for initialization too (thread creation,...).
	// 
	// @Speed I store keywords, instructions and registers in a unique Hash_Table to reduce memory usage and optimize token classification.
	// I offset enum values of instructions by Keyword::COUNT (after keywords) and registers by Keyword::COUNT + Instruction::COUNT (after instructions).
	static Hash_Table<uint16_t, string_view, uint16_t, 64>      compiler_tokens;

	// @TODO remove the use of the initializer list in the Hash_Table
	static lexer::Hash_Table<uint16_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_2 = {
		{punctuation_key_2((uint8_t*)"//"),	Punctuation::LINE_COMMENT},
		{punctuation_key_2((uint8_t*)"/*"),	Punctuation::OPEN_BLOCK_COMMENT},
		{punctuation_key_2((uint8_t*)"*/"),	Punctuation::CLOSE_BLOCK_COMMENT},
	};

	static lexer::Hash_Table<uint8_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_1 = {
		// White characters (aren't handle for an implicit skip/separation between tokens)
		{' ', Punctuation::WHITE_CHARACTER},       // space
		{'\t', Punctuation::WHITE_CHARACTER},      // horizontal tab
		{'\v', Punctuation::WHITE_CHARACTER},      // vertical tab
		{'\f', Punctuation::WHITE_CHARACTER},      // feed
		{'\r', Punctuation::WHITE_CHARACTER},      // carriage return
		{'\n', Punctuation::NEW_LINE_CHARACTER},   // newline
		{'~', Punctuation::TILDE},
		{'`', Punctuation::BACKQUOTE},
		{'{', Punctuation::OPEN_BRACE},
		{'}', Punctuation::CLOSE_BRACE},
		{':', Punctuation::COLON},
		{';', Punctuation::SEMICOLON},
		{'\'', Punctuation::SINGLE_QUOTE},
		{'"', Punctuation::DOUBLE_QUOTE},
		{',', Punctuation::COMMA},
	};

	static inline uint16_t* get_compiler_token(string_view& text)
	{
		uint64_t hash = SpookyHash::Hash64((const void*)to_utf8(text), get_string_size(text), 0);
		uint16_t short_hash = hash & 0xffff;

		return hash_table_get(compiler_tokens, short_hash, text);
	}

    static inline Keyword is_keyword(uint16_t compiler_token)
    {
		if (compiler_token < (uint16_t)Keyword::COUNT) {
			return (Keyword)compiler_token;
		}
		return Keyword::UNKNOWN;
    }

    static inline Instruction is_instruction(uint16_t compiler_token)
    {
		if (compiler_token > (uint16_t)Keyword::COUNT && compiler_token < (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT) {
			return (Instruction)(compiler_token - (uint16_t)Keyword::COUNT);
		}
		return Instruction::UNKNOWN;
	}

    static inline Register is_register(uint16_t compiler_token)
    {
		if (compiler_token > (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT) {
			return (Register)(compiler_token - (uint16_t)Keyword::COUNT - (uint16_t)Instruction::COUNT);
		}
		return Register::UNKNOWN;
	}

    enum class Numeric_Value_Flag
    {
        IS_DEFAULT = 0x0000,
        IS_FLOATING_POINT = 0x0001,
        HAS_EXPONENT = 0x0002,

        // @Warning suffixes should be at end for has_suffix method
        UNSIGNED_SUFFIX = 0x0004,
        LONG_SUFFIX = 0x0008,
        FLOAT_SUFFIX = 0x0010,
        DOUBLE_SUFFIX = 0x0020,
    };

    // @TODO remplace it by a nested inlined function in f-lang
	// @SpeedUp Generate a string_view litteral (aka static const), It should be possible to avoid language::assign and store the 
	// string_view in the binary directly to avoid construction at runtime. Ideally same for the Hash64 computation
	// With great compile time feature the entire Hash_Table initialization should be done at compile time.
#define HT_INSERT_VALUE(ENUM, ENUM_OFFSET, KEY, VALUE) \
    { \
        language::string_view   str_view; \
        language::assign(str_view, (uint8_t*)(KEY)); \
        uint64_t hash = SpookyHash::Hash64((const void*)to_utf8(str_view), get_string_size(str_view), 0); \
        uint16_t short_hash = hash & 0xffff; \
		uint16_t value = (uint16_t)(ENUM::VALUE) + (uint16_t)(ENUM_OFFSET); \
        hash_table_insert(compiler_tokens, short_hash, str_view, value); \
    }

    void initialize_lexer()
    {
		ZoneScopedN("f::ASM::initialize_lexer");

		hash_table_init(compiler_tokens, &language::are_equals);

		// Keywords
		{
			ZoneScopedN("keywords");

			HT_INSERT_VALUE(Keyword, 0, "IMPORTS", IMPORTS);
			HT_INSERT_VALUE(Keyword, 0, "MODULE", MODULE);
			HT_INSERT_VALUE(Keyword, 0, "SECTION", SECTION);

			// Pseudo instructions
			HT_INSERT_VALUE(Keyword, 0, "db", DB);
		}

		// @TODO Move this code into an auto-generated cpp file?
		// And just put the inclusion of the cpp file here?
		// Code could be generated from a small tool that extract data from insns.dat and regs.dat
		// See ASM_X64.h too

		// Instructions
		{
			ZoneScopedN("instructions");

			HT_INSERT_VALUE(Instruction, Keyword::COUNT, "add", ADD);
			HT_INSERT_VALUE(Instruction, Keyword::COUNT, "call", CALL);
			HT_INSERT_VALUE(Instruction, Keyword::COUNT, "hlt", HLT);
			HT_INSERT_VALUE(Instruction, Keyword::COUNT, "mov", MOV);
			HT_INSERT_VALUE(Instruction, Keyword::COUNT, "push", PUSH);
			HT_INSERT_VALUE(Instruction, Keyword::COUNT, "sub", SUB);
		}

		// Registers
		{
			ZoneScopedN("registers");

			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "al", AL);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "ah", AH);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "ax", AX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "eax", EAX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "rax", RAX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "bl", BL);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "bh", BH);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "bx", BX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "ebx", EBX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "rbx", RBX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cl", CL);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "ch", CH);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cx", CX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "ecx", ECX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "rcx", RCX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "dl", DL);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "dh", DH);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "dx", DX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "edx", EDX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "rdx", RDX);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "spl", SPL);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "sp", SP);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "esp", ESP);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "rsp", RSP);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "bpl", BPL);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "bp", BP);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "ebp", EBP);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "rbp", RBP);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "sil", SIL);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "si", SI);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "esi", ESI);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "rsi", RSI);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "dil", DIL);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "di", DI);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "edi", EDI);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "rdi", RDI);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r8", R8);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r8d", R8D);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r8w", R8W);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r8b", R8B);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r9", R9);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r9d", R9D);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r9w", R9W);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r9b", R9B);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r10", R10);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r10d", R10D);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r10w", R10W);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r10b", R10B);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r11", R11);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r11d", R11D);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r11w", R11W);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r11b", R11B);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r12", R12);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r12d", R12D);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r12w", R12W);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r12b", R12B);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r13", R13);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r13d", R13D);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r13w", R13W);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r13b", R13B);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r14", R14);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r14d", R14D);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r14w", R14W);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r14b", R14B);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r15", R15);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r15d", R15D);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r15w", R15W);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "r15b", R15B);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "es", ES);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cs", CS);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "ss", SS);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "ds", DS);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "fs", FS);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "gs", GS);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "segr6", SEGR6);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "segr7", SEGR7);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr0", CR0);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr1", CR1);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr2", CR2);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr3", CR3);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr4", CR4);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr5", CR5);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr6", CR6);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr7", CR7);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr8", CR8);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr9", CR9);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr10", CR10);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr11", CR11);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr12", CR12);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr13", CR13);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr14", CR14);
			HT_INSERT_VALUE(Register, (uint16_t)Keyword::COUNT + (uint16_t)Instruction::COUNT, "cr15", CR15);
		}

#if defined(FSTD_DEBUG)
        log_stats(compiler_tokens, *globals.logger);
#endif
    }

    static inline void polish_string_literal(Token& token)
    {
        ZoneScopedN("f::ASM::polish_string_literal");

        fstd::core::Assert(token.type == Token_Type::STRING_LITERAL);

        size_t              token_length = language::get_string_size(token.text);
        language::string* string = (language::string*)system::allocate(sizeof(language::string));

        init(*string);
        memory::reserve_array(string->buffer, token_length);

        size_t      position = 0;
        size_t      literal_length = 0;
        uint8_t* output = memory::get_array_data(string->buffer);//  (uint8_t*)&string->buffer[0];

        memory::reserve_array(string->buffer, token_length);

        while (position < token_length)
        {
            if (language::to_utf8(token.text)[position] == '\\') {
                position++;
                if (language::to_utf8(token.text)[position] == 'n') {
                    output[literal_length] = '\n';
                }
                else if (language::to_utf8(token.text)[position] == 'r') {
                    output[literal_length] = '\r';
                }
                else if (language::to_utf8(token.text)[position] == 't') {
                    output[literal_length] = '\t';
                }
                else if (language::to_utf8(token.text)[position] == 'v') {
                    output[literal_length] = '\v';
                }
                else if (language::to_utf8(token.text)[position] == '\\') {
                    output[literal_length] = '\\';
                }
                literal_length++;
            }
            else {
                output[literal_length] = language::to_utf8(token.text)[position];
                literal_length++;
            }
            position++;
        }

        language::resize(*string, literal_length);
        token.value.string = string;
    }

    void lex(const system::Path& path, memory::Array<Token>& tokens)
    {
        ZoneScopedNC("f::ASM::lex", 0x1b5e20);

        Lexer_Data		lexer_data;
        system::File	file;
        Token			file_token;

		file_token.type = Token_Type::UNKNOWN;
		file_token.file_path = system::to_string(path);
		file_token.line = 0;
		file_token.column = 0;

		if (system::open_file(file, path, system::File::Opening_Flag::READ) == false) {
            report_error(Compiler_Error::error, file_token, "Failed to open source file.");
        }

        defer{ system::close_file(file); };

		system::copy(lexer_data.file_path, path);
		lexer_data.file_buffer = system::get_file_content(file);
        memory::array_push_back(globals.asm_lexer_data, lexer_data);

        lex(path, lexer_data.file_buffer, tokens, globals.asm_lexer_data, file_token);
    }

	void lex(const system::Path& path, fstd::memory::Array<uint8_t>& file_buffer, fstd::memory::Array<Token>& tokens, fstd::memory::Array<f::ASM::Lexer_Data>& lexer_data, Token& file_token)
	{
		ZoneScopedN("lex");

		stream::Array_Read_Stream<uint8_t>   stream;
		size_t	                        nb_tokens_prediction = 0;
		language::string_view           current_view;
		int					            current_line = 1;
		int					            current_column = 1;

		stream::init<uint8_t>(stream, file_buffer);

		if (stream::is_eof(stream) == true) {
			return;
		}

		// @Warning
		//
		// We read the file in background asynchronously, so be careful to not relly on file states.
		// Instead prefer use the stream to get correct states.
		//
		// Flamaros - 01 february 2020

		nb_tokens_prediction = get_array_size(file_buffer) / tokens_length_heuristic + 512;

		memory::reserve_array(tokens, nb_tokens_prediction);

		language::assign(current_view, stream::get_pointer(stream), 0);

		bool has_utf8_boom = stream::is_uft8_bom(stream, true);

		if (has_utf8_boom == false) {
			report_error(Compiler_Error::warning, file_token, "This file doens't have a UTF8 BOM\n");
		}

		while (stream::is_eof(stream) == false)
		{
			Punctuation punctuation;
			Punctuation punctuation_2 = Punctuation::UNKNOWN;
			uint8_t     current_character;

			current_character = stream::get(stream);

			punctuation = punctuation_table_1[current_character];
			if (stream::get_remaining_size(stream) >= 2) {
				punctuation_2 = punctuation_table_2[f::punctuation_key_2(stream::get_pointer(stream))];
			}

			if (punctuation != Punctuation::UNKNOWN || punctuation_2 != Punctuation::UNKNOWN) { // Punctuation to analyse
				if (is_white_punctuation(punctuation)) {    // Punctuation to ignore
					if (punctuation == Punctuation::NEW_LINE_CHARACTER) {
						current_line++;
						current_column = 0; // @Warning 0 because the will be incremented just after
					}

					peek(stream, current_column);
				}
				else {
					Token	token;

					token.file_path = system::to_string(path);
					token.line = current_line;
					token.column = current_column;

					language::assign(current_view, stream::get_pointer(stream), 0);

					if (punctuation_2 == Punctuation::LINE_COMMENT || punctuation == Punctuation::SEMICOLON) {
						while (stream::is_eof(stream) == false)
						{
							uint8_t     current_character;

							current_character = stream::get(stream);
							if (current_character == '\n') {
								break;
							}
							peek(stream, current_column);   // @Warning We don't peek the '\n' character (it will be peeked later for the line count increment)
						}
					}
					else if (punctuation_2 == Punctuation::OPEN_BLOCK_COMMENT) {
						bool    comment_block_closed = false;
						while (stream::is_eof(stream) == false)
						{
							current_character = stream::get(stream);
							punctuation = punctuation_table_1[current_character];   // @TODO a simple if is enough, think to remove hash tables
							if (punctuation == Punctuation::NEW_LINE_CHARACTER) {
								current_line++;
								current_column = 0; // @Warning 0 because the will be incremented just after
							}

							if (stream::get_remaining_size(stream) >= 2) {
								punctuation_2 = punctuation_table_2[punctuation_key_2(stream::get_pointer(stream))];
							}

							if (punctuation_2 == Punctuation::CLOSE_BLOCK_COMMENT) {
								skip(stream, 2, current_column);
								comment_block_closed = true;
								break;
							}

							peek(stream, current_column);
						}

						if (comment_block_closed == false) {
							report_error(Compiler_Error::error, token, "Multiline comment block was not closed.");
						}
					}
					else if (punctuation == Punctuation::DOUBLE_QUOTE) {
						peek(stream, current_column);

						bool        string_closed = false;
						uint8_t* string_literal = stream::get_pointer(stream);
						size_t      string_size = 0;

						while (stream::is_eof(stream) == false)
						{
							uint8_t     current_character;

							current_character = stream::get(stream);
							if (punctuation_table_1[current_character] == Punctuation::DOUBLE_QUOTE) { // @TODO @SpeedUp we can just check the character here directly
								peek(stream, current_column);
								string_closed = true;
								break;
							}

							peek(stream, current_column);
							string_size++;
						}

						if (string_closed == false) {
							report_error(Compiler_Error::error, token, "String literal was not closed.");
						}
						else {
							language::assign(current_view, string_literal, string_size);
							token.text = current_view;
							token.type = Token_Type::STRING_LITERAL;
							polish_string_literal(token);

							memory::array_push_back(tokens, token);
						}
					}
					else if (punctuation == Punctuation::SINGLE_QUOTE) {
						peek(stream, current_column);
						// @TODO single character, can be the same as string literal???
					}
					else if (punctuation == Punctuation::BACKQUOTE) {
						peek(stream, current_column);

						bool        raw_string_closed = false;
						uint8_t* string_literal = stream::get_pointer(stream);
						size_t      string_size = 0;

						while (stream::is_eof(stream) == false)
						{
							uint8_t     current_character;

							current_character = stream::get(stream);
							if (punctuation_table_1[current_character] == Punctuation::SINGLE_QUOTE) { // @TODO @SpeedUp we can just check the character here directly
								peek(stream, current_column);
								raw_string_closed = true;
								break;
							}

							peek(stream, current_column);
							string_size++;
						}

						if (raw_string_closed == false) {
							report_error(Compiler_Error::error, token, "Raw string literal was not closed.");
						}
						else {
							language::assign(current_view, string_literal, string_size);
							token.text = current_view;
							token.type = Token_Type::STRING_LITERAL_RAW;

							memory::array_push_back(tokens, token);
						}
					}
					else {
						token.type = Token_Type::SYNTAXE_OPERATOR;

						if (punctuation_2 != Punctuation::UNKNOWN) {
							language::assign(current_view, stream::get_pointer(stream), 2);
							token.text = current_view;
							token.value.punctuation = punctuation_2;
							skip(stream, 2, current_column);
						}
						else {
							language::assign(current_view, stream::get_pointer(stream), 1);
							token.text = current_view;
							token.value.punctuation = punctuation;
							skip(stream, 1, current_column);
						}

						memory::array_push_back(tokens, token);
					}
				}
			}
			else if (is_digit(current_character)) { // Will be a numeric literal
				// @TODO
				// We may want to improve the floating point parsing quality and speed.
				// We also may want to add the support of hexa float
				// Put similar code in the base library
				// https://github.com/ulfjack/ryu/blob/master/ryu/s2f.c

				Token               token;
				Numeric_Value_Flag  numeric_literal_flags = Numeric_Value_Flag::IS_DEFAULT;
				uint8_t             next_char = 0;

				token.file_path = system::to_string(path);
				token.line = current_line;
				token.column = current_column;
				token.type = Token_Type::UNKNOWN;

				language::assign(token.text, stream::get_pointer(stream), 0); // Storing the starting pointer of the text string view

				if (stream::get_remaining_size(stream) >= 1) {
					next_char = stream::get_pointer(stream)[1];
				}

				if (current_character == '0' && (next_char == 'x' || next_char == 'b')) { // format prefix (0x or 0b)
					peek(stream, current_column);

					current_character = stream::get(stream);
					if (current_character == 'x') { // hexadecimal
						peek(stream, current_column);
						while (true) {
							current_character = stream::get(stream);

							if (current_character >= '0' && current_character <= '9') {
								token.value.unsigned_integer = token.value.unsigned_integer * 16 + (current_character - '0');
								peek(stream, current_column);
							}
							else if (current_character >= 'a' && current_character <= 'f') {
								token.value.unsigned_integer = token.value.unsigned_integer * 16 + (current_character - 'a') + 10;
								peek(stream, current_column);
							}
							else if (current_character >= 'A' && current_character <= 'F') {
								token.value.unsigned_integer = token.value.unsigned_integer * 16 + (current_character - 'A') + 10;
								peek(stream, current_column);
							}
							else if (current_character == '.') {
								// @TODO
								core::Assert(false);
								report_error(Compiler_Error::error, token, "f-lang does't support hexadecimal floating points for the moment.");
							}
							else if (current_character == 'p') {
								// @TODO
								core::Assert(false);
								report_error(Compiler_Error::error, token, "f-lang does't support hexadecimal floating points for the moment.");
							}
							else if (current_character == '_') {
								peek(stream, current_column);
							}
							else {
								break;
							}
						}
					}
					else if (current_character == 'b') { // binary
						peek(stream, current_column);
						while (true) {
							current_character = stream::get(stream);

							if (current_character == '0' || current_character == '1') {
								token.value.unsigned_integer = token.value.unsigned_integer * 2 + (current_character - '0');
								peek(stream, current_column);
							}
							else if (current_character == '_') {
								peek(stream, current_column);
							}
							else {
								break;
							}
						}
					}
					else {
						core::Assert(false);
					}
				}
				else {
					token.value.unsigned_integer = current_character - '0';

					peek(stream, current_column);
					while (true) {
						current_character = stream::get(stream);

						if (current_character >= '0' && current_character <= '9') {
							token.value.unsigned_integer = token.value.unsigned_integer * 10 + (current_character - '0');
							peek(stream, current_column);
						}
						else if (current_character == '.') {
							int64_t divider = 10;

							if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::HAS_EXPONENT) == true) {
								report_error(Compiler_Error::error, token, "Numeric literal can't have a floating point exponent.");
							}

							set_flag(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT);
							peek(stream, current_column);

							token.value.real_max = (long double)token.value.unsigned_integer;	// @Warning this operate a conversion from integer to floating point
							while (true) {
								current_character = stream::get(stream);

								if (current_character >= '0' && current_character <= '9') {
									token.value.real_max += (current_character - '0') / divider;
									divider *= 10;
									peek(stream, current_column);
								}
								else if (current_character == 'e') {
									break; // We don't peek this character to let the previous level handle it
								}
								else if (current_character == '_') {
									peek(stream, current_column);
								}
								else {
									break;
								}
							}

							if (divider == 10) {
								report_error(Compiler_Error::error, token, "Floating points literal must not ended by '.' character, a digit should follow the '.'.");
							}
						}
						else if (current_character == 'e') {
							int64_t exponent = 0;

							if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT) == false) {
								set_flag(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT);
								token.value.real_max = (long double)token.value.unsigned_integer;	// @Warning this operate a conversion from integer to floating point
							}
							set_flag(numeric_literal_flags, Numeric_Value_Flag::HAS_EXPONENT);
							peek(stream, current_column);

							while (true) {
								current_character = stream::get(stream);

								if (current_character == '+') {
									peek(stream, current_column);
								}
								else if (current_character == '-') {
									exponent = -0;
									peek(stream, current_column);
								}
								else if (current_character >= '0' && current_character <= '9') {
									exponent = exponent * 10 + (current_character - '0');
									peek(stream, current_column);
								}
								else if (current_character == '_') {
									peek(stream, current_column);
								}
								else {
									break;
								}
							}

							token.value.real_max = token.value.real_max * powl(10, (long double)exponent);
						}
						else if (current_character == 'u' && !is_flag_set(numeric_literal_flags, Numeric_Value_Flag::UNSIGNED_SUFFIX)) {
							if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT)) {
								report_error(Compiler_Error::error, token, "Floating point numeric literal can't have unsigned suffix.");
							}

							peek(stream, current_column);
							set_flag(numeric_literal_flags, Numeric_Value_Flag::UNSIGNED_SUFFIX);
						}
						else if (current_character == 'L' && !is_flag_set(numeric_literal_flags, Numeric_Value_Flag::LONG_SUFFIX)) {
							peek(stream, current_column);
							set_flag(numeric_literal_flags, Numeric_Value_Flag::LONG_SUFFIX);

							if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT)) {
								token.type = Token_Type::NUMERIC_LITERAL_REAL;
								break;
							}
						}
						else if (current_character == 'd') {
							if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT) == false) {
								set_flag(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT);
								token.value.real_64 = (double)token.value.unsigned_integer;	// @Warning this operate a conversion from integer to floating point
							}
							else {
								token.value.real_64 = (double)token.value.real_max;	// @Warning this operate a conversion from integer to floating point
							}

							peek(stream, current_column);
							set_flag(numeric_literal_flags, Numeric_Value_Flag::DOUBLE_SUFFIX);
							token.type = Token_Type::NUMERIC_LITERAL_F64;
							break;
						}
						else if (current_character == 'f') {
							if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT) == false) {
								set_flag(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT);
								token.value.real_32 = (float)token.value.unsigned_integer;	// @Warning this operate a conversion from integer to floating point
							}
							else {
								token.value.real_32 = (float)token.value.real_max;	// @Warning this operate a conversion from integer to floating point
							}

							peek(stream, current_column);
							set_flag(numeric_literal_flags, Numeric_Value_Flag::FLOAT_SUFFIX);
							token.type = Token_Type::NUMERIC_LITERAL_F32;
							break;
						}
						else if (current_character == '_') {
							peek(stream, current_column);
						}
						else {
							break;
						}
					}
				}

				// Polish numeric literal types
				if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::IS_FLOATING_POINT)) {
					// Float with suffix are already done
					if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::LONG_SUFFIX) == false
						&& is_flag_set(numeric_literal_flags, Numeric_Value_Flag::DOUBLE_SUFFIX) == false
						&& is_flag_set(numeric_literal_flags, Numeric_Value_Flag::FLOAT_SUFFIX) == false) {
						token.value.real_64 = (double)token.value.real_max;	// @Warning this operate a conversion from integer to floating point
						token.type = Token_Type::NUMERIC_LITERAL_F64;
					}
				}
				else {
					token.type = Token_Type::NUMERIC_LITERAL_I32;

					if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::UNSIGNED_SUFFIX)
						&& is_flag_set(numeric_literal_flags, Numeric_Value_Flag::LONG_SUFFIX)) {
						token.type = Token_Type::NUMERIC_LITERAL_UI64;
					}
					else if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::LONG_SUFFIX)) {
						token.type = Token_Type::NUMERIC_LITERAL_I64;
					}
					else if (is_flag_set(numeric_literal_flags, Numeric_Value_Flag::UNSIGNED_SUFFIX)) {
						token.type = token.value.unsigned_integer > 4'294'967'295 ? Token_Type::NUMERIC_LITERAL_UI64 : Token_Type::NUMERIC_LITERAL_UI32;
					}
					else if (token.value.unsigned_integer > 2'147'483'647) {
						token.type = Token_Type::NUMERIC_LITERAL_I64;
					}
				}

				// We can simply compute the size of text by comparing the position on the stream with the one at the beginning of the numeric literal
				language::resize(token.text, stream::get_pointer(stream) - language::to_utf8(token.text));
				memory::array_push_back(tokens, token);
			}
			else {  // Will be an identifier
				Token   token;

				token.file_path = system::to_string(path);
				token.line = current_line;
				token.column = current_column;

				language::assign(current_view, stream::get_pointer(stream), 0);
				while (stream::is_eof(stream) == false)
				{
					current_character = stream::get(stream);

					punctuation = punctuation_table_1[current_character];
					if (punctuation == Punctuation::UNKNOWN) {  // @Warning any kind of punctuation stop the definition of an identifier
						peek(stream, current_column);
						language::resize(current_view, language::get_string_size(current_view) + 1);

						token.type = Token_Type::IDENTIFIER;

						token.text = current_view;
					}
					else {
						break;
					}
				}

				Keyword		keyword;
				Instruction	instruction;
				Register	_register;
				
				uint16_t* compiler_token = get_compiler_token(token.text);

				if (compiler_token) {
					keyword = is_keyword(*compiler_token);
					if (keyword != Keyword::UNKNOWN) {
						token.type = Token_Type::KEYWORD;
						token.value.keyword = keyword;
						goto register_token;
					}

					instruction = is_instruction(*compiler_token);
					if (instruction != Instruction::UNKNOWN) {
						token.type = Token_Type::INSTRUCTION;
						token.value.instruction = instruction;
						goto register_token;
					}

					_register = is_register(*compiler_token);
					if (_register != Register::UNKNOWN) {
						token.type = Token_Type::REGISTER;
						token.value._register = _register;
						goto register_token;
					}
				}

			register_token:
				memory::array_push_back(tokens, token);
			}
		}

		if (nb_tokens_prediction < memory::get_array_size(tokens)) {
			// @TODO support print of floats
	//		log(*globals.logger, Log_Level::warning, "[lexer] Wrong token number prediction. Predicted :%d - Nb tokens: %d - Nb tokens/byte: %.3f\n",  nb_tokens_prediction, memory::get_array_size(tokens), (float)memory::get_array_size(tokens) / (float)get_array_size(file_buffer));

			// @TODO We should do a faster allocator of tokens than using array_push_back which check the size of the array.
			log(*globals.logger, Log_Level::warning, "[lexer] Wrong token number prediction. Predicted :%d - Nb tokens: %d\n", nb_tokens_prediction, memory::get_array_size(tokens));
			report_error(Compiler_Error::internal_error, "Overflow the maximum number of tokens that the compiler will be able to handle in future! Actually the buffer have a dynamic size but it will not stay like that for performances!");
		}

#if defined(FSTD_DEBUG)
		log_stats(compiler_tokens, *globals.logger);
#endif
	}

	void print(fstd::memory::Array<Token>& tokens)
	{
		ZoneScopedNC("f::print[tokens]", 0x1b5e20);

		String_Builder  string_builder;

		defer{ free_buffers(string_builder); };

		if (memory::get_array_size(tokens)) {
			print_to_builder(string_builder, "--- tokens list of: ");
			print_to_builder(string_builder, tokens[0].file_path);
			print_to_builder(string_builder, " ---\n");
		}

		for (size_t i = 0; i < memory::get_array_size(tokens); i++)
		{
			switch (tokens[i].type)
			{
			case Token_Type::UNKNOWN:
				print_to_builder(string_builder, "%d, %d - UNKNOWN\033[0m: %v", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::IDENTIFIER:
				print_to_builder(string_builder, "%d, %d - IDENTIFIER\033[0m: %v", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::KEYWORD:
				print_to_builder(string_builder, "%d, %d - \033[38;5;12mKEYWORD\033[0m: \033[38;5;12m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::INSTRUCTION:
				print_to_builder(string_builder, "%d, %d - \033[38;5;13mINSTRUCTION\033[0m: \033[38;5;13m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::REGISTER:
				print_to_builder(string_builder, "%d, %d - \033[38;5;9mREGISTER\033[0m: \033[38;5;9m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::SYNTAXE_OPERATOR:
				print_to_builder(string_builder, "%d, %d - \033[38;5;10mSYNTAXE_OPERATOR\033[0m: \033[38;5;10m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::STRING_LITERAL_RAW:
				print_to_builder(string_builder, "%d, %d - \033[38;5;3mSTRING_LITERAL_RAW\033[0m: \033[38;5;3m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::STRING_LITERAL:
				print_to_builder(string_builder, "%d, %d - \033[38;5;3mSTRING_LITERAL\033[0m: \033[38;5;3m\"%v\"\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::NUMERIC_LITERAL_I32:
				print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_I32\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::NUMERIC_LITERAL_UI32:
				print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_UI32\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::NUMERIC_LITERAL_I64:
				print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_I64\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::NUMERIC_LITERAL_UI64:
				print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_UI64\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::NUMERIC_LITERAL_F32:
				print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_F32\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::NUMERIC_LITERAL_F64:
				print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_F64\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			case Token_Type::NUMERIC_LITERAL_REAL:
				print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_REAL\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
				break;
			default:
				print_to_builder(string_builder, "%d, %d - Invalid token type!!!", tokens[i].line, tokens[i].column);
				break;
			}
			print_to_builder(string_builder, "\n");
		}

		if (memory::get_array_size(tokens)) {
			print_to_builder(string_builder, "---\n");
		}

		system::print(to_string(string_builder));
	}
}
