/**
 * @file OsInfo_Wmi.cpp
 *
 * @brief Wrapper for Wmi-library for using in OsInfo.c
 *
 * @author nzaborovsky@
 * @author owner is alexg@
 *
 * Copyright (c) 2005-2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <string>
#include <Libraries/WmiWrap/WmiWrap.h>
#include "OsInfo_Wmi.h"


static const std::wstring c_sSuffixOEM = L"OEM";


extern "C" int Wmi_IsOEM()
{
	WmiWrap ww;
	std::wstring serial = ww.GetProductID();
	if (6 <= serial.size() && 0 == serial.compare(6, 3, c_sSuffixOEM))
		return 1;
	return 0;
}


extern "C" unsigned int Wmi_GetLicenseStatus()
{
	WmiWrap ww;
	ww.UpdateActivationStatus();
	return ww.GetActivationStatus();
}
