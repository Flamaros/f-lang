#include "c_generator.hpp"

#include <fstream>

#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "third-party/microsoft_craziness.h"

#include <utilities/file.hpp>
#include <utilities/string.hpp>
#include <utilities/exit_scope.hpp>

#include <iostream>

static std::string   common_cl_options = "/W4";
static std::string   debug_cl_options = "/Od";
static std::string   release_cl_options = "/O2 /Oi /GL";

// @TODO give the possibility to build in debug/release and x86/x64 architecture

// @Warning https://docs.microsoft.com/en-us/cpp/build/reference/entry-entry-point-symbol?view=vs-2019
// We remove completely the c runtime
//
// /SUBSYSTEM:CONSOLE
static std::string	common_linker_options = "/NODEFAULTLIB /ENTRY:main";

static bool	compile_c_file(const std::filesystem::path& input_filepath, const std::filesystem::path& output_directory_path, const std::filesystem::path& output_file_name)
{
	Find_Result find_visual_studio_result = find_visual_studio_and_windows_sdk();

	defer
	{
		free_resources(&find_visual_studio_result);
	};

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

	utilities::replace(windows_sdk_ucrt_library_path, "\\um", "\\ucrt");

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
		+ "\"/Fo" + output_directory_path.generic_string() + "\"\\ "	// @Warning this option redirect the obj output directory
		+ input_filepath.generic_string() + " "
		+ "/link "
		+ common_linker_options + " "
		+ "/out:\"" + (output_directory_path / output_file_name).generic_string() + "\" "
//		+ "/LIBPATH:\"" + windows_sdk_ucrt_library_path + "\" "
		+ "/LIBPATH:\"" + windows_sdk_um_library_path + "\" "
//		+ "/LIBPATH:\"" + vs_library_path + "\" "
		+ "kernel32.lib"	// @Warning we need the kernel32.dll to be able to call win32 functions
		;

	// Running process (This method works with spaces in includes directories)
	// We create a file in which we redirect the cl.exe StdOutput
	{
		std::filesystem::path	outputFilePath = output_directory_path / "cl.log";
		std::filesystem::path	errorOutputFilePath = output_directory_path / "cl.error";
		char*					args = _strdup(command.c_str());
		STARTUPINFO				si;
		PROCESS_INFORMATION		pi;
		SECURITY_ATTRIBUTES		sa;
		DWORD					clReturnCode;

		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;

		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));
		si.cb = sizeof(si);
		si.dwFlags |= STARTF_USESTDHANDLES;
		si.hStdOutput = CreateFileA(outputFilePath.generic_string().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		si.hStdError = CreateFileA(errorOutputFilePath.generic_string().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		defer
		{
			CloseHandle(si.hStdOutput);
			CloseHandle(si.hStdError);
			free(args);
		};

		if (!CreateProcessA(cl_file_path.c_str(), args, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
		{
			std::cerr << "Execution of command:\n"
				<< command << " error: " << GetLastError() << "\n"
				<< " take a look to: " << errorOutputFilePath << std::endl;

			return false;
		}
		else
		{
			defer
			{
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			};

			WaitForSingleObject(pi.hProcess, INFINITE);

			if (GetExitCodeProcess(pi.hProcess, &clReturnCode) == 0
				|| clReturnCode != 0)
			{
				std::cerr << "Execution of command:\n"
					<< command << " exit value: " << clReturnCode << "\n"
					<< " take a look to: " << errorOutputFilePath << std::endl;

				return false;
			}
		}
	}

	return true;
}

bool c_generator::generate(const std::filesystem::path& output_directory_path, const std::filesystem::path& output_file_name, const f::AST& parsing_result)
{
    std::filesystem::create_directory(output_directory_path);

	std::filesystem::path	generated_file_path = output_directory_path / "generated.c";
    std::ofstream			file(generated_file_path, std::fstream::binary);
    std::streampos			file_size;

    if (file.is_open() == false) {
        return false;
    }

    file << "#include <Windows.h>\n"
            "\n"
            "size_t strlen(const char* str) {\n"
            "   size_t i = 0;\n"
            "\n"
            "   for (; str[i] != 0; i++) {\n"
            "   }\n"
            "\n"
            "   return i;\n"
            "}\n"
            "\n"
            "int main(int ac, char** argv)\n"
            "{\n"
            "   char* message = \"Hello World\";\n"
            "   DWORD written_bytes = 0;\n"
            "   void* hstdOut = GetStdHandle(STD_OUTPUT_HANDLE);\n"
            "\n"
            "   WriteFile(hstdOut, message, (DWORD)strlen(message), &written_bytes, 0);\n"
            "\n"
            "   // ExitProcess(0);\n"
            "   return 0;\n"
            "}\n";

	file.close();

	return compile_c_file(generated_file_path, output_directory_path, output_file_name);
}
