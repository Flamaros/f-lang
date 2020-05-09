#pragma once

#include "../lexer/lexer.hpp"

#include <fstd/memory/array.hpp>

#include <fstd/system/path.hpp>

namespace f
{
	// Forward declarations
	struct AST_Node;
	struct AST_Enum;
	struct AST_Alias;
	struct AST_Expression;
	struct AST_Statement_Type;
	struct AST_Statement_Type_Pointer;
	struct AST_Statement_Type_Array;
	struct AST_Statement_Variable;
	struct AST_Statement_Function;
	struct AST_Statement_Scope;

	//=============================================================================

	enum class Expression_Type
	{
		NUMBER,
		IDENTIFIER,				/// Variable identifier
		BINARY_OPERATION,		/// Operation that takes 2 arguments (+ - * / %)

		MODULE,					/// A source file is an implicit module
		IMPORT,
		ASSIGNMENT,
		FUNCTION_CALL,
		FUNCTION_DEFINITION,
		CONDITION,
	};

	enum class Node_Type
	{
		TYPE_ALIAS,
		TYPE_ENUM,
		TYPE_STRUCT,

		STATEMENT_MODULE,
		STATEMENT_BASIC_TYPE, 
		STATEMENT_USER_TYPE,
		STATEMENT_TYPE_POINTER,
		STATEMENT_TYPE_ARRAY,
		STATEMENT_VARIABLE,
		STATEMENT_FUNCTION,
		STATEMENT_SCOPE,

		ASSIGNMENT,
		BINARY_OP,
		UNARY_OP,
		FUNCTION_CALL
	};

	// @Warning
	// Don't use the constructor or default initialisation of AST_Node* types
	// constructors (even the default) aren't called
	//
	// Flamaros - 13 april 2020

	// @TODO
	// We may want to use indices instead of pointers to reference AST nodes to be able to resize
	// the node buffer without having to update all pointers.

	struct AST_Node
	{
		// @TODO in f-lang we should be able to use C style inheritance
		// https://youtu.be/ZHqFrNyLlpA?t=1905
		// uncomment the following line:
		// using node : AST_Node;	// node is both named and anonymous

		// @TODO
		// In f-lang the AST_Node should not reserve some memory, but instead
		// we should be able to look at all types that encapsulate AST_Node at compile
		// time and determinate the size of the largest struct.

		Node_Type	ast_type;
		AST_Node*	sibling;
	};

	struct AST_Alias
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		Token		type_name; // Should be a pointer to avoid the copy?
		AST_Node*	type;
	};

	struct AST_Expression
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
	};

	struct AST_Enum_Value
	{
		Node_Type		ast_type;
		AST_Node*		sibling;
		Token			value_name;
		AST_Expression* value;
	};

	struct AST_Enum
	{
		Node_Type		ast_type;
		AST_Node*		sibling;
		Token			type_name; // Should be a pointer to avoid the copy?
		AST_Enum_Value* values;
	};

	struct AST_Statement_Module
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		AST_Node*	first_child;
	};

	struct AST_Statement_Basic_Type
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		Keyword		keyword;
	};

	struct AST_Statement_User_Type
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		Token		identifier;
	};

	struct AST_Statement_Type_Pointer
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
	};

	struct AST_Statement_Type_Array
	{
		Node_Type		ast_type;
		AST_Node*		sibling;
		AST_Expression* array_size; // if null the array is dynamic
		// @TODO if the size is a constexpr we certainly want to have the value
		// then it is also a fixed size.
		// if array_size not null, can be a fixed size array or a dynamic one, only the attempt of the expression evaluation can tell it.
	};

	struct AST_Statement_Variable
	{
		Node_Type				ast_type;
		AST_Node*				sibling;
		Token					name;
		AST_Node*				type;
		bool					is_function_paramter;
		bool					is_optional;
		AST_Expression*			expression; // not null if is_optional is true
	};

	struct AST_Statement_Function
	{
		Node_Type				ast_type;
		AST_Node*				sibling;
		Token					name;
		int						nb_arguments;
		AST_Statement_Variable*	arguments;
		AST_Node*				return_type;
		AST_Statement_Scope*	scope;	 // nullptr is it's only the declaration
	};

	struct AST_Statement_Scope
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		AST_Node*	first_child;
	};

    struct AST
	{
		AST_Node*	root; // Should point on the first module
	};

	struct Parser_Data
	{
		fstd::memory::Array<uint8_t>	ast_nodes; // This is a raw buffer as all nodes don't have the same type
	};

    void parse(fstd::memory::Array<Token>& tokens, AST& ast);
	void generate_dot_file(AST& ast, const fstd::system::Path& output_file_path);
}
