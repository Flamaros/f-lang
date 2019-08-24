Project
{
    name: "f-lang"

    CppApplication
    {
        name: "f-lang"

        files: [
            "sources/f-lang.cpp",
            "sources/f-lang.hpp",
            "sources/hash_table.hpp",
            "sources/macro_language_definitions.hpp",
            "sources/macro_parser.cpp",
            "sources/macro_parser.hpp",
            "sources/macro_tokenizer.cpp",
            "sources/macro_tokenizer.hpp",
            "sources/main.cpp",
            "sources/native_generator.cpp",
            "sources/native_generator.hpp",
            "sources/utilities.cpp",
            "sources/utilities.hpp",
        ]

        cpp.cxxLanguageVersion: ["c++17"]
    }
}
