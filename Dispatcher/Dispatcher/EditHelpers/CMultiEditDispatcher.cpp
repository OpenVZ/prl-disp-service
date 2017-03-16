/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
/// @file
///		CMultiEditDispatcher.cpp
///
/// @author
///		sandro
///
/// @brief
///	This class implements logic for simultaneously edit same configuration (VM, user profile and etc).
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

#include "CMultiEditDispatcher.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlTime.h>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

CMultiEditDispatcher::CMultiEditDispatcher()
:QMutex( QMutex::Recursive )
{

}

CMultiEditDispatcher::~CMultiEditDispatcher()
{
}

void CMultiEditDispatcher::registerBeginEdit(const QString& objectUuid, const IOSender::Handle& userUuid)
{
	return registerBeginEdit(objectUuid, userUuid, 0);
}

void CMultiEditDispatcher::registerBeginEdit(const QString& objectUuid
		,const IOSender::Handle& userUuid, const PRL_UINT64& llObjectShotId)
{
	LOG_MESSAGE (DBG_DEBUG, "objectId=[%s], userId=[%s]", objectUuid.toUtf8().data(), userUuid.toUtf8().data());

	QMutexLocker lock( this );
	QHash<QString, EditCommitInfoPtr >::iterator
		itByObject=m_hashCommitsByObject.find(objectUuid);
	if (m_hashCommitsByObject.end()==itByObject)
	{
		SmartPtr<EditCommitInfo> pInfo( new EditCommitInfo() );
		itByObject=m_hashCommitsByObject.insert(objectUuid, pInfo);
	}

	QHash<IOSender::Handle, BeginEditInfo>::iterator
		itBeginEditByUser= itByObject.value()->m_hashBeginEditByUser.find(userUuid);
	if( itByObject.value()->m_hashBeginEditByUser.end() != itBeginEditByUser )
		removeObjectShot( itBeginEditByUser.value().llObjectShotId );
	itByObject.value()->m_hashBeginEditByUser[userUuid] = BeginEditInfo( PrlGetTickCount64(), llObjectShotId );
}

bool CMultiEditDispatcher::configWasChanged(
	const QString& objectUuid, const IOSender::Handle& userUuid, PRL_RESULT& nErrorCode)
{
	LOG_MESSAGE (DBG_DEBUG, "objectId=[%s], userId=[%s]", objectUuid.toUtf8().data(), userUuid.toUtf8().data());

	nErrorCode = PRL_ERR_SUCCESS;

	QMutexLocker lock( this );

	///////////////////////////////
	QHash<QString, EditCommitInfoPtr>::iterator
		itByObject=m_hashCommitsByObject.find(objectUuid);
	if (m_hashCommitsByObject.end()==itByObject)
	{
		WRITE_TRACE(DBG_FATAL, "internal error: can't found objectUuid [%s]. "
			" Probably ::registerBeginEdit() doesn't called", objectUuid.toUtf8().data());
		nErrorCode = PRL_ERR_CONFIG_BEGIN_EDIT_NOT_FOUND_OBJECT_UUID;
	  return true;
	}

	///////////////////////////////
	QHash<QString, BeginEditInfo>::iterator
		itBeginEditByUser=itByObject.value()->m_hashBeginEditByUser.find(userUuid);
	if (itByObject.value()->m_hashBeginEditByUser.end() == itBeginEditByUser)
	{
		WRITE_TRACE(DBG_FATAL, "internal error: can't found userUuid [%s]. "
			" Probably ::registerBeginEdit() doesn't called", userUuid.toUtf8().data());
	  nErrorCode = PRL_ERR_CONFIG_BEGIN_EDIT_NOT_FOUND_USER_UUID;
	  return true;
	}

	///////////////////////////////
	bool ret=true;
	if (itByObject.value()->tsCommitTimeStamp >= itBeginEditByUser.value().tsBeginEditTimeStamp)
		ret=true;
	else
		ret=false;
	LOG_MESSAGE (DBG_DEBUG, "return ret=[%s]", ret?"true":"false");
	return ret;
}

void CMultiEditDispatcher::registerCommit(const QString& objectUuid, const IOSender::Handle& userUuid)
{
	LOG_MESSAGE (DBG_DEBUG, "objectId=[%s], userId=[%s]", objectUuid.toUtf8().data(), userUuid.toUtf8().data());

	QMutexLocker lock( this );

	///////////////////////////////
	if (m_hashCommitsByObject.contains(objectUuid))
	{
		m_hashCommitsByObject[objectUuid]->tsCommitTimeStamp= PrlGetTickCount64();
		cleanupBeginEditMark( objectUuid, userUuid );

		if( m_hashCommitsByObject[objectUuid]->m_hashBeginEditByUser.isEmpty() )
			m_hashCommitsByObject.remove( objectUuid );
	}
}

PRL_UINT64 CMultiEditDispatcher::getShotId(const QString& objectUuid, const IOSender::Handle& userUuid)
{
	QMutexLocker lock( this );

	///////////////////////////////
	QHash<QString, EditCommitInfoPtr>::iterator
		itByObject=m_hashCommitsByObject.find(objectUuid);
	if (m_hashCommitsByObject.end()==itByObject)
	{
		WRITE_TRACE(DBG_FATAL, "internal error: can't found objectUuid [%s]. "
			" Probably ::registerBeginEdit() doesn't called", objectUuid.toUtf8().data());
		return PRL_UINT64(0);
	}

	///////////////////////////////
	QHash<QString, BeginEditInfo>::iterator
		itBeginEditByUser=itByObject.value()->m_hashBeginEditByUser.find(userUuid);
	if (itByObject.value()->m_hashBeginEditByUser.end() == itBeginEditByUser)
	{
		WRITE_TRACE(DBG_FATAL, "internal error: can't found userUuid [%s]. "
			" Probably ::registerBeginEdit() doesn't called", userUuid.toUtf8().data());
		return PRL_UINT64(0);
	}

	return itBeginEditByUser.value().llObjectShotId;
}

void CMultiEditDispatcher::cleanupBeginEditMark( const QString& objectUuid, const IOSender::Handle& userUuid)
{
	QMutexLocker lock( this );

	if(m_hashCommitsByObject.contains(objectUuid)
		&& m_hashCommitsByObject[objectUuid]->m_hashBeginEditByUser.contains(userUuid) )
	{
		BeginEditInfo begInfo = m_hashCommitsByObject[objectUuid]->m_hashBeginEditByUser.take(userUuid);
		removeObjectShot( begInfo.llObjectShotId );
	}
}

void CMultiEditDispatcher::cleanupAllBeginEditMarksByAccessToken( const IOSender::Handle& sUserAccessToken)
{
	QMutexLocker lock( this );

	QHash<QString, EditCommitInfoPtr>::iterator itObj;
	for( itObj = m_hashCommitsByObject.begin(); itObj!=m_hashCommitsByObject.end(); )
	{
		QHash<IOSender::Handle, BeginEditInfo>& hashBegEd = itObj.value()->m_hashBeginEditByUser;

		if( hashBegEd.contains( sUserAccessToken) )
			removeObjectShot( hashBegEd[sUserAccessToken].llObjectShotId );
		hashBegEd.remove(sUserAccessToken);

		if( hashBegEd.isEmpty() )
			itObj = m_hashCommitsByObject.erase(itObj);
		else
			itObj++;
	}
}



