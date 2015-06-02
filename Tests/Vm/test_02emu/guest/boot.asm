BITS 16

%include "eltorito.inc"

	; Enable the A20 line
	in  al, 0x92
	or  al, 2
	out 0x92, al

	mov ax, 0
	mov ds, ax
	mov es, ax
	mov si, bootstrap
	mov di, 0
	mov cx, bootstrap_end - bootstrap
	repne movsb

	jmp 0x0000:0x0000

bootstrap:
	incbin "bootstrap.bin"
bootstrap_end:
