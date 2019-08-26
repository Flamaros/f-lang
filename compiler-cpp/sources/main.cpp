#include "native_generator.hpp"

#include "f_tokenizer.hpp"
#include "f_parser.hpp"
#include "c_generator.hpp"

#include "utilities.hpp"

#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "third-party/microsoft_craziness.h"

#include <iostream>

#include <cstdlib>

static std::string   common_cl_options = "/W4";
static std::string   debug_cl_options = "/Od";
static std::string   release_cl_options = "/O2 /Oi /GL";

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

    std::string output_file_path = "./build/main.c";

    if (c_generator::generate(output_file_path, parsing_result)) {
        Find_Result find_visual_studio_result = find_visual_studio_and_windows_sdk();

        std::wstring w_vs_exe_path(find_visual_studio_result.vs_exe_path);
        std::string  vs_exe_path(w_vs_exe_path.begin(), w_vs_exe_path.end());

        std::wstring w_vs_library_path(find_visual_studio_result.vs_library_path);
        std::string  vs_library_path(w_vs_exe_path.begin(), w_vs_exe_path.end());

        std::string command = '"' + vs_exe_path
                              + "/cl.exe\" "
                              + common_cl_options + " "
                              + debug_cl_options + " "
                              + "\"/I" + vs_library_path + "\" "
                              + output_file_path;

        std::cout << command << std::endl;
        // @TODO replace this crappy thing by a ExecuteProcess to have something that can
        // parse the arguments correctly
        std::system(command.c_str());

        free_resources(&find_visual_studio_result);
    }

//    generate_hello_world();

    std::system("PAUSE");
	return 0;
}
