///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHwMonitorHandler_win.cpp
///
/// Platform dependent hardware configuration changes handler implementation for Windows
///
/// @author ilya
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
///////////////////////////////////////////////////////////////////////////////

#ifndef UNICODE
#define UNICODE
#endif

#include "CDspHwMonitorHandler.h"
#include <prlcommon/Logging/Logging.h>
#include <Devices/Usb/UsbDescriptors.h>
#include <System/Usb/Drivers/Win/prl_usb_mng/prl_usb_mng_ioctl.h>
#include <prlcommon/Std/PrlAssert.h>
#include <iphlpapi.h>

static CDspHwMonitorHandler *g_pHwMonitorHandler = NULL;

void g_onDeviceChange(PRL_DEVICE_TYPE dev_type, const QString &dev_name, unsigned int event_code)
{
	g_pHwMonitorHandler->onDeviceChange(dev_type, dev_name, event_code);
}

/**
* Class constructor
*/
CDspHwMonitorHandler::CDspHwMonitorHandler( QObject * parent )
			: QThread (parent),
			m_hResetEvent(INVALID_HANDLE_VALUE),
			m_FinalizationMutex(QMutex::Recursive)
{
	g_pHwMonitorHandler = this;

	/* Event to stop HWMonitor */
	m_hResetEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hResetEvent == INVALID_HANDLE_VALUE)
		WRITE_TRACE(DBG_FATAL, "Failed to create StopEvent");

	PRL_ASSERT(m_hResetEvent != INVALID_HANDLE_VALUE);
}

/**
* Starts handle device configuration changes
*/
void CDspHwMonitorHandler::startHandleDevices()
{
	QThread::start();
}

/**
* Stop handling device configuration changes
*/
void CDspHwMonitorHandler::stopHandleDevices()
{
	QMutexLocker _lock(&m_FinalizationMutex);
	if (m_hResetEvent != INVALID_HANDLE_VALUE) {

			// set reset event
			::SetEvent(m_hResetEvent);
			// Wait thread complition
			QThread::wait();
	}
}

/**
* Device changed handler function
* @param dev_name platform dependent device name
* @param event_code new device state connected/disconnected
*/
void CDspHwMonitorHandler::onDeviceChange(PRL_DEVICE_TYPE dev_type, const QString &dev_name, unsigned int event_code )
{
	emit deviceChanged(dev_type, dev_name, event_code );
}

// Open USB Manager and setup USB-changed event
// @return INVALID_HANDLE_VALUE if open failed
static HANDLE openUSBManager(HANDLE hUsbConnectEvent)
{
	HANDLE hUsbManager = ::CreateFile( PRL_USBMGR_SRV_LINK,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (INVALID_HANDLE_VALUE == hUsbManager)
		WRITE_TRACE(DBG_FATAL, "Can't open USB manager service (%d)", GetLastError());
	else {
		DWORD size = 0;
		// Pass to driver m_hConnectEvent, m_hDisconnectEvent events only!
		if (!::DeviceIoControl(	hUsbManager, IOCTL_SET_REPLUG_EVENT,
			&hUsbConnectEvent, sizeof(HANDLE),
			NULL, 0, &size, NULL)) {
				WRITE_TRACE(DBG_FATAL, "IOCTL_SET_REPLUG_EVENT failed (%d)", GetLastError());
		}
	}

	return hUsbManager;
}


/*
* Overridden method of thread working body
*/
void CDspHwMonitorHandler::run()
{
	if (m_hResetEvent == INVALID_HANDLE_VALUE)
		return;

	HANDLE hUsbConnectEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hUsbConnectEvent == INVALID_HANDLE_VALUE) {
		WRITE_TRACE(DBG_FATAL, "Can't create USB-changed event!");
		return;
	}

	HANDLE hNetConfigEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hNetConfigEvent == INVALID_HANDLE_VALUE) {
		::CloseHandle(hUsbConnectEvent);
		WRITE_TRACE(DBG_FATAL, "Can't create Net-changed event!");
		return;
	}

	HANDLE hUsbManager = openUSBManager(hUsbConnectEvent);

	HANDLE eventsArray[3] = {
		m_hResetEvent,
		hUsbConnectEvent,
		hNetConfigEvent
	};

	// Setup notification for network changes
	OVERLAPPED ovlNet;
	ovlNet.hEvent = hNetConfigEvent;

	/* Wait for hw-changed events or stop thread event */
	DWORD  waitResult;
	do {
		static bool errLogged = false;
		HANDLE h; // should not be closed.
		if (::NotifyAddrChange(&h, &ovlNet) != ERROR_IO_PENDING && !errLogged) {
			WRITE_TRACE(DBG_FATAL, "NotifyAddrChange(..) returned an error");
			errLogged = true;
		}

		waitResult = ::WaitForMultipleObjects(	sizeof(eventsArray)/sizeof(HANDLE),
												eventsArray,
												FALSE,
												INFINITE);
		switch(waitResult) {
		// USB-Event
		case (WAIT_OBJECT_0+1):
			g_onDeviceChange(PDE_USB_DEVICE, QString(""), 1);
			break;
		// Network Event
		case (WAIT_OBJECT_0+2):
			g_onDeviceChange(PDE_GENERIC_NETWORK_ADAPTER, QString(""), 1);
			break;
		default:
			break;
		}
	} while (waitResult != WAIT_OBJECT_0);

	::CloseHandle(hUsbConnectEvent);
	::CloseHandle(hNetConfigEvent);

	if (hUsbManager != INVALID_HANDLE_VALUE)
		::CloseHandle(hUsbManager);
}

CDspHwMonitorHandler::~CDspHwMonitorHandler()
{
	if (m_hResetEvent != INVALID_HANDLE_VALUE)
			::CloseHandle(m_hResetEvent);
}
