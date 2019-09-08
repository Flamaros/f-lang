#include "native_generator.hpp"

#include "f_tokenizer.hpp"
#include "f_parser.hpp"
#include "c_generator.hpp"

#include <utilities/file.hpp>
#include <utilities/string.hpp>

#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "third-party/microsoft_craziness.h"

#include <iostream>

#include <cstdlib>

static std::string   common_cl_options = "/W4";
static std::string   debug_cl_options = "/Od";
static std::string   release_cl_options = "/O2 /Oi /GL";

static std::string	common_linker_options = "/SUBSYSTEM:CONSOLE";

int main(int ac, char** av)
{
    std::string             input_file_content;
    std::vector<f::Token>   tokens;
    f::Parsing_Result       parsing_result;
	int						result = 0;

    if (utilities::read_all_file("./compiler-f/main.f", input_file_content) == false) {
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
        std::string  vs_library_path(w_vs_library_path.begin(), w_vs_library_path.end());
		std::string	 vs_include_basse_path = vs_library_path;

		utilities::replace(vs_include_basse_path, "lib\\x64", "include");

		std::wstring w_windows_sdk_um_library_path(find_visual_studio_result.windows_sdk_um_library_path);
		std::string  windows_sdk_um_library_path(w_windows_sdk_um_library_path.begin(), w_windows_sdk_um_library_path.end());
		std::string  windows_sdk_ucrt_library_path = windows_sdk_um_library_path;
		std::string  vs_windows_sdk_include_base_path(w_windows_sdk_um_library_path.begin(), w_windows_sdk_um_library_path.end());

		utilities::replace(windows_sdk_um_library_path, "\\um", "\\ucrt");

		utilities::replace(vs_windows_sdk_include_base_path, "Lib", "Include");
		utilities::replace(vs_windows_sdk_include_base_path, "\\um\\x64", "");

		std::string	cl_file_path = vs_exe_path + "/cl.exe";

        std::string command = '"' + cl_file_path + "\" "
                              + common_cl_options + " "
                              + debug_cl_options + " "
                              + "\"/I" + vs_windows_sdk_include_base_path + "\\shared\" "
	                          + "\"/I" + vs_windows_sdk_include_base_path + "\\ucrt\" "
						      + "\"/I" + vs_windows_sdk_include_base_path + "\\um\" "
			                  + "\"/I" + vs_include_basse_path + "\" "
			                  + output_file_path + " "
			                  + "/link "
			                  + common_linker_options + " "
			                  + "/LIBPATH:\"" + windows_sdk_ucrt_library_path + "\" "
			                  + "/LIBPATH:\"" + windows_sdk_um_library_path + "\" "
			                  + "/LIBPATH:\"" + vs_library_path + "\" "
			                  ;

		// Running process (This method works with spaces in includes directories)
		// We create a file in which we redirect the cl.exe StdOutput
		{
			std::string         outputFilePath = "cl.log";
			std::string         errorOutputFilePath = "cl.error";
			char*               args = _strdup(command.c_str());
			STARTUPINFO         si;
			PROCESS_INFORMATION pi;
			SECURITY_ATTRIBUTES sa;
			DWORD               clReturnCode;

			sa.nLength = sizeof(sa);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = TRUE;

			ZeroMemory(&si, sizeof(si));
			ZeroMemory(&pi, sizeof(pi));
			si.cb = sizeof(si);
			si.dwFlags |= STARTF_USESTDHANDLES;
			si.hStdOutput = CreateFileA(outputFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			si.hStdError = CreateFileA(errorOutputFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (!CreateProcessA(cl_file_path.c_str(), args, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
			{
				std::cerr << "Execution of command:\n"
					<< command << " error: " << GetLastError() << "\n"
					<< " take a look to: " << errorOutputFilePath << std::endl;

				result = 1;
			}
			else
			{
				WaitForSingleObject(pi.hProcess, INFINITE);

				if (GetExitCodeProcess(pi.hProcess, &clReturnCode) == 0
					|| clReturnCode != 0)
				{
					std::cerr << "Execution of command:\n"
						<< command << " exit value: " << clReturnCode << "\n"
						<< " take a look to: " << errorOutputFilePath << std::endl;

					result = 1;
				}

				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}

			CloseHandle(si.hStdOutput);
			CloseHandle(si.hStdError);
			free(args);
		}

        free_resources(&find_visual_studio_result);
    }

//    generate_hello_world();

//    std::system("PAUSE");
	return result;
}
