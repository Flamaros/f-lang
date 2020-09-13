#include "CPP_backend.hpp"

#include <fstd/core/string_builder.hpp>

#include <fstd/system/file.hpp>
#include <fstd/system/stdio.hpp>

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

static void write_dot_node(String_Builder& file_string_builder, AST_Node* node, int64_t parent_index = -1, int64_t left_node_index = -1)
{
	if (!node) {
		return;
	}

	language::string	dot_node;
	static uint32_t		nb_nodes = 0;
	uint32_t			node_index = nb_nodes++;

	defer {
		release(dot_node);
	};

	if (parent_index != -1) {
		if (left_node_index == -1) {
			print_to_builder(file_string_builder, "\t" "node_%ld -> node_%ld\n", parent_index, node_index);
		}
		else {
			print_to_builder(file_string_builder, "\t" "node_%ld -> node_%ld [color=\"dodgerblue\"]\n", parent_index, node_index);
		}
	}

	// @TODO Here we could make an arrow between siblings, but we have to use subgraph to make the nodes stay at the right position.
	// Take a look at:
	// https://stackoverflow.com/questions/3322827/how-to-set-fixed-depth-levels-in-dot-graphs
	//
	// Maybe using colors is enough to ease the understanding of links
	//
	// Flamaros - 09 may 2020

	print_to_builder(file_string_builder, "\n\t" "node_%ld [label=\"", node_index);
	if (node->ast_type == Node_Type::TYPE_ALIAS) {
		AST_Alias* alias_node = (AST_Alias*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%v", magic_enum::enum_name(node->ast_type), alias_node->type_name.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		AST_Statement_Basic_Type* basic_type_node = (AST_Statement_Basic_Type*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%Cv", magic_enum::enum_name(node->ast_type), magic_enum::enum_name(basic_type_node->keyword));
	}
	else if (node->ast_type == Node_Type::STATEMENT_USER_TYPE) {
		AST_Statement_User_Type* user_type_node = (AST_Statement_User_Type*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%v", magic_enum::enum_name(node->ast_type), user_type_node->identifier.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_POINTER) {
		AST_Statement_Type_Pointer* basic_type_node = (AST_Statement_Type_Pointer*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == Node_Type::STATEMENT_FUNCTION) {
		AST_Statement_Function* function_node = (AST_Statement_Function*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\nname: %v (nb_arguments: %d)", magic_enum::enum_name(node->ast_type), function_node->name.text, function_node->nb_arguments);
	}
	else if (node->ast_type == Node_Type::STATEMENT_VARIABLE) {
		AST_Statement_Variable* variable_node = (AST_Statement_Variable*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\nname: %v (is_parameter: %d is_optional: %d)", magic_enum::enum_name(node->ast_type), variable_node->name.text, variable_node->is_function_paramter, variable_node->is_optional);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_ARRAY) {
		AST_Statement_Type_Array* array_node = (AST_Statement_Type_Array*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == Node_Type::STATEMENT_SCOPE) {
		AST_Statement_Scope* scope_node = (AST_Statement_Scope*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == Node_Type::EXPRESSION) {
		AST_Expression* expression_node = (AST_Expression*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == Node_Type::STATEMENT_LITERAL) {
		AST_Literal* literal_node = (AST_Literal*)node;

		if (literal_node->value.type == Token_Type::STRING_LITERAL) {
			print_to_builder(file_string_builder,
				"%Cv"
				"\n%v", magic_enum::enum_name(node->ast_type), literal_node->value.text);
			// @TODO use the *literal_node->value.value.string instead of the token's text
		}
		else if (literal_node->value.type == Token_Type::NUMERIC_LITERAL_I32
			|| literal_node->value.type == Token_Type::NUMERIC_LITERAL_I64) {
			print_to_builder(file_string_builder,
				"%Cv"
				"\n%ld", magic_enum::enum_name(node->ast_type), literal_node->value.value.integer);
		}
		else {
			core::Assert(false);
			// @TODO implement it
			// Actually the string builder doesn't support floats and unsigned intergers
		}
	}
	else if (node->ast_type == Node_Type::STATEMENT_IDENTIFIER) {
		AST_Identifier* identifier_node = (AST_Identifier*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%v", magic_enum::enum_name(node->ast_type), identifier_node->value.text);
	}
	else if (node->ast_type == Node_Type::FUNCTION_CALL) {
		AST_Function_Call* function_call_node = (AST_Function_Call*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\nname: %v (nb_arguments: %d)", magic_enum::enum_name(node->ast_type), function_call_node->name.text, function_call_node->nb_arguments);
	}
	else if (node->ast_type == Node_Type::OPERATOR_ADDRESS_OF) {
		AST_ADDRESS_OF* address_of_node = (AST_ADDRESS_OF*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == Node_Type::OPERATOR_MEMBER_ACCESS) {
		AST_MEMBER_ACCESS* member_access_node = (AST_MEMBER_ACCESS*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else {
		core::Assert(false);
		print_to_builder(file_string_builder,
			"UNKNOWN type"
			"\nnode_%ld", node_index);
	}
	print_to_builder(file_string_builder, "\" shape=box, style=filled, color=black, fillcolor=lightseagreen]\n");

	// Children iteration
	if (node->ast_type == Node_Type::TYPE_ALIAS) {
		AST_Alias* alias_node = (AST_Alias*)node;
		write_dot_node(file_string_builder, alias_node->type, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		// No children
	}
	else if (node->ast_type == Node_Type::STATEMENT_USER_TYPE) {
		// No children
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_POINTER) {
		// No children
	}
	else if (node->ast_type == Node_Type::STATEMENT_FUNCTION) {
		AST_Statement_Function* function_node = (AST_Statement_Function*)node;
		write_dot_node(file_string_builder, (AST_Node*)function_node->arguments, node_index);
		write_dot_node(file_string_builder, function_node->return_type, node_index);
		write_dot_node(file_string_builder, (AST_Node*)function_node->scope, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_VARIABLE) {
		AST_Statement_Variable* variable_node = (AST_Statement_Variable*)node;

		write_dot_node(file_string_builder, variable_node->type, node_index);
		write_dot_node(file_string_builder, (AST_Node*)variable_node->expression, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_ARRAY) {
		AST_Statement_Type_Array* array_node = (AST_Statement_Type_Array*)node;

		write_dot_node(file_string_builder, (AST_Node*)array_node->array_size, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_SCOPE) {
		AST_Statement_Scope* scope_node = (AST_Statement_Scope*)node;

		write_dot_node(file_string_builder, (AST_Node*)scope_node->first_child, node_index);
	}
	else if (node->ast_type == Node_Type::EXPRESSION) {
		AST_Expression* expression_node = (AST_Expression*)node;

		write_dot_node(file_string_builder, (AST_Node*)expression_node->first_child, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_LITERAL) {
		// No children
	}
	else if (node->ast_type == Node_Type::STATEMENT_IDENTIFIER) {
		// No children
	}
	else if (node->ast_type == Node_Type::FUNCTION_CALL) {
		AST_Function_Call* function_call_node = (AST_Function_Call*)node;

		write_dot_node(file_string_builder, (AST_Node*)function_call_node->parameters, node_index);
	}
	else if (node->ast_type == Node_Type::OPERATOR_ADDRESS_OF) {
		AST_ADDRESS_OF* address_of_node = (AST_ADDRESS_OF*)node;

		write_dot_node(file_string_builder, (AST_Node*)address_of_node->right, node_index);
	}
	else if (node->ast_type == Node_Type::OPERATOR_MEMBER_ACCESS) {
		AST_MEMBER_ACCESS* member_access_node = (AST_MEMBER_ACCESS*)node;

		write_dot_node(file_string_builder, (AST_Node*)member_access_node->left, node_index);
		write_dot_node(file_string_builder, (AST_Node*)member_access_node->right, node_index);
	}
	else {
		core::Assert(false);
	}

	// Sibling iteration
	if (node->sibling) {
		write_dot_node(file_string_builder, node->sibling, parent_index, node_index);
	}

	dot_node = to_string(file_string_builder);
}

void f::CPP_backend::compile(IR& irl, const fstd::system::Path& output_file_path)
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
		close_file(file);
	};

	if (open_file(file, cpp_file_path, (system::File::Opening_Flag)
		((uint32_t)system::File::Opening_Flag::CREATE
			| (uint32_t)system::File::Opening_Flag::WRITE)) == false) {
		print_to_builder(string_builder, "Failed to open file: %s", to_string(output_file_path));
		system::print(to_string(string_builder));
		return;
	}

	language::string	win32_api_declarations;

	language::assign(win32_api_declarations, (uint8_t*)
		"// Calling conventions defines\n"
		"#define WINBASEAPI __declspec(dllimport)\n"
		"#define WINAPI      __stdcall\n"
		"#define WINAPIV     __cdecl\n"
		"#define APIENTRY    WINAPI\n"
		"\n"
		"// Types defines\n"
		"typedef unsigned long       DWORD;\n"
		"typedef int                 BOOL;\n"
		"typedef unsigned char       BYTE;\n"
		"typedef unsigned short      WORD;\n"
		"typedef float               FLOAT;\n"
		"typedef FLOAT               *PFLOAT;\n"
		"typedef BOOL near           *PBOOL;\n"
		"typedef BOOL far            *LPBOOL;\n"
		"typedef BYTE near           *PBYTE;\n"
		"typedef BYTE far            *LPBYTE;\n"
		"typedef int near            *PINT;\n"
		"typedef int far             *LPINT;\n"
		"typedef WORD near           *PWORD;\n"
		"typedef WORD far            *LPWORD;\n"
		"typedef long far            *LPLONG;\n"
		"typedef DWORD near          *PDWORD;\n"
		"typedef DWORD far           *LPDWORD;\n"
		"typedef void far            *LPVOID;\n"
		"typedef CONST void far      *LPCVOID;\n"
		"\n"
		"typedef int                 INT;\n"
		"typedef unsigned int        UINT;\n"
		"typedef unsigned int        *PUINT;\n"
		"\n"
		"typedef void *HANDLE;\n"
		"\n"
		"// From minwinbase.h\n"
		"typedef struct _SECURITY_ATTRIBUTES {\n"
		"    DWORD nLength;\n"
		"    LPVOID lpSecurityDescriptor;\n"
		"    BOOL bInheritHandle;\n"
		"} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;\n"
		"\n"
		"typedef struct _OVERLAPPED {\n"
		"    ULONG_PTR Internal;\n"
		"    ULONG_PTR InternalHigh;\n"
		"    union {\n"
		"        struct {\n"
		"            DWORD Offset;\n"
		"            DWORD OffsetHigh;\n"
		"        } DUMMYSTRUCTNAME;\n"
		"        PVOID Pointer;\n"
		"    } DUMMYUNIONNAME;\n"
		"\n"
		"    HANDLE  hEvent;\n"
		"} OVERLAPPED, *LPOVERLAPPED;\n"
		"\n"
		"typedef struct _OVERLAPPED_ENTRY {\n"
		"    ULONG_PTR lpCompletionKey;\n"
		"    LPOVERLAPPED lpOverlapped;\n"
		"    ULONG_PTR Internal;\n"
		"    DWORD dwNumberOfBytesTransferred;\n"
		"} OVERLAPPED_ENTRY, *LPOVERLAPPED_ENTRY;\n"
		"\n"
		"// Function declarations\n"
		"WINBASEAPI\n"
		"BOOL\n"
		"WINAPI\n"
		"WriteFile(\n"
		"	HANDLE hFile,\n"
		"	LPCVOID lpBuffer,\n"
		"	DWORD nNumberOfBytesToWrite,\n"
		"	LPDWORD lpNumberOfBytesWritten,\n"
		"	LPOVERLAPPED lpOverlapped\n"
		");\n");

	print_to_builder(string_builder, win32_api_declarations);

//	write_dot_node(string_builder, irl.ast->root);

//	print_to_builder(string_builder, "}\n");

	file_content = to_string(string_builder);

	system::write_file(file, language::to_utf8(file_content), (uint32_t)language::get_string_size(file_content));

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
}
