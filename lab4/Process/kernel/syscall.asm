
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_print			equ 1 ;
_NR_delay			equ 2 ;
_NR_P				equ	3 ;
_NR_V				equ 4 ;
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global	print
global	mydelay
global	P
global	V

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================
;                              print
; ====================================================================
print:
	push ebx			; notice that when you use sys_call with parameter, 
						; protect registers so that it will not occur weird errors 
	mov eax, _NR_print
	mov	ebx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret

; ====================================================================
;                              delay
; ====================================================================
mydelay:
	push ebx
	mov eax, _NR_delay
	mov ebx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret

; ====================================================================
;                              Proberen
; ====================================================================
P:
	push ebx
	mov eax, _NR_P
	mov ebx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret

; ====================================================================
;                              Verhogen
; ====================================================================
V:
	push ebx
	mov eax, _NR_V
	mov ebx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret