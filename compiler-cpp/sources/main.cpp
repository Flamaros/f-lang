#include "native_generator.hpp"

#include "lexer.hpp"
#include "ast.hpp"
#include "c_generator.hpp"

#include <utilities/file.hpp>
#include <utilities/string.hpp>

#include <iostream>

#include <cstdlib>

int main(int ac, char** av)
{
    std::string             input_file_content;
    std::vector<f::Token>   tokens;
    f::AST       parsing_result;
	int						result = 0;

    if (utilities::read_all_file("./compiler-f/main.f", input_file_content) == false) {
        return 1;
    }

    f::tokenize(input_file_content, tokens);
    f::build_ast(tokens, parsing_result);

	std::filesystem::path output_directory_path = "./build";
	std::filesystem::path output_file_name = "f-compiler.exe";

    std::filesystem::create_directory(output_directory_path);   // @Warning hacky thing, the canonical may failed if the directory doesn't exist
	if (output_directory_path.is_relative()) {
		output_directory_path = std::filesystem::canonical(output_directory_path);
	}

    if (c_generator::generate(output_directory_path, output_file_name, parsing_result)) {
		return 0;
    }

//    generate_hello_world();

//    std::system("PAUSE");
	return 1;
}
