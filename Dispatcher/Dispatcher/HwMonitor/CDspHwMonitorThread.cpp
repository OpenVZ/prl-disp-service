///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHwMonitorThread.cpp
///
/// Hardware changes monitor thread implementation
///
/// @author ilya
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
///////////////////////////////////////////////////////////////////////////////

#include "CDspHwMonitorThread.h"
#include <prlcommon/Logging/Logging.h>
#include "CDspHwMonitorNotifier.h"
#include "CDspCommon.h"
#include <prlsdk/PrlErrors.h>
#include "Interfaces/Config.h"
#include "CDspService.h"
#include "CDspHwMonitorHandler.h"

#include <prlcommon/Std/PrlAssert.h>

#include "Interfaces/Debug.h"

#define CHECK_HW_INTERVAL_IN_MSEC  (5 * 60 * 1000)


CDspHwMonitorThread::CDspHwMonitorThread()
{
	m_pNotifier			= NULL;
	m_pTimer			= NULL;
}

CDspHwMonitorThread::~CDspHwMonitorThread()
{
}

void CDspHwMonitorThread::run()
{
	WRITE_TRACE(DBG_INFO, "Start Hardware Monitor Thread");

	COMMON_TRY
    {
		// Create notification object
		m_pNotifier = new CDspHwMonitorNotifier;

		// Create hardware monitor thread & start hardware monitor thread
		CDspHwMonitorHandler * pHwMonitorHandler = new CDspHwMonitorHandler;
		qRegisterMetaType<PRL_DEVICE_TYPE>("PRL_DEVICE_TYPE");
		bool bConnected = connect( pHwMonitorHandler
			,SIGNAL( deviceChanged(PRL_DEVICE_TYPE, QString, unsigned int ) )
			, m_pNotifier
			, SLOT( onDeviceChange(PRL_DEVICE_TYPE, QString, unsigned int ) )
		);
		PRL_ASSERT(bConnected);

		pHwMonitorHandler->startHandleDevices();

		// Create timer	& start it for periodic work
		m_pTimer = new QTimer;
		bConnected = connect( m_pTimer, SIGNAL( timeout() ), m_pNotifier, SLOT( onCheckHwChanges() ) );
		PRL_ASSERT(bConnected);

		bConnected = connect( this, SIGNAL( checkHwChanges() ), m_pNotifier, SLOT( onCheckHwChanges() ),
								Qt::QueuedConnection );
		PRL_ASSERT(bConnected);

		Q_UNUSED(bConnected);

		m_pTimer->start( CHECK_HW_INTERVAL_IN_MSEC );

		// Start thread's event loop needed for QTimer
		QThread::exec();

		// Stop timer & delete it
		m_pTimer->stop();
		delete m_pTimer;
		m_pTimer = NULL;

		// Stop hardware detecting thread & delete object
		pHwMonitorHandler->stopHandleDevices();
		delete pHwMonitorHandler;

		// Delete notification object
		delete m_pNotifier;
		m_pNotifier = NULL;
	}
    COMMON_CATCH;

	WRITE_TRACE(DBG_INFO, "Stop Hardware Monitor Thread");
}

void CDspHwMonitorThread::FinalizeThreadWork()
{
	if( QThread::isRunning() )
	{
		// Exit from event loop
		QThread::exit();
		QThread::wait();
	}
}

/** add/remove USB device ID to list of Usb Id to exclude auto connect*/
void CDspHwMonitorThread::setUsbDeviceManualConnected( const QString & strDevId, bool bConnect )
{
	if( m_pNotifier )
		m_pNotifier->setUsbDeviceManualConnected( strDevId, bConnect );
}

/** add vm to list of vms which on deinitialization state in prl_vm_app*/
void CDspHwMonitorThread::addVmOnDeinitialization( const CVmIdent & ident )
{
	if( m_pNotifier )
		m_pNotifier->addVmOnDeinitialization( ident );
}

/** remove vm to list of vms which on deinitialization state in prl_vm_app*/
void CDspHwMonitorThread::removeVmOnDeinitialization( const CVmIdent & ident )
{
	if( m_pNotifier )
		m_pNotifier->removeVmOnDeinitialization( ident );
}

/** process vm stop */
void CDspHwMonitorThread::processVmStop( const CVmIdent & ident, CDspLockedPointer<CDspHostInfo> spHostInfo )
{
	if( m_pNotifier )
		m_pNotifier->processVmStop( ident, spHostInfo );
}

void CDspHwMonitorThread::forceCheckHwChanges()
{
	emit checkHwChanges();
}
