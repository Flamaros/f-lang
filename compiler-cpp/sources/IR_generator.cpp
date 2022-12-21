#include "IR_generator.hpp"

#include "globals.hpp"

#include "parser/parser.hpp"
#include "parser/symbol_solver.hpp"

#include <third-party/SpookyV2.h>

#include <tracy/Tracy.hpp>

using namespace fstd;
using namespace fstd::core;

using namespace f;

// @TODO Should we count imported libraries and function during the parsing to optimize the allocated size for
// structs Imported_Library and Imported_Function?
#define NB_PREALLOCATED_IMPORTED_LIBRARIES				128
#define NB_PREALLOCATED_IMPORTED_FUNCTIONS_PER_LIBRARY	4096

inline Imported_Library* allocate_imported_library()
{
	// Ensure that no reallocation could happen during the resize
	core::Assert(memory::get_array_size(globals.ir_data.imported_libraries) < memory::get_array_reserved(globals.ir_data.imported_libraries));

	memory::resize_array(globals.ir_data.imported_libraries, memory::get_array_size(globals.ir_data.imported_libraries) + 1);

	Imported_Library* new_imported_library = memory::get_array_last_element(globals.ir_data.imported_libraries);
	return new_imported_library;
}

inline Imported_Function* allocate_imported_function()
{
	// Ensure that no reallocation could happen during the resize
	core::Assert(memory::get_array_size(globals.ir_data.imported_functions) < memory::get_array_reserved(globals.ir_data.imported_functions));

	memory::resize_array(globals.ir_data.imported_functions, memory::get_array_size(globals.ir_data.imported_functions) + 1);

	Imported_Function* new_imported_function = memory::get_array_last_element(globals.ir_data.imported_functions);
	return new_imported_function;
}

// =============================================================================


static void parse_ast(Parsing_Result& parsing_result, IR& ir, AST_Node* node);
static void parse_function_declaration(IR& ir, AST_Statement_Function* function_node);
static size_t get_list_size(AST_Node* node);

// @TODO actually Parsing_Result isn't really used by parse_ast because the AST_Node can already contains a
// pointer to the right symbol table node.
// But it might be useful to check shadowing and some other errors.

static size_t get_list_size(AST_Node* node)
{
	size_t result = 0;
	for (AST_Node* current_node = node; current_node; current_node = current_node->sibling)
		result++;
	return result;
}

static void parse_ast(Parsing_Result& parsing_result, IR& ir, AST_Node* node)
{
	if (!node) {
		return;
	}

	bool	recurse_sibling = true;

	if (node->ast_type == Node_Type::TYPE_ALIAS) {
		// @Warning we do nothing here because the types were already stored in the symbol table
		// We may eventually want to replace the type base is underlying one if our aliases are links,
		// but if they acts like new types (like typedef in C++) then we should store all information to be able to
		// check type conversions (aka cast)


		//AST_Alias* alias_node = (AST_Alias*)node;

		////print_to_builder(file_string_builder, "typedef ");
		//parse_ast(parsing_result, ir, alias_node->type);
		////print_to_builder(file_string_builder, " %v;\n", alias_node->name.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		AST_Statement_Basic_Type* basic_type_node = (AST_Statement_Basic_Type*)node;

		//indented_print_to_builder(file_string_builder, "%v", basic_type_node->token.text);
	}
	else if (node->ast_type == Node_Type::USER_TYPE_IDENTIFIER) {
		AST_User_Type_Identifier* user_type_node = (AST_User_Type_Identifier*)node;

		AST_Node* underlying_type = get_user_type(user_type_node);

		//indented_print_to_builder(file_string_builder, "/*%v*/", user_type_node->identifier.text);
		if (underlying_type)
			parse_ast(parsing_result, ir, resolve_type(underlying_type));
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_POINTER) {
		AST_Statement_Type_Pointer* basic_type_node = (AST_Statement_Type_Pointer*)node;

		parse_ast(parsing_result, ir, basic_type_node->sibling);
		//print_to_builder(file_string_builder, "*");
		recurse_sibling = false;
	}
	else if (node->ast_type == Node_Type::STATEMENT_FUNCTION) {
		AST_Statement_Function* function_node = (AST_Statement_Function*)node;

		// @TODO generate C function declaration that should be done in the correct order (after declaration types)
		// Be careful of function pointers

		parse_function_declaration(ir, function_node);
		//write_function_declaration(file_string_builder, ir, function_node);

		if (function_node->scope) {
			// @TODO make it static
			fstd::language::string_view	main_string;
			fstd::language::assign(main_string, (uint8_t*)"main");

			if (language::are_equals(function_node->name.text, main_string)) {
				log(*globals.logger, Log_Level::info, "[IR] Found entry point\n");
				// @TODO register the RVA for the backend
			}

			



			//print_to_builder(file_string_builder, "\n");
			parse_ast(parsing_result, ir, function_node->return_type);
			//print_to_builder(file_string_builder, " %v(", function_node->name.text);
			//write_argument_list(file_string_builder, ir, (AST_Node*)function_node->arguments);
			//print_to_builder(file_string_builder, ")%Cs\n", function_node->scope ? "" : ";");
			parse_ast(parsing_result, ir, (AST_Node*)function_node->scope);
		}
	}
	else if (node->ast_type == Node_Type::STATEMENT_VARIABLE) {
		// For array take a look at STATEMENT_TYPE_ARRAY
		// Flamaros - 30 october 2020

		AST_Statement_Variable* variable_node = (AST_Statement_Variable*)node;
		AST_Node* type = variable_node->type;

		if (variable_node->is_function_parameter) {
			// @TODO copy the value to an allocated register (from reserverd register or stack, depending of the calling convention)
			core::Assert(false);
		}
		else {
			if (variable_node->expression) {
				// @TODO init the variable with the result of the expression
				// 1. Generate code for the expression
				// 2. Get the return value register to be able to asign this register or a copy to the variable
				// 3. Do the cast if return type and variable type didn't directly match (can be done by the copy) (check if this implicite cast is allowed)
				//    - Explicit cast should already have generated an instruction from the instruction parsing
				core::Assert(false);
			}
			else {
				// @TODO generate the value, for basic type it is an immediate value, for a struct the code to generate is a bit more complicated
				// I should iterate over all members
			}
		}


		// Write type
		if (type->ast_type == Node_Type::STATEMENT_TYPE_ARRAY && ((AST_Statement_Type_Array*)type)->array_size != nullptr) {
			// In this case we have to jump the array modifier
			parse_ast(parsing_result, ir, variable_node->type->sibling);
		}
		else {
			parse_ast(parsing_result, ir, variable_node->type);
		}

		// Write variable name
		//print_to_builder(file_string_builder, " %v", variable_node->name.text);

		// Write array
		if (type->ast_type == Node_Type::STATEMENT_TYPE_ARRAY && ((AST_Statement_Type_Array*)type)->array_size != nullptr) {
			parse_ast(parsing_result, ir, variable_node->type);
		}

		// End the declaration
		if (variable_node->is_function_parameter == false) {
			// @TODO
			// Write the initialization code if necessary (don't have an expression)
			if (variable_node->expression == nullptr) { // @TODO Later we should do something clever (the back-end optimizer should be able to detect double initializations (taking only last write before first read))
				//if (globals.cpp_backend_data.union_declaration_depth == 0) {
					//write_default_initialization(file_string_builder, variable_node, variable_node->type);
				//}
			}
			else {
				//print_to_builder(file_string_builder, " = ");
				//write_expression(file_string_builder, ir, variable_node->expression);
			}

			//print_to_builder(file_string_builder, ";\n");
		}

		// @TODO write the generated_code for expression
		// Get the register that contains the result
		//
		// We certainly have to implement the type checker here that create the register of sub-expression with the good type
		// then check types compatibilities when descending the in tree.
		// There is also the case of string type that should be declared (as struct) before getting assignement of his members.
		// For a string we have to allocate 2 (one for the data pointer, and an other for the size) registers and copy them.
		//parse_ast();

		if (variable_node->expression) {
			// @TODO
			// Est-ce que je dois déclencher un parsing particulier ici afin de pouvoir binder le résultat si l'expression est compiletime
			// ou récupérer un pointeur sur une section de code à executer?

			parse_ast(parsing_result, ir, variable_node->expression);
		}

	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_ARRAY) {
		// In cpp with can't declare dynamic arrays on the stack so for simplicity we convert them as pointers.
		// Flamaros - 30 october 2020
		AST_Statement_Type_Array* array_node = (AST_Statement_Type_Array*)node;

		if (array_node->array_size) {
			// It is the variable statement that is responsible to write the siblings of the array (the type and other modifiers)
			// Because the array modifier appears after the variable name in this case.
			//print_to_builder(file_string_builder, "[");
			if (array_node->array_size) {
				parse_ast(parsing_result, ir, (AST_Node*)array_node->array_size);
			}
			//print_to_builder(file_string_builder, "]");
		}
		else {
			parse_ast(parsing_result, ir, array_node->sibling);
			//print_to_builder(file_string_builder, "*");
		}
		recurse_sibling = false;
	}
	else if (node->ast_type == Node_Type::STATEMENT_SCOPE) {
		AST_Statement_Scope* scope_node = (AST_Statement_Scope*)node;

		if (node != ir.parsing_result->ast_root) {
			//indented_print_to_builder(file_string_builder, "{\n");
		}
		parse_ast(parsing_result, ir, (AST_Node*)scope_node->first_child);
		if (node != ir.parsing_result->ast_root) {
			//indented_print_to_builder(file_string_builder, "}\n");
		}
	}
	else if (node->ast_type == Node_Type::STATEMENT_LITERAL) {
		AST_Literal* literal_node = (AST_Literal*)node;

		if (literal_node->value.type == Token_Type::STRING_LITERAL) {
			Literal literal;

			// @TODO use the *literal_node->value.value.string instead of the token's text
			memory::reserve_array(literal.data, literal_node->value.text.size + 1);
			memory::array_copy(literal.data, 0, literal_node->value.text.ptr, literal_node->value.text.size);
			memory::array_push_back(literal.data, (uint8_t)'\0');
			literal.RVA = ir.read_only_data.current_RVA;

			// @TODO @SpeedUp see how this bufer can be pre-allocated
			memory::array_push_back(ir.read_only_data.literals, literal);
			ir.read_only_data.current_RVA += memory::get_array_size(literal.data);
		}
		else if (literal_node->value.type == Token_Type::NUMERIC_LITERAL_I32
			|| literal_node->value.type == Token_Type::NUMERIC_LITERAL_I64) {
			//print_to_builder(file_string_builder, "%ld", literal_node->value.value.integer);
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

		//indented_print_to_builder(file_string_builder, "tmp_register = \n");
		//print_to_builder(file_string_builder, "");
		//		//indented_print_to_builder(file_string_builder, "%v", basic_type_node->token.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_IDENTIFIER) {
		AST_Identifier* identifier_node = (AST_Identifier*)node;
		AST_Node* resolved_node = resolve_type(node);

		// parse_ast is suceptible to generate type declaration of resolved type.
		// Instead we want to generate a type reference, this is really different for struct, enum and unions
		// for which write_type will use there identifiers.
		//write_type(ir, resolved_node);

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// @TODO
		// Ici on atteint la limite de la fonction parse_ast qui est trop générique et tentante de faire une traduction
		// 1:1 du f vers le C++.
		// On a besoin du contexte, pour la déclaration d'un alias un type et attendu et doit être résolut avec symbol_solver,
		// mais si on est au milieu d'instructions on cherche certainement une variable.
		// Ceci à un impacte sur les symbol tables à tester et aussi sur les messages d'erreurs (comprendre l'analyse du code f).

		////print_to_builder(file_string_builder,
		//	"%Cv"
		//	"\n%v", magic_enum::enum_name(node->ast_type), identifier_node->value.text);
	}
	//else if (node->ast_type == Node_Type::FUNCTION_CALL) {
	//	AST_Function_Call* function_call_node = (AST_Function_Call*)node;

	//	//print_to_builder(file_string_builder,
	//		"%Cv"
	//		"\nname: %v (nb_arguments: %d)", magic_enum::enum_name(node->ast_type), function_call_node->name.text, function_call_node->nb_arguments);
	//}
	else if (node->ast_type == Node_Type::UNARY_OPERATOR_ADDRESS_OF) {
		AST_Unary_operator* address_of_node = (AST_Unary_operator*)node;

		parse_ast(parsing_result, ir, address_of_node->right); // @TODO should be write_type?
		//print_to_builder(file_string_builder, "*");
	}
	else if (node->ast_type == Node_Type::UNARY_OPERATOR_NEGATIVE) {
		AST_Unary_operator* negative_operator = (AST_Unary_operator*)node;

		report_error(Compiler_Error::warning, "UNARY_OPERATOR_NEGATIVE: TODO implement it\n");
		// @TODO I should try to evaluate the expression at compile time else generate the runtime code
		// Depending of kind of compile time expression I may get a "literal" value
		// Number literal can be inlined in the code as immediate value
		// String literals and other complexe literals (arrays, structs,...) should go in .rdata section
	}
	//else if (node->ast_type == Node_Type::BINARY_OPERATOR_MEMBER_ACCESS) {
	//	AST_MEMBER_ACCESS* member_access_node = (AST_MEMBER_ACCESS*)node;

	//	//print_to_builder(file_string_builder,
	//		"%Cv", magic_enum::enum_name(node->ast_type));
	//}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_STRUCT) {
		// @TODO
		// We do nothing here, because declaration of structs must be ordered in C unlike in f.
		// Putting the declaration here may not generate a valid C code if one of his dependencies isn't declared before.
		//
		// I need to find a better way to do the C code generation to be able to reorder declarations (types and functions).

		AST_Statement_Struct_Type* struct_node = (AST_Statement_Struct_Type*)node;

		if (struct_node->anonymous) {
			//indented_print_to_builder(file_string_builder, "struct\n");
		}
		else {
			//indented_print_to_builder(file_string_builder, "struct %v\n", struct_node->name.text);
		}
		//indented_print_to_builder(file_string_builder, "{\n");
		parse_ast(parsing_result, ir, struct_node->first_child);

		if (struct_node->anonymous) {
			//indented_print_to_builder(file_string_builder, "} "); // @Warning if struct is anonymous then the declaration is made at the same type as a variable declaration.
		}
		else {
			//indented_print_to_builder(file_string_builder, "};\n");
		}
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_UNION) {
		// @TODO
		// We do nothing here, because declaration of structs must be ordered in C unlike in f.
		// Putting the declaration here may not generate a valid C code if one of his dependencies isn't declared before.
		//
		// I need to find a better way to do the C code generation to be able to reorder declarations (types and functions).

		AST_Statement_Union_Type* union_node = (AST_Statement_Union_Type*)node;

		if (union_node->anonymous) {
			//indented_print_to_builder(file_string_builder, "union\n", union_node->name.text);
		}
		else {
			//indented_print_to_builder(file_string_builder, "union %v\n", union_node->name.text);
		}
		//indented_print_to_builder(file_string_builder, "{\n");
		//globals.cpp_backend_data.union_declaration_depth++;
		parse_ast(parsing_result, ir, union_node->first_child);
		//globals.cpp_backend_data.union_declaration_depth--;
		if (union_node->anonymous) {
			//indented_print_to_builder(file_string_builder, "} "); // @Warning if union is anonymous then the declaration is made at the same type as a variable declaration.
		}
		else {
			//indented_print_to_builder(file_string_builder, "};\n");
		}
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_ENUM) {
		// @TODO
		// We do nothing here, because declaration of structs must be ordered in C unlike in f.
		// Putting the declaration here may not generate a valid C code if one of his dependencies isn't declared before.
		//
		// I need to find a better way to do the C code generation to be able to reorder declarations (types and functions).

		core::Assert(false); // Fix enum parsing before, actually they don't have a name!!!

		//AST_Statement_Enum_Type* enum_node = (AST_Statement_Enum_Type*)node;

		////indented_print_to_builder(file_string_builder, "enum %v\n", enum_node->name.text);
		////indented_print_to_builder(file_string_builder, "{\n");
		//parse_ast(parsing_result, ir, enum_node->first_child);
		////indented_print_to_builder(file_string_builder, "};\n");
	}
	else if (node->ast_type == Node_Type::FUNCTION_CALL) {
		//write_function_call(ir, (AST_Function_Call*)node);

		//print_to_builder(file_string_builder, ";\n");
	}
	else {
		core::Assert(false);
		// @TODO report error here?
	}

	// Sibling iteration
	if (recurse_sibling && node->sibling) {
		parse_ast(parsing_result, ir, node->sibling);
	}
}

static void parse_function_declaration(IR& ir, AST_Statement_Function* function_node)
{
	fstd::language::string_view	win32_string;
	fstd::language::assign(win32_string, (uint8_t*)"win32");
	fstd::language::string_view	dll_import_string;
	fstd::language::assign(dll_import_string, (uint8_t*)"dll_import");
	bool win32_system_call = false;
	bool is_a_dll_import = false;
	Token<Keyword>* dll_token = nullptr;

	// win32 means:
	//   * __stdcall calling convention
	//   * it's a C function (should not have overloads)
	//
	// dll_import means:
	//   * function can't have implementation: the implementation is in the dll!!!

	// modifiers analysis
	for (AST_Function_Modifier* current_modifier = function_node->modifiers;
		current_modifier != nullptr; current_modifier = (AST_Function_Modifier*)current_modifier->sibling)
	{
		if (fstd::language::are_equals(current_modifier->value.text, win32_string)) {
			if (win32_system_call)
				report_error(Compiler_Error::error, current_modifier->value, "win32 modifier was already specified for the current function declaration.");
			win32_system_call = true;
		}
		else if (fstd::language::are_equals(current_modifier->value.text, dll_import_string)) {
			if (is_a_dll_import)
				report_error(Compiler_Error::error, current_modifier->value, "dll_import modifier can be used only once per function declaration.");
			is_a_dll_import = true;

			if (function_node->scope) {
				report_error(Compiler_Error::error, function_node->name, "Functions with dll_import modifier can't have implementation.");
			}

			if (get_list_size((AST_Node*)current_modifier->arguments) != 1) {
				report_error(Compiler_Error::error, current_modifier->value, "dll_import is taking a dll name as unique parameter.");
			}

			dll_token = &current_modifier->arguments->value;
		}
		else {
			report_error(Compiler_Error::error, current_modifier->value, "Unknown function modifier.");
		}
	}

	if (is_a_dll_import)
	{
		Imported_Library** found_imported_lib;

		uint64_t lib_hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(dll_token->text), fstd::language::get_string_size(dll_token->text), 0);
		uint16_t lib_short_hash = lib_hash & 0xffff;

		found_imported_lib = fstd::memory::hash_table_get(ir.imported_libraries, lib_short_hash, dll_token->text);
		if (found_imported_lib == nullptr) {
			Imported_Library* new_imported_lib = allocate_imported_library();

			new_imported_lib->name = dll_token->text;
			fstd::memory::hash_table_init(new_imported_lib->functions, &fstd::language::are_equals);

			found_imported_lib = fstd::memory::hash_table_insert(ir.imported_libraries, lib_short_hash, dll_token->text, new_imported_lib);
		}

		Imported_Function** found_imported_func;

		uint64_t func_hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(function_node->name.text), fstd::language::get_string_size(function_node->name.text), 0);
		uint16_t func_short_hash = func_hash & 0xffff;

		found_imported_func = fstd::memory::hash_table_get((*found_imported_lib)->functions, func_short_hash, dll_token->text);

		if (found_imported_func) {
			if (win32_system_call) {
				report_error(Compiler_Error::error, function_node->name, "win32 means that function is implemented in C, so overloading isn't supported. Please check you haven't already declared.");
			}
			else {
				report_error(Compiler_Error::error, function_node->name, "For the moment it is not allowed to import non win32 functions.");
			}
		}

		Imported_Function* new_imported_func = allocate_imported_function();

		new_imported_func->function = function_node;
		new_imported_func->name_RVA = 0;

		fstd::memory::hash_table_insert((*found_imported_lib)->functions, func_short_hash, dll_token->text, new_imported_func);
	}
	else
	{
		// @TODO
		//
		// f-lang code
	}

	// @TODO do we need to count arguments,... to ease function call code generation?
	// We certainly should use a new struct to store IR specific members
	// struct IR_Function_Decl
	// {
	//     AST_Statement_Function* ast_node;
	//     uint32_t                IAT_addr;
	//     ...
	// }
}

void f::generate_ir(Parsing_Result& parsing_result, IR& ir)
{
	ZoneScopedN("f::generate_ir");

	// Initialize data
	{
		memory::hash_table_init(ir.imported_libraries, &language::are_equals);
		memory::init(ir.code_data.code);
		memory::reserve_array(ir.code_data.code, 4096); // @TODO I really should do something cleaver

		memory::init(globals.ir_data.imported_libraries);
		memory::reserve_array(globals.ir_data.imported_libraries, NB_PREALLOCATED_IMPORTED_LIBRARIES);

		memory::init(globals.ir_data.imported_functions);
		memory::reserve_array(globals.ir_data.imported_functions, NB_PREALLOCATED_IMPORTED_LIBRARIES * NB_PREALLOCATED_IMPORTED_FUNCTIONS_PER_LIBRARY);
	}

	ir.parsing_result = &parsing_result;

	parse_ast(parsing_result, ir, parsing_result.ast_root);
}
