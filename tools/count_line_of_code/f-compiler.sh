#!/bin/bash

cd "$(dirname "$0")"
./cloc-1.80.exe --list-file=f-compiler_files_list.txt --exclude-list-file=f-compiler_ignore_files_list.txt
