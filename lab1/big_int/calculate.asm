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
%define EXIT_QUERY 113

section .data
    PromptMessage: db 0ah,"Please enter expression (x+y or x*y, without blankspace in the middle): ", 0ah ; 提示用户消息
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
    temp: resb 4

section .text
    global _start
_start:
    LOOP_START:
        mov ecx, PromptMessage ; 提示字符串地址
        mov edx, PromptMessageEnd - PromptMessage ; 提示字符串长度
        call DispStr

        call Reset_All

        call GetInput

        call Check_Quit

        cmp eax, 1

        jz FINISH_CALCULATE

        mov eax, 0

        call GetOperator

        cmp eax, 2
        jz EXIT_NO_OPERATOR

        mov eax, 0

        call GetOperands

        call Calculate

        call Print_Result

        jmp LOOP_START

    FINISH_CALCULATE:


    jmp EXIT; 退出程序
    

DispStr:
    pushad
    mov ebx, STDOUT    					; 参数：文件描述符(stdout)
	mov eax, SYS_WRITE						; 系统调用号
	int 0x80	
    popad					; 陷入内核
	ret

GetInput:
    pushad
    mov eax, SYS_READ ; 系统调用号
    mov ebx, STDIN ; 参数： 文件描述符
    mov ecx, expression ; 存储输入字符串的地址
    mov edx, MAX_LEN ; 最大长度
    int 80h ; 陷入内核
    mov dword[expression_len], eax
    popad
    ret

GetOperator:
    pushad
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
        popad
        ret
    find_mul:
        mov dword[operator_index], ecx
        mov byte[operator_char], MUL_OPERATOR
        popad
        mov eax, 1 ; find '*'
        ret
    not_found:
        popad
        mov eax, 2 ; no operator
        ret

GetOperands:
    pushad
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
    popad
    ret
    
EXIT:
    mov ebx, 0							; 参数一：退出代码
    mov eax, SYS_EXIT							; 系统调用号(sys_exit)
    int 0x80

EXIT_NO_OPERATOR: ; 操作符不合法
    pushad
    mov ecx, NO_OPERATOR_MSG
    mov edx, NO_OPERATOR_MSG_END - NO_OPERATOR_MSG
    call DispStr
    popad
    jmp LOOP_START

EXIT_OPERAND_NOT_VALID: ; 操作数不合法
    pushad
    mov ecx, OPERAND_NOT_VALID_MSG
    mov edx, OPERAND_NOT_VALID_MSG_END - OPERAND_NOT_VALID_MSG
    call DispStr
    popad
    jmp LOOP_START
    

ValidateDigit: ; 检查每个数位是否是数字
    cmp al, ZERO_ASCII
    jb EXIT_OPERAND_NOT_VALID
    cmp al, NINE_ASCII
    ja EXIT_OPERAND_NOT_VALID
    ret

Big_Add:
    pushad
    mov ecx, 0 ; offset
    add_loop:
        mov edx, operand1
        add edx, ecx
        mov ebx, 0
        mov bl, byte[edx]
        mov edx, operand2
        add edx, ecx
        mov eax, edx
        mov edx, 0
        mov dl, byte[eax]
        add bl, dl
        mov dl, byte[carry]
        add bl, dl ; add carry
        mov eax, ebx
        mov ebx, 0
        mov bl, 10
        div bl
        mov byte[result+ecx], ah
        mov byte[carry], al
        inc ecx
        call Check_Out_Of_Bound
        cmp eax, 1
        jz final_step
        jmp add_loop
    final_step:
        mov al, byte[carry]
        cmp eax, 0
        jz finish_add
        inc ecx
        mov byte[result+ecx], 1
    finish_add:
        mov dword[result_len], ecx
        popad
        ret

Big_Mul:
    pushad
    mov ecx, 0 ; outer loop variable
    mov edx, 0 ; inner loop variable
    outer_loop:
        cmp ecx, dword[operand1_len]
        jz finish_outer_loop
        mov byte[carry], 0
        mov edx, 0
        inner_loop:
            cmp edx, dword[operand2_len]
            jz finish_inner_loop
            mov ebx, ecx
            add ebx, edx
            mov eax, 0
            mov al, byte[result+ebx]
            mov ebx, 0
            mov bl, byte[carry]
            add al, bl
            mov ebx, 0
            mov bl, byte[operand1+ecx]
            push eax
            mov eax, 0
            mov al, byte[operand2+edx]
            mul bl
            mov ebx, 0
            mov ebx, eax
            pop eax
            add eax, ebx
            mov ebx, 0
            mov bl, 10
            div bl
            mov ebx, ecx
            add ebx, edx
            mov byte[result+ebx], ah
            mov byte[carry], al
            inc edx
            jmp inner_loop
        finish_inner_loop:
            mov ebx, dword[operand2_len]
            add ebx, ecx ; ebx := i + operand2_len
            mov eax, 0
            mov al, byte[carry]
            mov byte[result+ebx], al
            inc ecx
            jmp outer_loop
        finish_outer_loop:
            add ecx, edx
            remove_lead_zero:
                remove_loop:
                    mov edx, ecx
                    sub edx, 1
                    cmp byte[result+edx], 0
                    jne finish_remove
                    cmp ecx, 1
                    jz finish_remove
                    dec ecx
                    jmp remove_loop
                finish_remove:
                    mov dword[result_len], ecx
            popad
            ret


Check_Out_Of_Bound: ; offset in ecx
    pushad
    cmp ecx, dword[operand1_len]
    jb not_out_of_bound
    cmp ecx, dword[operand2_len]
    jb not_out_of_bound
    jmp out_of_bound 
    not_out_of_bound:
        popad
        mov eax, 0
        ret
    out_of_bound:
        popad
        mov eax, 1
        ret

Convert_To_Print_Format: ; convert result to printable format
    pushad
    mov edx, result
    mov ecx, 0
    convert_loop:
        mov bl, byte[edx]
        add bl, ZERO_ASCII
        mov byte[edx], bl
        inc edx
        inc ecx
        cmp ecx, dword[result_len]
        jz finish_convert
        jmp convert_loop
    finish_convert:
        popad
        ret

Reverse_String: ; reverse result string
    pushad
    mov eax, dword[result_len]
    sub eax, 1
    mov ecx, 0
    reverse_loop:
        cmp eax, ecx
        jna finish_reverse
        mov dl, byte[result+eax]
        mov bl, byte[result+ecx]
        mov byte[result+eax], bl
        mov byte[result+ecx], dl
        inc ecx
        dec eax
        jmp reverse_loop
    finish_reverse:
        popad
        ret

Check_Quit: ; check whether the user want to quit
    pushad
    cmp byte[expression], EXIT_QUERY
    jz EXIT_CASE
    popad
    mov eax, 0
    ret
    EXIT_CASE:
        popad
        mov eax, 1
        ret

Reset_All: ; reinit all data
    mov dword[operand1_len], 0
    mov dword[operand2_len], 0
    mov dword[expression_len], 0
    mov dword[operator_index], 0
    mov byte[operator_char], 0
    mov byte[carry], 0
    mov dword[result_len], 0
    mov ecx, 0
    reset_operand1_loop:
        cmp ecx, 42
        jz finish_reset_operand1
        mov byte[operand1+ecx], 0
        inc ecx
        jmp reset_operand1_loop
    finish_reset_operand1:
        mov ecx, 0
    reset_operand2_loop:
        cmp ecx, 42
        jz finish_reset_operand2
        mov byte[operand2+ecx], 0
        inc ecx
        jmp reset_operand2_loop
    finish_reset_operand2:
        mov ecx, 0
    reset_expr_loop:
        cmp ecx, 100
        jz finish_reset_expr
        mov byte[expression], 0
        inc ecx
        jmp reset_expr_loop
    finish_reset_expr:
        mov ecx, 0
    reset_res_loop:
        cmp ecx, 100
        jz finish_reset_res
        mov byte[result], 0
        inc ecx
        jmp reset_res_loop
    finish_reset_res:
        mov eax, 0
        mov ebx, 0
        mov ecx, 0
        mov edx, 0
        ret

Print_Result:
    pushad
    call Convert_To_Print_Format

    call Reverse_String

    mov ecx, result
    mov edx, dword[result_len]
    call DispStr
    popad
    ret

Calculate:
    pushad
    cmp byte[operator_char], ADD_OPERATOR
    jz CALL_ADD
    cmp byte[operator_char], MUL_OPERATOR
    jz CALL_MUL
    ret
    CALL_ADD:
        call Big_Add
        popad
        ret
    CALL_MUL:
        call Big_Mul
        popad
        ret