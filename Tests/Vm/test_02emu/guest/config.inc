%define CODE32_SEL		0x0008
%define DATA32_SEL		0x0010
%define CODE64_SEL		0x0018
%define TSS64_SEL		0x0020

%define PAGE_SIZE		0x1000


%define PM_START		1024*1024

; Test guest physical address space map

%define BOOTSTRAP_PA	0*PAGE_SIZE
%define TMP_GDT_PA		1*PAGE_SIZE
%define INT_HANDLER_PA	PM_START
%define GDT_PA			INT_HANDLER_PA + PAGE_SIZE
%define IDT_PA			GDT_PA + PAGE_SIZE/2
%define NTESTS_PA		GDT_PA + PAGE_SIZE

%define PML4_PA			NTESTS_PA + PAGE_SIZE
%define PDPT_PA			PML4_PA + PAGE_SIZE
%define PDT_PA			PDPT_PA + PAGE_SIZE
%define PT_PA			PDT_PA + PAGE_SIZE
%define PT2_PA			PT_PA + PAGE_SIZE
%define TSS64_PA		PT2_PA + PAGE_SIZE


%define TEST_START		PM_START + 16*PAGE_SIZE

%define STACK_PA		TEST_START
%define DATA_PA			TEST_START + PAGE_SIZE
%define CODE_PA         TEST_START + 2*PAGE_SIZE
%define CONTEXT_PA		TEST_START + 3*PAGE_SIZE

%define IDT_LIMIT		(32*16 - 1)

%define PAGES_USED		10


; Test guest virtual address space map

%define BOOTSTRAP_VA	BOOTSTRAP_PA
%define INT_HANDLER_VA	0x1000
%define GDT_VA			(1 << 21)
%define IDT_VA			GDT_VA + PAGE_SIZE/2
%define NTESTS_VA		GDT_VA + PAGE_SIZE
%define TEST_COUNTER_VA	NTESTS_VA + 0x10
%define ORIG_CR0_VA		NTESTS_VA + 0x18
%define LOOPEMU_FLAG	NTESTS_VA + 0x20
%define XCR_MASK_VA		NTESTS_VA + 0x28
%define XSR_AVL_VA		NTESTS_VA + 0x30
%define PT_VA			NTESTS_VA + PAGE_SIZE
%define TSS64_VA		PT_VA + PAGE_SIZE

%define STACK_VA		2*PAGE_SIZE
%define DATA_VA			3*PAGE_SIZE
%define CODE_VA         5*PAGE_SIZE
%define CONTEXT_VA		6*PAGE_SIZE


%define TEST_CODE_SEL	CODE64_SEL
