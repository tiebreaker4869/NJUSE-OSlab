%define STDIN 0
%define STDOUT 1
%define SYS_READ 3
%define SYS_WRITE 4
%define SYS_EXIT 1
%define MAX_LEN 128
%define ADD_OPERATOR 43
%define MUL_OPERATOR 42
%define ZERO_ASCII 48

section .data
    PromptMessage: db "Please enter expression (x+y or x*y, without blankspace in the middle): ", 0ah ; 提示用户消息
    PromptMessageEnd:
    NO_OPERATOR_MSG: db "Invalid expression: No valid operator found.", 0ah
    NO_OPERATOR_MSG_END:

section .bss
    operand1: resb 42 
    operand2: resb 42
    operand1_len: resb 4
    operand2_len: resb 4
    expression: resb 100
    expression_len: resb 4
    operator_index: resb 4

section .text
    global _start
_start:
    mov ecx, PromptMessage ; 提示字符串地址
    mov edx, PromptMessageEnd - PromptMessage ; 提示字符串长度
    call DispStr

    call GetInput

    call GetOperator

    cmp eax, 2
    mov ecx, NO_OPERATOR_MSG
    mov edx, NO_OPERATOR_MSG_END - NO_OPERATOR_MSG
    call DispStr
    jz EXIT

    call GetOperands

    ; call ValidateInput

    ; call Big_Add

    ; call Big_Mul

    jmp EXIT; 退出程序
    

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
    mov dword[expression_len], eax
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
        mov dword[operator_index], ecx
        mov eax, 0 ; find '+'
        ret
    find_mul:
        mov dword[operator_index], ecx
        mov eax, 1 ; find '*'
        ret
    not_found:
        mov eax, 2 ; no operator
        ret

GetOperands:
    GetFirstOperand:
        mov ebx, expression
        add ebx, dword[operator_index]
        sub ebx, 1
        mov edx, operand1
        mov ecx, 0
        first_loop:
            mov al, byte[ebx]
            sub al, ZERO_ASCII
            mov byte[edx], al
            inc ecx
            cmp expression, ebx
            jz finish_first
            dec ebx
            inc edx
            jmp first_loop
        finish_first:
            mov dword[operand1_len], ecx
    GetSecondOperand:
        mov ebx, expression
        add ebx, expression_len
        sub ebx, 2
        mov edx, operand2
        mov ecx, 0
        second_loop:
            mov al, byte[ebx]
            sub al, ZERO_ASCII
            mov byte[edx], al
            inc ecx
            cmp ebx, expression + operator_index + 1
            jz finish_second
            dec ebx
            inc edx
            jmp second_loop
        finish_second:
            mov dword[operand2_len], ecx
    ret

CheckZero:

Big_Add:

Big_Mul:

EXIT:
    mov ebx, 0							; 参数一：退出代码
    mov eax, SYS_EXIT							; 系统调用号(sys_exit)
    int 0x80