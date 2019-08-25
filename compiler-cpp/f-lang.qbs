Project
{
    name: "f-lang"

    CppApplication
    {
        name: "f-lang"

        files: [
            "sources/third-party/microsoft_craziness.h",

            "sources/f_language_definitions.hpp",
            "sources/f_parser.cpp",
            "sources/f_parser.hpp",
            "sources/f_tokenizer.cpp",
            "sources/f_tokenizer.hpp",
            "sources/hash_table.hpp",
            "sources/main.cpp",
            "sources/native_generator.cpp",
            "sources/native_generator.hpp",
            "sources/utilities.cpp",
            "sources/utilities.hpp",

            // compiler-f (Will not be compile, but appears in the project files list)
            "../compiler-f/f_language_definitions.f",
            "../compiler-f/main.f",
        ]

        cpp.cxxLanguageVersion: ["c++17"]

        cpp.dynamicLibraries: [
            "Advapi32",
            "Ole32",
            "OleAut32",
        ]
    }
}
