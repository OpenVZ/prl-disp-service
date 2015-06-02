%define GDT_SIZE gdt_end - gdt

%define	CR0_PE		1
%define CR0_NE		(1 << 5)
%define CR0_WP		(1 << 16)
%define CR0_PG		(1 << 31)

%define CR4_PAE		(1 << 5)
%define CR4_OSFXSR	(1 << 9)
%define CR4_OSXE	(1 << 10)
%define CR4_OSXSAVE	(1 << 18)

%define IA32_EFER		0xc0000080
%define IA32_EFER_LME	(1 << 8)


%macro INT_HANDLER 0
	mov [gs:CLOB_VA + 0x00], ORIG_AX
	mov	[gs:CLOB_VA + 0x08], ORIG_BX
	mov [gs:CLOB_VA + 0x10], ORIG_CX
	mov [gs:CLOB_VA + 0x18], ORIG_DX
	mov [gs:CLOB_VA + 0x20], ORIG_SI
	mov [gs:CLOB_VA + 0x28], ORIG_DI
%endm

%macro TEST_OTG 1
	mov eax, 0x7b6af8e		; register-only OTG command
	xor ecx, ecx
	mov ebx, 7				; generic OTG command
	mov edx, 25				; test support OTG
	mov esi, %1
	mov edi, 0
	rdpmc
%endm

%macro DO_SNAPSHOT 0
	TEST_OTG 2
%endm


; Arguments:
; 1 --- base
; 2 --- limit/offset
; 3 --- type
; 4 --- D/B
; 5 --- S
; 6 --- L

%macro DESCRIPTOR 4-6 1, 0

%if %2 > 0xfffff
	%define limit	((%2) >> 12)
	%define G		1
%else
	%define limit	(%2)
	%define G		0
%endif

	dw (limit & 0xFFFF)											; limit[15:0]
	dw (%1 & 0xFFFF)											; base[15:0]
	db (%1 >> 16) & 0xFF										; base[23:16]
	db (%3 | 0x80 | (%5 << 4))									; P DPL S type
	db (limit >> 16) & 0xF | (%6 << 5) | (%4 << 6) | (G << 7)	; G D/B L AVL limit[19:16]
	db (%1 >> 24) & 0xFF										; base[24:32]

%undef limit
%undef G

%endm

%macro DESCRIPTOR64 3
	DESCRIPTOR %1, %2, %3, 0, 1, 1
%endm

%macro SYSTEM_DESCRIPTOR 4
	DESCRIPTOR %1, %2, %3, %4, 0
%endm

%macro SYSTEM_DESCRIPTOR64 3
	DESCRIPTOR %1, %2, %3, 0, 0, 1
	dq 0
%endm


; 1 --- selector
; 2 --- offset
; 3 --- type

%macro IDT_DESCRIPTOR64 3
	dw (%2) & 0xffff
	dw %1
	db 0
	db (%3) | 0x80
	dw (%2 >> 16) & 0xffff
	dd (%2 >> 32)
	dd 0
%endm

%define PTE(pa, rw) (((pa) & ~((1 << 12) - 1)) | ((rw) << 1) | 1)

%define PTE_SIZE	8

%define PTE_OFF(va)	((((va) >> 12) & ((1 << 9) - 1))*PTE_SIZE)
%define PDE_OFF(va)	((((va) >> 21) & ((1 << 9) - 1))*PTE_SIZE)

%macro MKPTE 3
	mov dword [%1 + PTE_OFF(%2_VA)], PTE(%2_PA, %3)
%endm
