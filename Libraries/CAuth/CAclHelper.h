///////////////////////////////////////////////////////////////////////////////
///
/// @file CAclHelper.h
///
/// Wrappers set that simplify work with ACLs
///
/// @author sandro
/// @owner sergeym
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

#ifndef CAclHelper_H
#define CAclHelper_H

#include <QList>
#include <QString>

#include <prlcommon/Std/SmartPtr.h>
#include "Libraries/CAuth/CAuth.h"

namespace Parallels {

namespace CAclHelper {

/**
 * Enum that specifies ACL owner type
 */
typedef enum {
	UnknownOwnerType	= 0,
	User							= 1,
	Group							= 2
} TAclOwnerType;

#define WRONG_OWNER_ID quint32(-1)

/**
 * Class specifying abstract model of ACL which should be implemented for each platform individually
 */
struct CAclInfoPrivate
{
	/**
	 * Default class constructor.
	 * Serves for proper class members initialization
	 */
	CAclInfoPrivate();
	/**
	 * Virtual destructor for proper child instances deletion
	 */
	virtual ~CAclInfoPrivate();

	/**
	 * Returns ACL owner name (user or group)
	 */
	QString OwnerName() const;

	/**
	 * Returns ACL owner user system id (uid_t or gid_t)
	 */
	quint32 OwnerId() const;

	/**
	 * Returns ACL onwer type
	 */
	TAclOwnerType OwnerType() const;

	/**
	 * Returns whether ACL allowed permissions
	 */
	virtual bool IsAllow() const;

	/**
	 * Returns sign whether write permission set
	 */
	virtual bool IsWrite() const;

	/**
	 * Returns sign whether read permission set
	 */
	virtual bool IsRead() const;

	/**
	 * Returns sign whether execute permission set
	 */
	virtual bool IsExecute() const;

private:
	/**
	 * Concrete overridable implementations of extraction owner name, id and type
	 */
	virtual QString concreteOwnerName() const;
	virtual quint32 concreteOwnerId() const;
	virtual TAclOwnerType concreteOwnerType() const;

private:
	/**
	 * Cache parameters in order to optimize ACL entry info processing
	 */
	mutable QString m_sOwnerName;
	mutable TAclOwnerType m_nOwnerType;
	mutable quint32 m_nOwnerId;
};

/**
 * Simple struct describing ACL
 */
struct CAclInfo
{
	/**
	 * Default class constructor (to be it possible use object at containers)
	 */
	CAclInfo();

	/**
	 * Class constructor
	 * @param pointer to the platform dependent ACL info object
	 */
	CAclInfo(const SmartPtr<CAclInfoPrivate> &pAclImp);

	/**
	 * Compares current ACL entry with another one
	 * @param reference to comparing ACL entry object
	 * @param sign whether ACL entries equal
	 */
	bool operator==(const CAclInfo &_acl) const;

	/**
	 * Non equal operator provided for convenience
	 * @param reference to comparing ACL entry object
	 * @param sign whether ACL entries not equal
	 */
	bool operator!=(const CAclInfo &_acl) const;

	/**
	 * Returns sign whether ACL info object is null
	 */
	bool IsNull() const;

	/**
	 * Returns ACL owner name (user or group)
	 */
	QString OwnerName() const;

	/**
	 * Returns ACL owner user system id (uid_t or gid_t)
	 */
	quint32 OwnerId() const;

	/**
	 * Returns ACL onwer type
	 */
	TAclOwnerType OwnerType() const;

	/**
	 * Returns whether ACL allowed permissions
	 */
	bool IsAllow() const;

	/**
	 * Returns whether ACL deny permissions
	 */
	bool IsDeny() const;

	/**
	 * Returns sign whether write permission set
	 */
	bool IsWrite() const;

	/**
	 * Returns sign whether read permission set
	 */
	bool IsRead() const;

	/**
	 * Returns sign whether execute permission set
	 */
	bool IsExecute() const;

	/**
	 * Returns pointer to the object of platform dependent implementation of ACL entry
	 */
	SmartPtr<CAclInfoPrivate> GetAclImp() const;

private:
	/**
	 * Pointer to the private platform dependent ACL info object
	 */
	SmartPtr<CAclInfoPrivate> m_pAclImp;
};

/**
 * Wrapper around ACLs set
 */
struct CAclSet : public QList<CAclInfo>
{
	/**
	 * Returns ACLs set for specified owner
	 * @param owner type
	 * @param owner name
	 */
	CAclSet GetAclsByOwnerName(TAclOwnerType nOwnerType, const QString &sOwnerName);
};

//**************************************Common ACL API***************************************

/**
 * Checks whether ACL supported for specified file path
 * @param checking file path
 */
bool IsAclSupported(const QString &sFilePath);

/**
 * Returns ACLs list for specified file path
 * @param path to file which ACLs should be extracted
 */
CAclSet GetFileAcls(const QString &sFilePath);

/**
 * Applies specified ACLs entries list to the passed file path
 * @param target file path
 * @param applying ACL entries list
 * @return sign whether ACL entries list was successfully applied
 */
bool ApplyAclsToFile(const QString &sFilePath, const CAclSet &_acls);

/**
 * Returns effective rights mask for specified group
 * @param target file path
 * @param checking group name
 * @return effective rights mask
 */
CAuth::AccessMode GetEffectiveRightsForGroup(const QString &sFilePath, const QString &sGroupName);

/**
 * Returns effective rights mask for specified user
 * @param target file path
 * @param checking user name
 * @return effective rights mask
 */
CAuth::AccessMode GetEffectiveRightsForUser(const QString &sFilePath, const QString &sUserName);

}//namespace CAclHelper

using CAclHelper::CAclInfo;
using CAclHelper::CAclSet;

}//namespace Parallels

#endif
