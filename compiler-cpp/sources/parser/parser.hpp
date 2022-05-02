#pragma once

#include "../lexer/lexer.hpp"

#include <fstd/memory/array.hpp>
#include <fstd/memory/hash_table.hpp>

#include <fstd/system/path.hpp>

namespace f
{
	// Forward declarations
	struct AST_Node;
	struct AST_Enum;
	struct AST_Alias;
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
		STATEMENT_VARIABLE_TYPE,
		STATEMENT_TYPE_STRUCT,
		STATEMENT_TYPE_UNION,
		STATEMENT_TYPE_ENUM,
		STATEMENT_TYPE_POINTER,
		STATEMENT_TYPE_ARRAY,
		STATEMENT_VARIABLE,
		STATEMENT_FUNCTION,
		STATEMENT_SCOPE,
		STATEMENT_LITERAL,
		STATEMENT_IDENTIFIER,

		ASSIGNMENT,
		FUNCTION_CALL,

		// Unary operators
		UNARY_OPERATOR_ADDRESS_OF,

		// Binary operators
		BINARY_OPERATOR_ADDITION,
		BINARY_OPERATOR_SUBSTRACTION,
		BINARY_OPERATOR_MULTIPLICATION,
		BINARY_OPERATOR_DIVISION,
		BINARY_OPERATOR_REMINDER,
		BINARY_OPERATOR_MEMBER_ACCESS,
	};

	enum class Binary_Operator_Associativity
	{
		LEFT_TO_RIGHT,
		RIGHT_TO_LEFT
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

	struct AST_Binary_Operator
	{
		Node_Type	ast_type;
		AST_Node*	sibling;

		Token		token;
		AST_Node*	left;
		AST_Node*	right;
	};

	struct AST_Alias
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		Token		name; // Should be a pointer to avoid the copy?
		AST_Node*	type; // Is an expression that have to be evaluable at compile-time and return a Type (basic or struct or enum, Type, function,...)
	};

	struct AST_Enum_Value
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		Token		value_name;
		AST_Node*	value;
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
		Token		token;
	};

	struct AST_Statement_Struct_Type
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		bool		anonymous;
		Token		name; // @Warning is uninitialized if anonymous is true
		AST_Node*	first_child;
	};

	struct AST_Statement_Union_Type
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		bool		anonymous;
		Token		name; // @Warning is uninitialized if anonymous is true
		AST_Node*	first_child;
	};

	struct AST_Statement_Enum_Type
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
	};

	struct AST_Statement_Variable_Type
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
		Node_Type	ast_type;
		AST_Node*	sibling;
		AST_Node*	array_size; // if null the array is dynamic
		// @TODO if the size is a constexpr we certainly want to have the value
		// then it is also a fixed size.
		// if array_size not null, can be a fixed size array or a dynamic one, only the attempt of the expression evaluation can tell it.
	};

	struct AST_Statement_Variable
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		Token		name;
		AST_Node*	type;
		bool		is_function_paramter;
		bool		is_optional;
		AST_Node*	expression; // not null if is_optional is true
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

	struct AST_Function_Call
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		Token		name;
		int			nb_arguments;
		AST_Node*	parameters;
	};

	struct AST_Statement_Scope
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		AST_Node*	first_child;
	};

	struct AST_Literal
	{
		// There is no type inference to do on it
		// The token is a string or a numeric literal
		Node_Type	ast_type;
		AST_Node*	sibling;
		Token		value;
	};

	struct AST_Identifier
	{
		// Pretty similar to AST_Literal, should be merged?
		Node_Type	ast_type;
		AST_Node*	sibling;
		Token		value;
	};

	struct AST_Unary_operator
	{
		Node_Type	ast_type;
		AST_Node*	sibling;
		AST_Node*	right;
	};

	//=============================================================================

	enum class Scope_Type
	{
		MODULE,
		FUNCTION,
		STRUCT,
	};

	struct Scope
	{
		fstd::memory::Hash_Table<fstd::language::string, uint16_t, AST_Node*, 32> variables;
		fstd::memory::Hash_Table<fstd::language::string, uint16_t, AST_Node*, 32> user_types;
		fstd::memory::Hash_Table<fstd::language::string, uint16_t, AST_Node*, 32> functions; // Not really sure that it is different than a variable

		// @TODO Can we put function declarations in a struct?

		Scope_Type	type;

		Scope* parent;
		Scope* sibling;
		Scope* first_child;
	};

	//=============================================================================
	
	struct Parsing_Result
	{
		AST_Node*	ast_root; // Should point on the first module
		Scope*		scope_root;
	};

	struct Parser_Data
	{
		// This is a raw buffer as all nodes don't have the same type.
		// Actually allocate_AST_node doesn't let any padding between nodes.
		// It is possible to iterate over all nodes, but few computations are needed
		// to retrieve nodes correctly.
		//
		// It may be interesting to iterate over nodes like over an array for some algorithms
		// like searching functions,....
		//
		// We also may store the size of the AST_Node, as it might help with the CPU prediction.
		// It also remove the computation that depend of the type of the node.
		//
		// Flamaros - 07 january 2021
		fstd::memory::Array<uint8_t>	ast_nodes;

		// We do the same for scopes, as we certainly will have different types of scopes.
		fstd::memory::Array<uint8_t>	scope_nodes;

		Scope* current_scope;
	};

    void parse(fstd::memory::Array<Token>& tokens, Parsing_Result& ast);
	inline bool is_binary_operator(const AST_Node* node);
	inline bool is_unary_operator(const AST_Node* node);
	void generate_dot_file(const AST_Node* node, const fstd::system::Path& output_file_path);
	void generate_dot_file(const Scope* scope, const fstd::system::Path& output_file_path);

	inline bool is_binary_operator(const AST_Node* node)
	{
		if (node == nullptr)
			return false;

		return node->ast_type >= Node_Type::BINARY_OPERATOR_ADDITION
			&& node->ast_type <= Node_Type::BINARY_OPERATOR_MEMBER_ACCESS;
	}

	inline bool is_unary_operator(const AST_Node* node)
	{
		if (node == nullptr)
			return false;

		return node->ast_type >= Node_Type::UNARY_OPERATOR_ADDRESS_OF
			&& node->ast_type <= Node_Type::UNARY_OPERATOR_ADDRESS_OF;
	}

	inline bool does_left_operator_precedeed_right(Node_Type left, Node_Type right)
	{
		// For the moment I use exactly same values as C++
	    // https://en.cppreference.com/w/cpp/language/operator_precedence

		static int operator_priorities[] = {
			12, // BINARY_OPERATOR_ADDITION
			12, // BINARY_OPERATOR_SUBSTRACTION
			13, // BINARY_OPERATOR_MULTIPLICATION
			13, // BINARY_OPERATOR_DIVISION
			13, // BINARY_OPERATOR_REMINDER
			16, // BINARY_OPERATOR_MEMBER_ACCESS
		};

		static Binary_Operator_Associativity operator_associativities[] = {
			Binary_Operator_Associativity::LEFT_TO_RIGHT, // BINARY_OPERATOR_ADDITION
			Binary_Operator_Associativity::LEFT_TO_RIGHT, // BINARY_OPERATOR_SUBSTRACTION
			Binary_Operator_Associativity::LEFT_TO_RIGHT, // BINARY_OPERATOR_MULTIPLICATION
			Binary_Operator_Associativity::LEFT_TO_RIGHT, // BINARY_OPERATOR_DIVISION
			Binary_Operator_Associativity::LEFT_TO_RIGHT, // BINARY_OPERATOR_REMINDER
			Binary_Operator_Associativity::LEFT_TO_RIGHT, // BINARY_OPERATOR_MEMBER_ACCESS
		};

		int priority_distance = operator_priorities[(size_t)left - (size_t)Node_Type::BINARY_OPERATOR_ADDITION] - operator_priorities[(size_t)right - (size_t)Node_Type::BINARY_OPERATOR_ADDITION];

		// If the priority of operators are the same then the associativity will resolve the ambiguity
		if (priority_distance == 0)
			return operator_associativities[(size_t)left - (size_t)Node_Type::BINARY_OPERATOR_ADDITION] == Binary_Operator_Associativity::LEFT_TO_RIGHT;

		return priority_distance > 0;
	}
}
