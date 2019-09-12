Project
{
    name: "f-lang"

    CppApplication
    {
        name: "f-lang"

        consoleApplication: true

        files: [
            "sources/third-party/microsoft_craziness.h",

            "sources/utilities/exit_scope.hpp",
            "sources/utilities/file.cpp",
            "sources/utilities/file.hpp",
            "sources/utilities/flags.hpp",
            "sources/utilities/logger.cpp",
            "sources/utilities/logger.hpp",
            "sources/utilities/string.cpp",
            "sources/utilities/string.hpp",
            "sources/utilities/timer.hpp",
            "sources/utilities/vector.hpp",

            "sources/fight-std/string.hpp",
            "sources/fight-std/string_ref.hpp",

            "sources/ast.cpp",
            "sources/ast.hpp",
            "sources/c_generator.cpp",
            "sources/c_generator.hpp",
            "sources/globals.cpp",
            "sources/globals.hpp",
            "sources/hash_table.hpp",
            "sources/lexer.cpp",
            "sources/lexer.hpp",
            "sources/macros.hpp",
            "sources/main.cpp",
            "sources/native_generator.cpp",
            "sources/native_generator.hpp",
            "sources/platform.hpp",

            // compiler-f (Will not be compile, but appears in the project files list)
            "../compiler-f/f_language_definitions.f",
            "../compiler-f/main.f",
        ]

        cpp.windowsApiCharacterSet: "mbcs"

        cpp.cxxLanguageVersion: ["c++17"]

        cpp.dynamicLibraries: [
            "Advapi32",
            "Ole32",
            "OleAut32",
        ]

        cpp.includePaths: [
            "sources",
        ]
    }
}
