#pragma once

// Methods of this file are suceptible to report compilition errors

namespace f
{
	struct Token;

	struct AST_Node;
	struct AST_Identifier;
	struct AST_User_Type_Identifier;

	AST_Node* get_user_type(AST_User_Type_Identifier* user_type);
	AST_Node* get_user_type(AST_Identifier* user_type);
	AST_Node* resolve_type(AST_Node* user_type); // Return the underlying type if user_type is an alias (can be called with any AST_Node type)
}
