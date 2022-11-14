#include "instruction_db_loader.hpp"

#include "globals.hpp"
#include "lexer/lexer_base.hpp"

#include <fstd/core/string_builder.hpp>

#include <fstd/stream/array_stream.hpp>

#include <fstd/memory/array.hpp>
#include <fstd/memory/hash_table.hpp>

#include <fstd/system/file.hpp>

#include <fstd/language/defer.hpp>

using namespace fstd;
using namespace fstd::core;
using namespace fstd::stream;
using namespace fstd::system;

#define NB_INSTRUCTIONS     8192
#define NB_TOKENS_PER_LINE  64

namespace f
{
	namespace x86_DB
	{
        // @TODO Can't we reduce the size key?
// To avoid collisions, we may have to find a better way to generate a hash than simply encode the keyword size with the first
// two characters.
// @SpeedUp
//
// Flamaros - 19 february 2020
        static memory::Hash_Table<uint32_t, language::string_view, x86_Keyword, 512>  x86_keywords;

        static inline x86_Keyword x86_is_keyword(language::string_view& text)
        {
            x86_Keyword* result = memory::hash_table_get(x86_keywords, keyword_key(text), text);

            return result ? *result : x86_Keyword::UNKNOWN;
        }

        // @TODO remplace it by a nested inlined function in f-lang
#define INSERT_KEYWORD(KEY, VALUE) \
        { \
            language::string_view   str_view; \
            language::assign(str_view, (uint8_t*)(KEY)); \
            x86_Keyword value = (x86_Keyword::VALUE); \
            hash_table_insert(x86_keywords, keyword_key(str_view), str_view, value); \
        }

        static void x86_initialize_lexer()
        {
            memory::hash_table_init(x86_keywords, &language::are_equals);

            INSERT_KEYWORD("ignore", _IGNORE);

            INSERT_KEYWORD("resb", RESB);

            INSERT_KEYWORD("void", _VOID);
            INSERT_KEYWORD("reg8", REG8);
            INSERT_KEYWORD("reg16", REG16);
            INSERT_KEYWORD("reg32", REG32);
            INSERT_KEYWORD("reg64", REG64);
            INSERT_KEYWORD("reg_al", REG_AL);
            INSERT_KEYWORD("reg_ax", reg_ax);
            INSERT_KEYWORD("reg_eax", reg_eax);
            INSERT_KEYWORD("reg_rax", reg_rax);
            INSERT_KEYWORD("reg_dx", reg_dx);
            INSERT_KEYWORD("mem", MEM);
            INSERT_KEYWORD("mem16", MEM16);
            INSERT_KEYWORD("mem32", MEM32);
            INSERT_KEYWORD("mem64", MEM64);
            INSERT_KEYWORD("mem80", mem80);
            INSERT_KEYWORD("imm", IMM);
            INSERT_KEYWORD("imm8", IMM8);
            INSERT_KEYWORD("imm16", IMM16);
            INSERT_KEYWORD("imm32", IMM32);
            INSERT_KEYWORD("imm64", IMM64);
            INSERT_KEYWORD("sbyteword", SBYTEWORD);
            INSERT_KEYWORD("sbyteword16", SBYTEWORD16);
            INSERT_KEYWORD("sbyteword32", SBYTEWORD32);
            INSERT_KEYWORD("fpureg", fpureg);
            INSERT_KEYWORD("fpu0", fpu0);

            INSERT_KEYWORD("near", _NEAR);
            INSERT_KEYWORD("far", _FAR);
            INSERT_KEYWORD("to", TO);

            INSERT_KEYWORD("8086", ARCH_8086);
            INSERT_KEYWORD("386", ARCH_386);
            INSERT_KEYWORD("X64", ARCH_X64);
            INSERT_KEYWORD("NOLONG", ARCH_NOLONG);
            INSERT_KEYWORD("SM", ARCH_SM);
            INSERT_KEYWORD("LOCK", ARCH_LOCK);
            INSERT_KEYWORD("ND", ARCH_ND);

#if defined(FSTD_DEBUG)
            log_stats(x86_keywords, *globals.logger);
#endif
        }

        static inline Instruction* allocate_instruction()
        {
            ZoneScopedN("allocate_instruction");

            // Ensure that no reallocation could happen during the resize
            bool overflow_preallocated_buffer = memory::get_array_size(globals.x86_db_data.instructions) >= memory::get_array_reserved(globals.x86_db_data.instructions) * sizeof(Instruction);

            core::Assert(overflow_preallocated_buffer == false);
            if (overflow_preallocated_buffer) {
                report_error(Compiler_Error::internal_error, "The compiler did not allocate enough memory to store Instruction!");
            }

            // @TODO use a version that didn't check the array size, because we just did it and we don't want to trigger unplanned allocations
            memory::resize_array(globals.x86_db_data.instructions, memory::get_array_size(globals.x86_db_data.instructions) + sizeof(Instruction));

            Instruction* new_instruction = (Instruction*)(memory::get_array_last_element(globals.x86_db_data.instructions) - sizeof(Instruction));
            return new_instruction;
        }

        static void lex_instructions_DB()
        {
            ZoneScopedN("lex_instructions_DB");

            x86_initialize_lexer();

            static system::Path instructions_file_path; // @warning static and never released to be able to have a string_view on it
            File                instructions_file;
            bool                open;

            system::from_native(instructions_file_path, (uint8_t*)"./compiler-cpp/data/insns.dat");

            open = open_file(instructions_file, instructions_file_path, File::Opening_Flag::READ);

            if (open == false) {
                String_Builder		string_builder;
                language::string	message;

                defer{
                    free_buffers(string_builder);
                    release(message);
                };

                print_to_builder(string_builder, "Failed to open file: \"%v\"\n", to_string(instructions_file_path));

                message = to_string(string_builder);
                report_error(Compiler_Error::error, (char*)to_utf8(message));
            }

            defer{ close_file(instructions_file); };

            fstd::memory::Array<uint8_t> instructions_buffer = system::get_file_content(instructions_file);

            Array_Stream<uint8_t>   ins_stream;
            size_t	                nb_tokens_prediction = 0;
            language::string_view   current_view;
            int					    current_line = 1;
            int					    current_column = 1;

            stream::initialize_memory_stream<uint8_t>(ins_stream, instructions_buffer);

            if (stream::is_eof(ins_stream) == true) {
                return;
            }

            memory::reserve_array(globals.x86_db_data.tokens, NB_INSTRUCTIONS * NB_TOKENS_PER_LINE);

            language::assign(current_view, get_pointer(ins_stream), 0);

            while (is_eof(ins_stream) == false)
            {
                Punctuation         punctuation;
                uint8_t             current_character;
                Token<x86_Keyword>  token;

                current_character = get(ins_stream);
                punctuation = punctuation_table_1[current_character];

                if (punctuation != Punctuation::UNKNOWN) { // Punctuation to analyse
                    if (is_white_punctuation(punctuation)) {    // Punctuation to ignore
                        if (punctuation == Punctuation::NEW_LINE_CHARACTER) {
                            current_line++;
                            current_column = 0; // @Warning 0 because the will be incremented just after
                        }

                        peek(ins_stream, current_column);
                        continue; // Jump to next iteration loop to skip Token<x86_Keyword> analysis because it has not changed
                    }
                    else {
                        token.file_path = system::to_string(instructions_file_path);
                        token.line = current_line;
                        token.column = current_column;

                        language::assign(current_view, get_pointer(ins_stream), 0);

                        if (punctuation == Punctuation::COMMA) {
                            token.type = Token_Type::SYNTAXE_OPERATOR;

                            language::assign(current_view, get_pointer(ins_stream), 1);
                            token.text = current_view;
                            token.value.punctuation = punctuation;
                            skip(ins_stream, 1, current_column);
                        }
                        else if (punctuation == Punctuation::OPEN_BRACKET) {
                            token.type = Token_Type::SYNTAXE_OPERATOR;

                            language::assign(current_view, get_pointer(ins_stream), 1);
                            token.text = current_view;
                            token.value.punctuation = punctuation;
                            skip(ins_stream, 1, current_column);
                        }
                        else if (punctuation == Punctuation::CLOSE_BRACKET) {
                            token.type = Token_Type::SYNTAXE_OPERATOR;

                            language::assign(current_view, get_pointer(ins_stream), 1);
                            token.text = current_view;
                            token.value.punctuation = punctuation;
                            skip(ins_stream, 1, current_column);
                        }
                        else {
                            token.type = Token_Type::SYNTAXE_OPERATOR;

                            language::assign(current_view, get_pointer(ins_stream), 1);
                            token.text = current_view;
                            token.value.punctuation = punctuation;
                            skip(ins_stream, 1, current_column);
                        }
                    }
                }
                else {  // Will be an identifier
                    token.file_path = system::to_string(instructions_file_path);
                    token.line = current_line;
                    token.column = current_column;

                    language::assign(current_view, get_pointer(ins_stream), 0);
                    while (is_eof(ins_stream) == false)
                    {
                        current_character = get(ins_stream);

                        punctuation = punctuation_table_1[current_character];
                        if (punctuation == Punctuation::UNKNOWN) {  // @Warning any kind of punctuation stop the definition of an identifier
                            peek(ins_stream, current_column);
                            language::resize(current_view, language::get_string_size(current_view) + 1);

                            token.type = Token_Type::IDENTIFIER;

                            token.text = current_view;
                        }
                        else {
                            break;
                        }
                    }

                    token.value.keyword = x86_is_keyword(token.text);
                    if (token.value.keyword != x86_Keyword::UNKNOWN) {
                        token.type = Token_Type::KEYWORD;
                    }
                }

                memory::array_push_back(globals.x86_db_data.tokens, token);

                //        core::log(*globals.logger, Log_Level::verbose, "[backend] %v\n", token.text);
            }
        }

        static void parse_instructions_DB()
        {
            ZoneScopedN("parse_instructions_DB");

            enum class ParsingState
            {
                NAME,
                OPERANDS,
                TRANSLATION_INSTRUCTIONS,
                ARCHITECTURES
            };

            Token<x86_Keyword>  current_token;
            int                 current_line;
            int                 previous_line = 0;
            Instruction*        current_instruction = nullptr;
            ParsingState        parsing_state = ParsingState::NAME;

            stream::Array_Stream<Token<x86_Keyword>>	stream;

            stream::initialize_memory_stream<Token<x86_Keyword>>(stream, globals.x86_db_data.tokens);

            memory::reserve_array(globals.x86_db_data.instructions, NB_INSTRUCTIONS * sizeof(Instruction));

            while (stream::is_eof(stream) == false)
            {
                current_token = stream::get(stream);

                current_line = (int)current_token.line;

                if (previous_line != current_line) { // We have to start a new instruction
                    if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
                        // Skip until we reach the next line, because it's a comment
                        while (stream::get(stream).line == current_line) {
                            stream::skip<Token<x86_Keyword>>(stream, 1);
                        }
                    }
                    else {
                        current_instruction = allocate_instruction();

                        current_instruction->name = current_token.text;

                        parsing_state = ParsingState::OPERANDS;
                        stream::peek<Token<x86_Keyword>>(stream);
                    }
                }
                else {
                    if (parsing_state == ParsingState::OPERANDS) {
                        size_t  operand_index = 0;

                        if (current_token.type == Token_Type::KEYWORD) {
                            if (current_token.value.keyword == x86_Keyword::_IGNORE) {
                                // We can simply completely ignore this instruction, so we skip tokens directly to the next line
                                parsing_state = ParsingState::NAME;
                                while (stream::get(stream).line == current_line) {
                                    stream::skip<Token<x86_Keyword>>(stream, 1);
                                }
                            }
                            else if (current_token.value.keyword == x86_Keyword::IMM) {
                                current_instruction->operands[operand_index].type = Instruction::Operand::Type::ImmediateValue;
                                current_instruction->operands[operand_index].size = -1; // @TODO
                                stream::peek<Token<x86_Keyword>>(stream); // imm
                            }
                            else if (current_token.value.keyword == x86_Keyword::_VOID) {
                                current_instruction->operands[operand_index].type = Instruction::Operand::Type::Unused;
                                current_instruction->operands[operand_index].size = 0;
                                stream::peek<Token<x86_Keyword>>(stream); // void
                            }
                            else {
                                report_error(Compiler_Error::internal_error, current_token, "x86 backend: [DB instructions parsing] Unknown operand type!");
                            }
                        }
                        else if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
                            if (current_token.value.punctuation == Punctuation::COMMA) {
                                operand_index++;
                                stream::peek<Token<x86_Keyword>>(stream); // ,
                            }
                            else if (current_token.value.punctuation == Punctuation::OPEN_BRACKET) {
                                stream::peek<Token<x86_Keyword>>(stream); // [
                                parsing_state = ParsingState::TRANSLATION_INSTRUCTIONS;
                            }
                        }
                        else {
                            report_error(Compiler_Error::internal_error, current_token, "x86 backend: [DB instructions parsing] Syntax error, expecting an operand name or a comma");
                        }
                    }
                    else if (parsing_state == ParsingState::TRANSLATION_INSTRUCTIONS) {
                        if (current_token.type == Token_Type::KEYWORD) {
                            if (current_token.value.keyword == x86_Keyword::RESB) {
                                stream::peek<Token<x86_Keyword>>(stream); // resb
                            }
                        }
                        else if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
                            if (current_token.value.punctuation == Punctuation::CLOSE_BRACKET) {
                                stream::peek<Token<x86_Keyword>>(stream); // ]
                                parsing_state = ParsingState::ARCHITECTURES;
                            }
                        }
                        else {
                            report_error(Compiler_Error::internal_error, current_token, "x86 backend: [DB instructions parsing] Syntax error, translation instructions parsing failed!!!");
                        }
                    }
                    else if (parsing_state == ParsingState::ARCHITECTURES) {
                        if (current_token.type == Token_Type::KEYWORD) {
                            if (current_token.value.keyword == x86_Keyword::ARCH_8086) {
                                // @TODO push the instruction if valid (resb is not valid)

                                stream::peek<Token<x86_Keyword>>(stream); // resb
                            }
                        }
                        else if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
                            if (current_token.value.punctuation == Punctuation::COMMA) {
                                stream::peek<Token<x86_Keyword>>(stream); // ,
                            }
                        }
                        else {
                            report_error(Compiler_Error::internal_error, current_token, "x86 backend: [DB instructions parsing] Syntax error, expecting an architecture keyword.");
                        }
                    }
                }

                previous_line = current_line;
            }
        }

        void load_x86_instruction_DB()
        {
            lex_instructions_DB();
            parse_instructions_DB();
        }
	}
}
