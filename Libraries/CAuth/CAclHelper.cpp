///////////////////////////////////////////////////////////////////////////////
///
/// @file CAclHelper.cpp
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

#include "CAclHelper.h"

#include "Interfaces/ParallelsQt.h"
#include "Libraries/Logging/Logging.h"

namespace Parallels {

namespace CAclHelper {

CAclInfoPrivate::CAclInfoPrivate()
:
m_nOwnerType(UnknownOwnerType),
m_nOwnerId(WRONG_OWNER_ID)
{}

CAclInfoPrivate::~CAclInfoPrivate()
{}

QString CAclInfoPrivate::OwnerName() const
{
	if (m_sOwnerName.isEmpty())//Wasn't extracted yet
		m_sOwnerName =  concreteOwnerName();

	return (m_sOwnerName);
}

quint32 CAclInfoPrivate::OwnerId() const
{
	if (WRONG_OWNER_ID == m_nOwnerId)//Wasn't extracted yet
		m_nOwnerId = concreteOwnerId();

	return (m_nOwnerId);
}

TAclOwnerType CAclInfoPrivate::OwnerType() const
{
	if (UnknownOwnerType == m_nOwnerType)//Wasn't extracted yet
		m_nOwnerType = concreteOwnerType();

	return (m_nOwnerType);
}

bool CAclInfoPrivate::IsAllow() const
{
	return (false);
}

bool CAclInfoPrivate::IsWrite() const
{
	return (false);
}

bool CAclInfoPrivate::IsRead() const
{
	return (false);
}

bool CAclInfoPrivate::IsExecute() const
{
	return (false);
}

QString CAclInfoPrivate::concreteOwnerName() const
{
	return (QString());
}

quint32 CAclInfoPrivate::concreteOwnerId() const
{
	return (WRONG_OWNER_ID);
}

TAclOwnerType CAclInfoPrivate::concreteOwnerType() const
{
	return (UnknownOwnerType);
}

CAclInfo::CAclInfo()
{}

CAclInfo::CAclInfo(const SmartPtr<CAclInfoPrivate> &pAclImp)
:
m_pAclImp(pAclImp)
{}

bool CAclInfo::IsNull() const
{
	return (m_pAclImp.getImpl() == NULL);
}

bool CAclInfo::operator==(const CAclInfo &_acl) const
{
	return
	(
		IsNull()		== _acl.IsNull()		&&
		IsAllow()		== _acl.IsAllow()		&&
		OwnerType() == _acl.OwnerType() &&
		OwnerId()		== _acl.OwnerId()		&&
		OwnerName() == _acl.OwnerName() &&
		IsWrite()		== _acl.IsWrite()		&&
		IsRead()		== _acl.IsRead()		&&
		IsExecute()	== _acl.IsExecute()
	);
}

bool CAclInfo::operator!=(const CAclInfo &_acl) const
{
	return (!operator==(_acl));
}

QString CAclInfo::OwnerName() const
{
		if (!IsNull())
			return (m_pAclImp->OwnerName());

	return (QString());
}

quint32 CAclInfo::OwnerId() const
{
	if (!IsNull())
		return (m_pAclImp->OwnerId());

	return (WRONG_OWNER_ID);
}

TAclOwnerType CAclInfo::OwnerType() const
{
	if (!IsNull())
		return (m_pAclImp->OwnerType());

	return (UnknownOwnerType);
}

bool CAclInfo::IsAllow() const
{
	if (!IsNull())
		return (m_pAclImp->IsAllow());

	return (false);
}

bool CAclInfo::IsDeny() const
{
	return (!IsAllow());
}

bool CAclInfo::IsWrite() const
{
	if (!IsNull())
		return (m_pAclImp->IsWrite());

	return (false);
}

bool CAclInfo::IsRead() const
{
	if (!IsNull())
		return (m_pAclImp->IsRead());

	return (false);
}

bool CAclInfo::IsExecute() const
{
	if (!IsNull())
		return (m_pAclImp->IsExecute());

	return (false);
}

SmartPtr<CAclInfoPrivate> CAclInfo::GetAclImp() const
{
	return (m_pAclImp);
}

CAclSet CAclSet::GetAclsByOwnerName(TAclOwnerType nOwnerType, const QString &sOwnerName)
{
	CAclSet _res_acls;
	foreach(CAclInfo _acl, *this)
	{
		if (_acl.OwnerType() == nOwnerType && _acl.OwnerName() == sOwnerName)
			_res_acls.append(_acl);
	}

	return (_res_acls);
}

namespace {
/**
 * Internal helper: generates effective rights mask for specified owner
 * @param target file path
 * @param owner type
 * @Param owner name
 * @return effective rights mask
 */
CAuth::AccessMode GetEffectiveRightsForOwner(const QString &sFilePath, TAclOwnerType nOwnerType, const QString &sOwnerName)
{
	CAuth::AccessMode nEffectiveRigths = 0;
#ifndef _WIN_
	CAclSet _acls = GetFileAcls(sFilePath);

	//First phase: collect info from allow ACL entries
	foreach(CAclInfo _acl, _acls)
	{
		if (_acl.IsAllow())
		{
			TAclOwnerType nAclOwnerType = _acl.OwnerType();
			if ( (nAclOwnerType == nOwnerType && _acl.OwnerName() == sOwnerName) ||
					(nOwnerType == User && nAclOwnerType == Group && CAuth::isUserMemberOfGroup(sOwnerName, gid_t(_acl.OwnerId()))) )
			{
				if (_acl.IsWrite())
					nEffectiveRigths |= CAuth::fileMayWrite;
				if (_acl.IsRead())
					nEffectiveRigths |= CAuth::fileMayRead;
				if (_acl.IsExecute())
					nEffectiveRigths |= CAuth::fileMayExecute;
			}
		}
	}

	//Second phase: collect info from deny ACL entries (deny ACL entries have higher priority)
	foreach(CAclInfo _acl, _acls)
	{
		if (_acl.IsDeny())
		{
			TAclOwnerType nAclOwnerType = _acl.OwnerType();
			if ( (nAclOwnerType == nOwnerType && _acl.OwnerName() == sOwnerName) ||
					(nOwnerType == User && nAclOwnerType == Group && CAuth::isUserMemberOfGroup(sOwnerName, gid_t(_acl.OwnerId()))) )
			{
				if (_acl.IsWrite())
					nEffectiveRigths &= ~CAuth::fileMayWrite;
				if (_acl.IsRead())
					nEffectiveRigths &= ~CAuth::fileMayRead;
				if (_acl.IsExecute())
					nEffectiveRigths &= ~CAuth::fileMayExecute;
			}
		}
	}
#else
	Q_UNUSED(sFilePath);
	Q_UNUSED(nOwnerType);
	Q_UNUSED(sOwnerName);
#endif

	return (nEffectiveRigths);
}

}//namespace

CAuth::AccessMode GetEffectiveRightsForGroup(const QString &sFilePath, const QString &sGroupName)
{
	return (GetEffectiveRightsForOwner(sFilePath, Group, sGroupName));
}

CAuth::AccessMode GetEffectiveRightsForUser(const QString &sFilePath, const QString &sUserName)
{
	return (GetEffectiveRightsForOwner(sFilePath, User, sUserName));
}

}//namespace CAclHelper

}//namespace Parallels
