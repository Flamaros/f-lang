#include "native_generator.hpp"

#include "f_tokenizer.hpp"
#include "f_parser.hpp"

#include "utilities.hpp"

#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "third-party/microsoft_craziness.h"

int main(int ac, char** av)
{
    std::string             input_file_content;
    std::vector<f::Token>   tokens;
    f::Parsing_Result       parsing_result;

    if (read_all_file("./compiler-f/main.f", input_file_content) == false) {
        return 1;
    }

    f::tokenize(input_file_content, tokens);
    f::parse(tokens, parsing_result);

	Find_Result find_visual_studio_result = find_visual_studio_and_windows_sdk();

	free_resources(&find_visual_studio_result);

//    generate_hello_world();
	return 0;
}
