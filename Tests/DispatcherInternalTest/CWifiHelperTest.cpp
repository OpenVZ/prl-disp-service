/////////////////////////////////////////////////////////////////////////////
///
///	@file CWifiHelperTest.cpp
///
///	Tests fixture class for testing Wi-Fi settings storing helper
///
///	@author sandro
/// @owner  sergeym
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
/////////////////////////////////////////////////////////////////////////////

#include "CWifiHelperTest.h"

#include <prlcommon/PrlUuid/Uuid.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include "Libraries/WifiHelper/CWifiStoreHelper.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Interfaces/ParallelsQt.h>

#include <QDir>

using namespace Parallels;

void CWifiHelperTest::init()
{
	m_sStorePath = QDir::tempPath() + "/" + Uuid::createUuid().toString();
	QVERIFY(QDir().mkpath(m_sStorePath));
}

void CWifiHelperTest::cleanup()
{
	QVERIFY(CFileHelper::ClearAndDeleteDir(m_sStorePath));
}

namespace {
bool CheckProfilePresents( const SmartPtr<TWifiProfileInfo> &pTargetProfile, int nProfileIndex, const QList<SmartPtr<TWifiProfileInfo> > &_profiles )
{
	if ( nProfileIndex >= _profiles.size() )
		return ( false );
	const SmartPtr<TWifiProfileInfo> &pProfile = _profiles.at( nProfileIndex );
	if ( pProfile->m_sName == pTargetProfile->m_sName )
		if ( pProfile->m_sType == pTargetProfile->m_sType )
			if ( pProfile->m_sContent == pTargetProfile->m_sContent )
				return ( true );
	WRITE_TRACE(DBG_FATAL, "Profile wasn't found or incorrect profiles order: '%s' '%s'\n[%s]",
					QSTR2UTF8(pTargetProfile->m_sName),
					QSTR2UTF8(pTargetProfile->m_sType),
					QSTR2UTF8(pTargetProfile->m_sContent));
	return ( false );
}

bool CheckIfacePresents( const SmartPtr<TWifiIfaceInfo> &pTargetIface, const QList<SmartPtr<TWifiIfaceInfo> > &_ifaces )
{
	foreach( const SmartPtr<TWifiIfaceInfo> &pIface, _ifaces )
	{
		if ( pTargetIface->m_sPhysicalAddress == pIface->m_sPhysicalAddress )
		{
			int nProfileIndex = 0;
			foreach( const SmartPtr<TWifiProfileInfo> &pProfileInfo, pTargetIface->m_Profiles )
			{
				if ( !CheckProfilePresents( pProfileInfo, nProfileIndex++, pIface->m_Profiles ) )
					return ( false );
			}
			return ( true );
		}
	}
	WRITE_TRACE(DBG_FATAL, "Failed to find iface: '%s' '%s' '%s'",
					QSTR2UTF8(pTargetIface->m_sGuid),
					QSTR2UTF8(pTargetIface->m_sDescription),
					QSTR2UTF8(pTargetIface->m_sPhysicalAddress));
	return ( false );
}
}

void CWifiHelperTest::testStoreLoadWifiSettings()
{
	QList<SmartPtr<TWifiIfaceInfo> > _wifi_ifaces;

	SmartPtr<TWifiIfaceInfo> pIface1(new TWifiIfaceInfo(Uuid::createUuid().toString(), "iface1", HostUtils::generateMacAddress()));
	pIface1->m_Profiles.append(SmartPtr<TWifiProfileInfo>(new TWifiProfileInfo("SW-PEAP", "user", "some profile data")));
	pIface1->m_Profiles.append(SmartPtr<TWifiProfileInfo>(new TWifiProfileInfo("SW-WPA2", "user", "some profile data2")));
	_wifi_ifaces.append(pIface1);

	SmartPtr<TWifiIfaceInfo> pIface2(new TWifiIfaceInfo(Uuid::createUuid().toString(), "iface2", HostUtils::generateMacAddress()));
	pIface2->m_Profiles.append(SmartPtr<TWifiProfileInfo>(new TWifiProfileInfo("SW-GUEST", "user", "some profile data3")));
	_wifi_ifaces.append(pIface2);

	QVERIFY(CWifiStoreHelper::StoreWifiSettings( _wifi_ifaces, m_sStorePath ));

	QList<SmartPtr<TWifiIfaceInfo> > _wifi_ifaces2 = CWifiStoreHelper::LoadWifiSettings( m_sStorePath );
	QCOMPARE(_wifi_ifaces2.size(), 2);

	foreach( const SmartPtr<TWifiIfaceInfo> &pIface, _wifi_ifaces )
	{
		QVERIFY(CheckIfacePresents( pIface, _wifi_ifaces2 ));
	}
}

