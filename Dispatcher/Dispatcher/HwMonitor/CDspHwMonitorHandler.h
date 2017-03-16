///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHwMonitorHandler.h
///
/// Platform dependent hardware configuration changes handler implementation.
/// The framework calls onDeviceChange() member function to notify of a change
/// to the hardware configuration of a device or the computer.
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

#ifndef _DSP_HW_MONITOR_HANDLER_H_
#define _DSP_HW_MONITOR_HANDLER_H_

#include <QObject>
#include <QThread>
#include <QMutex>
#include "Interfaces/Config.h"
#include <prlsdk/PrlEnums.h>

#ifdef _WIN_
#include <windows.h>
#include <Dbt.h>
#endif //_WIN_

#ifdef _LIN_
#include <pthread.h>
#endif

/**
 * Hardware configuration changes handler implementation
 */
class CDspHwMonitorHandler : public QThread
{
	Q_OBJECT

public:
	/** Class constructor */
	CDspHwMonitorHandler( QObject * parent = 0 );

	~CDspHwMonitorHandler();
	/**
	* Starts handle device configuration changes
	*/
	void startHandleDevices();
	/**
	* Stop handling device configuration changes
	*/
	void stopHandleDevices();
	/**
	* Device changed handler function
	* @param dev_name platform dependent device name
	* @param event_code new device state connected/disconnected
	*/
	void onDeviceChange(PRL_DEVICE_TYPE dev_type, const QString &dev_name, unsigned int event_code);

protected:
	/** Overridden method of thread working body */
	void run();

#ifdef _WIN_
	HANDLE		m_hResetEvent;
	QMutex		m_FinalizationMutex;
#endif // _WIN_

#ifdef _LIN_
	BOOL		m_bRunFlag;
	pthread_t	m_thread_id;
#endif // _LIN_

signals:
	/** Device changed signal */
	void deviceChanged(PRL_DEVICE_TYPE dev_type, QString dev_name, unsigned int event_code );

};

#endif // _DSP_HW_MONITOR_HANDLER_H_
