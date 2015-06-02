///////////////////////////////////////////////////////////////////////////////
///
/// @file ManagerIf.h
///
/// Declaration of the manager interface
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
#ifndef __MNGIF_INCLUDED__
#define __MNGIF_INCLUDED__

#include <prlsdk/PrlTypes.h>

/*
 * Base interface class
 */
class IManager
{
public:
	static const PRL_INT32 ProgressCompleted = 1005;
	static const PRL_INT32 ProgressMax = 1000;
	static const PRL_INT32 ProgressMin = 1;
	// Callback type
	typedef bool (PRL_CALL *Callback)(PRL_INT32 Progress, PRL_VOID_PTR Param);
public:
	virtual void Release() = 0;
	virtual void Reset() = 0;
	virtual PRL_RESULT Execute() = 0;
	virtual PRL_RESULT Commit() = 0;
	virtual PRL_RESULT Finalize() = 0;
	virtual PRL_RESULT Rollback() = 0;
	virtual bool IsTerminated() const = 0;
	virtual void SetTerminate() = 0;
	virtual bool IsActive() const = 0;
	virtual void Wait() = 0;
protected:
	virtual ~IManager() {};
};

#endif
