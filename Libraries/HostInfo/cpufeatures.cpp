///
/// Originated by mnestratov@
///
/// Copyright (c) 2010-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///

#include <prlcommon/Interfaces/ParallelsTypes.h>
#include <prlcommon/Interfaces/ParallelsCompiler.h>

#include "cpufeatures.h"

#include <prlcommon/Logging/Logging.h>
#include "Interfaces/Config.h"

#include <prlcommon/HostUtils/HostUtils.h>
#define CpuId(uEax, uEbx, uEcx, uEdx) HostUtils::GetCpuid(uEax, uEcx, uEdx, uEbx)

#ifdef _WIN_
// warning C4793: 'vararg' : causes native code generation for function 'int LOG_MESSAGE(int,...)'
#pragma warning( disable : 4793 )
#define snprintf _snprintf
#endif

//
// Description:
//	The function reads host CPU features as is
//	and fills CPUID_INFO structure
//
// Return value:
// @return
//		0 - in case of failure
//		1 - in case of success
//
int GetHostCpuFeatures(CPUID_INFO* cpu_info)
{
	UINT	uEAX, uECX, uEDX, uEBX;

	memset(cpu_info, 0, sizeof(*cpu_info));

	//////////////////////////////////////////////////////////////////////////
	//
	// Get signature and processor features
	//
	//////////////////////////////////////////////////////////////////////////

	// Get maximum input value for basic CPUID information
	uEAX = 0;
	uECX = 0;
	CpuId(uEAX, uEBX, uECX, uEDX);
	if (uEAX < 1){
		WRITE_TRACE(DBG_FATAL,
			"CPUID validation error: Insufficient capabilities (0x%x)",	uEAX);
		return 0;
	}
	cpu_info->uMaxCount = uEAX;

	// Copy processor vendor signature
	*(UINT *) &cpu_info->szVendor[0] = uEBX;
	*(UINT *) &cpu_info->szVendor[4] = uEDX;
	*(UINT *) &cpu_info->szVendor[8] = uECX;
	cpu_info->szVendor[12] = 0;

	if (memcmp(cpu_info->szVendor, "GenuineIntel", 12) == 0)
		cpu_info->uVendorId = CPU_VENDOR_INTEL;
	else if (memcmp(cpu_info->szVendor, "AuthenticAMD", 12) == 0)
		cpu_info->uVendorId = CPU_VENDOR_AMD;
	else{

		cpu_info->uVendorId = CPU_VENDOR_UNKNOWN;
		WRITE_TRACE(DBG_FATAL, "Unsupported CPU vendor uEBX=0x%x, uEDX=0x%x, uECX=0x%x", uEBX, uEDX, uECX);
		return 0;
	}

	uEAX = 1;
	uECX = 0;
	CpuId(uEAX, uEBX, uECX, uEDX);
	cpu_info->Version.uCell = uEAX;
	cpu_info->BrandId.uCell = uEBX;
	cpu_info->Features.uCell = uEDX;
	cpu_info->ExtFeatures.uCell = uECX;

	// Read Performance Monitoring leaf
	if (cpu_info->uMaxCount >= 0x0a)
	{
		uEAX = 0xa;
		uECX = 0;
		CpuId(uEAX, uEBX, uECX, uEDX);
		cpu_info->Perfmon.Version.uCell = uEAX;
		cpu_info->Perfmon.Events.uCell = uEBX;
		if (cpu_info->Perfmon.Version.VersionId > 1)
			cpu_info->Perfmon.FixedPmc.uCell = uEDX;
	}


	if (cpu_info->uMaxCount >= 0x07){
		uEAX = 0x07;
		uECX = 0;
		CpuId(uEAX, uEBX, uECX, uEDX);
		cpu_info->EXT_00000007_EBX = uEBX;
	}else{
		cpu_info->EXT_00000007_EBX = 0;
	}

	// according to Intel docs, by default FPU/SIMD state can be 512 byte only
	cpu_info->SIMDState.sizeOfState = 512;

	if (cpu_info->uMaxCount >= 0x0d)
	{
		uEAX = 0x0d;
		uECX = 0;
		CpuId(uEAX, uEBX, uECX, uEDX);
		cpu_info->SIMDState.sizeOfState = uEBX;
		uEAX = 0x0000000D;
		uECX = 0x00000001;
		CpuId(uEAX, uEBX, uECX, uEDX);
		cpu_info->EXT_0000000D_EAX = uEAX;
	}

	// Get maximum input value for extended CPUID information
	uEAX = 0x80000000;
	uECX = 0;
	CpuId(uEAX, uEBX, uECX, uEDX);
	cpu_info->uMaxExtCount = uEAX;

	if (cpu_info->uMaxExtCount >= 0x80000001)
	{
		uEAX = 0x80000001;
		uECX = 0;
		CpuId(uEAX, uEBX, uECX, uEDX);
		cpu_info->EXT_80000001_ECX = uECX;
		cpu_info->EXT_80000001_EDX = uEDX;
	}

	if (cpu_info->uMaxExtCount >= 0x80000004)
	{
		UINT i, uEax, uEbx, uEcx, uEdx;
		UCHAR* p = cpu_info->BrandString;

		for (i = 0x80000002; i <= 0x80000004; i++, p += 4 * 4)
		{
			uEax = i;
			uEcx = 0;
			CpuId(uEax, uEbx, uEcx, uEdx);
			*(UINT*) &p[0] = uEax;
			*(UINT*) &p[4] = uEbx;
			*(UINT*) &p[8] = uEcx;
			*(UINT*) &p[12] = uEdx;
		}
	}

	if (cpu_info->uMaxExtCount >= 0x80000007){
		uEAX = 0x80000007;
		uECX = 0;
		CpuId(uEAX, uEBX, uECX, uEDX);
		cpu_info->EXT_80000007_EDX = uEDX;
	}

	if (cpu_info->uMaxExtCount >= 0x00000006)
	{
		uEAX = 0x00000006;
		uECX = 0;
		CpuId(uEAX, uEBX, uECX, uEDX);
		cpu_info->PowerManagement.uCell = uECX;
	}

	if (cpu_info->uMaxExtCount >= 0x80000008){

		uEAX = 0x80000008;
		uECX = 0;
		CpuId(uEAX, uEBX, uECX, uEDX);

		cpu_info->EXT_80000008_EAX.uCell = uEAX;
	}

	cpu_info->Family = cpu_info->Version.Family;
	cpu_info->Model = cpu_info->Version.Model;
	if (cpu_info->uVendorId == CPU_VENDOR_INTEL)
	{
		if (cpu_info->Version.Family == 0x0F)
			cpu_info->Family += cpu_info->Version.ExtFamily;

		if (cpu_info->Version.Family == 0x06 || cpu_info->Version.Family == 0x0F)
			cpu_info->Model += (cpu_info->Version.ExtModel << 4);
	}
	else // if CPU_VENDOR_AMD
	{
		if (cpu_info->Version.Family >= 0x0F)
		{
			cpu_info->Family += cpu_info->Version.ExtFamily;
			cpu_info->Model += (cpu_info->Version.ExtModel << 4);
		}
	}

	return 1;
}


/**
 * @brief
 *		Checks processor ability to run VMM and retrieves CPUID information of
 *		physical processor for VMM purposes. CPUID instruction was introduced
 *		in late 80486 processors, lets hope nobody starts VM on older CPUs.
 *		Currently we need:
 *		- Intel or AMD processor;
 *		- Time Stamp Counter (introduced in Intel Pentium);
 *		- Page Attribute Table (introduced in Intel Pentium III (Xeon));
 *		- FXSAVE/FXRSTOR instructions (introduced in Intel Pentium III);
 *		- SYSENTER/SYSEXIT instructions (introduced in Intel Pentium II).
 *
 * @return
 *		0 if there is insufficient CPU capabilities (very old processor),
 *		1 if we can work on this processor model
 */
int ValidateCpuFeatures(CPUID_INFO* cpu_info)
{
	if (!cpu_info->Features.Tsc)
	{
		WRITE_TRACE(DBG_FATAL,
			"CPUID validation error: Time Stamp Counter (TSC) is not supported (0x%x)",
			cpu_info->Features.uCell);
		return 0;
	}

	if (!cpu_info->Features.Fxsr)
	{
		WRITE_TRACE(DBG_FATAL,
			"CPUID validation error: FXSAVE/FXRSTOR instructions are not supported (0x%x)",
			cpu_info->Features.uCell);
		return 0;
	}

	// For Intel PAT is bit_16 in 1.EDX while for AMD the bit is in either 1.EDX or in 80000001.EDX
	if (!cpu_info->Features.Pat)
	{
		WRITE_TRACE(DBG_FATAL,
			"CPUID validation error: PAT is not supported (0x%x)",
			cpu_info->Features.uCell);
		return 0;
	}


	// Get physical and virtual addresses bits count. If leaf 80000008h does
	// not exist then CPU supports 36 bits of physical address and 32 bits of
	// virtual by default. Processors that lack PAE support (introduced in
	// Intel Pentium Pro) have 32-bit physical address space, but VMM won't
	// run on them because we don't support legacy paging.
	if (cpu_info->uMaxExtCount >= 0x80000008)
	{
		if (cpu_info->EXT_80000008_EAX.PhysAddrBits < 32)
		{
			WRITE_TRACE(DBG_FATAL,
				"CPUID validation error: physical address width is less than 4GB (0x%x)",
				cpu_info->EXT_80000008_EAX.PhysAddrBits);
			return 0;
		}

		if (cpu_info->EXT_80000008_EAX.VirtAddrBits < 32)
		{
			WRITE_TRACE(DBG_FATAL,
				"CPUID validation error: virtual address width is less than 4GB (0x%x)",
				cpu_info->EXT_80000008_EAX.VirtAddrBits);
			return 0;
		}
	}

	// Check SYSENTER/SYSEXIT support. Intel Pentium Pro (CPUID signature 0633)
	// reports SEP flag but does not support this feature (Intel SDM Vol. 2B,
	// SYSENTER description). Anyway, above we check CPU features that were
	// introduced later (e.g. FXSAVE/FXRSTOR instructions), so no need to check
	// for Pentium Pro.
	if (!cpu_info->Features.Sep  &&  !(cpu_info->EXT_80000001_EDX & F_SYSCALL))
	{
		WRITE_TRACE(DBG_FATAL,
			"CPUID validation error: SYSENTER/SYSEXIT and SYSCALL/SYSRET are not supported "
			"(0x%x, 0x%x)",
			cpu_info->Features.uCell,
			cpu_info->EXT_80000001_EDX);
		return 0;
	}

	return 1;
}

//
// Description:
//	The function compares two CPUID_INFO structures
//
// Parameters:
//	src_mask - current VM CPU features mask adjusted with source host features set
//	dst_mask - current VM CPU features mask adjusted with destination host features set

// Return value:
//   1 - if CPUs are compatible
//   0 - if they are not
//

int
	IsCpuCompatible(
		CPU_FEATURES_MASKS* src_mask,
		CPU_FEATURES_MASKS* dst_mask
		)
{
	bool bIsCompatible = true;
	CPU_FEATURES_MASKS res_mask;
	char features_str[1500];

	res_mask.FEATURES_MASK =
		(src_mask->FEATURES_MASK ^ dst_mask->FEATURES_MASK) & src_mask->FEATURES_MASK;
	if(res_mask.FEATURES_MASK)
		bIsCompatible = false;

	res_mask.EXT_FEATURES_MASK =
		(src_mask->EXT_FEATURES_MASK ^ dst_mask->EXT_FEATURES_MASK) & src_mask->EXT_FEATURES_MASK;
	if(res_mask.EXT_FEATURES_MASK)
		bIsCompatible = false;

	res_mask.EXT_80000001_ECX_MASK =
		(src_mask->EXT_80000001_ECX_MASK ^ dst_mask->EXT_80000001_ECX_MASK) & src_mask->EXT_80000001_ECX_MASK;
	if(res_mask.EXT_80000001_ECX_MASK)
		bIsCompatible = false;

	res_mask.EXT_80000001_EDX_MASK =
		(src_mask->EXT_80000001_EDX_MASK ^ dst_mask->EXT_80000001_EDX_MASK) & src_mask->EXT_80000001_EDX_MASK;
	if(res_mask.EXT_80000001_EDX_MASK)
		bIsCompatible = false;

	res_mask.EXT_80000007_EDX_MASK =
		(src_mask->EXT_80000007_EDX_MASK ^ dst_mask->EXT_80000007_EDX_MASK) & src_mask->EXT_80000007_EDX_MASK;
	if(res_mask.EXT_80000007_EDX_MASK)
		bIsCompatible = false;

	res_mask.EXT_80000008_EAX.uCell = dst_mask->EXT_80000008_EAX.uCell;
	if(src_mask->EXT_80000008_EAX.PhysAddrBits > dst_mask->EXT_80000008_EAX.PhysAddrBits)
		bIsCompatible = false;
	if(src_mask->EXT_80000008_EAX.VirtAddrBits > dst_mask->EXT_80000008_EAX.VirtAddrBits)
		bIsCompatible = false;

	res_mask.EXT_00000007_EBX_MASK =
		(src_mask->EXT_00000007_EBX_MASK ^ dst_mask->EXT_00000007_EBX_MASK) & src_mask->EXT_00000007_EBX_MASK;
	if(res_mask.EXT_00000007_EBX_MASK)
		bIsCompatible = false;

	res_mask.EXT_0000000D_EAX_MASK =
		(src_mask->EXT_0000000D_EAX_MASK ^ dst_mask->EXT_0000000D_EAX_MASK) & src_mask->EXT_0000000D_EAX_MASK;
	if(res_mask.EXT_0000000D_EAX_MASK)
		bIsCompatible = false;

	if(!bIsCompatible){

		WRITE_TRACE(DBG_FATAL, "Destination CPU is incompatible with source one");

		PrintCpuFeaturesMaskDetails(dst_mask, features_str, sizeof(features_str));
		WRITE_TRACE(DBG_FATAL, "Destination CPU features: %s", features_str);

		PrintCpuFeaturesMaskDetails(src_mask, features_str, sizeof(features_str));
		WRITE_TRACE(DBG_FATAL, "Source CPU features     : %s",  features_str);

		PrintCpuFeaturesMaskDetails(&res_mask, features_str, sizeof(features_str));
		WRITE_TRACE(DBG_FATAL, "Absent CPU features     : %s",  features_str);
	}
	return (int)bIsCompatible;
}
//
// to handle problem of vice versa migration for hosts with different CPU features
//
void
	AdjustCpuFeaturesCompatibility (
		CPUID_INFO* src_cpu_info,
		CPU_FEATURES_MASKS* src_mask,
		CPU_FEATURES_MASKS* dst_mask
		)
{
	ULONG syscall_bit = 0;
	dst_mask->FEATURES_MASK = src_cpu_info->Features.uCell & src_mask->FEATURES_MASK;
	dst_mask->EXT_FEATURES_MASK = src_cpu_info->ExtFeatures.uCell & src_mask->EXT_FEATURES_MASK;
	dst_mask->EXT_80000001_ECX_MASK = src_cpu_info->EXT_80000001_ECX & src_mask->EXT_80000001_ECX_MASK;
#ifndef _AMD64_
	// workaround the fact that SYSCALL feature is not reported if CPUID is called by x32 code
	syscall_bit = src_mask->EXT_80000001_EDX_MASK & F_SYSCALL;
#endif
	dst_mask->EXT_80000001_EDX_MASK = (src_cpu_info->EXT_80000001_EDX & src_mask->EXT_80000001_EDX_MASK) | syscall_bit;
	dst_mask->EXT_80000007_EDX_MASK = src_cpu_info->EXT_80000007_EDX & src_mask->EXT_80000007_EDX_MASK;
	dst_mask->EXT_00000007_EBX_MASK = src_cpu_info->EXT_00000007_EBX & src_mask->EXT_00000007_EBX_MASK;
	dst_mask->EXT_0000000D_EAX_MASK = src_cpu_info->EXT_0000000D_EAX & src_mask->EXT_0000000D_EAX_MASK;

	dst_mask->EXT_80000008_EAX.uCell = src_mask->EXT_80000008_EAX.uCell;
	if(dst_mask->EXT_80000008_EAX.PhysAddrBits > src_cpu_info->EXT_80000008_EAX.PhysAddrBits)
		dst_mask->EXT_80000008_EAX.PhysAddrBits = src_cpu_info->EXT_80000008_EAX.PhysAddrBits;

	if(dst_mask->EXT_80000008_EAX.VirtAddrBits > src_cpu_info->EXT_80000008_EAX.VirtAddrBits)
		dst_mask->EXT_80000008_EAX.VirtAddrBits = src_cpu_info->EXT_80000008_EAX.VirtAddrBits;

}

void
	CombineCpuFeaturesMasks(
		CPU_FEATURES_MASKS* src_mask,
		CPU_FEATURES_MASKS* dst_mask
		)
{
	dst_mask->FEATURES_MASK &= src_mask->FEATURES_MASK;
	dst_mask->EXT_FEATURES_MASK &= src_mask->EXT_FEATURES_MASK;
	dst_mask->EXT_80000001_ECX_MASK &= src_mask->EXT_80000001_ECX_MASK;
	dst_mask->EXT_80000001_EDX_MASK &= src_mask->EXT_80000001_EDX_MASK;
	dst_mask->EXT_80000007_EDX_MASK &= src_mask->EXT_80000007_EDX_MASK;
	dst_mask->EXT_00000007_EBX_MASK &= src_mask->EXT_00000007_EBX_MASK;
	dst_mask->EXT_0000000D_EAX_MASK &= src_mask->EXT_0000000D_EAX_MASK;

	if(dst_mask->EXT_80000008_EAX.PhysAddrBits > src_mask->EXT_80000008_EAX.PhysAddrBits)
		dst_mask->EXT_80000008_EAX.PhysAddrBits = src_mask->EXT_80000008_EAX.PhysAddrBits;

	if(dst_mask->EXT_80000008_EAX.VirtAddrBits > src_mask->EXT_80000008_EAX.VirtAddrBits)
		dst_mask->EXT_80000008_EAX.VirtAddrBits = src_mask->EXT_80000008_EAX.VirtAddrBits;
}

//
// construct correct CR4_MASK that depends on other features set
//
void
	SetCR4Mask(
		CPU_FEATURES_MASKS* dst_mask
		)
{
	dst_mask->CR4_MASK =
		~(CR4_PVI | CR4_TSD | CR4_DE | CR4_PSE | CR4_MCE | CR4_PGE | CR4_PCE | CR4_PAE);

	if(dst_mask->FEATURES_MASK & (F_MMX | F_FXSR))
		dst_mask->CR4_MASK &= ~CR4_OSFXSR;
	else
		dst_mask->CR4_MASK |=  CR4_OSFXSR;

	if(dst_mask->FEATURES_MASK & (F_SSE | F_SSE2) ||
		dst_mask->EXT_FEATURES_MASK & (F_SSE3 | F_SSSE3 | F_SSE4))
		dst_mask->CR4_MASK &= ~CR4_OSXMMEXCPT;
	else
		dst_mask->CR4_MASK |=  CR4_OSXMMEXCPT;

	if(dst_mask->EXT_FEATURES_MASK & (F_XSAVE | F_AVX))
		dst_mask->CR4_MASK &= ~CR4_OSXSAVE;
	else
		dst_mask->CR4_MASK |=  CR4_OSXSAVE;

	if(dst_mask->EXT_FEATURES_MASK & F_PCID)
		dst_mask->CR4_MASK &= ~CR4_PCIDE;
	else
		dst_mask->CR4_MASK |=  CR4_PCIDE;

	if(dst_mask->FEATURES_MASK & F_VME)
		dst_mask->CR4_MASK &= ~(CR4_VME | CR4_PVI);
	else
		dst_mask->CR4_MASK |=  (CR4_VME | CR4_PVI);

	if(dst_mask->EXT_FEATURES_MASK & F_VMX )
		dst_mask->CR4_MASK &= ~(CR4_VMXE);
	else
		dst_mask->CR4_MASK |= CR4_VMXE;

	if (dst_mask->EXT_00000007_EBX_MASK & F_RDFSGSBASE)
		dst_mask->CR4_MASK &= ~CR4_RDFSGSBASE;
	else
		dst_mask->CR4_MASK |=  CR4_RDFSGSBASE;

	if (dst_mask->EXT_00000007_EBX_MASK & F_SMEP)
		dst_mask->CR4_MASK &= ~CR4_SMEP;
	else
		dst_mask->CR4_MASK |=  CR4_SMEP;
}

static UINT AddrBits(ULONG64 addr)
{
	UINT bits = 0;
	while (addr) {
		bits++;
		addr = addr >> 1;
	}
	return bits;
}

UINT MemPhysAddrBits(ULONG64 ramSizeMb)
{
#define MAX_PHYSICAL_ADDR_BITS	(40)

	ULONG64 maxAddr = GUEST_PHY_SPACE(ramSizeMb * SIZE_1MB) - 1;
	UINT bits = ALIGNAT(AddrBits(maxAddr), 4);
	bits = MIN(bits, MAX_PHYSICAL_ADDR_BITS);
	return bits;
}

//
// Description:
//	The function fills default list of CPU features we supported that could be seen by VM
//  This features mask doesn't depend on VM's configuration and could be altered
//  during particular VM start according to its system flags
//
void GetDefaultVMCpuFeaturesMask(
		ULONG64 uHvtFeatures,
		CPUID_INFO *cpu_info,
		CPU_FEATURES_MASKS *features_mask)
{
	const BOOL bHvtSupport = (uHvtFeatures & (HVF_STATUS)) == HSS_ENABLED;

	memset(features_mask, 0, sizeof(*features_mask));

	features_mask->FEATURES_MASK =
		F_FPU | F_DE | F_PSE | F_TSC | F_MSR | F_MCE | F_CX8 | F_SEP | F_PGE |
		F_PAE | F_MTRR | F_CMOV | F_MCA | F_PAT | F_CFLUSH | F_SS;

	features_mask->EXT_FEATURES_MASK = F_PCLMULQDQ | F_CX16 | F_MOVBE | F_AES |
		F_RDRAND;
	features_mask->EXT_80000008_EAX.PhysAddrBits = MemPhysAddrBits(VM_MAX_MEM);

	// assume x64
	if(bHvtSupport){
		features_mask->EXT_80000001_ECX_MASK |= F_LAHF_64;
		features_mask->EXT_80000001_EDX_MASK |= F_EM64T | F_SYSCALL;
	}else{

		if (cpu_info->uVendorId == CPU_VENDOR_AMD)
			features_mask->EXT_80000001_EDX_MASK |= F_SYSCALL;
	}

	features_mask->EXT_80000001_EDX_MASK |= F_NX | F_RDTSCP;

	features_mask->FEATURES_MASK |= F_MMX | F_FXSR;

	// assume Sse4
	features_mask->FEATURES_MASK |= F_SSE;
	features_mask->FEATURES_MASK |= F_SSE2;
	features_mask->EXT_FEATURES_MASK |= F_SSE3 | F_SSSE3 | F_FMA;
	features_mask->EXT_FEATURES_MASK |= F_SSE4;

	features_mask->EXT_FEATURES_MASK |= F_XSAVE;
	features_mask->EXT_FEATURES_MASK |= F_AVX;
	features_mask->EXT_FEATURES_MASK |= F_F16C;
	features_mask->EXT_FEATURES_MASK |= F_X2APIC;

	features_mask->FEATURES_MASK |= F_PSE36;
	features_mask->FEATURES_MASK |= F_APIC;

	features_mask->EXT_0000000D_EAX_MASK |= F_SAVEOPT;


	if(bHvtSupport){
		features_mask->FEATURES_MASK |= F_VME;
	}

	if (cpu_info->uVendorId == CPU_VENDOR_AMD){

		features_mask->EXT_80000001_EDX_MASK |= F_AMD_MMX | F_FFXSR;
		features_mask->EXT_80000001_EDX_MASK |= F_AMD_3DNOW_EXT | F_AMD_3DNOW;
		features_mask->EXT_80000001_ECX_MASK |= F_SSE4A | F_ABM | F_SVM | F_SKINIT;

		// Duplicate some of flags from CPUID_1_EDX into CPUID_80000001_EDX
		features_mask->EXT_80000001_EDX_MASK |=
			(features_mask->FEATURES_MASK &
			   (F_FPU | F_DE | F_VME | F_PSE | F_TSC | F_MSR | F_MCE |
				F_CX8 | F_APIC | F_PGE | F_PAE | F_MTRR | F_CMOV |
				F_MCA | F_PAT | F_PSE36 | F_MMX | F_FXSR));
	}

	features_mask->EXT_80000007_EDX_MASK |= F_TSC_INVARIANT;
	features_mask->EXT_FEATURES_MASK |= F_MONITOR_MWAIT | F_VMX;

	features_mask->EXT_00000007_EBX_MASK = F_RDFSGSBASE | F_SMEP | F_ERMSB | F_AVX2 | F_BMI1 | F_BMI2 | F_ADX | F_HLE;

	if (uHvtFeatures & HVF_NPT)
	{
		features_mask->EXT_FEATURES_MASK |= F_PCID;
		features_mask->EXT_00000007_EBX_MASK |= F_INVPCID;
	}
}

#define BIT_NAME(n) case F_##n: return #n
#define BIT_NAME_CR4(n) case CR4_##n: return #n

static const char *StrNameFEATURES(UINT bit)
{
	switch(1<<bit){
	BIT_NAME(FPU);
	BIT_NAME(VME);
	BIT_NAME(DE);
	BIT_NAME(PSE);
	BIT_NAME(TSC);
	BIT_NAME(MSR);
	BIT_NAME(PAE);
	BIT_NAME(MCE);
	BIT_NAME(CX8);
	BIT_NAME(APIC);
	BIT_NAME(SEP);
	BIT_NAME(MTRR);
	BIT_NAME(PGE);
	BIT_NAME(MCA);
	BIT_NAME(CMOV);
	BIT_NAME(PAT);
	BIT_NAME(PSE36);
	BIT_NAME(PN);
	BIT_NAME(CFLUSH);
	BIT_NAME(DS);
	BIT_NAME(AMD_MMX);
	BIT_NAME(MMX);
	BIT_NAME(FXSR);
	BIT_NAME(SSE);
	BIT_NAME(SSE2);
	BIT_NAME(SS);
	BIT_NAME(MTT);
	BIT_NAME(ACC);
	BIT_NAME(PBE);
	default:
		return "UNK";
	}
	return "";
}

static const char *StrNameEXT_FEATURES(UINT bit)
{
	switch(1<<bit){
	BIT_NAME(SSE3);
	BIT_NAME(PCLMULQDQ);
	BIT_NAME(DTE64);
	BIT_NAME(MONITOR_MWAIT);
	BIT_NAME(DSCPL);
	BIT_NAME(VMX);
	BIT_NAME(SMX);
	BIT_NAME(EST);
	BIT_NAME(TM2);
	BIT_NAME(SSSE3);
	BIT_NAME(CX16);
	BIT_NAME(XTPR);
	BIT_NAME(PMDC);
	BIT_NAME(FMA);
	BIT_NAME(PCID);
	BIT_NAME(SSE41);
	BIT_NAME(SSE42);
	BIT_NAME(X2APIC);
	BIT_NAME(MOVBE);
	BIT_NAME(POPCNT);
	BIT_NAME(TSCDL);
	BIT_NAME(AES);
	BIT_NAME(XSAVE);
	BIT_NAME(OSXSAVE);
	BIT_NAME(AVX);
	BIT_NAME(F16C);
	BIT_NAME(RDRAND);
	BIT_NAME(HYPERVISOR);
	default:
		return "UNK";
	}
	return "";
}

static const char *StrName80000001_ECX(UINT bit)
{
	switch(1<<bit){
	BIT_NAME(LAHF_64);
	BIT_NAME(SVM);
	BIT_NAME(ABM);
	BIT_NAME(SSE4A);
	BIT_NAME(WDT);
	default:
		return "UNK";
	}
	return "";
}

static const char *StrName80000001_EDX(UINT bit)
{
	switch(1<<bit){
	BIT_NAME(SYSCALL);
	BIT_NAME(NX);
	BIT_NAME(FFXSR);
	BIT_NAME(1GBPG);
	BIT_NAME(RDTSCP);
	BIT_NAME(EM64T);
	BIT_NAME(AMD_3DNOW_EXT);
	BIT_NAME(AMD_3DNOW);
	BIT_NAME(AMD_MMX);
	BIT_NAME(FPU);
	BIT_NAME(VME);
	BIT_NAME(DE);
	BIT_NAME(PSE);
	BIT_NAME(TSC);
	BIT_NAME(MSR);
	BIT_NAME(PAE);
	BIT_NAME(MCE);
	BIT_NAME(CX8);
	BIT_NAME(APIC);
	BIT_NAME(PGE);
	BIT_NAME(MCA);
	BIT_NAME(CMOV);
	BIT_NAME(PAT);
	BIT_NAME(MMX);
	BIT_NAME(FXSR);
	default:
		return "UNK";
	}
	return "";
}

static const char *StrName80000007_EDX(UINT bit)
{
	switch(1<<bit){
	BIT_NAME(TSC_INVARIANT);
	default:
		return "UNK";
	}
	return "";
}

static const char *StrName00000007_EBX(UINT bit)
{
	switch(1<<bit){
	BIT_NAME(RDFSGSBASE);
	BIT_NAME(ADJTSC);
	BIT_NAME(BMI1);
	BIT_NAME(HLE);
	BIT_NAME(AVX2);
	BIT_NAME(SMEP);
	BIT_NAME(BMI2);
	BIT_NAME(ERMSB);
	BIT_NAME(INVPCID);
	BIT_NAME(RTM);
	BIT_NAME(QM);
	BIT_NAME(FPUCS);
	BIT_NAME(RDSEED);
	BIT_NAME(ADX);
	default:
		return "UNK";
	}
	return "";
}

static const char *StrName0000000D_EAX(UINT bit)
{
	switch(1<<bit){
		BIT_NAME(SAVEOPT);
	default:
		return "UNK";
	}
	return "";
}

static const char *StrNamePowerManagement_ECX(UINT bit)
{
	switch(1<<bit){
	BIT_NAME(APERF_MPERF);
	BIT_NAME(PERFBIAS);
	default:
		return "UNK";
	}
	return "";
}

typedef const char* (StrNameMASK_Func)(UINT bit);

static void PrintCpuFeaturesDetailsCommon(UINT v, char *pStr, int strSize, StrNameMASK_Func func)
{
	int i;
	char tmpStr[100];

	if(!v)
		return;

	pStr[strSize - 1] = 0;
	if (*pStr)
		strncat(pStr, " | ", strSize - 1 - strlen(pStr));

	for(i = 0; v; i++,v >>= 1){
		if(!(v & 1))
			continue;
		snprintf(tmpStr, sizeof(tmpStr)-1, "%s", func(i));
		strncat(pStr, tmpStr, strSize - 1 - strlen(pStr));
		if(v>>1)
			strncat(pStr, ", ", strSize - 1 - strlen(pStr));
	}
}

void PrintCpuInfo(CPUID_INFO *cpu, char *pStr, int strSize)
{
	snprintf(pStr, strSize, "%s %02X%X%X [%s] (id=%u) max(%u, 0x%x)"
			" 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
			cpu->szVendor,
			cpu->Family, cpu->Model, cpu->Version.Stepping,
			cpu->BrandString[0] ? (char*)cpu->BrandString : "no brand string",
			cpu->uVendorId, cpu->uMaxCount, cpu->uMaxExtCount,
			cpu->Features.uCell,
			cpu->ExtFeatures.uCell,
			cpu->EXT_80000001_ECX,
			cpu->EXT_80000001_EDX,
			cpu->EXT_80000007_EDX,
			cpu->EXT_80000008_EAX.uCell,
			cpu->EXT_00000007_EBX,
			cpu->PowerManagement.uCell
			);

}

void PrintCpuFeaturesDetails(CPUID_INFO *cpu, char *pStr, int strSize)
{
	pStr[0] = 0;
	PrintCpuFeaturesDetailsCommon(cpu->Features.uCell, pStr, strSize, StrNameFEATURES);
	PrintCpuFeaturesDetailsCommon(cpu->ExtFeatures.uCell, pStr, strSize, StrNameEXT_FEATURES);
	PrintCpuFeaturesDetailsCommon(cpu->EXT_80000001_ECX,  pStr, strSize, StrName80000001_ECX);
	PrintCpuFeaturesDetailsCommon(cpu->EXT_80000001_EDX,  pStr, strSize, StrName80000001_EDX);
	PrintCpuFeaturesDetailsCommon(cpu->EXT_80000007_EDX, pStr, strSize, StrName80000007_EDX);
	PrintCpuFeaturesDetailsCommon(cpu->EXT_00000007_EBX, pStr, strSize, StrName00000007_EBX);
	PrintCpuFeaturesDetailsCommon(cpu->PowerManagement.uCell, pStr, strSize, StrNamePowerManagement_ECX);
	PrintCpuFeaturesDetailsCommon(cpu->EXT_0000000D_EAX, pStr, strSize, StrName0000000D_EAX);
}

void PrintCpuFeaturesMask(CPU_FEATURES_MASKS *f, char *pStr, int strSize)
{
	snprintf(pStr, strSize,
		"0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
		f->FEATURES_MASK,
		f->EXT_FEATURES_MASK,
		f->EXT_80000001_ECX_MASK,
		f->EXT_80000001_EDX_MASK,
		f->EXT_80000007_EDX_MASK,
		f->EXT_80000008_EAX.uCell,
		f->EXT_00000007_EBX_MASK,
		f->EXT_0000000D_EAX_MASK
		);
}

void PrintCpuFeaturesMaskDetails(CPU_FEATURES_MASKS *f, char *pStr, int strSize)
{
	pStr[0] = 0;
	PrintCpuFeaturesDetailsCommon(f->FEATURES_MASK, pStr, strSize, StrNameFEATURES);
	PrintCpuFeaturesDetailsCommon(f->EXT_FEATURES_MASK, pStr, strSize, StrNameEXT_FEATURES);
	PrintCpuFeaturesDetailsCommon(f->EXT_80000001_ECX_MASK, pStr, strSize, StrName80000001_ECX);
	PrintCpuFeaturesDetailsCommon(f->EXT_80000001_EDX_MASK, pStr, strSize, StrName80000001_EDX);
	PrintCpuFeaturesDetailsCommon(f->EXT_80000007_EDX_MASK, pStr, strSize, StrName80000007_EDX);
	PrintCpuFeaturesDetailsCommon(f->EXT_00000007_EBX_MASK, pStr, strSize, StrName00000007_EBX);
	PrintCpuFeaturesDetailsCommon(f->EXT_0000000D_EAX_MASK, pStr, strSize, StrName0000000D_EAX);
}
