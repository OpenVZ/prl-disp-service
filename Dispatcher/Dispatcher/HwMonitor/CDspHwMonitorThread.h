///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHwMonitorThread.h
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

#ifndef _DSP_HW_MONITOR_THREAD_H_
#define _DSP_HW_MONITOR_THREAD_H_

#include <QThread>
#include <QTimer>
#include "CDspSync.h"
#include "CDspClient.h"

class CDspHostInfo;

/**
 * Thread that react on host system hardware configuration changes
 */
class CDspHwMonitorThread : public QThread
{
	Q_OBJECT

public:
	/** Class default constructor */
	CDspHwMonitorThread();
	/** Class destructor */
	~CDspHwMonitorThread();
	/** Thread finalization method*/
	void FinalizeThreadWork();

	/** process USB device manual connect/disconnect action */
	void setUsbDeviceManualConnected(const QString &, bool);

	/** add vm to list of vms which on deinitialization state in prl_vm_app*/
	void addVmOnDeinitialization( const CVmIdent & );

	/** remove vm to list of vms which on deinitialization state in prl_vm_app*/
	void removeVmOnDeinitialization( const CVmIdent & );

	/** process vm stop */
	void processVmStop( const CVmIdent &, CDspLockedPointer<CDspHostInfo> );

	void forceCheckHwChanges();

protected:
	/** Overridden method of thread working body */
	void run();

signals:

	void checkHwChanges();

private:
	/** Pointer to notifier object */
	class CDspHwMonitorNotifier *m_pNotifier;

	/** Timer object.  !!! Probably, this is temporary decision to sutisfy #8976 !!! */
	QTimer* m_pTimer;
};

#endif //_DSP_HW_MONITOR_THREAD_H_
