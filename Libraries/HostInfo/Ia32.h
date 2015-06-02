//////////////////////////////////////////////////////////////////////////
///
/// @file Ia32.h
///
/// @brief Opisanie osnovniih struktur processora IA32
///
/// @author misha, maximk
///
/// Copyright (c) 2006-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
//////////////////////////////////////////////////////////////////////////

#ifndef __IA32_H__
#define __IA32_H__

// We need some standard types definitions here
#include "Interfaces/ParallelsTypes.h"

#include <Interfaces/packed.h>

// Avoid of names problems because linux kernel have the same names inside
#if defined(_LIN_) && defined(_KERNEL_)
# undef APIC_ID
# undef EFER_SCE
# undef EFER_LME
# undef EFER_LMA
# undef _PAGE_TABLE
#endif


// 256 TB address masks
#define SHIFT_256TB	(48)
#define SIZE_256TB	((ULONG64)1<<SHIFT_256TB)
#define MASK_256TB	(~(SIZE_256TB -1))


// 512 GB address masks
#define SHIFT_512GB	(39)
#define SIZE_512GB	((ULONG64)1<<SHIFT_512GB)
#define MASK_512GB	(~(SIZE_512GB -1))



//4gb
#define SHIFT_4GB	(32)
#define SIZE_4GB	((ULONG64)1<<SHIFT_4GB)
#define MASK_4GB	(~(SIZE_4GB -1))


// 1 GB address masks
#define SHIFT_1GB	(30)
#define SIZE_1GB	((1<<SHIFT_1GB))
#define MASK_1GB	(~(SIZE_1GB -1))



// 4 MB address masks (non-PAE mode)
#define SHIFT_4MB	(22)
#define SIZE_4MB	((1<<SHIFT_4MB))
#define MASK_4MB	(~(SIZE_4MB-1))



// 2 MB address masks
#define SHIFT_2MB	(21)
#define SIZE_2MB	((1<<SHIFT_2MB))
#define MASK_2MB	(~(SIZE_2MB -1))


// 1 MB address masks
#define SHIFT_1MB	(20)
#define SIZE_1MB	(1<<SHIFT_1MB)
#define MASK_1MB	(~(SIZE_1MB-1))


// 4 KB address masks
#define SHIFT_4KB	(12)
#define SIZE_4KB	(1<<SHIFT_4KB)
#define MASK_4KB	(~(SIZE_4KB-1))

// Pages in 1 MB
#define PAGES_PER_MB (SIZE_1MB / PAGE_SIZE)

// Calculation of paging entries (bit range) indexes
#define SIZE_IDX(bottom, top)	(1 << ((top) - (bottom)))
#define MASK_IDX(bottom, top)	(SIZE_IDX(bottom, top) - 1)

// Align PDPT table on 32 byte offsets
#define MASK_PAE_PDPT_ADDR  (~0x1f)

// maximum values
#define MAX_8B		0xff
#define MAX_16B		0xffff
#define MAX_32B		0xffffffff
#define MAX_64B		UL64(0xffffffffffffffff)

// maximum values (sz - size in bytes)
#define MAX_VAL(sz) ( ((ULONG_PTR)1<<((sz)*8-1)) | (((ULONG_PTR)1<<((sz)*8-1))-1) )


#define PAGE_SIZE_ALIGN(size)  ( ((size) + PAGE_SIZE -1) & MASK_4KB )

#define SHIFT_INDEX_4KB(phy_idx) ( ((ULONG64)phy_idx) << SHIFT_4KB )


#define PAGE_LEVEL	0
#define PTBL_LEVEL	1
#define PDIR_LEVEL	2
#define PDPT_LEVEL	3

#define X86_PAGING_LEVELS	2
#define PAE_PAGING_LEVELS	3
#define EM64_PAGING_LEVELS	4

#define PAGING_LEVELS_MAX		\
	MAX(						\
		MAX(					\
			X86_PAGING_LEVELS,	\
			PAE_PAGING_LEVELS ),\
		EM64_PAGING_LEVELS )

// Virtual CPU Statuses
#define VCPU_UNINITIALIZED	0x0
#define VCPU_WAIT_FOR_SIPI	0x2
#define VCPU_STARTED		0x4
#define VCPU_STOPPED		0x8

/**
 * @brief
 *		Imenovanniij dostup k strukture deskriptora
 *      (segment-descriptor and system-descriptor in legacy mode,
 *      segment-descriptor in long mode)
 */
typedef union _DESCRIPTOR
{
	struct {
		USHORT	wLimitLo;		// limit 0..15
		USHORT	wBaseLo;		// base  0..15
		UCHAR	cBaseCentral;	// base  16..23
		USHORT	Type : 5;		// S/type field
		USHORT	Dpl : 2;		// DPL
		USHORT	P : 1;			// present
		USHORT	LimitHi4 : 4;	// limit 16..19
		USHORT	Avl : 1;		// Available for use by system software
		USHORT	Lb   : 1;		// in legacy mode - reserved 0. In long mode 0 = compatibilty, 1 = 64-bit mode code-seg
		USHORT	Db : 1;			// D/B
		USHORT	G : 1;			// granularity
		UCHAR	cBaseHi;		// base 24..31
	};
	ULONG64 u64Cell;
} DESCRIPTOR;


/**
 * @brief
 *		Imenovanniij dostup k bitam selektora
 */
typedef union _SELECTOR
{
	USHORT	wCell;													// polniij
	struct
	{
		USHORT	Rpl : 2;
		USHORT	Ti : 1;
		USHORT	Index : 13;
	};											// bitii
} SELECTOR;


// Urovni privelegij deskriptora
#define DESCR_DPL0	0
#define DESCR_DPL1	1
#define DESCR_DPL2	2
#define DESCR_DPL3	3

// Granulyarnost' deskriptora
#define DESCR_BYTE_GRAN 0											// bajtovaya
#define DESCR_PAGE_GRAN 1											// stranichnaya = 4K

// Razryadnost' deskriptora
#define DESCR_DEFSIZE_16	0
#define DESCR_DEFSIZE_32	1

// Prisutstvie deskriptora
#define DESCR_NOT_PRESENT	0
#define DESCR_PRESENT		1

// Tipii deskriptorov
#define DESCR_SEGMENT				0x10							// segment
#define DESCR_CODE					0x08
#define DESCR_EXPAND_DOWN			0x04							// rasshiryaemiij vniz (segment danniih)
#define DESCR_CONFORMING			0x04							// podchinenniij		(segment koda)
#define DESCR_WRITE_DATA			0x02							// zapisiivaemiij		(segment danniih)
#define DESCR_READ_CODE				0x02							// chitaemiij			(segment koda)
#define DESCR_ACCESSED				0x01

#define DESCR_TYPE_DATA				0x10
#define DESCR_TYPE_CODE				0x18

#define DESCR_TYPE_LDT				0x02							// segment LDT
#define DESCR_TYPE_TASK				0x05							// shluz zadachi
#define DESCR_TYPE_TSS				0x09							// segment TSS 386
#define DESCR_TYPE_CALL_GATE		0x0C							// shluz viizova 386
#define DESCR_TYPE_INTERRUPT_GATE	0x0E							// shluz preriivaniya 386
#define DESCR_TYPE_TRAP_GATE		0x0F							// shluz lovushki 386

// dostupniij bit deskriptora
#define DESCR_AVL0	0
#define DESCR_AVL1	1


/**
 * @brief
 *		Simovlicheskie nazvaniya polya Type v system-segment &
 *		gate-descriptors.
 */
enum SYSTEM_SEGMENT_AND_GATE_DESCRIPTORS_TYPES_ENUM
{
	SSGD_RESERVED0				= 0x0,
	SSGD_16BIT_TSS_AVAIL		= 0x1,
	SSGD_LDT					= 0x2,
	SSGD_16BIT_TSS_BUSY			= 0x3,
	SSGD_16BIT_CALL_GATE		= 0x4,
	SSGD_TASK_GATE				= 0x5,
	SSGD_16BIT_INT_GATE			= 0x6,
	SSGD_16BIT_TRAP_GATE		= 0x7,
	SSGD_RESERVED8				= 0x8,
	SSGD_32BIT_TSS_AVAIL		= 0x9,
	SSGD_RESERVED_10			= 0xa,
	SSGD_32BIT_TSS_BUSY			= 0xb,
	SSGD_32BIT_CALL_GATE		= 0xc,
	SSGD_RESERVED_13			= 0xd,
	SSGD_32BIT_INT_GATE			= 0xe,
	SSGD_32BIT_TRAP_GATE		= 0xf,
};

#define DESCRIPTOR_INITIALIZER(B, LIM, D_G, DB, AVL, D_P, DPL, T, L_FL) \
	{{ \
		/*.wLimitLo =		*/ (USHORT)((LIM) & 0x0ffff), \
		/*.wBaseLo =		*/ (USHORT)((B) & 0xffff), \
		/*.cBaseCentral =	*/ (UCHAR)(((B) >> 16) & 0xff), \
		/*.Type =			*/ T, \
		/*.Dpl =			*/ DPL, \
		/*.P =				*/ D_P, \
		/*.LimitHi4 =		*/ (USHORT)(((LIM) & 0xf0000) >> 16), \
		/*.Avl =			*/ AVL, \
		/*.Lb =				*/ L_FL, \
		/*.Db =				*/ DB, \
		/*.G =				*/ D_G, \
		/*.cBaseHi =		*/ (UCHAR)(((B) >> 24) & 0xff) \
	 }}

#define GATHER_DESCRIPTOR(Descr, B, LIM, D_G, DB, AVL, D_P, DPL, T, L_FL) \
	{ \
		DESCRIPTOR tmp = \
			DESCRIPTOR_INITIALIZER(B, LIM, D_G, DB, AVL, D_P, DPL, T, L_FL); \
		(Descr) = tmp; \
	}

#define SET_DESCRIPTOR_BASE(Descr, B)								\
	{																\
		(Descr).cBaseHi = (B) >> 24;								\
		(Descr).cBaseCentral = ((B) >> 16) & 0xff;					\
		(Descr).wBaseLo = (B) & 0xffff;								\
	}

#define SET_DESCRIPTOR_LIMIT(Descr, L)								\
	{																\
		(Descr).LimitHi4 = ((L) & 0xf0000) >> 16;					\
		(Descr).wLimitLo = ((L) & 0x0ffff);							\
	}

#define SET_DESCRIPTOR_ATTR(Descr, A)								\
	do {															\
		*(UINT16*) ((UCHAR*) &(Descr) + 5) = (A) & 0xF0FF;			\
	} while (0)

/**
 * @brief
 *		Poluchenie predela deskriptora
 *
 * @params
 *		Descr	[in]	desktriptor
 *
 *		Ne realizovana proverka dejstvitel'no li segment?
 *		Ne sovsem yasen smiisl
 *
 * @author
 *		AlexKo
 */
#define GET_DESCRIPTOR_LIMIT(Descr)													\
		(																			\
			(Descr).G ?	/* granulyarnost'? */										\
				/* stranichnaya */													\
				((((Descr).Type & 0xC) == 0x4) ?	/* tip? */						\
						/* stek 0x000 */											\
						((((UINT32)(Descr).LimitHi4 << 16) | (Descr).wLimitLo) << 12) :		\
						/* danniie 0xfff */											\
						(((UINT32)(Descr).LimitHi4 << 16) | (Descr).wLimitLo) << 12 | 0xfff) :	\
				/* bajtnaya */														\
				(((UINT32)(Descr).LimitHi4 << 16) | (Descr).wLimitLo)							\
		)


/**
 * @brief
 *		Poluchenie bazii deskriptora
 *
 * @params
 *		Descr	[in]	desktriptor
 *
 * @author
 *		AlexKo
 */
#define GET_DESCRIPTOR_BASE(Descr)	\
		(((Descr).wBaseLo | ((UINT32)(Descr).cBaseCentral << 16) | \
		((UINT32)(Descr).cBaseHi << 24)) & 0xffffffff)

#define GET_DESCRIPTOR_BASE16(Descr)	\
		(((Descr).wBaseLo | ((UINT32)(Descr).cBaseCentral << 16)) & \
		0xffff)


/**
 * Get segment descriptor attributes (bits 40:47 and 52:55)
 *
 * @params DESCRIPTOR structure
 */
#define GET_DESCRIPTOR_ATTR(Descr)	(*(UINT16*) ((UCHAR*) &(Descr) + 5) & 0xF0FF)


/**
 * @brief
 *		Sozdanie selektora iz ego polej
 *
 * @params
 *		Sel	[in]	indeks selektora
 *		Ti	[in]	indeks tablicii
 *		Rpl	[in]	requested uroven' privioegij
 *
 * @author
 *		AlexKo
 */
#define BUILD_SELECTOR(Sel, Ti, Rpl)	(((Sel) << 3) | ((Ti) << 2) | (Rpl))


// RPL levels
#define RPL0	0
#define RPL1	1
#define RPL2	2
#define RPL3	3

// DPL levels
#define DPL0	0
#define DPL1	1
#define DPL2	2
#define DPL3	3

// CPL levels
#define CPL0	0
#define CPL1	1
#define CPL2	2
#define CPL3	3



/**
 * @brief
 *		Opisanie formata shluza v tablicah global'niih, lokal'niih deskriptorov i
 *		tablice preriivanij.
 */
typedef struct _GATE_DESCRIPTOR_STRUCT
{
	// Mladshee slovo smesheniya tochki vhoda v proceduru obrabotki
	USHORT		wLoOffset;

	// Selektor tochki vhoda v proceduru obrabotki
	SELECTOR	Selector;

	// IST field
	UCHAR       Ist : 3;

	UCHAR		cReserved35_39 : 5;

	// Tip shluza
	UCHAR		Type : 5;

	// Trebuemiij uroven' privilegij dlya viisova shluza
	UCHAR		Dpl : 2;

	// Priznak prisutstviya obrabotchika v pamyati
	UCHAR		P : 1;

	// Starshee slovo smesheniya tochki vhoda v proceduru obrabotki
	USHORT		wHiOffset;

} GATE_DESCRIPTOR_STRUCT;
typedef GATE_DESCRIPTOR_STRUCT* PGATE_DESCRIPTOR_STRUCT;


/**
 * @brief
 *		call-gate - is just as gate-descriptor, but in cReserved field has
 *      additional field ParamCount
 */
typedef struct _CALL_GATE_DESCRIPTOR
{
    // Mladshee slovo smesheniya tochki vhoda v proceduru obrabotki
    USHORT		wLoOffset;

    // Selektor tochki vhoda v proceduru obrabotki
    SELECTOR	Selector;

    // Parameter count
    UCHAR		ParamCount : 5;

    // unused
    UCHAR       cReserved  : 3;

    // Tip shluza
    UCHAR		Type : 5;

    // Trebuemiij uroven' privilegij dlya viisova shluza
    UCHAR		Dpl : 2;

    // Priznak prisutstviya obrabotchika v pamyati
    UCHAR		P : 1;

    // Starshee slovo smesheniya tochki vhoda v proceduru obrabotki
    USHORT		wHiOffset;

} CALL_GATE_DESCRIPTOR;


/**
 * @brief
 *      calculate offset for 16/32-bit call/int/trap gate
 *
 * @params
 *      Desc - [in] pointer at descriptor
 *      b32  - [in] 1 if 32-bit, 0 if 16-bit
 *
 * @return
 *      ULONG_PTR - offset
 */
#define GATHER_GATE_OFFSET(pDesc,b32)    \
    ((ULONG_PTR)( ((CALL_GATE_DESCRIPTOR*)(pDesc))->wLoOffset \
        | ((b32)?( ((CALL_GATE_DESCRIPTOR*)(pDesc))->wHiOffset << 16 ):(0)) ) &0xffffffff)






/**
 * @brief
 *		Deskriptor tablicii global'niih deskriptorov i tablicii vektorov preriivaniya.
 */
typedef struct _DESCRIPTOR_TABLE_REG
{
	// Predel (razmer v bajtah - 1) tablicii
	USHORT	wLimit;

	// Bazoviij linejniij adres tablicii
    // in 32-bit monitor it would be 32-bit long. And will be used completely
    // in 64-bit it would be 64-bit long, so in emulation only the lower part
    // of it would be used, but on mode-control it would be loaded entirely
	ULONG_PTR	uBase;

} DESCRIPTOR_TABLE_REG;



/**
 * @brief
 *		Deskriptor tablicii global'niih deskriptorov i tablicii vektorov preriivaniya.
 */
typedef struct _DESCRIPTOR_TABLE_REG_32
{
	// Predel (razmer v bajtah - 1) tablicii
	USHORT	wLimit;

	// Bazoviij linejniij adres tablicii
    // in 32-bit monitor it would be 32-bit long. And will be used completely
    // in 64-bit it would be 64-bit long, so in emulation only the lower part
    // of it would be used, but on mode-control it would be loaded entirely
	UINT	uBase;

} DESCRIPTOR_TABLE_REG_32;


/**
 * @brief
 *		Deskriptor tablicii global'niih deskriptorov i tablicii vektorov preriivaniya.
 */
typedef struct _DESCRIPTOR_TABLE_REG_64
{
	// Predel (razmer v bajtah - 1) tablicii
	USHORT	wLimit;

	// Bazoviij linejniij adres tablicii
    // in 32-bit monitor it would be 32-bit long. And will be used completely
    // in 64-bit it would be 64-bit long, so in emulation only the lower part
    // of it would be used, but on mode-control it would be loaded entirely
	ULONG64	uBase;

} DESCRIPTOR_TABLE_REG_64;


/**
 * @brief
 *		Makros osushestvlyaet zapolnenenie deskriptora iz tablicii GDT, LDT
 *		ili IDT, kak shluz preriivaniya.
 *
 * @params
 *		Descr - [out] zapolnyaemiij deskriptor;
 *		S - [in] selektor procedurii obrabotki preriivaniya;
 *		O - [in] smeshenie procedurii obrabotki preriivaniya;
 *		I_P - [in] priznak prisutstviyaya obrabotchika v pamyati;
 *		DPL - [in] trebuemiij uroven' privilegij;
 *		D - [in] tip deskriptora.
 *
 *
 *		Makros ne mozhet ispol'zovat'sya v viirazheniyah, kak operand.
 *
 * @author
 *		AlexeyK
 */
#define GATHER_INTERRUPT_GATE_DESCRIPTOR(Descr, S, O, I_P, DPL, D)	\
	{																\
		Descr.Selector.wCell = (S);									\
		Descr.wHiOffset = (O) >> 16;								\
		Descr.wLoOffset = (O) & 0xffff;								\
		Descr.cReserved35_39 = 0;									\
		Descr.Type = ((D) << 3) | SSGD_16BIT_INT_GATE;				\
		Descr.Dpl = (DPL);											\
		Descr.P = (I_P);											\
	}



/**
 * @brief
 *		Makros osushestvlyaet zapolnenenie deskriptora iz tablicii GDT, LDT
 *		ili IDT, kak shluz lovushki.
 *
 * @params
 *		Descr - [out] zapolnyaemiij deskriptor;
 *		S - [in] selektor procedurii obrabotki preriivaniya;
 *		O - [in] smeshenie procedurii obrabotki preriivaniya;
 *		I_P - [in] priznak prisutstviyaya obrabotchika v pamyati;
 *		DPL - [in] trebuemiij uroven' privilegij;
 *		D - [in] tip deskriptora.
 *
 *
 *		Makros ne mozhet ispol'zovat'sya v viirazheniyah, kak operand.
 *
 * @author
 *		AlexeyK
 */
#define GATHER_TRAP_GATE_DESCRIPTOR(Descr, S, O, I_P, DPL, D)	\
	{															\
		Descr.Selector.wCell = (S);								\
		Descr.wHiOffset = (O) >> 16;							\
		Descr.wLoOffset = (O) & 0xffff;							\
		Descr.cReserved = 0;									\
		Descr.Type = ((D) << 3) | SSGD_16BIT_TRAP_GATE;			\
		Descr.Dpl = (DPL);										\
		Descr.P = (I_P);										\
	}



/**
 * @brief
 *		Makros osushestvlyaet zapolnenenie deskriptora iz tablicii GDT, LDT
 *		ili IDT, kak shluz viizova.
 *
 * @params
 *		Descr - [out] zapolnyaemiij deskriptor;
 *		S - [in] selektor procedurii obrabotki preriivaniya;
 *		O - [in] smeshenie procedurii obrabotki preriivaniya;
 *		I_P - [in] priznak prisutstviyaya obrabotchika v pamyati;
 *		DPL - [in] trebuemiij uroven' privilegij;
 *		D - [in] tip deskriptora.
 *		ParamCount - [in] kolichestvo parametrov.
 *
 *		Makros ne mozhet ispol'zovat'sya v viirazheniyah, kak operand.
 *
 * @author
 *		AlexeyK
 */
#define GATHER_CALL_GATE_DESCRIPTOR(Descr, S, O, I_P, DPL, D, ParamCount)	\
	{																		\
		Descr.Selector = (S);												\
		Descr.wHiOffset = (O) >> 16;										\
		Descr.wLoOffset = (O) & 0xffff;										\
		Descr.cReserved = (ParamCount & 0xf);								\
		Descr.Type = ((D) << 3) | SSGD_16BIT_CALL_GATE;						\
		Descr.Dpl = (DPL);													\
		Descr.P = (I_P);													\
	}





/**
 * @brief
 *		Sravnenie dvuh deskriptorov na predmet ravenstva.
 */
static __inline BOOL AreGatesTheSame( GATE_DESCRIPTOR_STRUCT* gate1, GATE_DESCRIPTOR_STRUCT* gate2 )
{
	return (
		(*(((UINT*)gate1)+0)) == (*(((UINT*)gate2)+0)) &&
		(*(((UINT*)gate1)+1)) == (*(((UINT*)gate2)+1))
		);
}


static __inline const char* GetSegmentGateName( UINT uType )
{
	switch ( uType )
	{
		case SSGD_RESERVED0:			return "reserved_0";
		case SSGD_16BIT_TSS_AVAIL:		return "16_bit_tss_avail";
		case SSGD_LDT:					return "ldt";
		case SSGD_16BIT_TSS_BUSY:		return "16_bit_tss_busy";
		case SSGD_16BIT_CALL_GATE:		return "16_bit_call_gate";
		case SSGD_TASK_GATE:			return "task_gate";
		case SSGD_16BIT_INT_GATE:		return "16_bit_int_gate";
		case SSGD_16BIT_TRAP_GATE:		return "16_bit_trap_gate";
		case SSGD_RESERVED8:			return "reserved_8";
		case SSGD_32BIT_TSS_AVAIL:		return "32_bit_tss_avail";
		case SSGD_RESERVED_10:			return "reserved_10";
		case SSGD_32BIT_TSS_BUSY:		return "32_bit_tss_busy";
		case SSGD_32BIT_CALL_GATE:		return "32_bit_call_gate";
		case SSGD_RESERVED_13:			return "reserved_13";
		case SSGD_32BIT_INT_GATE:		return "32_bit_int_gate";
		case SSGD_32BIT_TRAP_GATE:		return "32_bit_trap_gate";
	}

	return "unknown";
}



/**
 * @brief
 *		Dal'nij viizov funkcii.
 *
 * @params
 *		wSel  - [in] selektor funkcii (ili shluza viizova);
 *		uOffs - [in] smeshenie funkcii v segmente (ili 0 dlya shluza viizova).
 *
 *		Net.
 *
 * @author
 *		AlexeyK
 */
#define GATHER_CALL_FAR(wSel, uOffs)	\
	__asm _emit 0x9a					\
	__asm _emit(uOffs & 0xff)			\
	__asm _emit((uOffs >> 8) & 0xff)	\
	__asm _emit((uOffs >> 16) & 0xff)	\
	__asm _emit((uOffs >> 24) & 0xff)	\
	__asm _emit(wSel & 0xff)			\
	__asm _emit((wSel >> 8) & 0xff)


/**
 * @brief
 *		Parametr dal'nego perehoda.
 */
typedef struct _ADDRESS_JUMP_FAR
{
	// Smeshenie dal'nego perehoda(32/64 in dependance on _X86_/_AMD64_
	ULONG_PTR	uOffset;

	// Selektor dal'nego perehoda
	USHORT	wSelector;

} ADDRESS_JUMP_FAR;


#ifdef _AMD64_
#define INVALID_LINEAR_ADDRESS	0x8000000000000000
#else
#define INVALID_LINEAR_ADDRESS	0xffffffff
#endif


// Priznak togo, chto na stranicu proizvodilas' zapis'
#define PAGE_BIT_D	0x00000040

// Priznak togo, chto k stranice/tablice stranic osushestvlyalsya dostup (chtenie/zapis')
#define PAGE_BIT_A	0x00000020

// PTE/PDE Present bit
#define PAGE_BIT_P	0x00000001

// Bit r/w permissions to page
#define PAGE_BIT_RW	0x00000002

// Bit u/s permissions to page
#define PAGE_BIT_US	0x00000004

/**
 * @brief
 *		Opisanie zapisi iz direktorii tablic stranic i tablicii stranic.
 */
typedef union _PTE_INFO
{
	// Celikom zapis' v tablice
	UINT	uCell;

	// Predstavlenie zapisi po polyam
	struct
	{
		// Nalichie stranicii/tablicii stranic v pamyati (1-prisutstvuet)
		UINT	P : 1;

		// Priznak togo, chto na stranicu (gruppu stranic dlya direktorii) razreshena zapis'
		// razreshena zapis' iz kol'ca pol'zovatelya 3 pri US==1
		UINT	RW : 1;

		// Priznak togo, chto k stranice (gruppu stranic dlya direktorii) razreshen dostup
		// (zavisit ot RW) iz kol'ca pol'zovatelya 3
		UINT	US : 1;

		// Priznak togo, chto zapis' na stranicu/tablicu stranic viiziivaet odnovremennoe
		// obnovlenie fizicheskoj stranicii (zavisit ot polya CD v registre CR0). Pri CD==1
		// znachenie PWT ignoriruetsya.
		UINT	PWT : 1;

		// Priznak togo, chto keshirovanie stranicii/tablicii stranic otklucheno (zavisit
		// ot polya CD v registre CR0). Pri CD==1 znachenie PWT ignoriruetsya.
		UINT	PCD : 1;

		// Priznak togo, chto k stranice/tablice stranic osushestvlyalsya dostup (chtenie/zapis').
		// Priznak ustanavlivaetsya processorom pri pervom obrashenii, a sbrasiivaetsya
		// programmniim putem.
		UINT	A : 1;

		// Priznak togo, chto na stranicu osushestvlyalas' zapis'. Priznak ustanavlivaetsya
		// processorom pri pervom obrashenii, a sbrasiivaetsya programmniim putem.
		UINT	D : 1;

		// Opredelyaet razmer stranic (0-4kb, 1-zavisit ot rezhima fizicheskoj adresacii).
		// Ispol'zuetsya tol'ko dlya direktorii tablic stranic.
		UINT	PS : 1;

		// Priznak global'noj stranicii, pomechautsya naibolee chasto ispol'zuemiie stranicii.
		// Zavisit ot znacheniya PGE registra CR4.
		UINT	G : 1;

		// Bitii dostupniie dlya programmnogo ispol'zovaniya
		UINT	ProgAvail : 3;

		// Bazoviij adres stranicii
		UINT	Base : 20;

	};

	// Predstavlenie zapisi po polyam dlya bolee udobnogo sravneniya v MapGuestLinPage
	struct
	{

		// Nalichie stranicii/tablicii stranic v pamyati (1-prisutstvuet)
		UINT	P : 1;

		// Ob`edinenniie polya US i RW
		UINT	US_RW : 2;

		// Priznak togo, chto zapis' na stranicu/tablicu stranic viiziivaet odnovremennoe
		// obnovlenie fizicheskoj stranicii (zavisit ot polya CD v registre CR0). Pri CD==1
		// znachenie PWT ignoriruetsya.
		UINT	PWT : 1;

		// Priznak togo, chto keshirovanie stranicii/tablicii stranic otklucheno (zavisit
		// ot polya CD v registre CR0). Pri CD==1 znachenie PWT ignoriruetsya.
		UINT	PCD : 1;

		// Priznak togo, chto k stranice/tablice stranic osushestvlyalsya dostup (chtenie/zapis').
		// Priznak ustanavlivaetsya processorom pri pervom obrashenii, a sbrasiivaetsya
		// programmniim putem.
		UINT	A : 1;

		// Priznak togo, chto na stranicu osushestvlyalas' zapis'. Priznak ustanavlivaetsya
		// processorom pri pervom obrashenii, a sbrasiivaetsya programmniim putem.
		UINT	D : 1;

		// Opredelyaet razmer stranic (0-4kb, 1-zavisit ot rezhima fizicheskoj adresacii).
		// Ispol'zuetsya tol'ko dlya direktorii tablic stranic.
		UINT	PS : 1;

		// Priznak global'noj stranicii, pomechautsya naibolee chasto ispol'zuemiie stranicii.
		// Zavisit ot znacheniya PGE registra CR4.
		UINT	G : 1;

		// Bitii dostupniie dlya programmnogo ispol'zovaniya
		// Ob`edinenniie polya US i RW iz gostevogo deskriptora
		UINT	CopyGuest_US_RW : 2;

		UINT	Reserved : 1;

		// Bazoviij adres stranicii
		UINT	Base : 20;

	} CompDetails;
} PTE_INFO;



/**
 * @brief
 *		PAE mode PDTP/PDE/PTE 8 bytes descriptor.
 */
typedef union _PTE_INFO_PAE
{
	// Celikom zapis' v tablice
	ULONG64		u64Cell;

	// Predstavlenie zapisi po polyam
	struct
	{
		// Nalichie stranicii/tablicii stranic v pamyati (1-prisutstvuet)
		ULONG64		P : 1; // 0

		// Priznak togo, chto na stranicu (gruppu stranic dlya direktorii) razreshena zapis'
		// razreshena zapis' iz kol'ca pol'zovatelya 3 pri US==1
		ULONG64		RW : 1; // 1

		// Priznak togo, chto k stranice (gruppu stranic dlya direktorii) razreshen dostup
		// (zavisit ot RW) iz kol'ca pol'zovatelya 3
		ULONG64		US : 1; // 2

		// Priznak togo, chto zapis' na stranicu/tablicu stranic viiziivaet odnovremennoe
		// obnovlenie fizicheskoj stranicii (zavisit ot polya CD v registre CR0). Pri CD==1
		// znachenie PWT ignoriruetsya.
		ULONG64		PWT : 1; // 3

		// Priznak togo, chto keshirovanie stranicii/tablicii stranic otklucheno (zavisit
		// ot polya CD v registre CR0). Pri CD==1 znachenie PWT ignoriruetsya.
		ULONG64		PCD : 1; // 4

		// Priznak togo, chto k stranice/tablice stranic osushestvlyalsya dostup (chtenie/zapis').
		// Priznak ustanavlivaetsya processorom pri pervom obrashenii, a sbrasiivaetsya
		// programmniim putem.
		ULONG64		A : 1; // 5

		// Priznak togo, chto na stranicu osushestvlyalas' zapis'. Priznak ustanavlivaetsya
		// processorom pri pervom obrashenii, a sbrasiivaetsya programmniim putem.
		ULONG64		D : 1; // 6

		// Opredelyaet razmer stranic (0-4kb, 1-zavisit ot rezhima fizicheskoj adresacii).
		// Ispol'zuetsya tol'ko dlya direktorii tablic stranic.
		ULONG64		PS : 1; // 7

		// Priznak global'noj stranicii, pomechautsya naibolee chasto ispol'zuemiie stranicii.
		// Zavisit ot znacheniya PGE registra CR4.
		ULONG64		G : 1; // 8

		// Bitii dostupniie dlya programmnogo ispol'zovaniya
		ULONG64		ProgAvail : 3; // 9

		// Bazoviij adres stranicii
		ULONG64		Base : 40; //12

		// available
		ULONG64		Available : 11; // 52

		// NonExecutio
		ULONG64		NX:1; //63

	};

	// Predstavlenie zapisi po polyam dlya bolee udobnogo sravneniya v MapGuestLinPage
	struct
	{

		// Nalichie stranicii/tablicii stranic v pamyati (1-prisutstvuet)
		ULONG64		P : 1;

		// Ob`edinenniie polya US i RW
		ULONG64		US_RW : 2;

		// Priznak togo, chto zapis' na stranicu/tablicu stranic viiziivaet odnovremennoe
		// obnovlenie fizicheskoj stranicii (zavisit ot polya CD v registre CR0). Pri CD==1
		// znachenie PWT ignoriruetsya.
		ULONG64		PWT : 1;

		// Priznak togo, chto keshirovanie stranicii/tablicii stranic otklucheno (zavisit
		// ot polya CD v registre CR0). Pri CD==1 znachenie PWT ignoriruetsya.
		ULONG64		PCD : 1;

		// Priznak togo, chto k stranice/tablice stranic osushestvlyalsya dostup (chtenie/zapis').
		// Priznak ustanavlivaetsya processorom pri pervom obrashenii, a sbrasiivaetsya
		// programmniim putem.
		ULONG64		A : 1;

		// Priznak togo, chto na stranicu osushestvlyalas' zapis'. Priznak ustanavlivaetsya
		// processorom pri pervom obrashenii, a sbrasiivaetsya programmniim putem.
		ULONG64		D : 1;

		// Opredelyaet razmer stranic (0-4kb, 1-zavisit ot rezhima fizicheskoj adresacii).
		// Ispol'zuetsya tol'ko dlya direktorii tablic stranic.
		ULONG64		PS : 1;

		// Priznak global'noj stranicii, pomechautsya naibolee chasto ispol'zuemiie stranicii.
		// Zavisit ot znacheniya PGE registra CR4.
		ULONG64		G : 1;

		// Bitii dostupniie dlya programmnogo ispol'zovaniya
		// Ob`edinenniie polya US i RW iz gostevogo deskriptora
		ULONG64		CopyGuest_US_RW : 2;

		ULONG64		Reserved : 1;

		// Bazoviij adres stranicii
		ULONG64		Base : 40;

		// available
		ULONG64		Available : 11;

		// NonExecutio
		ULONG64		NX:1;

	} CompDetails;
} PTE_INFO_PAE;

typedef union
{
	PTE_INFO		Std;

	PTE_INFO_PAE	Pae;

} PTE_INFO_UNIFIED;

#define PTE_A_MASK          0x20
#define PTE_D_MASK          0x40
#define PTE_PCD_PWT_MASK	0x18
#define PTE_PAT_MASK		0x80
#define PDE_PS_MASK			0x80
#define PDE_PAT_MASK		0x1000
#define PTE_PRESENT		(1 << 0)
#define PTE_RW			(1 << 1)
#define PTE_US			(1 << 2)
#define PTE_G_MASK		(1 << 8)
#define PTE_NX_MASK		(UL64(1) << 63)

/**
 * @brief
 *		Direktoriya tablic stranic/tablica stranic/stranica.
 */
typedef union _PAGE_TABLE
{
	// Predstavlenie s pomosh'u massiva bajt
	UCHAR			aBytes[PAGE_SIZE];

	// Predstavlenie kak mak massiv zapisej kataloga tablic stranic/tablicii stranic
	PTE_INFO		aPteInfo[PAGE_SIZE / sizeof(PTE_INFO)];

	// View as a set of PAE mode PDE/PTE descriptor
	PTE_INFO_PAE	aPteInfoPae[PAGE_SIZE / sizeof(PTE_INFO_PAE)];

} PAGE_TABLE;



/**
 * @brief
 *		Direktoriya tablic stranic/tablica stranic/stranica.
 */
typedef union _PAGE_DIRECTORY_POINTER_TABLE_PAE
{
	// Predstavlenie s pomosh'u massiva bajt
	UCHAR		aBytes[32];

	// Predstavlenie kak mak massiv zapisej kataloga tablic stranic/tablicii stranic
	PTE_INFO_PAE	aPdpeInfo[4];

} PAGE_DIRECTORY_POINTER_TABLE_PAE;

#define DIR_PTES_MASK			0x3ff
#define DIR_PTES_PAE_MASK		0x1ff


#define PML4E_IDX(uLinAddr) ( ((uLinAddr) >> SHIFT_512GB) & DIR_PTES_PAE_MASK )
#define PDPTE_IDX(LinAddr)  ( ((LinAddr) >> SHIFT_1GB) & DIR_PTES_PAE_MASK )
#define PDE_PAE_IDX(LinAddr)  ( ((LinAddr) >> SHIFT_2MB) & DIR_PTES_PAE_MASK )
#define PTE_PAE_IDX(LinAddr)  ( ((LinAddr) >> SHIFT_4KB) & DIR_PTES_PAE_MASK )
#define PDE_IDX(LinAddr)  ( ((LinAddr) >> SHIFT_4MB) & DIR_PTES_MASK )
#define PTE_IDX(LinAddr)  ( ((LinAddr) >> SHIFT_4KB) & DIR_PTES_MASK )



#define ACC_RW_SHIFT 1
#define ACC_US_SHIFT 2
#define ACC_X_SHIFT  4

// memory access flags, must correspond to #PF error flags
#define ACC_R (0 << ACC_RW_SHIFT)	// ...0. read
#define ACC_W (1 << ACC_RW_SHIFT)	// ...1. write
#define ACC_S (0 << ACC_US_SHIFT)	// ..0.. supervisor
#define ACC_U (1 << ACC_US_SHIFT)	// ..1.. user
#define ACC_X (1 << ACC_X_SHIFT)	// 1.... execute

// memory access flags aliases
#define ACC_SR (ACC_S|ACC_R)		// ..00. supervisor + read
#define ACC_SW (ACC_S|ACC_W)		// ..01. supervisor + write

#define ACC_MASK	0x16			// 10110
#define ACC_US_MASK 0x04			// 00100

// return current user/supervisor memory access mode
#define GUEST_US(vcpu) ((GUEST_CPL_VCPU(vcpu) == 3) ? ACC_U : ACC_S)

// flag means that we try to access paging struct
// required for generating EPT_VIOLATION in nested paging case
#define ACC_PTE (1 << 31)

/**
 * Error on #PF
 */
typedef union _PF_ERROR
{
	UINT uCell;

	struct
	{
		// 1 if NOT-present is met
		UINT	bPresent:	1;

		// 1 for WRITE operation
		UINT	bRw		:	1;

		// 1 if USER, 0 is SUPERVISOR
		UINT	bUs     :   1;

		// 1 if the fault was caused by reserved bits set to 1 in a page directory.
		UINT	bRsvd	:   1;

		// 1 if the fault was caused by an instruction fetch.
		UINT	bExec	:   1;

		UINT    uReserved5_31: 27;

	};

}PF_ERROR;


/**
 * @brief
 *		Opisanie upravlyaushego registra CR0.
 */
typedef union _CTRL_REG_CR0
{
    // Entire value of the register (for pushing/poping in 32/64)
    // guaranatee extending of the union in long mode
    ULONG_PTR   uCell;

	// Znachenie registra v vide nabora flagov
	struct
	{
		// (Protection Enable) priznak zashishennogo rezhima
		ULONG_PTR	Pe : 1;

		// (Monitor Coprocessor) kontrol' viipolneniya instrukcij WAIT, Fwait
		ULONG_PTR	Mp : 1;

		// (Emulation FPU) processor dolzhen emulirovat' komandii FPU
		ULONG_PTR	Em : 1;

		// (Task Switched) pozvolyaet sohranit' kontekst FPU pri perekluchenii
		// zadach
		ULONG_PTR	Ts : 1;

		// (Extension Type) tip rasshireniya soprocessora
		ULONG_PTR	Et : 1;

		// (Numeric Error) razreshaet vnutrennij mehanizm informirovaniya ob
		// oshibkah FPU
		ULONG_PTR	Ne : 1;

		// Zarezervirovano
		ULONG_PTR	Reserved_6_15 : 10;

		// (Write Protect) zashita zapisi s urovnya supervizora na stranicii
		// pol'zovatelya s flagom tol'ko dlya chteniya
		ULONG_PTR	Wp : 1;

		// Zarezervirovano
		ULONG_PTR	Reserved_17 : 1;

		// (Alignment Mask) avtomaticheskaya proverka viiravnivaniya
		ULONG_PTR	Am : 1;

		// Zarezervirovano
		ULONG_PTR	Reserved_19_28 : 10;

		// (Not Write-through) vozmozhno ispol'zovat' kesh zapisi v operativnuu
		// pamyat'
		ULONG_PTR	Nw : 1;

		// (Cache Disable) razreshenie keshirovaniya
		ULONG_PTR	Cd : 1;

		// (Paging) ispol'zuetsya stranichnoe preobrazovanie linejnogo adresa
		// v fizicheskij
		ULONG_PTR	Pg : 1;

		#ifdef _AMD64_
        ULONG_PTR Reserved32_61 : 32;
     	#endif

	};
} CTRL_REG_CR0;


/**
 * @brief
 *		Opisanie upravlyaushego registra CR4.
 */
typedef union _CTRL_REG_CR4
{

    // Entire value of the register (for pushing/poping in 32/64)
    // guaranatee extending of the union in long mode
    ULONG_PTR   uCell;

	// Znachenie registra v vide nabora flagov
	struct
	{
		// (Virtual-8086 Mode Extensions) pozvolyaet rsshirenie preriivanij i
		// iskluchitel'niih situacij v V86 rezhime
		ULONG_PTR	Vme : 1;

		// (Protected-Mode Virtual Interrupts) razreshaet apparatnuu podderzhku
		// VIF flaga v EFLAGS
		ULONG_PTR	Pvi : 1;

		// (Time Stamp Disable) razreshaet/zapreshaet chtenie timestamp schetchika
		// na urovne otlichnom ot 0.
		ULONG_PTR	Tsd : 1;

		// (Debugging Extensions) razreshaet rasshirenie otladki.
		ULONG_PTR	De : 1;

		// (Page Size Extensions) pozvolyaet ispol'zovanie stranic razmerom
		// v 4 megabajtom
		ULONG_PTR	Pse : 1;

		// (Physical Address Extension) pozvolyaet ispol'zovat' rasshirennuu
		// adresaciu fizicheskoj pamyati (36-bit)
		ULONG_PTR	Pae : 1;

		// (Machine-Check Enable) razreshaet iskluchenie mashinnoj proverki
		ULONG_PTR	Mce : 1;

		// (Page Global Enable) razreshaet ustanovku priznaka global'nosti
		// stranicii
		ULONG_PTR	Pge : 1;

		// (Performance-Monitoring Counter Enable) razreshaet/zapreshaet chtenie
		// performance schetchika (RDPMC) na urovne otlichnom ot 0.
		ULONG_PTR	Pce : 1;

		// (OS Fast Save/Restore FPU/MMX/SSE) podderzhka OS dlya ispol'zovaniya
		// komand biistrogo pereklucheniya konteksta FXSAVE i FXRESTORE
		ULONG_PTR	Osfxsr : 1;

		// (OS XMM Exception) podderzhka OS iskluchenij ob bloka XMM
		ULONG_PTR	Osxmmexcpt : 1;

		// Zarezervirovano
		ULONG_PTR	Reserved_12_11 : 2;

		// Support for VT-x mode (Intel Vanderpool)
		ULONG_PTR	Vmxe : 1;

		// Support for SMX mode (Intel TXT)
		ULONG_PTR	Smxe : 1;

		// Zarezervirovano
		ULONG_PTR	Reserved_15 : 1;

		// Support for RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE instructions
		ULONG_PTR	Rdfsgsbase : 1;

		// Process Context Identifier Enabled
		ULONG_PTR	Pcide : 1;

		// Enable XSAVE/XRSTOR
		ULONG_PTR	Osxsave : 1;

		ULONG_PTR	Reserved_19 : 1;

		// Supervisor mode execution protection
		ULONG_PTR	Smep : 1;

		// Zarezervirovano
    #if defined(_X86_)
		ULONG_PTR	Reserved_31_21 : 11;
    #elif defined(_AMD64_)
        ULONG_PTR Reserved_63_21 : 43;
    #endif

	};
} CTRL_REG_CR4;


#define EXCEPTION_NONE	256		// No exception specified
#define EXCEPTION_DE 0			/* Divide Error (fault) */
#define EXCEPTION_DB 1			/* Debug (fault/trap) */
#define EXCEPTION_NMI 2			/* non-maskable-interrupt */
#define EXCEPTION_BP 3			/* Breakpoint (trap) */
#define EXCEPTION_OF 4			/* Overflow (trap) */
#define EXCEPTION_BR 5			/* BOUND (fault) */
#define EXCEPTION_UD 6			/* UnDefined opcode (fault) */
#define EXCEPTION_NM 7			/* No Math coprocessor (fault) */
#define EXCEPTION_DF 8			/* Double Fault (abort,EC=0) */
#define EXCEPTION_TS 10			/* Task Segment (fault,EC) */
#define EXCEPTION_NP 11			/* Not Present (fault,EC) */
#define EXCEPTION_SS 12			/* Stack Segment (fault,EC) */
#define EXCEPTION_GP 13			/* General Protection (fault,EC) */
#define EXCEPTION_PF 14			/* Page Fault (fault,EC) */
#define EXCEPTION_MF 16			/* Math coprocessor Fault (fault) */
#define EXCEPTION_AC 17			/* Align Check (fault,EC=0) */
#define EXCEPTION_MC 18			/* Machine Check (abort,EC) */
#define EXCEPTION_XF 19			/* Xmm Fault (fault) */



/**
 * Definitions for exception bitmap bits.
 */
#define EXC_BITMAP_DE					(1 << 0)	/* Divide Error (fault) */
#define EXC_BITMAP_DB					(1 << 1)	/* Debug (fault/trap) */
#define EXC_BITMAP_NMI					(1 << 2)	/* NMI */
#define EXC_BITMAP_BP					(1 << 3)	/* Breakpoint (trap) */
#define EXC_BITMAP_OF					(1 << 4)	/* Overflow (trap) */
#define EXC_BITMAP_BR					(1 << 5)	/* BOUND (fault) */
#define EXC_BITMAP_UD					(1 << 6)	/* UnDefined opcode (fault) */
#define EXC_BITMAP_NM					(1 << 7)	/* No Math coprocessor (fault) */
#define EXC_BITMAP_DF					(1 << 8)	/* Double Fault (abort,EC=0) */
#define EXC_BITMAP_SO					(1 << 9)	/* FPU segment exception */
#define EXC_BITMAP_TS					(1 << 10)	/* Task Segment (fault,EC) */
#define EXC_BITMAP_NP					(1 << 11)	/* Not Present (fault,EC) */
#define EXC_BITMAP_SS					(1 << 12)	/* Stack Segment (fault,EC) */
#define EXC_BITMAP_GP					(1 << 13)	/* General Protection (fault,EC) */
#define EXC_BITMAP_PF					(1 << 14)	/* Page Fault (fault,EC) */
#define EXC_BITMAP_RES					(1 << 15)   /* Reserved */
#define EXC_BITMAP_MF					(1 << 16)	/* Math coprocessor Fault (fault) */
#define EXC_BITMAP_AC					(1 << 17)	/* Align Check (fault,EC=0) */
#define EXC_BITMAP_MC					(1 << 18)	/* Machine Check (abort,EC) */
#define EXC_BITMAP_XF					(1 << 19)	/* Xmm Fault (fault) */
													/* 20-31 are reserved */



#define EFLAGS_CF	(1 << 0)
#define EFLAGS_PF	(1 << 2)
#define EFLAGS_AF	(1 << 4)
#define EFLAGS_ZF	(1 << 6)
#define EFLAGS_SF	(1 << 7)
#define EFLAGS_TF	(1 << 8)
#define EFLAGS_IF	(1 << 9)
#define EFLAGS_DF	(1 << 10)
#define EFLAGS_OF	(1 << 11)
#define EFLAGS_IOPL (3 << 12)
#define EFLAGS_NT	(1 << 14)
#define EFLAGS_RF	(1 << 16)
#define EFLAGS_VM	(1 << 17)
#define EFLAGS_AC	(1 << 18)
#define EFLAGS_VIF	(1 << 19)
#define EFLAGS_VIP	(1 << 20)
#define EFLAGS_ID	(1 << 21)

#define DR6_B0	(1 << 0)	// 0x1
#define DR6_B1	(1 << 1)	// 0x2
#define DR6_B2	(1 << 2)	// 0x4
#define DR6_B3	(1 << 3)	// 0x8
#define DR6_BD	(1 << 13)	// 0x2000 // debug-register-access detected
#define DR6_BS	(1 << 14)	// 0x4000 // single step
#define DR6_BT	(1 << 15)	// 0x8000 // task-switch
#define DR6_RTM (1 << 16)       // 0x10000 // RTM section
#define DR6_RESERVED1 0xff0 // bits 11-4 reserved, read as 1
#define DR6_MASK (DR6_B0|DR6_B1|DR6_B2|DR6_B3|DR6_BD|DR6_BS|DR6_BT)

#define DR6_ARTIFICIAL_HLT		(1 << 31)
#define DR6_ARTIFICIAL_HLT_ONCE	(1 << 30)

#define DR7_GD	(1 << 13)	// general-detect enable

#define CR0_PE		(1 << 0)
#define CR0_MP		(1 << 1)
#define CR0_EM		(1 << 2)
#define CR0_TS		(1 << 3)
#define CR0_ET		(1 << 4)
#define CR0_NE		(1 << 5)
#define CR0_WP		(1 << 16)
#define CR0_AM		(1 << 18)
#define CR0_NW		(1 << 29)
#define CR0_CD		(1 << 30)
#define CR0_PG		(1u << 31)

#define CR0_COPROCESSOR	( CR0_NE | CR0_ET | CR0_TS | CR0_EM | CR0_MP )

/**
 * CR4 flags
 */
#define CR4_VME			(1 << 0)	// Virtual-8086 Mode Extensions
#define CR4_PVI			(1 << 1)	// Protected-Mode Virtual Interrupts
#define CR4_TSD			(1 << 2)	// Time Stamp Disable
#define CR4_DE			(1 << 3)	// Debugging Extensions
#define CR4_PSE			(1 << 4)	// Page Size Extensions
#define CR4_PAE			(1 << 5)	// Physical-Address Extension
#define CR4_MCE			(1 << 6)	// Machine Check Enable
#define CR4_PGE			(1 << 7)	// Page-Global Enable
#define CR4_PCE			(1 << 8)	// Performance-Monitoring Counter Enable
#define CR4_OSFXSR		(1 << 9)	// Operating System FXSAVE/FXRSTOR Support
#define CR4_OSXMMEXCPT	(1 << 10)	// Operating System Unmasked Exception Support
#define CR4_VMXE		(1 << 13)
#define CR4_SMXE		(1 << 14)
#define CR4_RDFSGSBASE	(1 << 16)	// Support for RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE instructions
#define CR4_PCIDE		(1 << 17)
#define CR4_OSXSAVE		(1 << 18)
#define CR4_SMEP		(1 << 20)	// Super-visor mode execution prevention


/**
 * @brief
 *		Registr flagov.
 */
typedef union _REG_EFLAGS
{
    ULONG_PTR	uCell;
    struct
    {
        ULONG_PTR	Cf : 1;
        ULONG_PTR	Reserved_1 : 1;
        ULONG_PTR	Pf : 1;
        ULONG_PTR	R3 : 1;
        ULONG_PTR	Af : 1;
        ULONG_PTR	R5 : 1;
        ULONG_PTR	Zf : 1;
        ULONG_PTR	Sf : 1;
        ULONG_PTR	Tf : 1;
        ULONG_PTR	If : 1;
        ULONG_PTR	Df : 1;
        ULONG_PTR	Of : 1;
        ULONG_PTR	Iopl : 2;
        ULONG_PTR	Nt : 1;
        ULONG_PTR	Reserved_15 : 1;
        ULONG_PTR	Rf : 1;
        ULONG_PTR	Vm : 1;
        ULONG_PTR	Ac : 1;
        ULONG_PTR	Vif : 1;
        ULONG_PTR	Vip : 1;
        ULONG_PTR	Id : 1;
        ULONG_PTR	Reserved_22_31 : 10;
    };
} REG_EFLAGS;

typedef union _REG_EFLAGS32
{
	UINT	uCell;
	struct
	{
		UINT	Cf : 1;
		UINT	Reserved_1 : 1;
		UINT	Pf : 1;
		UINT	R3 : 1;
		UINT	Af : 1;
		UINT	R5 : 1;
		UINT	Zf : 1;
		UINT	Sf : 1;
		UINT	Tf : 1;
		UINT	If : 1;
		UINT	Df : 1;
		UINT	Of : 1;
		UINT	Iopl : 2;
		UINT	Nt : 1;
		UINT	Reserved_15 : 1;
		UINT	Rf : 1;
		UINT	Vm : 1;
		UINT	Ac : 1;
		UINT	Vif : 1;
		UINT	Vip : 1;
		UINT	Id : 1;
		UINT	Reserved_22_31 : 10;
	};
} REG_EFLAGS32;

/**
 * @brief
 *      the aim of the union to provide work with registers more effective
 *      with different op_size/operating_modes
 */
typedef union _REG_VAL
{
#ifdef _AMD64_
    ULONG64     uReg64;
#endif
    UINT        uReg32;
    USHORT      uReg16;
    UCHAR       uReg8 [2];
    ULONG_PTR   uCell; //maximum info
}REG_VAL;


/**
 * @brief
 *      relation of symbolic names with their digit values
 *      for general purpose registers
 */
#define	    REG_ID_AX    0
#define		REG_ID_CX    1
#define		REG_ID_DX    2
#define		REG_ID_BX    3
#define		REG_ID_SP    4
#define		REG_ID_BP    5
#define		REG_ID_SI    6
#define		REG_ID_DI    7
#define		REG_ID_R8	 8
#define		REG_ID_R9	 9
#define		REG_ID_R10	10
#define		REG_ID_R11	11
#define		REG_ID_R12	12
#define		REG_ID_R13	13
#define		REG_ID_R14	14
#define		REG_ID_R15	15
#define		REG_ID_MAX	15
#define		REG_ID_BANK	16

/**
 * @brief
 *      identifiers of the segment registers
 */
#define SEG_REG_ES			0
#define SEG_REG_CS			1
#define SEG_REG_SS			2
#define SEG_REG_DS			3
#define SEG_REG_FS			4
#define SEG_REG_GS			5
#define SEG_REG_COUNT		6
#define SEG_REG_INVALID		7

// int/iretd stack content
// 2 last fields in 32bit mode CAN be unallocated (depending upon cpl)!
struct INT_STACK
{
	ULONG_PTR	uEip;
	ULONG_PTR	uCs;
	REG_EFLAGS	uEflags;
	ULONG_PTR	uEsp;
	ULONG_PTR	uSs;
};

struct INT_STACK32
{
	UINT			uEip;
	UINT			uCs;
	REG_EFLAGS32	uEflags;
	UINT			uEsp;
	UINT			uSs;
};


/**
 * @brief
 *      structure describing SS+SP pair in 16-bit TSS
 */
typedef struct _SS_ESP_16
{
    USHORT      uSp;
    SELECTOR    uSs;

}SS_ESP_16;

/**
 * @brief
 *      structure describing SS+ESP pair in 32-bit TSS
 */
typedef struct _SS_ESP_32
{
    UINT        uEsp;
    SELECTOR    uSs;
    USHORT      uAignment;

}SS_ESP_32;


/**
 * @brief
 *      32-bit TSS
 *
 *      size of 32-bit TSS must be 0x68 bytes
 */
typedef struct _TSS_32
{
    // selector of the previous task
    SELECTOR    uPrevLink;
    USHORT      uReserved2_3;

    // pairs of SS:ESP for dpl 0,1,2
    SS_ESP_32   StackSwitch[3];

    UINT        uCr3;

    // EIP, flags and General-Purpose Registers
    UINT        uEip;
    UINT        uEflags;
    UINT        uEax;
    UINT        uEcx;
    UINT        uEdx;
    UINT        uEbx;
    UINT        uEsp;
    UINT        uEbp;
    UINT        uEsi;
    UINT        uEdi;


    // aligned segment registers. Actually they are not UINT, only USHORT(SELECTOR) is used
    SELECTOR    uEs;
    USHORT      uReserved4A_4B;
    SELECTOR    uCs;
    USHORT      uReserved4E_4F;
    SELECTOR    uSs;
    USHORT      uReserved52_53;
    SELECTOR    uDs;
    USHORT      uReserved56_57;
    SELECTOR    uFs;
    USHORT      uReserved5A_5B;
    SELECTOR    uGs;
    USHORT      uReserved5E_5F;

    // LDT selector + alignment
    SELECTOR    LdtSelector;
    USHORT      uReserved62_63;

    // Trap-bit. Only lowest bit of USHORT is used.
    USHORT      bT;

    // I/O permission bitmap base address.
    // This bitmap specfies protection for I/O port addresses
    USHORT      uIoPermissionBmpAddr;

}TSS_32;


/**
 * @brief
 *      16-bit TSS
 *
 *      size of 16-bit TSS must be 0x2d bytes
 */
typedef struct _TSS_16
{
    // selector of the previous task
    SELECTOR    uPrevLink;

    // pairs of SS:ESP for dpl 0,1,2
    SS_ESP_16   StackSwitch[3];

    // EIP, flags and General-Purpose Registers
    USHORT      wIp;
    USHORT      wFlags;
    USHORT      wAx;
    USHORT      wCx;
    USHORT      wDx;
    USHORT      wBx;
    USHORT      wSp;
    USHORT      wBp;
    USHORT      wSi;
    USHORT      wDi;

    // segment registers
    SELECTOR     uEs;
    SELECTOR     uCs;
    SELECTOR     uSs;
    SELECTOR     uDs;

    // task LDT selector
    SELECTOR    LdtSelector;

}TSS_16;

#define CPUID_MODEL_FEATURES		0x00000001
	#define CPUID_MSR_SUPPORT			(1 << 5)
	#define CPUID_APIC_FEATURE			(1 << 9)

typedef union _EXT_80000008_EAX_T
{
	UINT	uCell;
	struct
	{
		UINT	PhysAddrBits   : 8;
		UINT	VirtAddrBits   : 8;
		UINT	Reserved_31_16 : 16;
	};
} EXT_80000008_EAX_T;


/**
 * @brief
 *      Synthetic vendor numeric identifiers. See CPUID_INFO::uVendorId field.
 */
#define CPU_VENDOR_UNKNOWN	0
#define CPU_VENDOR_INTEL	1
#define CPU_VENDOR_AMD		2


/**
 * @brief
 *		Informaciya o podderzhivaemiih vozmozhnostyah processorpa
 */
typedef struct _CPUID_INFO
{
	UINT	uMaxCount;			/* maximum val to pass to CPUID instruction */
	UCHAR	szVendor[12 + 1];	/* 12 packed Vendor ID string bytes plus null */
	UCHAR	BrandString[48 + 1];

	union
	{
		UINT	uCell;
		struct
		{
			UINT	Stepping : 4;
			UINT	Model : 4;
			UINT	Family : 4;
			UINT	Type : 2;
			UINT	Reserved_15_14 : 2;
			UINT	ExtModel : 4;
			UINT	ExtFamily : 8;
			UINT	Reserved_31_28 : 4;
		};
	} Version;

	union
	{
		UINT	uCell;
		struct
		{
			UINT	BrandId : 8;
			UINT	ClflushCount : 8;
			UINT	CpuCount : 8;
			UINT	ApicId : 8;
		};
	} BrandId;

	union
	{
		UINT	uCell;
		struct
		{
			UINT	Sse3 : 1;
			UINT	Reserved_2_1 : 2;
			UINT	Monitor : 1;
			UINT	DsCpl : 1;
			UINT	Vmx : 1;
			UINT	Reserved_6 : 1;
			UINT	Est : 1;
			UINT	Tm2 : 1;
			UINT	Ssse3 : 1;
			UINT	CntxId : 1;
			UINT	Reserved_12_11 : 2;
			UINT	CmpXchg16B : 1;
			UINT	xTPRUpdateControl : 1;
			UINT	PDCM : 1;
			UINT	Reserved_18_16 : 3;
			UINT	Sse41 : 1;
			UINT	Sse42 : 1;
			UINT	Reserved_22_21 : 2;
			UINT	Popcnt : 1;
			UINT	Reserved_25_24 : 2;
			UINT	Xsave : 1;
			UINT	Osxsave : 1;
			UINT	Avx : 1;
			UINT	Reserved_31_29 : 3;
		};
	} ExtFeatures;

	union
	{
		UINT	uCell;
		struct
		{
			UINT	Fpu			: 1;
			UINT	Vme			: 1;
			UINT	De			: 1;
			UINT	Pse			: 1;
			UINT	Tsc			: 1;
			UINT	Msr			: 1;
			UINT	Pae			: 1;
			UINT	Mce			: 1;
			UINT	Cx8			: 1;
			UINT	Apic		: 1;
			UINT	Reserved_10 : 1;
			UINT	Sep			: 1;	// SYSENTER - SYSEXIT support
			UINT	Mtrr		: 1;
			UINT	Pge			: 1;
			UINT	Mca			: 1;
			UINT	Cmov		: 1;
			UINT	Pat			: 1;
			UINT	Pse36		: 1;
			UINT	Psn			: 1;
			UINT	Clfl		: 1;
			UINT	Reserved_20	: 1;
			UINT	Dtes		: 1;
			UINT	Acpi		: 1;
			UINT	Mmx			: 1;
			UINT	Fxsr		: 1;
			UINT	Sse			: 1;
			UINT	Sse2		: 1;
			UINT	Ss			: 1;
			UINT	Htt			: 1;
			UINT	Tm1			: 1;
			UINT	Ia64		: 1;
			UINT	Pbe			: 1;
		};
	} Features;

	struct
	{
		union
		{
			UINT uCell;
			struct
			{
				UINT VersionId	: 8;
				UINT PmcCount	: 8;
				UINT PmcWidth	: 8;
				UINT EventCount	: 8;
			};
		} Version;

		union
		{
			UINT uCell;
			struct
			{
				UINT CoreCycles	: 1;
				UINT InstRetire	: 1;
				UINT RefCycles	: 1;
				UINT CacheRefs	: 1;
				UINT CacheMiss	: 1;
				UINT BrnchRetire: 1;
				UINT BrnchMiss	: 1;
				UINT Reserved	: 25;
			};
		} Events;

		union
		{
			UINT uCell;
			struct
			{
				UINT Count	: 5;
				UINT Width	: 8;
				UINT Reserved	: 19;
			};
		} FixedPmc;

	} Perfmon;

	struct
	{
		UINT	sizeOfState;
	} SIMDState;

	UINT	uMaxExtCount;

	UINT	EXT_80000001_ECX;
	UINT	EXT_80000001_EDX;

	UINT	EXT_80000007_EDX;

	EXT_80000008_EAX_T EXT_80000008_EAX;

	union
	{
		UINT uCell;
		struct
		{
			UINT AperfMperf : 1;
			UINT Reserved_1_2 : 2;
			UINT PerfBias : 1;
			UINT Reserved_5_31 : 28;
		};
	} PowerManagement;
	UINT	EXT_00000007_EBX;
	UINT	EXT_0000000D_EAX;

	// Generic vendor numeric identifier gotten from szVendor
	UINT	uVendorId;

	// Real CPU Family calculated from Family and ExtFamily
	UINT Family;
	// Real CPU Model calculated from Model and ExtModel
	UINT Model;
} CPUID_INFO;

/**
 * @brief
 *		emulated features
 *	  moved from MonitorState.h
 */
typedef struct _CPU_FEATURES_MASKS
{
	UINT	CR4_MASK;
	UINT	FEATURES_MASK;
	UINT	EXT_FEATURES_MASK;
	UINT	EXT_80000001_ECX_MASK;
	UINT	EXT_80000001_EDX_MASK;

	UINT	EXT_80000007_EDX_MASK;
	EXT_80000008_EAX_T	EXT_80000008_EAX;

	UINT	EXT_00000007_EBX_MASK;
	UINT	EXT_0000000D_EAX_MASK;
}CPU_FEATURES_MASKS;


/*
* CPUID 1 EDX flags
*/

// 0 FPU sovmeshen s osnovniim processorom
#define F_FPU			(1u << 0)

// 1 VME: Rasshireniya Virtual-8086 Mode
#define F_VME			(1u << 1)

// 2 DE: Rasshireniya otladki (Debug Extensions) (preriivaniya I/O)
#define F_DE			(1u << 2)

// 3 PSE: Page Size Extensions
#define F_PSE			(1u << 3)

// 4 TSC: Time Stamp Counter
#define F_TSC			(1u << 4)

// 5 MSR: Podderzhka RDMSR i WRMSR
#define F_MSR			(1u << 5)

// [6:6]   PAE: Physical Address Extensions
#define F_PAE			(1u << 6)

// 7 MCE: Machine Check Exception
#define F_MCE			(1u << 7)

// 8 CX8: Instrukciya CMPXCHG8B
#define F_CX8			(1u << 8)

// 9 APIC: APIC na prcessore
#define F_APIC			(1u << 9)

// 11 SEP: Sysenter present (support fast SYSENTER/SYSEXIT)
#define F_SEP			(1u << 11)

// 12 MTRR: Memory Type Range Reg
#define F_MTRR			(1u << 12)

// 13 PGE/PTE Global Bit
#define F_PGE			(1u << 13)

// 14 MCA: Machine Check Architecture
#define F_MCA			(1u << 14)

// 15 CMOV: Cond Mov/Cmp Instructions
#define F_CMOV			(1u << 15)

// 16 PAT: Page Attribute table (podderzhka tablicii atributov stranic)
#define F_PAT			(1u << 16)

// 17 PSE-36: 36-bit physical addresses
#define F_PSE36			(1u << 17)

// 18 PN: Processor number support
#define F_PN			(1u << 18)

//19: support of CFLUSH instruction
#define F_CFLUSH		(1u << 19)

// 21 DS Storage support
#define F_DS			(1u << 21)

// 22 AMD MMX Extensions
#define F_AMD_MMX		(1u << 22)

// 23 MMX
#define F_MMX			(1u << 23)

// 24 FXSR: fast floating-point save/restore
#define F_FXSR			(1u << 24)

// 25 SSE support
#define F_SSE			(1u << 25)

// 26 SSE2 support
#define F_SSE2			(1u << 26)

// 27 Selfsnoop
#define F_SS			(1u << 27)

// 28 MTT: Multi-Threading Technology (former Hyper-Threading)
#define F_MTT			(1u << 28)

// 29 Automatic clock control
#define F_ACC			(1u << 29)

// 31 Pending break enable
#define F_PBE			(1u << 31)

/*
* CPUID 1 ECX flags
*/

// 0 SSE3
#define F_SSE3			(1u << 0)
// 1 PCLMULQDQ: Carryless Multiplication
#define F_PCLMULQDQ		(1u << 1)
// 2 DTE64: Debug Store 64bit records extention
#define F_DTE64			(1u << 2)
// 3 MONITOR and MWAIT instructions
#define F_MONITOR_MWAIT	(1u << 3)
// 4 DS-CPL: CPL Qualified debuf store
#define F_DSCPL			(1u << 4)
// 5 VMX
#define F_VMX			(1u << 5)
// 6 Safer mode
#define	F_SMX			(1u << 6)
// 7 Enhanced Speedstep
#define F_EST			(1u << 7)
// 8 Thermal monitor 2
#define F_TM2			(1u << 8)
// 9 SSSE3: Supplemental SSE3
#define F_SSSE3			(1u << 9)
// 12 FMA - fast multiply and add
#define F_FMA			(1u << 12)
// 13 CMPXCHG16B
#define F_CX16			(1u << 13)
// 14 Extended task priority messages
#define F_XTPR			(1u << 14)
// 15 Perfmon and Debug capabilities
#define F_PMDC			(1u << 15)
// 17 Process Context Identifiers
#define F_PCID			(1u << 17)
// 19 SSE4.1
#define F_SSE41			(1u << 19)
// 20 SSE4.2
#define F_SSE42			(1u << 20)
// 21 x2APIC
#define F_X2APIC		(1u << 21)
// 22 MOVBE Instruction
#define F_MOVBE			(1u << 22)
// 23 POPCNT
#define F_POPCNT		(1u << 23)
// 24 TSC Deadline timer
#define F_TSCDL			(1u << 24)
// 25 AES-NI: Advanced Encryption Standard New Instructions
#define F_AES			(1u << 25)
// 26 XSAVE
#define F_XSAVE			(1u << 26)
// 27 OSXSAVE
#define F_OSXSAVE		(1u << 27)
// 28 AVX
#define F_AVX			(1u << 28)
// 29 F16C: VCVTPS2PH/VCVTPH2PS instructions support
#define F_F16C			(1u << 29)
// 30 RDRAND
#define F_RDRAND		(1u << 30)
// 31 HYPERVISOR: Run under some hypervisor
#define F_HYPERVISOR		(1u << 31)

// SSE4
#define F_SSE4			(F_SSE41 | F_SSE42 | F_POPCNT)

// CPUID 6 EAX flags
// 0 Digital temperature sensor
#define F_DTHERM		(1u << 0)
// 2 APIC-Timer-always-running feature (ARAT)
#define F_ARAT			(1u << 2)
// 4 Power limit notifications
#define F_PLN			(1u << 4)
// 6 Package thermal management
#define F_PTM			(1u << 6)

/*
* CPUID 0x00000006 ECX flags
*/

// 0 Hardware Coordination Feedback capability ( presence of IA_32_APERF, IA_32_MPERF )
#define F_APERF_MPERF	(1u << 0)
// 3 Performance-energy bias capability
#define F_PERFBIAS	(1u << 3)

/*
* CPUID 0x80000001 ECX flags
*/

#define F_LAHF_64		(1u << 0)	// lahf/sahf support in 64 bit mode
#define F_SVM			(1u << 2)   // SVM support (AMD)
#define F_ABM			(1u << 5)	// LZCNT instruction (AMD)
#define F_SSE4A			(1u << 6)	// SSE4A (AMD)
#define F_SKINIT		(1u << 12)	// SKINIT + STGI support (AMD)
#define F_WDT			(1u << 13)  // watchdog timer support (AMD)


/*
* CPUID 0x80000001 EDX flags
*/

#define F_SYSCALL		(1u << 11)	// syscall/sysret in 64 bit mode
#define F_NX			(1u << 20)	// NX bit support
#define F_FFXSR			(1u << 25)	// Fast FXSAVE/FXRSTOR
#define F_1GBPG			(1u << 26)  // 1GB page support (AMD)
#define F_RDTSCP		(1u << 27)  // RDTSCP support
#define F_EM64T			(1u << 29)	// EM64t support
#define F_AMD_3DNOW_EXT (1u << 30)	// 30 AMD 3DNow Extensions
#define F_AMD_3DNOW		(1u << 31)	// 31 AMD 3DNow Instructions

#define IS_RDTSCP_AVAILABLE() (MonState.FeaturesInfo.EXT_80000001_EDX_MASK & F_RDTSCP)

/*
* CPUID 0x80000007 EDX flags
*/
#define F_TSC_INVARIANT	(1u << 8)	// invariant TSC rate

/*
 * CPUID 0x00000007 EBX flags
 * Structured Extended Feature Flags Enumeration Leaf
 */
#define F_RDFSGSBASE (1u << 0)	// Supports RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE instructions
#define F_ADJTSC	(1u << 2)	// MSR IA32_TSC_ADJUST is supported
#define F_BMI1		(1u << 3)	// BMI1 support (VEX GPR)
#define F_HLE		(1u << 4)	// XACQUIRE/XRELEASE recongnized as instructions, not prefixes
#define F_AVX2		(1u << 5)	// AVX2 instructions support
#define F_SMEP		(1u << 7)	// Supervisor Mode Execution Protection
#define F_BMI2		(1u << 8)	// BMI2 support (VEX GPR)
#define F_ERMSB		(1u << 9)	// Enhanced REP MOVSB/STOSB
#define F_INVPCID	(1u << 10)	// Supports INVPCID instruction
#define F_RTM		(1u << 11)	// RTM support (XBEGIN/XEND/XABORT/XTEST)
#define F_QM		(1u << 12)	// Quality monitoring
#define F_FPUCS		(1u << 13)	// Deprecated FPU CS/DS fields
#define F_RDSEED	(1u << 18)	// RDSEED instruction support
#define F_ADX		(1u << 19)	// ADCX/ADOX instruction support


/*
* CPUID 0x0000000D EAX flags
* Structured Extended Feature Flags Enumeration Leaf
*/
#define F_SAVEOPT	(1u << 0)	// XSAVEOPT supported

// Opisanie strukturii adresa dlya dlinnogo viizova.
typedef	struct TAG_FAR_CALL_PTR
{
	UINT uOffset;
	unsigned short wSelector;
} FAR_CALL_PTR;
typedef FAR_CALL_PTR* PFAR_CALL_PTR;


// Opisanie strukturii adresa dlya dlinnogo viizova.
typedef	struct TAG_FAR_CALL_64_PTR
{
	ULONG64 uOffset;
	unsigned short wSelector;
} FAR_CALL_64_PTR;
typedef FAR_CALL_64_PTR* PFAR_CALL_64_PTR;



// 133 MHZ Bus clock
#define APIC_BUS_SPEED		66

/**
 * APIC default base address.
 */
#define APIC_BASE_ADDRESS	0xFEE00000

#define APIC_BASE_ENABLE	0x00000800
#define APIC_BASE_ENABLE_X2	0x00000400

#define APIC_ID_REG_OFFS			0x20
#define APIC_VER_REG_OFFS			0x30
#define APIC_EOI_REG_OFFS			0xB0
#define APIC_ERROR_STATUS_OFFS		0x280
#define	APIC_ICR_LOW_OFFS			0x300
#define	APIC_ICR_HI_OFFS			0x310
#define APIC_TPR_OFFS				0x80

/**
 * APIC Local Vector Table Register offsets.
 */
#define APIC_LVT_ID					0x20
#define APIC_LVT_VERSION			0x30
#define APIC_LVT_TIMER_REG_OFFS		0x320
#define APIC_LVT_THERMAL_REG_OFFS	0x330
#define APIC_LVT_PERFMON_REG_OFFS	0x340
#define APIC_LVT_LINT0_REG_OFFS		0x350
#define APIC_LVT_LINT1_REG_OFFS		0x360
#define APIC_LVT_ERR_REG_OFFS		0x370

/**
 * Utility macro to read values.
 */
#define APIC_LVT_DELVMODE_NMI	(0x400)
#define APIC_LVT_DELVMODE_FIXED	(0x000)
#define APIC_LVT_DELVMODE(val)	(val & 0x700)

#define APIC_LVT_MASK			(0x10000)
#define APIC_LVT_ISMASKED(val)  (val & APIC_LVT_MASK)

/**
 * @brief
 *		MSR register to control Local APIC
 */
typedef union _ia32_apic_base_msr_
{
	/**
	 * @brief Definition by fields
	 */
	struct __fields__
	{
		// Reserved
		ULONG64 Res1 :8;
		// Bootstrap processor
		ULONG64 BSP    :1;
		// Reserved
		ULONG64 Res2   :2;
		// Is local APIC enabled
		ULONG64 Enable :1;
		// APIC base
		ULONG64 Base   :20;
		// Reserved
		ULONG64 Res3   :32;
	} Fields;
	// Raw value
	ULONG64 Raw;
} IA32_APIC_BASE_MSR;

/**
 * @brief
 *		APIC ID register description
 */
typedef union _apic_id_
{
	/**
	 * @brief Definition by fields
	 */
	struct
	{
		// Reserved
		UINT  Res : 24;
		// ID
		UINT  ID  : 8;
	} Fields;
	// Raw value
	UINT Raw;
} APIC_ID;

/**
 * @brief
 *		APIC version register
 */
typedef union _apic_version_register_
{
	/**
	 * @brief Definition by fields
	 */
	struct
	{
		// Version
		UINT Version:8;
		// Reserved
		UINT Res1	:8;
		// Maximum entries in vector table
		UINT MaxLVT	:8;
		// Reserved
		UINT Res2	:8;
	} Fields;
	// Raw value
	UINT Raw;
} APIC_VER;

// Delivery mode
#define DELIVERY_FIXED		0x0
#define DELIVERY_LOWEST_PRI	0x1
#define DELIVERY_SMI		0x2
#define DELIVERY_REMOTE_READ	0x3
#define DELIVERY_NMI		0x4
#define DELIVERY_INIT		0x5
#define DELIVERY_STARTUP	0x6
#define DELIVERY_ExtINT		0x7

#define APIC_REG_DELIVERY_MODE(val) (((val) >> 8) & 7)

#define LAPIC_TIMER_ONESHOT  (0x00)
#define LAPIC_TIMER_PERIODIC (0x01)

#define LAPIC_TIMER_MODE(TimerReg) (((TimerReg) >> 17) & 3)

/**
 * @brief
 *		Local vector table entry
 */
typedef union _local_vector_table_entry_
{
	/**
	 * @brief Definition by fields
	 */
	struct
	{
		// Associated vector
		UINT Vector         : 8;
		// Delivery mode
		UINT DeliveryMode	: 3;
		// Reserved
		UINT Res1			: 1;
		// Delivery status
		UINT DeliverySts	: 1;
		// Interrupt polarity
		UINT IntPolarity	: 1;
		// Remote IRR
		UINT RemoteIRR		: 1;
		// Trigger mode (level, edge)
		UINT TriggerMode	: 1;
		// Is masked
		UINT Mask			: 1;
		// 1 = periodic, 0 - one-shot
		UINT Periodic		: 1;
		// Reserved
		UINT Res2			: 14;
	} Fields;
	// Raw value
	UINT Raw;
} LVT_ENTRY;
typedef LVT_ENTRY* PLVT_ENTRY;

/**
 * @brief
 *		Local vectors table
 */
typedef struct _LVT_table_
{
	// Timer register
	LVT_ENTRY Timer;
	// Thermal monitor
	LVT_ENTRY ThermalSens;
	// Performance counter
	LVT_ENTRY PerfMonCounter;
	// Interrupts from pin 0
	LVT_ENTRY LINT0;
	// Interrupts from pin 1
	LVT_ENTRY LINT1;
	// Error register
	LVT_ENTRY Error;
} LVT_TABLE;
typedef LVT_TABLE* PLVT_TABLE;

/**
 * @brief
 *		APIC Error status register
 */
typedef union _apic_error_status_register_
{
	/**
	 * @brief Definition by fields
	 */
	struct
	{
		// Send checksum error
		UINT SendChecksum		: 1;
		// Receive checksum error
		UINT RcvChecksum		: 1;
		// Send accept error
		UINT SendAccept			: 1;
		// Receive accept error
		UINT RcvAccept			: 1;
		// Reserved
		UINT Res1				: 1;
		// Send illegal vector
		UINT SendIllegalVector	: 1;
		// Receive illegal vector
		UINT RcvIllegalVector	: 1;
		// Illegar register address
		UINT IllegalRegAddr		: 1;
		// Reserved
		UINT Res2				: 24;
	} Fields;
	// Raw value
	UINT Raw;
} ERR_STATUS;

#define APIC_ERR_SEND_CHKSUM			(1 << 0)
#define APIC_ERR_RECV_CHKSUM			(1 << 1)
#define APIC_ERR_SEND_ACCEPT			(1 << 2)
#define APIC_ERR_RECV_ACCEPT			(1 << 3)
#define APIC_ERR_RESERVED1				(1 << 4)
#define APIC_ERR_SEND_ILLEGAL_VECTOR	(1 << 5)
#define APIC_ERR_RECV_ILLEGAL_VECTOR    (1 << 6)
#define APIC_ERR_ILLEGAL_REG_ADDR  (1 << 7)

// Divide value
#define DIVIDE_BY_1			1011b
#define DIVIDE_BY_2			0000b
#define DIVIDE_BY_4			0001b
#define DIVIDE_BY_8			0010b
#define DIVIDE_BY_16		0011b
#define DIVIDE_BY_32		1000b
#define DIVIDE_BY_64		1001b
#define DIVIDE_BY_128		1010b

#define APIC_ILLEGAL_SPEED	0xFFFFFFFF

/**
 * @brief
 *		Divide configuration register
 */
typedef union _divide_configuration_reg_
{
	/**
	 * @brief Definition by fields
	 */
	struct
	{
		// Divide value. Note - bit2 is always 0
		UINT DivideValue	:4;
		// Reserved
		UINT Res			:28;
	} Fields;
	// Raw value
	UINT Raw;
} DIVIDE_CONF;

#define APIC_ICR_PENDING	0x01000

/**
 * @brief
 *		Interrupt command register
 */
typedef union _interrupt_command_register_
{
	/**
	 * @brief Definition by fields
	 */
	struct
	{
		// Associated vector
		UINT32 Vector			: 8;
		// Delivery mode
		UINT32 DeliveryMode		: 3;
		// Destination mode 0-phys, 1-logical
		UINT32 DestMode			: 1;
		// Delivery status
		UINT32 DeliverySts		: 1;
		// Reserved
		UINT32 Res1				: 1;
		// Level 0 - deassert, 1 - assert
		UINT32 Level			: 1;
		// Trigger mode (level, edge)
		UINT32 TriggerMode		: 1;
		// Reserved
		UINT32 Res2				: 2;
		// Destination Shorthand
		UINT32 DestShort		: 2;
		// Reserved
		UINT32 Res3				: 12;
		UINT32 res1[3];
		union {
			struct {
				// Reserved
				UINT32 Res4		: 24;
				// Destination
				UINT32 Dest		: 8;
			};
			// x2APIC Destination
			UINT32 x2Dest;
		};
		UINT32 res2[3];
	} Fields;
	struct
	{
		UINT32 Lo;
		UINT32 res1[3];
		UINT32 Hi;
		UINT32 res2[3];
	};
} APIC_INTERRUPT_CMD;

#define SET_ICR_VECTOR(raw_icr, vector) \
	do { \
		raw_icr &= ~0xffULL; \
		raw_icr |= vector & 0xffULL; \
	} while (0)

#define SET_X2APIC_ICR_DEST(raw_icr, apic_id) \
	do { \
		raw_icr &= ~(0xffffffffULL << 32); \
		raw_icr |= (((ULONG64)(apic_id & 0xffffffffULL)) << 32); \
	} while (0)

#define SET_XAPIC_ICR_DEST(raw_icr, apic_id) \
	do { \
		raw_icr &= ~(0xffULL << 56); \
		raw_icr |= (((ULONG64)(apic_id & 0xffULL)) << 56); \
	} while (0)

#define GET_XAPIC_ICR_HI(raw_icr)  ((UINT32)(raw_icr >> 32))

#define GET_XAPIC_ICR_LO(raw_icr)  ((UINT32)raw_icr)


/**
 * @brief
 *		Logical destination register
 */
typedef union _logical_destination_regitster_
{
	/**
	 * @brief Definition by fields
	 */
	union
	{
		// Flat model
		struct
		{
			UINT Res	: 24;
			UINT LogID	: 8;
		} Flat;

		// Cluster model
		struct
		{
			UINT Res		: 24;
			UINT ApicId		: 4;
			UINT Id			: 4;
		} Cluster;
	}
	Fields;
	// Raw value
	UINT Raw;
} LOGICAL_DEST;

/**
 * @brief
 *		Divide configuration register
 */
typedef union _destination_format_regitster_
{
	/**
	 * @brief Definition by fields
	 */
	struct
	{
		UINT Res : 28;
		UINT Model : 4;
	} Fields;
	// Raw value
	UINT Raw;
} DEST_FORMAT;

/**
 * @brief
 *		Priority registers
 */
typedef union _priority_register_
{
	/**
	 * @brief Definition by fields
	 */
	struct
	{
		UINT SubClass	: 4;
		UINT Class		: 4;
		UINT Res		: 24;
	} Fields;
	// Raw value
	UINT Raw;
} PRIO_REG;

#define APIC_INTERRUPT_REG_ENTRIES_COUNT 8
/**
 * @brief
 *		256 bit registers (IMR, ISR, TMR)
 */
typedef struct __long_registers__
{
	struct {
		UINT32 Entry;
		UINT32 res[3];
	} Array[APIC_INTERRUPT_REG_ENTRIES_COUNT];
} APIC_INTERRUPT_REG;

/**
 * @brief
 *		Spurious INT
 */
typedef union __spurious_interrupt__
{
	/**
	 * @brief Definition by fields
	 */
	struct
	{
		// Spurious vector
		UINT Vector		:8;
		// APIC software Enable/Disable
		UINT Enabled	:1;
		// Focus processor checking
		UINT Focus		:1;
		// Pad
		UINT Res		:22;
	} Fields;
	// Raw value
	UINT Raw;
} SPURIOUS_REG;

// IOAPIC definitions

#define IOAPIC_BASE_ADDRESS   0xFEC00000

// IOAPIC memory-mapped registers
#define IOAPIC_REG_SELECT 0x0
#define IOAPIC_REG_WINDOW 0x10

// IOAPIC indirect registers
#define IOAPIC_REG_ID		0x0
#define IOAPIC_REG_VER		0x1
#define IOAPIC_REG_ARB		0x2
#define IOAPIC_REG_REDIRETION_TABLE 0x10

/**
 * @brief
 *		IO APIC register select
 */
typedef union _ioapic_regsel_
{
	struct
	{
		UINT AddrReg : 8;
		UINT Res	 : 24;
	} PACKED Fields;
	UINT Raw;
} PACKED IOAPIC_REGSEL;
#include <Interfaces/unpacked.h>

#include "Ia32e.h"

#include <Interfaces/packed.h>
#include <Interfaces/unpacked.h>

/**
 *	MSI Address format
 */
#define MSIADDR_DEST_MODE		(1 << 2)
#define MSIADDR_REDIR_HINT		(1 << 3)
#define MSIADDR_REDIR_AND_DEST	(MSIADDR_DEST_MODE | MSIADDR_REDIR_HINT)

#define MSIADDR_DEST_ID_MASK	(0x000ff000)
#define MSIADDR_DEST_FROM_ADDR(addr) (((addr) >> 12) & 0xff)

// Magic MSI Address constant
#define MSI_MAGIC_CONST 0xFEE

/**
 *	MSI message format
 */
#define MSIMSG_TRIGER_MODE		(1 << 15)
#define MSIMSG_DELIVMODE_FROM_DATA(data) (((data) >> 8) & 7)
#define MSIMSG_VEC_FROM_DATA(data) ((data) & 0xff)
#define MSIMSG_IS_LEVEL(data) ((data & 0x400) != 0)

#define MAX_INSTR_LEN 16

// Monitor line size in bytes (MONITOR/MWAIT)
#define MONITOR_LINE_SIZE PAGE_SIZE

// number of DR registers in x86 CPU
#define NUM_REAL_DEBUG_REGS 4

#endif // __IA32_H__
