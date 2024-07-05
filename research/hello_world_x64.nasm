; Source from: https://stackoverflow.com/questions/77114093/hello-world-using-nasm-and-golink-on-x64-64-bit-windows 

; How to build (you'll need nasm compiler and Visual Studio)

; Be sure that nasm compiler path is in your PATH env
; nasm -fwin64 hello_world_x64.nasm
; 
; From a native Visual Studio Tools Command prompt
; link /subsystem:console /nodefaultlib /entry:main hello_world_x64.obj kernel32.lib

default rel

extern GetStdHandle
extern WriteFile
extern ExitProcess

section .text
global main
main:
    sub     rsp, 40          ; reserve shadow space and align stack by 16

    mov     rcx, -11         
    call    GetStdHandle

    mov     rcx, rax         ; HANDLE is a 64-byte type on x64
    lea     rdx, [msg]       ; lpBuffer = RIP-relative LEA (default rel)
    mov     r8d, msg.len     ; DWORD nNumberOfBytesToWrite = 32-bit immediate constant length
    lea     r9, [rsp+48]     ; lpNumberOfBytesWritten pointing into main's shadow space
    mov     qword [rsp + 32], 0   ; lpOverlapped = NULL; This is a pointer, needs to be qword.
    call    WriteFile        ; WriteFile(handle, msg, len, &our_shadow_space, NULL)

;;; BOOL return value in EAX (BOOL is a 4-byte type, unlike bool).
;;; NumberOfBytesWritten in dword [rsp+48]

    xor     ecx, ecx
    call ExitProcess
  ; add     rsp, 40      ; alternative to calling a noreturn function like ExitProcess
  ; ret


section .data         ; or section .rdata to put it in a read-only page
    msg:  db "Hello, World!", 13, 10    ; including a CR LF newline is a good idea for a line of output text
    .len equ  $-msg    ; let the assembler calculate the string length
                       ; .len is a local label that appends to the most recent non-dot label, so this is msg.len
