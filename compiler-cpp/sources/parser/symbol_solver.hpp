#pragma once

// Methods of this file are suceptible to report compilition errors

namespace f
{
	struct Token;

	struct AST_Node;
	struct AST_Identifier;
	struct AST_User_Type_Identifier;

	AST_Node* get_user_type(AST_User_Type_Identifier* user_type);
}
