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

static void write_generated_code(String_Builder& file_string_builder, AST_Node* node, int64_t parent_index = -1, int64_t left_node_index = -1)
{
	if (!node) {
		return;
	}

	static uint32_t		nb_nodes = 0;
	uint32_t			node_index = nb_nodes++;
	bool				recurse_sibling = true;

	//if (parent_index != -1) {
	//	if (left_node_index == -1) {
	//		print_to_builder(file_string_builder, "\t" "node_%ld -> node_%ld\n", parent_index, node_index);
	//	}
	//	else {
	//		print_to_builder(file_string_builder, "\t" "node_%ld -> node_%ld [color=\"dodgerblue\"]\n", parent_index, node_index);
	//	}
	//}

	//print_to_builder(file_string_builder, "\n\t" "node_%ld [label=\"", node_index);
	if (node->ast_type == Node_Type::TYPE_ALIAS) {
		AST_Alias* alias_node = (AST_Alias*)node;

		print_to_builder(file_string_builder, "typedef ");
		write_generated_code(file_string_builder, alias_node->type);
		print_to_builder(file_string_builder, " %v;\n", alias_node->type_name.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		AST_Statement_Basic_Type* basic_type_node = (AST_Statement_Basic_Type*)node;

		print_to_builder(file_string_builder, "%v", basic_type_node->token.text);
	}
	//else if (node->ast_type == Node_Type::STATEMENT_USER_TYPE) {
	//	AST_Statement_User_Type* user_type_node = (AST_Statement_User_Type*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv"
	//		"\n%v", magic_enum::enum_name(node->ast_type), user_type_node->identifier.text);
	//}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_POINTER) {
		AST_Statement_Type_Pointer* basic_type_node = (AST_Statement_Type_Pointer*)node;

		write_generated_code(file_string_builder, node->sibling, parent_index, node_index);
		print_to_builder(file_string_builder, "*");
		recurse_sibling = false;
	}
	else if (node->ast_type == Node_Type::STATEMENT_FUNCTION) {
		AST_Statement_Function* function_node = (AST_Statement_Function*)node;

		print_to_builder(file_string_builder, "\n");
		write_generated_code(file_string_builder, function_node->return_type);
		print_to_builder(file_string_builder, " %v(", function_node->name.text);
		write_generated_code(file_string_builder, (AST_Node*)function_node->arguments);
		print_to_builder(file_string_builder, ")\n{\n");
		write_generated_code(file_string_builder, (AST_Node*)function_node->scope);
		print_to_builder(file_string_builder, "}\n");
	}
	//else if (node->ast_type == Node_Type::STATEMENT_VARIABLE) {
	//	AST_Statement_Variable* variable_node = (AST_Statement_Variable*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv"
	//		"\nname: %v (is_parameter: %d is_optional: %d)", magic_enum::enum_name(node->ast_type), variable_node->name.text, variable_node->is_function_paramter, variable_node->is_optional);
	//}
	//else if (node->ast_type == Node_Type::STATEMENT_TYPE_ARRAY) {
	//	AST_Statement_Type_Array* array_node = (AST_Statement_Type_Array*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv", magic_enum::enum_name(node->ast_type));
	//}
	//else if (node->ast_type == Node_Type::STATEMENT_SCOPE) {
	//	AST_Statement_Scope* scope_node = (AST_Statement_Scope*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv", magic_enum::enum_name(node->ast_type));
	//}
	//else if (node->ast_type == Node_Type::EXPRESSION) {
	//	AST_Expression* expression_node = (AST_Expression*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv", magic_enum::enum_name(node->ast_type));
	//}
	//else if (node->ast_type == Node_Type::STATEMENT_LITERAL) {
	//	AST_Literal* literal_node = (AST_Literal*)node;

	//	if (literal_node->value.type == Token_Type::STRING_LITERAL) {
	//		print_to_builder(file_string_builder,
	//			"%Cv"
	//			"\n%v", magic_enum::enum_name(node->ast_type), literal_node->value.text);
	//		// @TODO use the *literal_node->value.value.string instead of the token's text
	//	}
	//	else if (literal_node->value.type == Token_Type::NUMERIC_LITERAL_I32
	//		|| literal_node->value.type == Token_Type::NUMERIC_LITERAL_I64) {
	//		print_to_builder(file_string_builder,
	//			"%Cv"
	//			"\n%ld", magic_enum::enum_name(node->ast_type), literal_node->value.value.integer);
	//	}
	//	else {
	//		core::Assert(false);
	//		// @TODO implement it
	//		// Actually the string builder doesn't support floats and unsigned intergers
	//	}
	//}
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
	//else if (node->ast_type == Node_Type::OPERATOR_ADDRESS_OF) {
	//	AST_ADDRESS_OF* address_of_node = (AST_ADDRESS_OF*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv", magic_enum::enum_name(node->ast_type));
	//}
	//else if (node->ast_type == Node_Type::OPERATOR_MEMBER_ACCESS) {
	//	AST_MEMBER_ACCESS* member_access_node = (AST_MEMBER_ACCESS*)node;

	//	print_to_builder(file_string_builder,
	//		"%Cv", magic_enum::enum_name(node->ast_type));
	//}
	//else {
	//	core::Assert(false);
	//	print_to_builder(file_string_builder,
	//		"UNKNOWN type"
	//		"\nnode_%ld", node_index);
	//}
	//print_to_builder(file_string_builder, "\" shape=box, style=filled, color=black, fillcolor=lightseagreen]\n");

	// Children iteration
	if (node->ast_type == Node_Type::TYPE_ALIAS) {
		// No-op
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
		// No-op
	}
	else if (node->ast_type == Node_Type::STATEMENT_VARIABLE) {
		AST_Statement_Variable* variable_node = (AST_Statement_Variable*)node;

		write_generated_code(file_string_builder, variable_node->type, node_index);
		write_generated_code(file_string_builder, (AST_Node*)variable_node->expression, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_ARRAY) {
		AST_Statement_Type_Array* array_node = (AST_Statement_Type_Array*)node;

		write_generated_code(file_string_builder, (AST_Node*)array_node->array_size, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_SCOPE) {
		AST_Statement_Scope* scope_node = (AST_Statement_Scope*)node;

		write_generated_code(file_string_builder, (AST_Node*)scope_node->first_child, node_index);
	}
	else if (node->ast_type == Node_Type::EXPRESSION) {
		AST_Expression* expression_node = (AST_Expression*)node;

		write_generated_code(file_string_builder, (AST_Node*)expression_node->first_child, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_LITERAL) {
		// No children
	}
	else if (node->ast_type == Node_Type::STATEMENT_IDENTIFIER) {
		// No children
	}
	else if (node->ast_type == Node_Type::FUNCTION_CALL) {
		AST_Function_Call* function_call_node = (AST_Function_Call*)node;

		write_generated_code(file_string_builder, (AST_Node*)function_call_node->parameters, node_index);
	}
	else if (node->ast_type == Node_Type::OPERATOR_ADDRESS_OF) {
		AST_ADDRESS_OF* address_of_node = (AST_ADDRESS_OF*)node;

		write_generated_code(file_string_builder, (AST_Node*)address_of_node->right, node_index);
	}
	else if (node->ast_type == Node_Type::OPERATOR_MEMBER_ACCESS) {
		AST_MEMBER_ACCESS* member_access_node = (AST_MEMBER_ACCESS*)node;

		write_generated_code(file_string_builder, (AST_Node*)member_access_node->left, node_index);
		write_generated_code(file_string_builder, (AST_Node*)member_access_node->right, node_index);
	}
	else {
		core::Assert(false);
	}

	// Sibling iteration
	if (recurse_sibling && node->sibling) {
		write_generated_code(file_string_builder, node->sibling, parent_index, node_index);
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
		"typedef BOOL				 *PBOOL;\n"
		"typedef BOOL				 *LPBOOL;\n"
		"typedef BYTE				 *PBYTE;\n"
		"typedef BYTE				 *LPBYTE;\n"
		"typedef int				 *PINT;\n"
		"typedef int				 *LPINT;\n"
		"typedef WORD				 *PWORD;\n"
		"typedef WORD				 *LPWORD;\n"
		"typedef long				 *LPLONG;\n"
		"typedef DWORD				 *PDWORD;\n"
		"typedef DWORD				 *LPDWORD;\n"
		"typedef void				 *LPVOID;\n"
		"typedef const void			 *LPCVOID;\n"
		"\n"
		"typedef int                 INT;\n"
		"typedef unsigned int        UINT;\n"
		"typedef unsigned int        *PUINT;\n"
		"\n"
		"// From basestd.h\n"
		"#if !defined(_MAC) && (defined(_M_MRX000) || defined(_WIN64)) && (_MSC_VER >= 1100) && !(defined(MIDL_PASS) || defined(RC_INVOKED))\n"
		"#define POINTER_64 __ptr64\n"
		"		typedef unsigned __int64 POINTER_64_INT;\n"
		"#if defined(_WIN64)\n"
		"#define POINTER_32 __ptr32\n"
		"#else\n"
		"#define POINTER_32\n"
		"#endif\n"
		"#else\n"
		"#if defined(_MAC) && defined(_MAC_INT_64)\n"
		"#define POINTER_64 __ptr64\n"
		"		typedef unsigned __int64 POINTER_64_INT;\n"
		"#else\n"
		"#if (_MSC_VER >= 1300) && !(defined(MIDL_PASS) || defined(RC_INVOKED))\n"
		"#define POINTER_64 __ptr64\n"
		"#else\n"
		"#define POINTER_64\n"
		"#endif\n"
		"		typedef unsigned long POINTER_64_INT;\n"
		"#endif\n"
		"#define POINTER_32\n"
		"#endif\n"
		"\n"
		"#if defined(_WIN64)\n"
		"		typedef __int64 INT_PTR, * PINT_PTR;\n"
		"	typedef unsigned __int64 UINT_PTR, * PUINT_PTR;\n"
		"\n"
		"	typedef __int64 LONG_PTR, * PLONG_PTR;\n"
		"	typedef unsigned __int64 ULONG_PTR, * PULONG_PTR;\n"
		"\n"
		"#define __int3264   __int64\n"
		"\n"
		"#else\n"
		"		typedef _W64 int INT_PTR, * PINT_PTR;\n"
		"	typedef _W64 unsigned int UINT_PTR, * PUINT_PTR;\n"
		"\n"
		"	typedef _W64 long LONG_PTR, * PLONG_PTR;\n"
		"	typedef _W64 unsigned long ULONG_PTR, * PULONG_PTR;\n"
		"\n"
		"#define __int3264   __int32\n"
		"\n"
		"#endif\n"
		"\n"
		"// From winnt.h\n"
		"typedef void* PVOID;\n"
		"typedef void* POINTER_64 PVOID64;\n"
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
		");\n"
		"\n");

	print_to_builder(string_builder, "// Beginning of compiler code\n");
	print_to_builder(string_builder, win32_api_declarations);

	print_to_builder(string_builder, "// Beginning of user code\n");
	write_generated_code(string_builder, ir.ast->root);

//	print_to_builder(string_builder, "}\n");

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
	print_to_builder(string_builder, "\"%s\"", cpp_file_path);
	if (!system::execute_process(cl_path, to_string(string_builder))) {
		f::Token dummy_token;

		dummy_token.type = f::Token_Type::UNKNOWN;
		dummy_token.file_path = system::to_string(cl_path);
		dummy_token.line = 0;
		dummy_token.column = 0;
		report_error(Compiler_Error::warning, dummy_token, "Failed to launch cl.exe.\n");
	}
}
