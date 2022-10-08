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
    operator_index: resb 4 ; 操作符在表达式中的下标
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

        cmp eax, 1      ; 检查是否需要退出，需要退出的话 Check_Quit 返回的是 1

        jz FINISH_CALCULATE

        mov eax, 0

        call GetOperator    ; 从表达式中抽取操作符

        cmp eax, 2          ; 检查是否找不到合法的操作符，找不到的话 GetOperator 返回的是 2
        jz EXIT_NO_OPERATOR

        mov eax, 0

        call GetOperands     ; 从表达式中抽取出操作数, 按从低到高排列每个十进制位

        call Calculate       ; 调用 Big_Add 或者 Big_Mul

        call Print_Result   ; 打印结果

        jmp LOOP_START

    FINISH_CALCULATE:


    jmp EXIT; 退出程序
    

DispStr:
    pushad
    mov ebx, STDOUT    					; 参数：文件描述符(stdout)
	mov eax, SYS_WRITE					; sys write 的系统调用号
	int 0x80	
    popad
	ret

GetInput:
    pushad
    mov eax, SYS_READ           ; sys read 的系统调用号
    mov ebx, STDIN              ; 参数： 文件描述符
    mov ecx, expression         ; 存储输入字符串的地址
    mov edx, MAX_LEN            ; 最大长度
    int 80h
    mov dword[expression_len], eax ; eax 里面是读入的表达式的实际长度（包含末尾的 '\0')
    popad
    ret

GetOperator:
    pushad
    mov ebx, expression         ; ebx 里面存 expression 的首地址
    mov ecx, 0                  ; ecx 里面存偏移
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
        mov ebx, expression             ; ebx 里面存 expression 首地址
        add ebx, dword[operator_index]  
        sub ebx, 1                      ; 得到expression 中 operand1 最后一位地址
        mov edx, operand1               
        mov ecx, 0
        first_loop:                     ; 从 operand1 最后一位开始往前扫
            mov al, byte[ebx]
            call ValidateDigit          ; 检查该位是不是数字
            sub al, ZERO_ASCII
            mov byte[edx], al
            inc ecx
            mov eax, expression         
            cmp eax, ebx                ; 检查是否扫完 operand1 部分
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
    mov ebx, 0							        ; 参数一：退出代码
    mov eax, SYS_EXIT							; 系统调用号(sys_exit)
    int 0x80

EXIT_NO_OPERATOR:                               ; 操作符不合法
    pushad
    mov ecx, NO_OPERATOR_MSG
    mov edx, NO_OPERATOR_MSG_END - NO_OPERATOR_MSG
    call DispStr
    popad
    jmp LOOP_START

EXIT_OPERAND_NOT_VALID:                         ; 操作数不合法
    pushad
    mov ecx, OPERAND_NOT_VALID_MSG
    mov edx, OPERAND_NOT_VALID_MSG_END - OPERAND_NOT_VALID_MSG
    call DispStr
    popad
    jmp LOOP_START
    

ValidateDigit:                                  ; 检查每个数位是否是数字
    cmp al, ZERO_ASCII
    jb EXIT_OPERAND_NOT_VALID
    cmp al, NINE_ASCII
    ja EXIT_OPERAND_NOT_VALID
    ret

Big_Add:
    pushad
    mov ecx, 0              ; ecx 存的是当前在算结果的第几位
    add_loop:
        mov edx, operand1
        add edx, ecx
        mov ebx, 0
        mov bl, byte[edx]   ; bl := operand1[ecx]
        mov edx, operand2
        add edx, ecx
        mov eax, edx
        mov edx, 0
        mov dl, byte[eax]   ; dl := operand2[ecx]
        add bl, dl
        mov dl, byte[carry]
        add bl, dl          ; add carry
        mov eax, ebx
        mov ebx, 0
        mov bl, 10         
        div bl
        mov byte[result+ecx], ah ; ah 存的是除法的余数，也就是本位结果
        mov byte[carry], al      ; al 存的是除法的商，也就是 carry
        inc ecx
        call Check_Out_Of_Bound
        cmp eax, 1
        jz final_step
        jmp add_loop
    final_step:
        mov al, byte[carry]
        cmp eax, 0
        jz finish_add
        mov byte[result+ecx], 1  ; 有进位需要处理
        inc ecx
    finish_add:
        mov dword[result_len], ecx ; 保存结果的位数
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
            mov al, byte[result+ebx]   ; al := result[ecx+edx]
            mov ebx, 0
            mov bl, byte[carry]
            add al, bl                  ; result[ecx+edx] += carry
            mov ebx, 0
            mov bl, byte[operand1+ecx]
            push eax
            mov eax, 0
            mov al, byte[operand2+edx]
            mul bl
            mov ebx, 0
            mov ebx, eax
            pop eax
            add eax, ebx                ; result[ecx+edx] += operand1[ecx] * operand2[edx]
            mov ebx, 0
            mov bl, 10
            div bl
            mov ebx, ecx
            add ebx, edx
            mov byte[result+ebx], ah    ; result[ecx+edx] %= 10
            mov byte[carry], al         ; carry := result[ecx+edx] / 10
            inc edx
            jmp inner_loop
        finish_inner_loop:
            mov ebx, dword[operand2_len]
            add ebx, ecx
            mov eax, 0
            mov al, byte[carry]
            mov byte[result+ebx], al    ; result[ecx+operand2_len] = carry
            inc ecx
            jmp outer_loop
        finish_outer_loop:
            add ecx, edx
            remove_lead_zero:            ; 去除前导零
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
                    mov dword[result_len], ecx ; 保存结果长度
            popad
            ret


Check_Out_Of_Bound:              ;检查是否越界，下标存在 ecx 里面
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

Convert_To_Print_Format:            ; 把结果转换成可以打印的形式，每位加上 0 的 ASCII
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

Reverse_String:                 ; 反转结果字符串，双指针法
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

Check_Quit:                             ; 如果用户输入 q 就退出
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

Reset_All:                          ; 重置所有 bss 区的变量和寄存器
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
        mov byte[expression+ecx], 0
        inc ecx
        jmp reset_expr_loop
    finish_reset_expr:
        mov ecx, 0
    reset_res_loop:
        cmp ecx, 100
        jz finish_reset_res
        mov byte[result+ecx], 0
        inc ecx
        jmp reset_res_loop
    finish_reset_res:
        mov eax, 0
        mov ebx, 0
        mov ecx, 0
        mov edx, 0
        ret

Print_Result:                       ; 计算之后 result 里面每个字节存储该位的十进制计算结果，从低位到高位
    pushad                          ; 把 result 中存的计算结果转换成打印的形式（ASCII）并反转
    call Convert_To_Print_Format

    call Reverse_String

    mov ecx, result                 ; 打印的字符串地址
    mov edx, dword[result_len]      ; 字符串长度
    call DispStr
    popad
    ret

Calculate:                                  ; 根据运算符决定调用加法或者乘法
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