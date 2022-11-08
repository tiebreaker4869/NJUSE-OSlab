	org 07c00h ; load program at 0x7c00
	mov ax, cs ; set seg reg
	mov ds, ax
	mov es, ax
	call DispStr ; call display string
	jmp $

DispStr:
	mov ax, BootMessage 
	mov bp, ax ; es:bp = string address
	mov cx, 16 ; string length
	mov ax, 01301h
	mov bx, 000ch
	mov dl, 0
	int 10h
	ret
BootMessage: db "Hello, OS world!"
times 510-($-$$) db 0 ; padding to 512 bytes
dw 0xaa55 ; end flag
