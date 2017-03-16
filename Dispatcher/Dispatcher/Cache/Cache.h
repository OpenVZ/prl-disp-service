///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmConfigManager
///
/// this object wraps load/store to disk vm configuration files
/// it synchronize threads for simultaneous access
/// @author Artemr
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <prlsdk/PrlErrors.h>
#include <prlcommon/Std/SmartPtr.h>
#include <QReadWriteLock>
#include <QHash>
#include <QDateTime>
#include <QPair>

class CDspClient;

template <class T>
class CacheBase
{
public:
				// FIXME: for Windows we should store path with username:
				//		Network shares can be mounted to the same Disk letter from different users!
			virtual SmartPtr<T> getFromCache( const QString& strFileName, SmartPtr<CDspClient> pUserSession ) = 0;
			virtual void updateCache( const QString& path
				, const SmartPtr<T>& pConfig, SmartPtr<CDspClient> pUserSession) = 0;

			virtual ~CacheBase() {}
};
