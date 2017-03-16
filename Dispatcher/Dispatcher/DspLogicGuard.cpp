/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include "DspLogicGuard.h"


#include "Guards/SentryBase.h"
//#include "Guards/ConcurrentExecution_Sentry.h"
//#include "Guards/ForbiddenToVmRunning_Sentry.h"
#include "Guards/Server_Sentry.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


DspLogicGuard::DspLogicGuard( )
{
//  m_SentryList.push_back( new ConcurrentExecution_Sentry );
//	m_SentryList.push_back( new ForbiddenToVmRunning_Sentry );
	m_SentryList.push_back( SmartPtr<SentryBase>(new Server_Sentry) );
}

PRL_RESULT DspLogicGuard::isCommandAllowed(
	SmartPtr<CDspClient>& pUser,
	const SmartPtr<IOPackage>& p,
	CVmEvent&  outErrParams)
{
	PRL_RESULT ret = PRL_ERR_SUCCESS ;
	foreach( SmartPtr<SentryBase> pSentry, m_SentryList)
	{
		ret = pSentry->isCommandAllowed( pUser, p, outErrParams );
		if ( ! PRL_SUCCEEDED (ret) )
			break;
	}
	return ret;
}
