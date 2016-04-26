/////////////////////////////////////////////////////////////////////////////
///
///	@file SimpleServerWrapper.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Simple server handle wrapper for convenience.
///
///	@author sandro
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

#ifndef SimpleServerWrapper_H
#define SimpleServerWrapper_H

#include <prlsdk/helpers/SdkWrap/SdkHandleWrap.h>
#include "Tests/CommonTestsUtils.h"

class SimpleServerWrapper
{
public:
	/**
	 * Class constructor - establishes connection to the localhost dispatcher
	 * with specified credentials
	 * @param creating connectio user login (NULL can be used for login local connection mode)
	 */
	SimpleServerWrapper(char *sUserLogin = TestConfig::getUserLogin(), bool bUseNonInteractiveMode = true);

	~SimpleServerWrapper();

	bool IsConnected();

	bool Logoff();
	bool Login(char *sUserLogin = TestConfig::getUserLogin(), bool bUseNonInteractiveMode = true );
	bool LoginLocal( bool bUseNonInteractiveMode = true )
	{
		return Login(0, bUseNonInteractiveMode);
	}

	inline operator PRL_HANDLE () const
	{
		return m_ServerHandle;
	}

	inline operator SdkHandleWrap()
	{
		return (GetServerHandle());
	}
	inline SdkHandleWrap GetServerHandle() const
	{
		return (m_ServerHandle);
	}

	bool CreateTestVm(const SdkHandleWrap &hVm = SdkHandleWrap());
	bool RegisterVm(const QString &sVmConfigPath);
	SdkHandleWrap GetTestVm(QString sSearchVmUuid = QString());

	SdkHandleWrap GetVmByUuid( const QString& vmUuid );
	SdkHandleWrap GetServerHandle();

	SdkHandleWrap GetUserProfile();
	bool isLocalAdmin();

private:
	static void GetUserProfile( bool& bOutRes
		, const SdkHandleWrap& hServerHandle, SdkHandleWrap& out_hUserProfile );

private:
	SdkHandleWrap m_ServerHandle;
	SdkHandleWrap m_VmHandle;
	bool m_bIsVmWasCreatedByThisInstance;

	bool m_isLocalAdmin;
	bool m_isLocalAdminInited;
};

#endif//SimpleServerWrapper_H
