///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHwMonitorNotifier.h
///
/// Hardware changes notifier class implementation
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

#ifndef _DSP_HW_MONITOR_NOTIFIER_H_
#define _DSP_HW_MONITOR_NOTIFIER_H_

#include <QObject>
#include <QString>
#include <QStringList>
#include "Interfaces/Config.h"
#include <prlsdk/PrlEnums.h>
#include "CDspSync.h"
#include <QMutex>
#include <QPair>
#include <QHash>

#include "CDspClient.h"

class CHwUsbDevice;
class CUsbAuthenticNameList;
class CHostHardwareInfo;
class CUsbAuthenticNameList;
class CHwPrinter;
class CDspHostInfo;

/**
* Class that notify clients and running VMs about host system hardware changes
*/
class CDspHwMonitorNotifier : public QObject
{
	Q_OBJECT

public:
	/** Class default constructor */
	CDspHwMonitorNotifier ();
	~CDspHwMonitorNotifier ();

	/** add/remove USB device ID to list of Usb Id to exclude auto connect*/
	void setUsbDeviceManualConnected(const QString &, bool);

	/** add USB device ID to list of Usb Id to exclude auto connect*/
	void addUsbDevExcludeAutoconnect( const QString &);

	/** remove USB device ID from list of Usb Id to exclude auto connect*/
	void removeUsbDevExcludeAutoconnect( const QString &);

	/** find USB device ID from list of Usb Id to exclude auto connect*/
	bool findUsbDevExcludeAutoconnect( const QString & );

	/** add vm to list of vms which on deinitialization state in prl_vm_app*/
	void addVmOnDeinitialization( const CVmIdent & ident );

	/** remove vm to list of vms which on deinitialization state in prl_vm_app*/
	void removeVmOnDeinitialization( const CVmIdent & ident );

	/** process vm stop */
	void processVmStop( const CVmIdent &, CDspLockedPointer<CDspHostInfo> );

private:
	// mutex for m_lstUsbDevExcludeAutoconnect
	QMutex	m_mtxUsbExcludeDev;

	/** Pointer to notifier object */
	QList<QString> m_lstUsbDevExcludeAutoconnect;

	/** list of vms Idents which on deinitialization state in prl_vm_app
	* if list is not empty - auto connect disabled!
	*/
	QList<CVmIdent> m_lstVmsOnDeinitialization;

	QList<CHwUsbDevice*> m_lstUsbDev;
	QHash< QString/*device Id*/, SmartPtr<CHwPrinter> > m_hashPrinters;

	quint64 m_delayedRefreshFlags;

private:
	/** Notify connected clients */
	void notifyClients();

	/** Notify desktop client */
	bool notifyUsbDesktopClient(const SmartPtr< CHostHardwareInfo >& pHostInfo,
		PRL_DEVICE_TYPE dev_type,
		const QString &dev_name,
		unsigned int event_code,
		PRL_USB_DEVICE_AUTO_CONNECT_OPTION connectType,
		const QString &userFriendlyName,
		PRL_USB_DEVICE_TYPE usbType = PUDT_OTHER);

	/** Notify running VMs */
	//	TODO NOTE: Need send all params through 'sDevConfig'
	//		TODO Need drop all params exclude 'dev_type' and 'uiDevState' and 'sDevConfig'
	static void notifyVMs( const SmartPtr< CHostHardwareInfo >& pHostInfo,
		PRL_DEVICE_TYPE dev_type, const QString &dev_name,
		unsigned int uiDevState, const QString &userFriendlyName,
		PRL_USB_DEVICE_TYPE usbType = PUDT_OTHER,
		quint32 changeFlags = PVE::DevParamChangeNone,
		const QString& sDevConfig = "" );

	// check is need notify client about autoconnect
	bool isNeedAutoconnect(const QString&					strDeviceId,
						   PRL_USB_DEVICE_TYPE				pudt,
						   const CUsbAuthenticNameList&		authList);

	void updateUsbPreferences( const CUsbAuthenticNameList& lstAuth );

	static void sendPrinterChangeNotifyToVms(const SmartPtr<CHwPrinter>& pDev
		, quint32 event_code, quint32 changed_flags
		, const SmartPtr< CHostHardwareInfo >& pHostInfo );

private slots:

	/** Process timeout() signal */
	void onCheckHwChanges();

	/** Process deviceChanged signal */
	void onDeviceChange(PRL_DEVICE_TYPE dev_type = PDE_GENERIC_DEVICE,
						QString dev_name = "",
						unsigned int event_code = 0);

private:
	void processUsbDeviceChange( const SmartPtr< CHostHardwareInfo >& pHostInfo
		, const CUsbAuthenticNameList& lstAuth
		, bool& bClientsWasNotified /*[OUT]*/ );

	void processPrinterChange( const SmartPtr< CHostHardwareInfo >& pHostInfo );

	bool hasConnectedClientsOrRunningVMs() const;

};

#endif//_DSP_HW_MONITOR_NOTIFIER_H_
