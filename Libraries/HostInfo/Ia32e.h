//////////////////////////////////////////////////////////////////////////
///
/// @file Ia32e.h
///
/// @brief Description of main processor structures available in long-mode
///
/// @author anyav
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

#ifndef __IA32E_H__
#define __IA32E_H__

#include <prlcommon/Interfaces/packed.h>


/**
* @brief
*		Simovlicheskie nazvaniya polya Type v system-segment &
*		gate-descriptors.
*/
enum SYSTEM_SEGMENT_AND_GATE_DESCRIPTORS_TYPES_ENUM64
{
	SSGD64_RESERVED0				= 0x0,
	SSGD64_RESERVED1		        = 0x1,
	SSGD64_LDT					    = 0x2,
	SSGD64_RESERVED3			    = 0x3,
	SSGD64_RESERVED4		        = 0x4,
	SSGD64_RESERVED5				= 0x5,
	SSGD64_RESERVED6			    = 0x6,
	SSGD64_RESERVED7		        = 0x7,
	SSGD64_RESERVED8				= 0x8,
	SSGD64_64BIT_TSS_AVAIL		    = 0x9,
	SSGD64_RESERVED10			    = 0xa,
	SSGD64_64BIT_TSS_BUSY			= 0xb,
	SSGD64_64BIT_CALL_GATE		    = 0xc,
	SSGD64_RESERVED13			    = 0xd,
	SSGD64_64BIT_INT_GATE			= 0xe,
	SSGD64_64BIT_TRAP_GATE		    = 0xf,
};


/**
 * @brief
 *      system descriptor in long mode
 *      (in GDT,IDT - trap-, int-, call- gate descriptor)
 */
typedef struct _DESCRIPTOR_SYS64
{
    USHORT	wLimitLo;												// limit 0..15
    USHORT	wBaseLo;												// base  0..15
    UCHAR	cBaseCentral;											// base  16..23
    USHORT	Type : 5;												// S/type field
    USHORT	Dpl : 2;												// DPL
    USHORT	P : 1;													// present
    USHORT	LimitHi4 : 4;											// limit 16..19
    USHORT	Avl : 1;												// Available for use by system software
    USHORT	Lb : 1;													// LongMode desc
    USHORT	Db : 1;													// D/B
    USHORT	G : 1;													// granularity
    UCHAR	cBaseHi;												// base 24..31
    ULONG	uBaseHi64;
    ULONG	uReserved;
} DESCRIPTOR_SYS64;



/**
 * @brief
 *		Poluchenie bazii sistemnogo deskriptora dlinou v 16-bajt
 *
 * @params
 *		Descr	[in]	DESCRIPTOR_SYS64
 *
 * @author
 *		AnyaV
 */
#define GET_DESCRIPTOR_BASE64(Descr)			\
	(((ULONG64)(Descr).wBaseLo) |				\
	(((ULONG64)(Descr).cBaseCentral) << 16) |	\
	(((ULONG64)(Descr).cBaseHi) << 24) |		\
	(((ULONG64)(Descr).uBaseHi64) << 32))


/**
 * @brief
 *		Creating of system-descriptor for 64-bit mode
 *
 * @params
 *		Descr[out]	desktriptor (DESCRIPTOR_SYS64)
 *		B	[in]	baza (8 byte long)
 *		L	[in]	predel
 *		D_G	[in]
 *		DB	[in]
 *		AVL	[in]
 *		D_P	[in]	prisutstvie
 *		DPL [in]	uroven' privilegij
 *		T	[in]	tip
 *		L_FL [IN]   LongMode ( in all other modes always must be zero )
 *
 * @author
 *		MichaelEr, AlexKo
 *
 *      segment descriptors and system descriptors are the same size in legacy mode
 *      but in long mode since segment descriptors are mostly ignored
 *      they are only 4-bytes long, while system-descriptors extended to 8bytes
 */
#define GATHER_DESCRIPTOR_SYS64(Descr, B, LIM, D_G, DB, AVL, D_P, DPL, T, L_FB)\
{																\
    (Descr).uBaseHi64 = (B) >> 32;								\
    (Descr).cBaseHi = ((B) >> 24) & 0xff;						\
    (Descr).cBaseCentral = ((B) >> 16) & 0xff;					\
    (Descr).wBaseLo = (B) & 0xffff;								\
    (Descr).LimitHi4 = ((LIM) & 0xf0000) >> 16;					\
    (Descr).wLimitLo = ((LIM) & 0x0ffff);						\
    (Descr).G = (D_G);											\
    (Descr).Db = (DB);											\
    (Descr).uReserved = 0;										\
    (Descr).Lb = (L_FB);										\
    (Descr).Avl = (AVL);										\
    (Descr).P = (D_P);											\
    (Descr).Dpl = (DPL);										\
    (Descr).Type = (T);											\
	}


/**
 * @brief
 *      64 call-gate descriptor
 */
typedef struct _CALL_GATE_DESCRIPTOR64
{
    // Mladshee slovo smesheniya tochki vhoda v proceduru obrabotki
    USHORT		wLoOffset;

    // Selektor tochki vhoda v proceduru obrabotki
    SELECTOR	Selector;

    // Neispol'zuemiij bajt
    UCHAR		cReserved;

    // Tip shluza
    UCHAR		Type : 5;

    // Trebuemiij uroven' privilegij dlya viisova shluza
    UCHAR		Dpl : 2;

    // Priznak prisutstviya obrabotchika v pamyati
    UCHAR		P : 1;

    // Starshee slovo smesheniya tochki vhoda v proceduru obrabotki
    USHORT		wHiOffset;

    // Starshee dvojnoe slovo smesheniya tochki vhoda v proceduru obrabotki
    UINT        uTopOffset;

    // Reserved
    UINT        uReserved96_128;

} CALL_GATE_DESCRIPTOR64;

/**
 * @brief
 *       gather descriptor of 64-bit call-gate
 */
#define GATHER_CALL_GATE_DESCRIPTOR_64(Descr, S, O, I_P, DPL, D)	        \
	{																		\
		Descr.Selector = (S);												\
		Descr.uTopOffset = (O) >> 32;										\
		Descr.wHiOffset = ((O) >> 16)&0xffff;								\
		Descr.wLoOffset = (O) & 0xffff;										\
		Descr.Type = ((D) << 3) | 0x4;										\
		Descr.Dpl = (DPL);													\
		Descr.P = (I_P);													\
	}



/**
 * @brief
 *      64- interrupt-gate and 64- trap-gate descriptor
 *
 *      must be 16-bytes long
 */
typedef struct _GATE_DESCRIPTOR_STRUCT_64
{
    // Mladshee slovo smesheniya tochki vhoda v proceduru obrabotki
    USHORT		wLoOffset;

    // Selektor tochki vhoda v proceduru obrabotki
    SELECTOR	Selector;

    // IST field
    UCHAR       Ist : 3;

    // Neispol'zuemiij
    UCHAR		cReserved35_39 : 5;

    // Tip shluza
    UCHAR		Type : 5;

    // Trebuemiij uroven' privilegij dlya viisova shluza
    UCHAR		Dpl : 2;

    // Priznak prisutstviya obrabotchika v pamyati
    UCHAR		P : 1;

    // Starshee slovo smesheniya tochki vhoda v proceduru obrabotki
    USHORT		wHiOffset;

    // Starshee dvojnoe slovo smesheniya tochki vhoda v proceduru obrabotki
    UINT        uTopOffset;

    // Reserved
    UINT        uReserved96_128;

} GATE_DESCRIPTOR_STRUCT_64;

/**
 * @brief
 *    gather interrupt- gate descriptor
 */
#define GATHER_INTERRUPT_GATE_DESCRIPTOR_64(Descr, S, O, I_P, DPL, IST)	\
	{																	\
		Descr.Selector.wCell = (S);										\
		Descr.uTopOffset = (O) >> 32;									\
		Descr.wHiOffset = ((O) >> 16)&0xffff;							\
		Descr.wLoOffset = (O) & 0xffff;									\
		Descr.cReserved35_39 = 0;										\
		Descr.Type = SSGD64_64BIT_INT_GATE;								\
		Descr.Dpl = (DPL);												\
		Descr.P = (I_P);												\
		Descr.Ist = (IST);												\
		Descr.uReserved96_128 = 0;                                      \
	}



/**
 * @brief
 *    gather trap- gate descriptor
 */
#define GATHER_TRAP_GATE_DESCRIPTOR_64(Descr, S, O, I_P, DPL, IST)		\
	{																	\
		Descr.Selector.wCell = (S);								\
		Descr.uTopOffset = (O) >> 32;									\
		Descr.wHiOffset = ((O) >> 16)&0xffff;							\
		Descr.wLoOffset = (O) & 0xffff;									\
		Descr.cReserved35_39 = 0;										\
		Descr.Type = SSGD64_64BIT_TRAP_GATE;							\
		Descr.Dpl = (DPL);												\
		Descr.P = (I_P);												\
		Descr.Ist = (IST);												\
		Descr.uReserved96_128 = 0;                                      \
	}


static __inline const char* GetSegmentGateName64( UINT uType )
{
	switch ( uType )
	{
		case SSGD64_LDT:					return "ldt";
		case SSGD64_64BIT_TSS_AVAIL:		return "64_bit_tss_avail";
		case SSGD64_64BIT_TSS_BUSY:		    return "64_bit_tss_busy";
		case SSGD64_64BIT_CALL_GATE:		return "64_bit_call_gate";
		case SSGD64_64BIT_INT_GATE:		    return "64_bit_int_gate";
		case SSGD64_64BIT_TRAP_GATE:		return "64_bit_trap_gate";
		case SSGD64_RESERVED0:
		case SSGD64_RESERVED1:
		case SSGD64_RESERVED3:
		case SSGD64_RESERVED4:
		case SSGD64_RESERVED5:
		case SSGD64_RESERVED6:
		case SSGD64_RESERVED7:
		case SSGD64_RESERVED8:
		case SSGD64_RESERVED10:
		case SSGD64_RESERVED13:
		     return "reserved";

	}

	return "unknown";
}

/**
 * @brief
 *      calculate offset for 64-bit call/int/trap gate
 *
 * @params
 *      Desc - [in] pointer at descriptor
 *
 * @return
 *      ULONG64 - offset
 */
#define GATHER_GATE64_OFFSET(pDesc) (										\
	( (ULONG64)(((GATE_DESCRIPTOR_STRUCT_64*)(pDesc))->wLoOffset) ) |		\
	( (ULONG64)(((GATE_DESCRIPTOR_STRUCT_64*)(pDesc))->wHiOffset) << 16 ) |	\
	( (ULONG64)(((GATE_DESCRIPTOR_STRUCT_64*)(pDesc))->uTopOffset) << 32 ) )


/**
 * @brief
 *      TSS for long mode
 *
 * COMMENTS
 *      TSS_64 must be 0x68 bytes
 */
typedef struct _TSS_64
{
    UINT    uReserved0_3;

    // Full 64-bit canonical forms of RSP for priveledge levels 0 though 2
    UINT64  StackSwitch[3];

	// "IST0" - reserved, since IST_indx == 0 is used
	// for specifying legacy scheme of task-switch usage
#define IST_LEGACY	0
#define IST_DF		1

    // The full 64-bit canonical forms of the interrupt-stack-table (IST) pointers
    UINT64  u64Ist[8];

    UINT64  uReserved5C_63;
    USHORT  uReserved64_65;

    // 16-bit offset to the I/O-permission bit map from the 64-bit TSS base
    USHORT  uIoPermissionBmpAddr;

}TSS_64;



/**
 * @brief
 *      extended feature enable MSR.
 */
typedef union _EFER_MSR
{
    UINT64  uCell;

    struct
    {
        // syscall enable
        UINT64 Sce            : 1;  // 0

        UINT64 Reserved_1_7   : 7;  //1..7

        // IA32e enable
        UINT64 Lme            : 1;  // 8

        UINT64 Reserved_9     : 1;  // 9

        // IA32e active
        UINT64 Lma            : 1;  // 10

        // execution disable bit
        UINT64 Nxe            : 1;  // 11

        // SVM enable bit
        UINT64 Svme           : 1;  // 12

        UINT64 Reserved_13    : 1;  // 13

        UINT64 Ffxsr          : 1;  // 14

        UINT64 Reserved_15_63 : 49;
    };

}EFER_MSR;


#define MAX_VIRTADDR_BITS (48)
#define MAX_VIRTADDR_HIGHESTBIT (UL64(1) << (MAX_VIRTADDR_BITS-1))
#define MAX_VIRTADDR_BITMASK (~(MAX_VIRTADDR_HIGHESTBIT-1))

//
// Check if the address in the canonical form
//
#define IS_ADDR_CANONICAL(addr)     \
    ( (((UINT64)addr) & MAX_VIRTADDR_BITMASK) == 0 || \
      (((UINT64)addr) & MAX_VIRTADDR_BITMASK) == MAX_VIRTADDR_BITMASK )


#define CANONIZE_ADDR(uLinAddr) \
	( (MAX_VIRTADDR_HIGHESTBIT & (uLinAddr)) ? \
	(uLinAddr) | MAX_VIRTADDR_BITMASK : \
	(uLinAddr) & ~MAX_VIRTADDR_BITMASK)


// EFER flags masks
#define EFER_SCE	(1 << 0)
#define EFER_LME	(1 << 8)
#define EFER_LMA	(1 << 10)
#define EFER_NXE	(1 << 11)
#define EFER_FFXSR	(1 << 14)

// XFEATURE_ENABLED_MASK bits
#define XFEATURE_X87_MMX	(1u << 0)
#define XFEATURE_SSE		(1u << 1)
#define XFEATURE_AVX		(1u << 2)

// maximum of supported features in SIMD state
#define XFEATURE_SUPPORTED	(XFEATURE_X87_MMX | XFEATURE_SSE | XFEATURE_AVX)

#include <prlcommon/Interfaces/unpacked.h>


#endif//__IA32E_H__
