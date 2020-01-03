#!/bin/bash

cd "$(dirname "$0")"
./cloc-1.80.exe --list-file=cpp-compiler_files_list.txt --exclude-list-file=cpp-compiler_ignore_files_list.txt
