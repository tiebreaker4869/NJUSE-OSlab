section .data
red_color:      db  1Bh,"[31m"
red_size:       equ $ - red_color
default_color:  db  1Bh,"[0m"
defalut_size:    equ $ - default_color


section .text
    global my_print_red
    global my_print_default

my_print_red:
    mov eax, 4  ; switch to red color
    mov ebx, 1
    mov ecx, red_color
    mov edx, red_size
    int 80h

    mov eax, 4  ; print string
    mov ebx, 1
    mov ecx, [esp+4]
    mov edx, [esp+8]
    int 80h

    call switch_to_default ; switch back to default color
    ret

my_print_default:      ; print string in default color
    call switch_to_default
    mov eax, 4
    mov ebx, 1
    mov ecx, [esp+4]
    mov edx, [esp+8]
    int 80h
    ret

switch_to_default:  ; switch back to default color
    mov eax, 4
    mov ebx, 1
    mov ecx, default_color
    mov edx, defalut_size
    int 80h
    ret