#include "IR_generator.hpp"

#include "globals.hpp"

#include "parser/parser.hpp"
#include "parser/symbol_solver.hpp"

#include <tracy/Tracy.hpp>

using namespace fstd;
using namespace fstd::core;

using namespace f;

// @TODO actually Parsing_Result isn't really used by parse_ast because the AST_Node can already contains a
// pointer to the right symbol table node.
// But it might be useful to check shadowing and some other errors.


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

		//write_function_declaration(file_string_builder, ir, function_node);

		if (function_node->scope) {
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
				if (globals.cpp_backend_data.union_declaration_depth == 0) {
					//write_default_initialization(file_string_builder, variable_node, variable_node->type);
				}
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

		if (node != ir.parsing_result->ast_root)
			//indented_print_to_builder(file_string_builder, "{\n");
		parse_ast(parsing_result, ir, (AST_Node*)scope_node->first_child);
		if (node != ir.parsing_result->ast_root) {
			//indented_print_to_builder(file_string_builder, "}\n");
		}
	}
	else if (node->ast_type == Node_Type::STATEMENT_LITERAL) {
		AST_Literal* literal_node = (AST_Literal*)node;

		if (literal_node->value.type == Token_Type::STRING_LITERAL) {
			//print_to_builder(file_string_builder, literal_node->value.text);
			// @TODO use the *literal_node->value.value.string instead of the token's text
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
		// Ici on atteint la limite de la fonction parse_ast qui est trop g�n�rique et tentante de faire une traduction
		// 1:1 du f vers le C++.
		// On a besoin du contexte, pour la d�claration d'un alias un type et attendu et doit �tre r�solut avec symbol_solver,
		// mais si on est au milieu d'instructions on cherche certainement une variable.
		// Ceci � un impacte sur les symbol tables � tester et aussi sur les messages d'erreurs (comprendre l'analyse du code f).

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
		parse_ast(parsing_result, ir, struct_node->first_child + 1);

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
		globals.cpp_backend_data.union_declaration_depth++;
		parse_ast(parsing_result, ir, union_node->first_child + 1);
		globals.cpp_backend_data.union_declaration_depth--;
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
		//parse_ast(parsing_result, ir, enum_node->first_child + 1);
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

void f::generate_ir(Parsing_Result& parsing_result, IR& ir)
{
	ZoneScopedN("f::generate_ir");

	ir.parsing_result = &parsing_result;

	parse_ast(parsing_result, ir, parsing_result.ast_root);
}
