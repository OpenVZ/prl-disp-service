///////////////////////////////////////////////////////////////////////////////
///
/// @file SystemFlags.h
///
/// Module for all CPU features related stuff
///
///
/// @author mnestratov@
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __CPUFEATURES_H__
#define __CPUFEATURES_H__

#include "Ia32.h"
#include <prlsdk/PrlTypes.h> // for PRL_CONST_STR, PRL_UINT32, etc.

/**
 *  * HVT support status.
 *   * Enum contains status-codes returned by IOCTL_CHECK_HVT_SUPPORT.
 *    */
enum HvtSupportStatus
{
	HSS_NOT_PRESENT = 0,    // HVT is not present
	HSS_ENABLED = 1,        // HVT is present, enabled
	HSS_VTX_ENABLED_ONLY_IN_SMX = 2,        // VTX only in SMX operation
	HSS_DISABLED = 3,       // HVT is present, disabled in BIOS
};
/**
 *  * Explotable HVT features
 *   */
enum HvtFeaturesMask
{
	HVF_STATUS = 3,
	HVF_NPT = (1 << 2),     //common for EPT and RVI
	HVF_SHADOW_TPR = (1 << 3),
	HVF_MSR_BITMAPS = (1 << 4),
	HVF_VIRTUAL_APIC = (1 << 5),
	HVF_VPID = (1 << 6),
	HVF_SVM_LBR_VIRT = (1 << 7),
	HVF_SVM_LOCK = (1 << 8),
	HVF_SVM_NRIP_SAVE = (1 << 9),
	HVF_SVM_SSSE3_SSE5_DISABLE = (1 << 10),
	HVF_VTX_UNRESTRICTED = (1 << 11),
	HVF_SVM_ERRATA383 = (1 << 12),
	HVF_SVM_TSC_RATE_MSR = (1 << 13),
	HVF_SVM_CLEAN_BITS = (1 << 14),
	HVF_SVM_FLUSH_BY_ASID = (1 << 15),
	HVF_SVM_DECODE_ASSISTS = (1 << 16),
	HVF_SVM_PAUSE_FILTER_COUNT = (1 << 17),

	HVF_ALL = -1
};


int GetHostCpuFeatures(CPUID_INFO* cpu_info);
int ValidateCpuFeatures(CPUID_INFO* cpu_info);
void
	AdjustCpuFeaturesCompatibility (
		CPUID_INFO* src_cpu_info,
		CPU_FEATURES_MASKS* src_mask,
		CPU_FEATURES_MASKS* dst_mask
		);
void
	CombineCpuFeaturesMasks(
		CPU_FEATURES_MASKS* src_mask,
		CPU_FEATURES_MASKS* dst_mask
		);

void
	SetCR4Mask(
		CPU_FEATURES_MASKS* dst_mask
		);

int
	IsCpuCompatible(
		CPU_FEATURES_MASKS* src_mask,
		CPU_FEATURES_MASKS* dst_mask
		);

void
	GetDefaultVMCpuFeaturesMask(
		ULONG64 uHvtFeatures,
		CPUID_INFO *cpu_info,
		CPU_FEATURES_MASKS *features_mask
		);

UINT
	MemPhysAddrBits(
		ULONG64 ramSizeMb
		);

void
	PrintCpuInfo(
		CPUID_INFO *cpu,
		char *pStr,
		int strSize
		);

void PrintCpuFeaturesDetails(
		CPUID_INFO *cpu,
		char *pStr,
		int strSize
		);

void
	PrintCpuFeaturesMask(
		CPU_FEATURES_MASKS *f,
		char *pStr,
		int strSize
		);

void
	PrintCpuFeaturesMaskDetails(
		CPU_FEATURES_MASKS *f,
		char *pStr,
		int strSize);


#endif //__CPUFEATURES_H__
