#include "CPP_backend.hpp"

#include "globals.hpp"

#include <fstd/core/string_builder.hpp>

#include <fstd/system/file.hpp>
#include <fstd/system/stdio.hpp>
#include <fstd/system/process.hpp>

#include <fstd/language/defer.hpp>
#include <fstd/language/flags.hpp>
#include <fstd/language/string.hpp>

#undef max
#include <tracy/Tracy.hpp>

#include <third-party/magic_enum.hpp> // @TODO remove it

#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include <third-party/microsoft_craziness.h> // @TODO remove it

using namespace fstd;
using namespace fstd::core;

using namespace f;

inline static void write_indentation(String_Builder& file_string_builder, uint8_t indentation)
{
	if (indentation == 0) return;

	// Up to 50 tabulations
	static uint8_t* spaces_buffer = (uint8_t*)u8"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

	language::string_view	indentation_string;

	if (indentation > 50) indentation = 50; // @TODO use a max intrinsic
	language::assign(indentation_string, spaces_buffer, indentation);
	print_to_builder(file_string_builder, "%v", indentation_string);
}

inline static void indented_print_to_builder(String_Builder& file_string_builder, uint8_t indentation, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	write_indentation(file_string_builder, indentation);

	language::string	format_string;

	language::assign(format_string, (uint8_t*)format);
	defer{ language::release(format_string); };

	print_to_builder(file_string_builder, &format_string, args);

	va_end(args);
}

static void write_default_initialization(String_Builder& file_string_builder, AST_Statement_Variable* variable_node)
{
	if (variable_node->type->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		AST_Statement_Basic_Type* basic_type = (AST_Statement_Basic_Type*)variable_node->type;

		switch (basic_type->keyword)
		{
		case Keyword::BOOL:
			print_to_builder(file_string_builder, "= false");
			break;
		case Keyword::I8:
		case Keyword::UI8:
		case Keyword::I16:
		case Keyword::UI16:
		case Keyword::I32:
		case Keyword::UI32:
		case Keyword::I64:
		case Keyword::UI64:
			print_to_builder(file_string_builder, "= 0");
			break;
		case Keyword::F32:
			print_to_builder(file_string_builder, "= 0.0f");
			break;
		case Keyword::F64:
			print_to_builder(file_string_builder, "= 0.0");
			break;
		case Keyword::STRING:
			core::Assert(false); // Not implemented, should only initialized the length (but have to print "; VARIABLE_NAME = length"
			break;
		case Keyword::STRING_VIEW:
			core::Assert(false); // Not implemented
			break;
		case Keyword::TYPE:
			core::Assert(false); // Not implemented
			break;
		default:
			core::Assert(false);
			break;
		}
		return;
	}
	else if (variable_node->type->ast_type == Node_Type::STATEMENT_USER_TYPE) {
		AST_Statement_User_Type* user_type = (AST_Statement_User_Type*)variable_node->type;

		// @TODO
		// Je dois récupérer le scope et procéder à une résolution du type ou faire en sorte ce soit fait avant et qu'ici j'ai directement
		// accès à la déclaration du type

		int foo = 0; //@Nocheckin
		user_type->identifier;
		// @TODO
		// No User type, only struct, alias, enum,...
		// the initialization of an alias should be forwarded to the underlying type initialization code
		// for struct all members should be initialized
	}
	else if (variable_node->type->ast_type == Node_Type::STATEMENT_TYPE_ARRAY) {
		int foo = 1;
	}
	else if (variable_node->type->ast_type == Node_Type::STATEMENT_TYPE_POINTER) {
		print_to_builder(file_string_builder, "= 0");
	}
	//else if (variable_node->type->ast_type == Node_Type::TYPE_STRUCT) {
	//	// @TODO
	//	// Recurse over all members
	//}

	// @TODO not implemented
	core::Assert(false);
}

static void write_generated_code(String_Builder& file_string_builder, IR& ir, AST_Node* node, uint8_t indentation = 0)
{
	if (!node) {
		return;
	}

	bool	recurse_sibling = true;

	if (node->ast_type == Node_Type::TYPE_ALIAS) {
		AST_Alias* alias_node = (AST_Alias*)node;

		print_to_builder(file_string_builder, "typedef ");
		write_generated_code(file_string_builder, ir, alias_node->type);
		print_to_builder(file_string_builder, " %v;\n", alias_node->name.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		AST_Statement_Basic_Type* basic_type_node = (AST_Statement_Basic_Type*)node;

		indented_print_to_builder(file_string_builder, indentation, "%v", basic_type_node->token.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_USER_TYPE) {
		AST_Statement_User_Type* user_type_node = (AST_Statement_User_Type*)node;

		indented_print_to_builder(file_string_builder, indentation, "%v", user_type_node->identifier.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_POINTER) {
		AST_Statement_Type_Pointer* basic_type_node = (AST_Statement_Type_Pointer*)node;

		write_generated_code(file_string_builder, ir, basic_type_node->sibling, indentation);
		print_to_builder(file_string_builder, "*");
		recurse_sibling = false;
	}
	else if (node->ast_type == Node_Type::STATEMENT_FUNCTION) {
		AST_Statement_Function* function_node = (AST_Statement_Function*)node;

		print_to_builder(file_string_builder, "\n");
		write_generated_code(file_string_builder, ir, function_node->return_type);
		print_to_builder(file_string_builder, " %v(", function_node->name.text);
		write_generated_code(file_string_builder, ir, (AST_Node*)function_node->arguments);
		print_to_builder(file_string_builder, ")\n");
		write_generated_code(file_string_builder, ir, (AST_Node*)function_node->scope);
	}
	else if (node->ast_type == Node_Type::STATEMENT_VARIABLE) {
		// For array take a look at STATEMENT_TYPE_ARRAY
		// Flamaros - 30 october 2020

		AST_Statement_Variable*	variable_node = (AST_Statement_Variable*)node;
		AST_Node*				type = variable_node->type;

		uint8_t type_indentation = variable_node->is_function_paramter ? 0 : indentation;

		// Write type
		if (type->ast_type == Node_Type::STATEMENT_TYPE_ARRAY && ((AST_Statement_Type_Array*)type)->array_size != nullptr) {
			// In this case we have to jump the array modifier
			write_generated_code(file_string_builder, ir, variable_node->type->sibling, type_indentation);
		}
		else {
			write_generated_code(file_string_builder, ir, variable_node->type, type_indentation);
		}

		// Write variable name
		print_to_builder(file_string_builder, " %v", variable_node->name.text);

		// Write array
		if (type->ast_type == Node_Type::STATEMENT_TYPE_ARRAY && ((AST_Statement_Type_Array*)type)->array_size != nullptr) {
			write_generated_code(file_string_builder, ir, variable_node->type);
		}

		// End the declaration
		if (variable_node->is_function_paramter == false) {
			// @TODO
			// Write the initialization code if necessary (don't have an expression)
			if (variable_node->expression == nullptr) { // @TODO Later we should do something clever (the back-end optimizer should be able to detect double initializations (taking only last write before first read))
				write_default_initialization(file_string_builder, variable_node);
			}

			print_to_builder(file_string_builder, ";\n");
		}

		// @TODO write the generated_code for expression
		// Get the register that contains the result
		//
		// We certainly have to implement the type checker here that create the register of sub-expression with the good type
		// then check types compatibilities when descending the in tree.
		// There is also the case of string type that should be declared (as struct) before getting assignement of his members.
		// For a string we have to allocate 2 (one for the data pointer, and an other for the size) registers and copy them.
		//write_generated_code();


	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_ARRAY) {
		// In cpp with can't declare dynamic arrays on the stack so for simplicity we convert them as pointers.
		// Flamaros - 30 october 2020
		AST_Statement_Type_Array* array_node = (AST_Statement_Type_Array*)node;

		if (array_node->array_size) {
			// It is the variable statement that is responsible to write the siblings of the array (the type and other modifiers)
			// Because the array modifier appears after the variable name in this case.
			print_to_builder(file_string_builder, "[");
			if (array_node->array_size) {
				write_generated_code(file_string_builder, ir, (AST_Node*)array_node->array_size);
			}
			print_to_builder(file_string_builder, "]");
		}
		else {
			write_generated_code(file_string_builder, ir, array_node->sibling, indentation);
			print_to_builder(file_string_builder, "*");
		}
		recurse_sibling = false;
	}
	else if (node->ast_type == Node_Type::STATEMENT_SCOPE) {
		AST_Statement_Scope* scope_node = (AST_Statement_Scope*)node;

		if (node != ir.ast->ast_root)
			indented_print_to_builder(file_string_builder, indentation, "{\n");
		write_generated_code(file_string_builder, ir, (AST_Node*)scope_node->first_child, indentation + 1);
		if (node != ir.ast->ast_root)
			indented_print_to_builder(file_string_builder, indentation, "}\n");
	}
	else if (node->ast_type == Node_Type::STATEMENT_LITERAL) {
		AST_Literal* literal_node = (AST_Literal*)node;

		if (literal_node->value.type == Token_Type::STRING_LITERAL) {
			print_to_builder(file_string_builder, literal_node->value.text);
			// @TODO use the *literal_node->value.value.string instead of the token's text
		}
		else if (literal_node->value.type == Token_Type::NUMERIC_LITERAL_I32
			|| literal_node->value.type == Token_Type::NUMERIC_LITERAL_I64) {
			print_to_builder(file_string_builder,
				"%ld", literal_node->value.value.integer);
		}
		else {
			core::Assert(false);
			// @TODO implement it
			// Actually the string builder doesn't support floats and unsigned intergers
		}
		recurse_sibling = false;
	}
	else if (is_binary_operator(node)) {
		AST_Binary_Operator* binary_operator_node = (AST_Binary_Operator*)node;

		indented_print_to_builder(file_string_builder, indentation, "tmp_register = \n");
		print_to_builder(file_string_builder, "");
//		indented_print_to_builder(file_string_builder, indentation, "%v", basic_type_node->token.text);
	}
	//else if (node->ast_type == Node_Type::STATEMENT_IDENTIFIER) {
	//	AST_Identifier* identifier_node = (AST_Identifier*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv"
	//		"\n%v", magic_enum::enum_name(node->ast_type), identifier_node->value.text);
	//}
	//else if (node->ast_type == Node_Type::FUNCTION_CALL) {
	//	AST_Function_Call* function_call_node = (AST_Function_Call*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv"
	//		"\nname: %v (nb_arguments: %d)", magic_enum::enum_name(node->ast_type), function_call_node->name.text, function_call_node->nb_arguments);
	//}
	//else if (node->ast_type == Node_Type::UNARY_OPERATOR_ADDRESS_OF) {
	//	AST_Unary_operator* address_of_node = (AST_Unary_operator*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv", magic_enum::enum_name(node->ast_type));
	//}
	//else if (node->ast_type == Node_Type::BINARY_OPERATOR_MEMBER_ACCESS) {
	//	AST_MEMBER_ACCESS* member_access_node = (AST_MEMBER_ACCESS*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv", magic_enum::enum_name(node->ast_type));
	//}
	//else {
	//	core::Assert(false);
	// @TODO report error here?
	//}

	// Sibling iteration
	if (recurse_sibling && node->sibling) {
		write_generated_code(file_string_builder, ir, node->sibling, indentation);
	}
}

void f::CPP_backend::compile(IR& ir, const fstd::system::Path& output_file_path)
{
	ZoneScopedNC("f::generate_cpp", 0xc43e00);

	system::File		file;
	String_Builder		string_builder;
	language::string	file_content;


	system::Path		cpp_file_path;

	defer{ system::reset_path(cpp_file_path); };

	print_to_builder(string_builder, "%v.cpp", to_string(output_file_path));
	system::from_native(cpp_file_path, to_string(string_builder));
	free_buffers(string_builder);

	defer {
		free_buffers(string_builder);
		release(file_content);
	};

	if (open_file(file, cpp_file_path, (system::File::Opening_Flag)
		((uint32_t)system::File::Opening_Flag::CREATE
			| (uint32_t)system::File::Opening_Flag::WRITE)) == false) {
		print_to_builder(string_builder, "Failed to open file: %s", to_string(output_file_path));
		system::print(to_string(string_builder));
		return;
	}

	language::string	f_lang_basic_types;

	// This part can be specific to the targetted architecture, OS, and compiler!!!
	language::assign(f_lang_basic_types, (uint8_t*)
		"// We want to be able to use float without C runtime!!!\n"
		"extern \"C\" int _fltused;\n"
		"\n"
		"// f-lang basic types\n"
		"typedef int                   i8;\n"
		"typedef unsigned int          ui8;\n"
		"typedef int                   i16;\n"
		"typedef unsigned int          ui16;\n"
		"typedef int                   i32;\n"
		"typedef unsigned int          ui32;\n"
		"typedef long long             i64;\n"
		"typedef unsigned long long    ui64;\n"
		"\n"
		"// Types that depend on the arch\n" // This section should be generated depending of the targeted architecture
		"typedef ui64                  size_t;\n"
		"\n"
		"struct string {\n"
		"	ui8*   data;\n"
		"	size_t size;\n"
		"};\n"
		"\n"
	);

	print_to_builder(string_builder, "// Beginning of compiler code\n");
	print_to_builder(string_builder, f_lang_basic_types);

	print_to_builder(string_builder, "// Beginning of user code\n");
	write_generated_code(string_builder, ir, ir.ast->ast_root);

	file_content = to_string(string_builder);

	system::write_file(file, language::to_utf8(file_content), (uint32_t)language::get_string_size(file_content));
	system::close_file(file);

	// =========================================================================
	// Compile the cpp generated file
	// =========================================================================

	Find_Result find_result = find_visual_studio_and_windows_sdk();
	
	system::Path	cl_path;
	system::Path	link_path;

	defer {
		system::reset_path(cl_path);
		system::reset_path(link_path);
	};

	free_buffers(string_builder);
	print_to_builder(string_builder, "%Cw\\cl.exe", find_result.vs_exe_path);
	system::from_native(cl_path, to_string(string_builder));

	free_buffers(string_builder);
	print_to_builder(string_builder, "%Cw\\link.exe", find_result.vs_exe_path);
	system::from_native(link_path, to_string(string_builder));

	free_resources(&find_result);

	// Start compilation
	free_buffers(string_builder);
	// No C++ runtime
	// https://hero.handmade.network/forums/code-discussion/t/94-guide_-_how_to_avoid_c_c++_runtime_on_windows#530
	print_to_builder(string_builder, "/nologo /Gm- /GR- /EHa- /Oi /GS- /Gs9999999 /utf-8 \"%s\" -link -nodefaultlib /ENTRY:main /SUBSYSTEM:CONSOLE /STACK:0x100000,0x100000", cpp_file_path);
	if (!system::execute_process(cl_path, to_string(string_builder))) {
		f::Token dummy_token;

		dummy_token.type = f::Token_Type::UNKNOWN;
		dummy_token.file_path = system::to_string(cl_path);
		dummy_token.line = 0;
		dummy_token.column = 0;
		report_error(Compiler_Error::warning, dummy_token, "Failed to launch cl.exe.\n");
	}
}
