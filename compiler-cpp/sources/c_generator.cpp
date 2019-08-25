#include "c_generator.hpp"

#include <fstream>

bool c_generator::generate(const std::filesystem::path& output_filepath, const f::Parsing_Result& parsing_result)
{
    std::filesystem::create_directory(output_filepath.parent_path());

    std::ofstream   file(output_filepath, std::fstream::binary);
    std::streampos  file_size;

    if (file.is_open() == false) {
        return false;
    }

    file << "#include <Windows.h>\n"
            "\n"
            "int main(int ac, char** argv)\n"
            "{\n"
            "   char* message = \"Hello World\"\n"
            "   DWORD written_bytes = 0;\n"
            "   void* hstdOut = GetstdHandle(STD_OUTPUT_HANDLE);\n"
            "\n"
            "   WriteFile(hstdOut, message.c_string, message.length, &bytes, 0);\n"
            "\n"
            "   // ExitProcess(0);\n"
            "   return 0;\n"
            "}\n";

    return true;
}
