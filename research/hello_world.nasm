; How to build (you'll need nasm compiler and Visual Studio)

; Be sure that nasm compiler path is in your PATH env
; nasm -fwin32 hello_world.nasm
; 
; From a native Visual Studio Tools Command prompt
; link /subsystem:console /nodefaultlib /entry:main hello_world.obj kernel32.lib user32.dll


   global _main
    extern  _GetStdHandle@4
    extern  _WriteFile@20
    extern  _ExitProcess@4

    section .text

_main:
    ; DWORD  bytes;    
    mov     ebp, esp
    sub     esp, 4

    ; hStdOut = GetstdHandle( STD_OUTPUT_HANDLE)
    push    -11
    call    _GetStdHandle@4
    mov     ebx, eax    

    ; WriteFile( hstdOut, message, length(message), &bytes, 0);
    push    0
    lea     eax, [ebp-4]
    push    eax
    push    (message_end - message)
    push    message
    push    ebx
    call    _WriteFile@20

    ; ExitProcess(0)
    push    0
    call    _ExitProcess@4

    ; never here
    hlt

message:
    db      'Hello World', 9

message_end:
