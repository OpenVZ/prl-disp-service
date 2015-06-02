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
///		CMultiEditDispatcher.h
///
/// AUTHOR:
///		sandro
///
/// DESCRIPTION:
///	This class implements logic for simultaneously edit same configuration (VM, user profile and etc).
///
/// COMMENTS:
///	sergeyt
///
/////////////////////////////////////////////////////////////////////////////
#ifndef CMultiEditDispatcher_H
#define CMultiEditDispatcher_H

#include <QHash>
#include <QMutex>

#include "Libraries/IOService/src/IOCommunication/IOServer.h"
#include <prlsdk/PrlErrors.h>
#include <prlsdk/PrlTypes.h>

using namespace IOService;


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// class CMultiEditMergeHelper
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CMultiEditDispatcher
	: public QMutex  // ( to use QMutexLocker )
{
public:
	CMultiEditDispatcher();
	virtual ~CMultiEditDispatcher();

	// method to synchronization
	void lock()
	{
		QMutex::lock();
	}

	// method to synchronization
	void unlock()
	{
		QMutex::unlock();
	}

	/**
	 * Registries beginning of editing config by user
	 * @param editing config id
	 * @param began edit config user access token
	 * @NOTE: METHOD SHOULD BE CALLED BETWEEN lock() AND unlock() CALLS.
	 */
	void registerBeginEdit(const QString& sObjectId, const IOSender::Handle& sUserAccessToken);


	/**
	 * Checks whether specified config was changed
	 * @param config id
	 * @param user access token
	 * @param [out] nErrorCode Exists an error code if registerCommit() was used without registerBeginEdit()
	 * @NOTE: METHOD SHOULD BE CALLED WITH registerCommit() BETWEEN lock() AND unlock() CALLS.
	 */
	bool configWasChanged(
		const QString& sObjectId,
		const IOSender::Handle& sUserAccessToken,
		PRL_RESULT& nErrorCode);


	/**
	 * Registries commit of config by user
	 * @param commiting config id
	 * @param user access token that commiting config
	 * @NOTE: METHOD SHOULD BE CALLED WITH configWasChanged() BETWEEN lock() AND unlock() CALLS.
	 */
	void registerCommit(const QString& sObjectId, const IOSender::Handle& sUserAccessToken);

protected:
	virtual void removeObjectShot( const PRL_UINT64& ) {}
	PRL_UINT64 getShotId(const QString& sObjectId, const IOSender::Handle& sUserAccessToken);

	void registerBeginEdit(const QString& sObjectId, const IOSender::Handle& sUserAccessToken
		, const PRL_UINT64& llObjectShotId);

	void cleanupBeginEditMark(const QString& sObjectId
		, const IOSender::Handle& sUserAccessToken);

	// should be done for session disconnect
	void cleanupAllBeginEditMarksByAccessToken( const IOSender::Handle& sUserAccessToken);

private:

	struct BeginEditInfo
	{
		PRL_UINT64 tsBeginEditTimeStamp;
		PRL_UINT64 llObjectShotId;

		BeginEditInfo()
			:tsBeginEditTimeStamp(0), llObjectShotId(0) {}
		BeginEditInfo( const PRL_UINT64& ts, const PRL_UINT64& shotId)
			:tsBeginEditTimeStamp(ts), llObjectShotId(shotId){}
	};

	struct EditCommitInfo
	{
		PRL_UINT64	tsCommitTimeStamp;
		QHash<IOSender::Handle, BeginEditInfo>  m_hashBeginEditByUser;

		EditCommitInfo()
			:tsCommitTimeStamp( 0 ){}
	};
	typedef SmartPtr<EditCommitInfo> EditCommitInfoPtr;

	QHash<QString, EditCommitInfoPtr >  m_hashCommitsByObject;
};

#endif
