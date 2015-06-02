//////////////////////////////////////////////////////////////////////////
///
/// @file ParallelsSDKPrivate.h
///
/// @brief Private internal constants with used by SDK
///
/// @author sergeyt
///
///	This file contains various definitions and macro,
///	which are used throughout the project modules
///
/// Copyright (c) 2009-2015 Parallels IP Holdings GmbH
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

#ifndef __PARALLELS_SDK_PRIVATE__
#define __PARALLELS_SDK_PRIVATE__

#include <prlsdk/PrlOses.h>

static inline int PVS_GUEST_VIRTIO_SUPPORTED(unsigned int nVersion)
{
	return (   IS_LINUX(nVersion)
			&& nVersion != PVS_GUEST_VER_LIN_KRNL_24
			&& nVersion != PVS_GUEST_VER_LIN_RHLES3 );
}

static inline int PVS_GUEST_E1000_SUPPORTED(unsigned int nVersion)
{
	return (IS_WINDOWS(nVersion) && nVersion >= PVS_GUEST_VER_WIN_VISTA) ||
		(IS_LINUX(nVersion) && nVersion != PVS_GUEST_VER_LIN_KRNL_24) ||
		(IS_MACOS(nVersion) && nVersion >= PVS_GUEST_VER_MACOS_SNOW_LEOPARD) ||
		(!IS_WINDOWS(nVersion) && !IS_LINUX(nVersion) && !IS_MACOS(nVersion) && !IS_OS2(nVersion));
}

/**
 * TRUE if network should be patched from RTL to E1000 while upgrade
 */
static inline int PVS_GUEST_IS_E1000_UPGRADABLE(unsigned int nVersion)
{
	return PVS_GUEST_E1000_SUPPORTED(nVersion) && !IS_SOLARIS(nVersion);
}

static inline int PVS_GUEST_SCSI_LSI_SPI_SUPPORTED(unsigned int nVersion)
{
	return IS_LINUX(nVersion);
}

static inline int PVS_GUEST_SCSI_LSI_SAS_SUPPORTED(unsigned int nVersion)
{
	return nVersion >= PVS_GUEST_VER_WIN_VISTA && nVersion <= PVS_GUEST_VER_WIN_LAST;
}

#endif // __PARALLELS_SDK_PRIVATE__

