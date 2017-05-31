////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	CDspUserHelper.h
///
/// @brief
///	Definition of the class CDspUserHelper
///
/// @brief
///	This class implements user dispatching logic
///
/// @author sergeyt
///	SergeyM
///
/// @date
///	2006-04-04
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspUserHelper_H_
#define __CDspUserHelper_H_

#include <QObject>
#include <QMutex>
#include <QHash>

#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>
#include "CDspClient.h"
#include "CDspHostSettingsHelper.h"
#include <prlxmlmodel/DispConfig/CDispUser.h>

using namespace IOService;

class CDspClientManager;

/**
 * @brief This class implements user dispatching logic
 * @author SergeyM
 */
class CDspUserHelper
{
public:
	// constructor
	CDspUserHelper();

	// destructor
	~CDspUserHelper();



	// Process dispacher logon request
	SmartPtr<CDspClient> processDispacherLogin (
				    const IOSender::Handle& h,
				    const SmartPtr<IOPackage>& p );

	// Process user's logon request
	SmartPtr<CDspClient> processUserLogin ( const IOSender::Handle&,
											const SmartPtr<IOPackage>&,
											bool &bWasPreAuthorized );

	// Process user logon via user & password
	SmartPtr<CDspClient> processLogin (
		    const QString user, const QString& password, const QString prevSessionUuid,
		    const IOSender::Handle& h, const SmartPtr<IOPackage>& p, quint32 nFlags = 0, bool *pbWasPreAuthorized = NULL );

	// Process user's local login request
	SmartPtr<CDspClient> processUserLoginLocal ( const IOSender::Handle&,
								 const SmartPtr<IOPackage>& );

	// Process user's logoff request
	bool processUserLogoff ( SmartPtr<CDspClient>&,
							 const  SmartPtr<IOPackage>& );

	// Sets noninteractive session mode
	void setNonInteractiveSession ( SmartPtr<CDspClient>&,
							 const  SmartPtr<IOPackage>& );

	// Sends user's profile
	void sendUserProfile ( const IOSender::Handle&,
							SmartPtr<CDspClient>&,
							const SmartPtr<IOPackage>&);

	// Sends list information of all users
	void sendUserInfoList ( const IOSender::Handle&,
							SmartPtr<CDspClient>&,
							const SmartPtr<IOPackage>&);

	// Sends user information
	void sendUserInfo ( const IOSender::Handle&,
						SmartPtr<CDspClient>&,
						const SmartPtr<IOPackage>&);

	// Perform  user's profile begin edit operation
	void userProfileBeginEdit ( const IOSender::Handle&,
								SmartPtr<CDspClient>&,
								const SmartPtr<IOPackage>& );

	// Perform  user's profile commit operation
	void userProfileCommit ( const IOSender::Handle&,
							 SmartPtr<CDspClient>&,
							 const SmartPtr<IOPackage>& );

	// Sends list information of all host users
	void sendAllHostUsers ( const IOSender::Handle&,
							const SmartPtr<IOPackage>& );

	// search from qsettings proxy user passwords
	bool searchCachedProxyPassword(/*IN*/ const QString & strProxyName,
							/*OUT*/CredentialsList& creds );

	// search from qsettings proxy user defined settings
	bool searchUserDefinedProxySettings(/*IN*/ const QString & strUrl,
							/*IN*/const QList<QString> & ignoreUsers,
							/*OUT*/QString & host,
							/*OUT*/uint & port,
							/*OUT*/QString & user,
							/*OUT*/QString & password,
							/*OUT*/QString & userId,
							/*IN*/bool bSecure );

	enum {
		MAX_PASSWORD_LENGTH = 256,
	};

public:
	/////////////////////////////////////
	//
	//  dispatcher internal requests
	//
	/////////////////////////////////////

	/**
	* @brief Make list of users sessions by userUuid
	* @return list of session ( CDspClients ).
	**/
	QList< SmartPtr<CDspClient> > getClientSessionByUserUuid( const QString& userSettingsUuid );

	//QList< SmartPtr<CDspClient> > getClientSessionByVmUuid( const QString& userSettingsUuid );

	// make login response packet
	SmartPtr<IOPackage> makeLoginResponsePacket( const SmartPtr<CDspClient>& pSession
		, const SmartPtr<IOPackage>& pkg );

private:
	/**
	 * @brief Check whether advanced authorization required over trusted channel connection.
	 * @param h pointer to procedure initiator object
	 * @param p pointer to login package
	 * @return true, when advanced authorization required.
	 */
	bool shouldUseAdvancedAuthMode( const IOSender::Handle& h,
									const SmartPtr<IOPackage>& p);

	// Setup user's defaults
	bool setupUserDefaults ( CDispUser* p_user, CAuthHelper& pAuthHelper );

	/**
	 * Fills user preferences and adds it in logged users hash
	 * @param pointer to procedure initiator object
	 * @param pointer to processing user instance
	 * @param processing command type
	 * @param remote hostname from which user was logon
	 * @return true if success, false otherwise
	 */
	bool fillUserPreferences ( const IOSender::Handle&,
							   const SmartPtr<IOPackage>&,
							   SmartPtr<CDspClient>& );

	/**
	 * Returns user information list (see XML Model class UserInfo)
	 * @param sUserId if it is empty then all users' information returns
	 * @return user information list
	 */
	QStringList getUserInfoList(const QString& sUserId = QString());

	// save to dispatcher qsettings password cached paroxy data
	PRL_RESULT saveCachedProxyData( SmartPtr<CDispUserCachedData> pData,
									const QString & strUserId );

	// save to dispatcher qsettings password cached paroxy data
	PRL_RESULT loadCachedProxyData( SmartPtr<CDispUserCachedData> pData,
									const QString & strUserId );

	// save to dispatcher qsettings user defined data
	PRL_RESULT saveUserDefinedProxySecure( SmartPtr<CDispUserSettings> pData,
									const QString & strUserId );

	// load from dispatcher qsettings user defined data and adds it to CDispUserSettings object
	PRL_RESULT addUserDefinedProxySecure( SmartPtr<CDispUserSettings> pData,
									const QString & strUserId );


#ifdef SENTILLION_VTHERE_PLAYER
	// Check sentillion client
	bool isSentillionClient(const QString& qsSessionId) const;
#endif	// SENTILLION_VTHERE_PLAYER

}; // class CDspUserHelper

#endif // __CDspUserHelper_H_
