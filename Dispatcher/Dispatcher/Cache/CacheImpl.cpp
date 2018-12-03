///////////////////////////////////////////////////////////////////////////////
///
/// @file Cache
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

// #define FORCE_LOGGING_ON

#include "CacheImpl.h"

#include <QReadLocker>
#include <QWriteLocker>
#include "CDspVmDirManager.h"
#include "CDspService.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Std/PrlTime.h>
#include <prlcommon/Interfaces/ParallelsDomModel.h>


template<class T> Cache<T>::FileTimestamp::FileTimestamp ( const QString& sPath )
: dt( QFileInfo(sPath).lastModified() )
{
}

template<class T> Cache<T>::FileTimestamp::FileTimestamp ( const FileTimestamp& ts )
:dt( ts.dt )
{
}

template<class T> QString Cache<T>::FileTimestamp::toString() const
{
	return dt.toString(XML_DATETIME_FORMAT_LONG);
}
template<class T> bool Cache<T>::FileTimestamp::operator==(const FileTimestamp& ts) const
{
	return dt == ts.dt;
}
template<class T> bool Cache<T>::FileTimestamp::operator!=(const FileTimestamp& ts) const
{
	return !operator==(ts);
}

template<class T> Cache<T>::ConfigInfo::ConfigInfo(
	const SmartPtr<T>& pConfigOrig, const FileTimestamp& ts )
: pConfig( new T( pConfigOrig.getImpl() ) )
	, dtChangeTime(ts)
{
	lastAccess = PrlGetTickCount64();
	PRL_ASSERT(pConfigOrig);
}

template<> Cache<CVmConfiguration>::ConfigInfo::ConfigInfo(
	const SmartPtr<CVmConfiguration>& pConfigOrig, const FileTimestamp& ts )
: pConfig( new CVmConfiguration( pConfigOrig.getImpl() ) )
	, dtChangeTime(ts)
{
	lastAccess = PrlGetTickCount64();
	// will keep only absolute paths in cache (https://jira.sw.ru/browse/PSBM-13477)
	pConfig->setAbsolutePath();
	PRL_ASSERT(pConfigOrig);
}

template<class T>
typename Cache<T>::CacheKey
Cache<T>::makeKey( const QString& path, SmartPtr<CDspClient> pClient ) const
{
	QString userName;
#ifdef _WIN
	if( pClient )
		userName = pClient->getAuthHelper().getFullUserName();
#else
	Q_UNUSED(pClient);
#endif

 return qMakePair(userName, path) ;
}

template<class T> Cache<T>::Cache(int ttlSec )
{
	m_ttl = ttlSec * PrlGetTicksPerSecond();
	m_nextTtlCheck = m_ttl ? PrlGetTickCount64() + m_ttl : 0;
}

template<class T>
void Cache<T>::ttlCheck()
{
	QStringList keys;

	{
		QWriteLocker locker(&m_rwHashLock);
		PRL_UINT64 t = PrlGetTickCount64() - m_ttl;

		typename QHash<CacheKey, ConfigInfo>::const_iterator it = m_hashConfigs.constBegin();
		while (it != m_hashConfigs.constEnd()) {
			if (it.value().lastAccess < t)
				keys += it.key().second;
			++it;
		}
		m_nextTtlCheck = PrlGetTickCount64() + (m_ttl / 2);
	}

	foreach (QString key, keys)
		updateCache(key, SmartPtr<T>(0), SmartPtr<CDspClient>(0));
}

template<class T> SmartPtr<T> Cache<T>::getFromCache( const QString& strFileName, SmartPtr<CDspClient> pUserSession )
{
	if( !CDspDispConfigGuard::isConfigCacheEnabled() )
		return SmartPtr<T>();

	if (m_ttl != 0 && m_nextTtlCheck < PrlGetTickCount64())
		ttlCheck();

	QReadLocker locker(&m_rwHashLock);

	// FIXME: Need compare by QFileInfo ??
	// FIXME2: TODO: First compare by QString(path).toLower(), second by QFileInfo
	// FIXME3: We think NOT! All configs are unique because they have got from VmDirectory, but VmDirectory contains unique paths
	CacheKey key = makeKey(strFileName, pUserSession);
	typename QHash<CacheKey, ConfigInfo>::const_iterator cIt = m_hashConfigs.constFind(key);
	if(cIt != m_hashConfigs.constEnd())
	{
		LOG_MESSAGE( DBG_FATAL, "xxx ZZZ-2: Config was got from cache. path = %s", QSTR2UTF8(strFileName) );
		cIt->lastAccess = PrlGetTickCount64();
		SmartPtr<T> pConfig( new T( cIt->pConfig.getImpl() ) );
		return pConfig;
	}
	LOG_MESSAGE( DBG_FATAL, "xxx WWW: Config was NOT got from cache. path = %s", QSTR2UTF8(strFileName) );

	return SmartPtr<T>(0);
}

template<class T> void Cache<T>::updateCache( const QString& path
		, const SmartPtr<T>& pConfig, SmartPtr<CDspClient> pUserSession)
{
	if( !CDspDispConfigGuard::isConfigCacheEnabled() )
		return;

	CacheKey key = makeKey(path, pUserSession);
	if(key.second.isEmpty()) // if path.isEmpty
		return;

	QWriteLocker locker(&m_rwHashLock);
	if(pConfig.isValid())
	{
		m_hashConfigs.insert(key, ConfigInfo(pConfig, FileTimestamp(path)));
		LOG_MESSAGE(DBG_FATAL, "xxx XXX: Config was updated in cache. key= %s+%s",
			qPrintable(key.second), qPrintable(key.first));
	}
	else
	{
		m_hashConfigs.remove(key);
		LOG_MESSAGE(DBG_FATAL, "xxx XXX: Config was removed from cache. key= %s+%s",
			qPrintable(key.second), qPrintable(key.first));
	}
}

