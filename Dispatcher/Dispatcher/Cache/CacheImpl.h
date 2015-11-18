///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmConfigManager
///
/// this object wraps load/store to disk vm configuration files
/// it synchronize threads for simultaneous access
/// @author Artemr
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

#pragma once

#include "Cache.h"

namespace {
template<class T>
class Cache : public CacheBase< T>
{
		public:
			Cache(int ttlSec = 0);
			SmartPtr<T> getFromCache( const QString& strFileName, SmartPtr<CDspClient> pUserSession );
			void updateCache( const QString& path
				, const SmartPtr<T>& pConfig, SmartPtr<CDspClient> pUserSession);

		private:
			struct	FileTimestamp
			{
				FileTimestamp ( const QString& sPath );
				FileTimestamp ( const FileTimestamp& ts );

				QString toString() const;
				bool operator==(const FileTimestamp& ts) const;
				bool operator!=(const FileTimestamp& ts) const;
			private:
				// QDateTime implementation has low precession ( time without millisecs (on linux ) )
				// It should be improved
				QDateTime dt;
			};
			struct ConfigInfo
			{
				ConfigInfo(  const SmartPtr<T>& pConfig, const FileTimestamp& changeTime );
				SmartPtr<T> pConfig;
				FileTimestamp dtChangeTime;
				mutable PRL_UINT64 nextTimeStampCheck;
				mutable PRL_UINT64 lastAccess;

				// FIXME: Need check also for Config ModificationTime - in PSBM on PCS we have disabled ConfigWatcher.
			};
		private:
			// NOTE:
			// 	For Windows we should store path with username:
			//	Network shares can be mounted to the same Disk letter from different users!
			typedef QPair<QString/*userName*/, QString/*Path*/> CacheKey;
			CacheKey makeKey( const QString& path, SmartPtr<CDspClient> pClient ) const;

			void ttlCheck();

		private:
			QReadWriteLock	m_rwHashLock;
			QHash< CacheKey, ConfigInfo > m_hashConfigs;

			/* time in ticks */
			int m_ttl;
			PRL_UINT64 m_nextTtlCheck;
};

#include "CacheImpl.cpp"
};
