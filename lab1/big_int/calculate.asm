%define STDIN 0
%define STDOUT 1
%define SYS_READ 3
%define SYS_WRITE 4
%define SYS_EXIT 1
%define MAX_LEN 128
%define ADD_OPERATOR 43
%define MUL_OPERATOR 42
%define ZERO_ASCII 48
%define NINE_ASCII 57

section .data
    PromptMessage: db "Please enter expression (x+y or x*y, without blankspace in the middle): ", 0ah ; 提示用户消息
    PromptMessageEnd:
    NO_OPERATOR_MSG: db "Invalid expression: No valid operator found.", 0ah
    NO_OPERATOR_MSG_END:
    OPERAND_NOT_VALID_MSG: db "Invalid expression: Operands not valid.", 0ah
    OPERAND_NOT_VALID_MSG_END:

section .bss
    operand1: resb 42 
    operand2: resb 42
    operand1_len: resb 4
    operand2_len: resb 4
    expression: resb 100
    expression_len: resb 4
    operator_index: resb 4
    operator_char: resb 1
    carry: resb 1
    result: resb 100
    result_len: resb 4

section .text
    global _start
_start:
    mov ecx, PromptMessage ; 提示字符串地址
    mov edx, PromptMessageEnd - PromptMessage ; 提示字符串长度
    call DispStr

    call GetInput

    call GetOperator

    cmp eax, 2
    jz EXIT_NO_OPERATOR

    call GetOperands

    mov ecx, operand1
    mov edx, dword[operand1_len]
    call CheckZero
    mov ebx, eax

    mov ecx, operand2
    mov edx, dword[operand2_len]
    call CheckZero
    and eax, ebx

    cmp eax, 0

    ; jz ZERO_CASE

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
        mov byte[operator_char], ADD_OPERATOR
        mov eax, 0 ; find '+'
        ret
    find_mul:
        mov dword[operator_index], ecx
        mov byte[operator_char], MUL_OPERATOR
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
            call ValidateDigit ; 检查该位是不是数字
            sub al, ZERO_ASCII
            mov byte[edx], al
            inc ecx
            mov eax, expression
            cmp eax, ebx
            jz finish_first
            dec ebx
            inc edx
            jmp first_loop
        finish_first:
            mov dword[operand1_len], ecx
    GetSecondOperand:
        mov ebx, expression
        add ebx, dword[expression_len]
        sub ebx, 2
        mov edx, operand2
        mov ecx, 0
        second_loop:
            mov al, byte[ebx]
            call ValidateDigit
            sub al, ZERO_ASCII
            mov byte[edx], al
            inc ecx
            mov eax, expression
            add eax, dword[operator_index]
            inc eax
            cmp ebx, eax
            jz finish_second
            dec ebx
            inc edx
            jmp second_loop
        finish_second:
            mov dword[operand2_len], ecx
    ret

CheckZero: ; 操作数地址放在 ecx, 操作数长度放在 edx
    mov ebx, 0
    check_loop:
        cmp ebx, edx
        jz is_zero
        cmp byte[ecx], 0
        jne not_zero
        inc ebx
        inc ecx
        jmp check_loop
    is_zero:
        mov eax, 0
        ret
    not_zero:
        mov eax, 1
        ret
    
EXIT:
    mov ebx, 0							; 参数一：退出代码
    mov eax, SYS_EXIT							; 系统调用号(sys_exit)
    int 0x80

EXIT_NO_OPERATOR: ; 操作符不合法
    mov ecx, NO_OPERATOR_MSG
    mov edx, NO_OPERATOR_MSG_END - NO_OPERATOR_MSG
    call DispStr
    jmp EXIT

EXIT_OPERAND_NOT_VALID: ; 操作数不合法
    mov ecx, OPERAND_NOT_VALID_MSG
    mov edx, OPERAND_NOT_VALID_MSG_END - OPERAND_NOT_VALID_MSG
    call DispStr
    jmp EXIT

ValidateDigit: ; 检查每个数位是否是数字
    cmp al, ZERO_ASCII
    jb EXIT_OPERAND_NOT_VALID
    cmp al, NINE_ASCII
    ja EXIT_OPERAND_NOT_VALID
    ret

Big_Add:
    mov ecx, 0 ; offset
    add_loop:
        mov edx, operand1
        add edx, ecx
        mov ebx, byte[edx]
        mov edx, operand2
        add edx, ecx
        mov edx, byte[edx]
        add ebx, edx
        mov edx, byte[carry]
        add ebx, edx ; add carry
        mov eax, ebx
        div 10
        mov byte[result+ecx], ah
        mov byte[carry], al
        inc ecx
        call Check_Out_Of_Bound
        cmp eax, 1
        jz final_step
        jmp add_loop
    final_step:
        cmp byte[carry], 0
        jz finish_add
        inc ecx
        mov byte[result+ecx], 1
    finish_add:
        ret


Check_Out_Of_Bound: ; offset in ecx
    cmp dword[operand1_len], ecx
    jb not_out_of_bound
    cmp dword[operand2_len], ecx
    jb not_out_of_bound
    jmp out_of_bound 
    not_out_of_bound:
        mov eax, 0
        ret
    out_of_bound:
        mov eax, 1
        ret

