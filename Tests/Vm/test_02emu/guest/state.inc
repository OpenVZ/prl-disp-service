%include "ctxmap.inc"

%define FPU_OFF		FCW_OFF

%define R_AX		rax
%define R_BX		rbx
%define R_CX		rcx
%define R_DX		rdx
%define R_SI		rsi
%define R_DI		rdi
%define R_SP		rsp
%define R_BP		rbp
%define R_R8		r8
%define R_R9		r9
%define R_R10		r10
%define R_R11		r11
%define R_R12		r12
%define R_R13		r13
%define R_R14		r14
%define R_R15		r15



%macro LOAD_TEST_CONTEXT 1
	cmp byte [XSR_AVL_VA], 1
	jne %%no_avx

	mov R_AX, [XCR_MASK_VA]
	mov R_DX, 0
	and [%1 + FPU_OFF + 512], R_AX
	xrstor [%1 + FPU_OFF]

	mov R_CX, 0
	and R_AX, [%1 + XCR0_OFF]
	xsetbv

	jmp %%fpu_restored

%%no_avx:
	fxrstor [%1 + FPU_OFF]

%%fpu_restored:
	xor R_DX, R_DX
	mov cr2, R_DX

	mov R_AX, [ORIG_CR0_VA]
	or  R_AX, [%1 + CR0_OFF]
	mov cr0, R_AX

	mov R_AX, [%1 + DR0_OFF]
	mov dr0, R_AX
	mov R_AX, [%1 + DR1_OFF]
	mov dr1, R_AX
	mov R_AX, [%1 + DR2_OFF]
	mov dr2, R_AX
	mov R_AX, [%1 + DR3_OFF]
	mov dr3, R_AX
	mov R_AX, dr6
	and R_AX, ~0xe00f
	or  R_AX, [%1 + DR6_OFF]
	mov dr6, R_AX
	mov R_AX, [%1 + DR7_OFF]
	or  R_AX, 1 << 10
	mov dr7, R_AX

	mov R_SP,  [%1 + RSP_OFF]
	mov R_BP,  [%1 + RBP_OFF]
	mov R_R8,  [%1 + R8_OFF]
	mov R_R9,  [%1 + R9_OFF]
	mov R_R10, [%1 + R10_OFF]
	mov R_R11, [%1 + R11_OFF]
	mov R_R12, [%1 + R12_OFF]
	mov R_R13, [%1 + R13_OFF]
	mov R_R14, [%1 + R14_OFF]
	mov R_R15, [%1 + R15_OFF]

	cmp dword [LOOPEMU_FLAG], 0
	je  %%no_loopemu
	TEST_OTG 0
%%no_loopemu:
	mov R_AX,  [%1 + RAX_OFF]
	mov R_BX,  [%1 + RBX_OFF]
	mov R_CX,  [%1 + RCX_OFF]
	mov R_DX,  [%1 + RDX_OFF]
	mov R_SI,  [%1 + RSI_OFF]
	mov R_DI,  [%1 + RDI_OFF]
%endm

%macro SAVE_TEST_CONTEXT 1
	mov [%1 + RAX_OFF], R_AX
	mov [%1 + RBX_OFF], R_BX
	mov [%1 + RCX_OFF], R_CX
	mov [%1 + RDX_OFF], R_DX
	mov [%1 + RSI_OFF], R_SI
	mov [%1 + RDI_OFF], R_DI

	cmp dword [LOOPEMU_FLAG], 0
	je  %%no_loopemu
	TEST_OTG 1
%%no_loopemu:
	mov [%1 + RSP_OFF], R_SP
	mov [%1 + RBP_OFF], R_BP
	mov [%1 + R8_OFF],  R_R8
	mov [%1 + R9_OFF],  R_R9
	mov [%1 + R10_OFF], R_R10
	mov [%1 + R11_OFF], R_R11
	mov [%1 + R12_OFF], R_R12
	mov [%1 + R13_OFF], R_R13
	mov [%1 + R14_OFF], R_R14
	mov [%1 + R15_OFF], R_R15

	mov R_AX, dr6
	mov [%1 + DR6_OFF], R_AX

	mov R_AX, 1 << 10
	mov dr7, R_AX

	mov R_AX, [ORIG_CR0_VA]
	mov cr0, R_AX

	mov rax, cr2
	mov [%1 + CR2_OFF], rax

	cmp byte [XSR_AVL_VA], 1
	jne %%no_avx

	mov R_CX, 0
	mov R_AX, [XCR_MASK_VA]
	mov R_DX, 0
	xsetbv
	xsave [%1 + FPU_OFF]
	jmp %%fpu_saved

%%no_avx:
	fxsave [%1 + FPU_OFF]

%%fpu_saved:
	mov rdx, 0xdeaddeaddeaddead
	mov [%1 + FPU_OFF + 6], dx
	mov [%1 + FPU_OFF + 8], rdx
	mov [%1 + FPU_OFF + 16], rdx
	or  byte [%1 + FPU_OFF + 24], 0x38
%endm
