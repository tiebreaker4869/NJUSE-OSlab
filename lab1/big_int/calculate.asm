%define STDIN 0
%define STDOUT 1
%define SYS_READ 3
%define SYS_WRITE 4
%define SYS_EXIT 1
%define MAX_LEN 128
%define ADD_OPERATOR 43
%define MUL_OPERATOR 42

section .data
    PromptMessage: db "Please enter expression (x+y or x*y, without blankspace in the middle): ", 0ah ; 提示用户消息
    PromptMessageEnd:

section .bss
    operand1: resb 42 
    operand2: resb 42
    expression: resb 100

section .text
    global _start
_start:
    mov ecx, PromptMessage ; 提示字符串地址
    mov edx, PromptMessageEnd - PromptMessage ; 提示字符串长度
    call DispStr

    call GetInput

    ; test GetInput
    ; mov ecx, expression
    ; mov edx, eax
    ; call DispStr
    ; test GetInput

    call GetOperator

    ; call GetOperands

    ; call ValidateInput

    ; call Big_Add

    ; call Big_Mul

    ; 退出程序
    mov ebx, 0							; 参数一：退出代码
    mov eax, SYS_EXIT							; 系统调用号(sys_exit)
    int 0x80

DispStr:
    mov ebx, STDOUT    					; 参数：文件描述符(stdout)
	mov eax, SYS_WRITE						; 系统调用号
	int 0x80						; 陷入内核
	ret

GetInput:
    mov eax, SYS_READ ; 系统调用号
    mov ebx, STDIN ; 参数： 文件描述符
    mov ecx, expression ; 存储输入字符串的地址
    mov edx, MAX_LEN ; 最大长度
    int 80h ; 陷入内核
    ret

GetOperator:
    mov ebx, expression
    mov ecx, 0
    begin_loop:
        mov edx, ecx
        add edx, expression
        cmp byte[edx], 0
        jz not_found
        cmp byte[edx], ADD_OPERATOR
        jz find_add
        cmp byte[edx], MUL_OPERATOR
        jz find_mul
        inc ecx
        jmp begin_loop
    find_add:
        mov eax, 0 ; find '+'
        ret
    find_mul:
        mov eax, 1 ; find '*'
        ret
    not_found:
        mov eax, 2 ; no operator
        ret

GetOperands:

ValidateInput:

CheckZero:

Big_Add:

Big_Mul: