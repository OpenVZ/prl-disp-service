///////////////////////////////////////////////////////////////////////////////
///
/// @file AtomicOpsTest.h
///
/// Tests for atomic operations. Unfortunately couldn't find another way how to
/// test atomic ops on ARM. In order to add unit tests to project just uncomment
/// correspond lines at PrlCommonUtils.pro
///
/// @author sandro@
///
/// Copyright (c) 1999-2015 Parallels IP Holdings GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef AtomicOpsTest_H
#define AtomicOpsTest_H

#include <QString>

struct AtomicOpsTest
{
	QString testAtomicRead64();
	QString testAtomicWrite64();
	QString testAtomicAdd64();
	QString testAtomicAdd64HighPart();
	QString testAtomicAdd64Overflow();
	QString testAtomicAdd64Overflow2();
	QString testAtomicAdd64Overflow3();
	QString testAtomicInc64();
	QString testAtomicSwap64();
	QString testAtomicCompareSwap64();
	QString testAtomicCompareSwap64NonEqual();
};

#endif

