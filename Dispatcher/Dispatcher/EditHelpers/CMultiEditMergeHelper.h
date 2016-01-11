///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
/// http://www.parallelssoft.com
///
/// MODULE:
///		CMultiEditMergeHelper.h
///
/// AUTHOR:
///		sergeyt
///
/// DESCRIPTION:
///	This class implements base logic for simultaneously edit same configuration (VM, user profile and etc)
///	WITH MERGE.
///
/// COMMENTS:
///	sergeyt
///
/////////////////////////////////////////////////////////////////////////////
#ifndef CMultiEditMergeHelper_H
#define CMultiEditMergeHelper_H

#include "CMultiEditDispatcher.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Interfaces/ParallelsQt.h>

//////////////////////////////////////////////////////////////////////////
// We create new class to work through template to prevent move CMultiEditDispatcher implementation to .h file
//////////////////////////////////////////////////////////////////////////
template<class CONF> class CMultiEditMergeHelper : public CMultiEditDispatcher
{
private:
	// hide registerBeginEdit() without shot ID
	using CMultiEditDispatcher::registerBeginEdit;

	virtual void removeObjectShot( const PRL_UINT64& );

public:
	using CMultiEditDispatcher::cleanupBeginEditMark;
	using CMultiEditDispatcher::cleanupAllBeginEditMarksByAccessToken;

public:
	void registerBeginEdit(const QString& sObjectId
		, const IOSender::Handle& sUserAccessToken
		, SmartPtr<CONF>& );

	bool tryToMerge(const QString& sObjectId
		, const IOSender::Handle& sUserAccessToken
		, SmartPtr<CONF>& pNewConf
		, const SmartPtr<CONF>& pCurrConf);

protected:
	CMultiEditMergeHelper():m_llCounter(1){}

	SmartPtr<CONF> getConfigShot( PRL_UINT64 llShotId )
	{
		QMutexLocker lock( this );
		return m_hashIdToShot.value(llShotId);
	}

	virtual bool merge( const SmartPtr<CONF>& pPrevConf
		, SmartPtr<CONF>& pNewConf, const SmartPtr<CONF>& pCurrConf) = 0;

private:
	PRL_UINT64 m_llCounter;
	QHash<PRL_UINT64, SmartPtr<CONF> > m_hashIdToShot;
};

template<class CONF>
void CMultiEditMergeHelper<CONF>::removeObjectShot( const PRL_UINT64& key)
{
	QMutexLocker lock( this );
	m_hashIdToShot.remove( key );
}

template<class CONF>
void CMultiEditMergeHelper<CONF>::registerBeginEdit(
	const QString& sObjectId
	, const IOSender::Handle& sUserAccessToken
	, SmartPtr<CONF>& pPrevConf)
{

	QMutexLocker lock( this );
	PRL_ASSERT( pPrevConf );
	m_hashIdToShot.insert( ++m_llCounter, pPrevConf );
	registerBeginEdit( sObjectId, sUserAccessToken, m_llCounter );
}

template<class CONF>
bool CMultiEditMergeHelper<CONF>::tryToMerge(
	const QString& sObjectId
	 , const IOSender::Handle& sUserAccessToken
	 , SmartPtr<CONF>& pNewConf
	 , const SmartPtr<CONF>& pCurrConf)
{

	QMutexLocker lock( this );
	PRL_UINT64 llId = getShotId( sObjectId, sUserAccessToken );
	if( !llId )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to found previous config ID for obj %s, user %s"
			, QSTR2UTF8(sObjectId)
			, QSTR2UTF8(sUserAccessToken ));
		return false;
	}

	SmartPtr<CONF> pPrevConf = m_hashIdToShot.value( llId );
	lock.unlock();

	if( pPrevConf )
		return merge( pPrevConf, pNewConf, pCurrConf );

	WRITE_TRACE( DBG_FATAL, "Unable to found previous config for obj %s, id %#llx", QSTR2UTF8(sObjectId), llId );
	return false;
}

#endif
