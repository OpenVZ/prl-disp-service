; Test virtual address space map:
;
; +-------+---+-------+------+------+---------+-  --+-----+-----+---+---+
; | Boot  | I | Stack | Data | Test | Context | \ \ | GDT | TSS | S | P |
; | strap | n |       |      | code |         | / / |     |     | t | T |
; |       | t |       |      |      |         | \ \ | IDT |     | a | 1 |
; |       |   |       |      |      |         | / / |     |     | t |   |
; |   R   | R |  R/W  | R/W  |  R   |   R/W   | \ \ | R/W | R/W | e |   |
; +-------+---+-------+------+------+---------+--  -+-----+-----+---+---+
;
;     0     1     2      3       4       5            512   513  514 515
;
;
; Test physical address space map:
;
; +-------+---+-----+---------+---+---+---+---+---+-  --+-------+------+------+---------+-
; | Boot  | I | GDT | N tests | P | P | P | P | P | \ \ | Stack | Data | Test | Context | \
; | strap | n |     |         | M | D | D | T | T | / / |       |      | code |         | /
; |       | t | IDT |  TSS64  | L | P |   | 1 | 2 | \ \ |       |      |      |         | \
; |       |   |     |         | 4 | T |   |   |   | / / |       |      |      |         | /
; |   R   | R | R/W |   R/W   |   |   |   |   |   | \ \ |  R/W  | R/W  |  R   |   R/W   | \
; +-------+---+-----+---------+---+---+---+---+---+--  -+-------+------+------+---------+--
;
;     0     1    2       3      4   5   6   7   8           4N   4N + 1 4N + 2  4N + 3

%include "config.inc"
%include "common.inc"
%include "state.inc"

ORG 0

BITS 16
	mov ebp, 0
	mov bp, cs
	mov ds, bp

	; Copy the GDT
	mov bp, TMP_GDT_PA >> 4
	mov es, bp
	mov di, 0
	mov si, gdt
	mov cx, GDT_SIZE
	rep movsb

	lgdt [tmp_gdtr_pm]

	mov eax, cr0
	or  eax, CR0_PE
	mov cr0, eax

	jmp CODE32_SEL:pm

tmp_gdtr_pm:
	dw GDT_SIZE - 1
	dd TMP_GDT_PA


BITS 32
pm:
	mov ax, DATA32_SEL
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov gs, ax
	mov fs, ax

	; Initialize TSS
	mov dword [TSS64_PA + 36], STACK_VA + PAGE_SIZE - 16	; IST1
	mov word [TSS64_PA + 102], 0x0fff						; I/O map address base


	; Copy the interrupt handler
	mov edi, INT_HANDLER_PA
	mov esi, int_handler_once
	mov ecx, int_hander_end - int_handler_once
	repne movsb


	; Fill the interrupt table
	mov esi, idt
	mov edi, IDT_PA
	mov ecx, 32*16
	repne movsb

	mov byte [IDT_PA + 8*16 + 4], 1							; use IST = 1 for the #DF handler


	; Initialize the paging tree
	mov dword [PML4_PA], PTE(PDPT_PA, 1)					; PML4
	mov dword [PDPT_PA], PTE(PDT_PA, 1)						; PDPT
	mov dword [PDT_PA], PTE(PT_PA, 1)						; PD
	mov dword [PDT_PA + PDE_OFF(GDT_VA)], PTE(PT2_PA, 1)	; The PD for the GDT

	MKPTE PT_PA,  DATA, 1
	MKPTE PT_PA,  STACK, 1
	MKPTE PT_PA,  CODE, 0
	MKPTE PT_PA,  CONTEXT, 1
	MKPTE PT_PA,  INT_HANDLER, 0
	MKPTE PT_PA,  BOOTSTRAP, 0
	MKPTE PT2_PA, GDT, 1
	MKPTE PT2_PA, NTESTS, 1
	MKPTE PT2_PA, TSS64, 1
	MKPTE PT2_PA, PT, 1

	mov esi, CR4_PAE | CR4_OSFXSR
	mov eax, 0x01
	cpuid
	test ecx, 1 << 26
	jz init_cr4
	or esi, CR4_OSXSAVE | CR4_OSXE
init_cr4:
	; Enable the PAE paging, the SSE instruction set, and the instruction XSAVE
	mov eax, cr4
	or  eax, esi
	mov cr4, eax

	; Enable the 64-bit mode
	mov ecx, IA32_EFER
	rdmsr
	or  eax, IA32_EFER_LME
	wrmsr

	; Setup the paging root
	mov eax, PML4_PA
	mov cr3, eax

	mov edi, GDT_PA
	mov esi, gdt
	mov ecx, GDT_SIZE
	rep movsb

	; Use the IA32e mode GDT
	lgdt [gdtr_lm]

	; Enable paging and disable writing to R/O pages
	mov eax, cr0
	or  eax, CR0_NE | CR0_PG | CR0_WP
	mov cr0, eax

	jmp CODE64_SEL:ia32e

gdtr_lm:
	dw GDT_SIZE - 1
	dd GDT_VA



BITS 64
ia32e:
	mov eax, 0x01
	cpuid
	test ecx, 1 << 26
	jz no_avx

	mov byte [XSR_AVL_VA], 1

	mov eax, 0x0d
	mov ecx, 0
	cpuid
	mov [XCR_MASK_VA], eax

	mov edx, 0
	mov ecx, 0
	xsetbv

no_avx:
	lidt [idtr]

	mov ax, TSS64_SEL
	ltr ax

	mov rax, 0xf
	mov cr8, rax

	mov rax, cr0
	mov [ORIG_CR0_VA], rax

	DO_SNAPSHOT

	jmp qword far [rel test_gate]
test_gate:
	dq INT_HANDLER_VA
	dw TEST_CODE_SEL


; This code is located at the physical address INT_HANDLER_PA.
; This loop is broken by the test driver that writes 2 90H bytes at that location
; after the test VM is bootstrapped.
int_handler_once:
	jmp int_handler_once

	cli
	mov rax, 0
	mov [PT_VA + PTE_OFF(0)], rax
	invlpg [rax]					; unmap the 0th page
	jmp load_context

int_handler:
	cli

save_context:
	SAVE_TEST_CONTEXT CONTEXT_VA

	mov r15, [TEST_COUNTER_VA]
	cmp r15, [NTESTS_VA]
	je  do_snap

	inc r15
	mov [TEST_COUNTER_VA], r15

	; Switch to the next test
	add dword [PT_VA + PTE_OFF(STACK_VA)], 4*PAGE_SIZE
	and dword [PT_VA + PTE_OFF(STACK_VA)], 0xffffff9f
	add dword [PT_VA + PTE_OFF(DATA_VA)], 4*PAGE_SIZE
	and dword [PT_VA + PTE_OFF(DATA_VA)], 0xffffff9f
	add dword [PT_VA + PTE_OFF(CODE_VA)], 4*PAGE_SIZE
	and dword [PT_VA + PTE_OFF(CODE_VA)], 0xffffff9f
	add dword [PT_VA + PTE_OFF(CONTEXT_VA)], 4*PAGE_SIZE
	and dword [PT_VA + PTE_OFF(CONTEXT_VA)], 0xffffff9f
	invlpg [STACK_VA]
	invlpg [DATA_VA]
	invlpg [CODE_VA]
	invlpg [CONTEXT_VA]

load_context:
	LOAD_TEST_CONTEXT CONTEXT_VA

	jmp qword [rel test_code]
test_code:
	dq CODE_VA



%macro HANDLE_INT 1
handle_int_%1:
	mov  byte [CONTEXT_VA + VECTOR_OFF], %1
	jmp  int_handler
%endm

%assign i 0
%rep 32
	HANDLE_INT i
	%assign i i + 1
%endrep


do_snap:
	DO_SNAPSHOT

hang:
	jmp hang

int_hander_end:



idt:
%macro MAKE_IDTE 1
	IDT_DESCRIPTOR64 CODE64_SEL, INT_HANDLER_VA + handle_int_%1 - int_handler_once, 14
%endm

%assign i 0
%rep 32
	MAKE_IDTE i
	%assign i i + 1
%endrep


idtr:
	dw 32*16
	dd IDT_VA

gdt:
	dd 0x00000000	; Unused
	dd 0x00000000

	DESCRIPTOR		0x0000, 0xffffffff, 8, 1	; the flat 32-bit code segment (0008)
	DESCRIPTOR		0x0000, 0xffffffff, 2, 1	; the flat 32-bit data segment (0010)
	DESCRIPTOR64	0x0000, 0xffffffff, 8		; the flat 64-bit code segment (0018)

	SYSTEM_DESCRIPTOR64	TSS64_VA, (PAGE_SIZE - (TSS64_VA & 0xfff)), 9
gdt_end:
