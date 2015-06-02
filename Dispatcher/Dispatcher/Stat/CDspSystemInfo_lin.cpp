///////////////////////////////////////////////////////////////////////////////
///
/// @file @file CDspSystemInfo_lin.cpp
///
/// Platform dependent hardware information collector implementation for Linux
///
/// @author ilya
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "CDspSystemInfo.h"
#include "Libraries/Logging/Logging.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"
#include <stdio.h>
#include <unistd.h>

quint32 CDspSystemInfo::GetNumberOfProcessors()
{
	int nCpuNum = sysconf(_SC_NPROCESSORS_ONLN);
	if(nCpuNum < 1) nCpuNum = 1;
	return (quint32)nCpuNum;
}
