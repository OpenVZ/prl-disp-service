///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHwMonitorNotifier.cpp
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

#include "CDspHwMonitorNotifier.h"
#include "CDspService.h"
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"
#include "CDspUserHelper.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "CDspClientManager.h"
#include "CDspVmManager.h"
#include "CProtoSerializer.h"
#include "Tasks/Task_BackgroundJob.h"
#include <Libraries/Std/PrlAssert.h>
#include <Libraries/HostInfo/CHostInfo.h>
#include <Libraries/Logging/Logging.h>
#include <Libraries/PrlCommonUtilsBase/EnumToString.h>
#include <prlsdk/PrlEnums.h>
#include <Libraries/PowerWatcher/PowerWatcher.h>

using namespace Parallels;

const quint64 HI_ALL_EVENT_DEVICES =
		( CDspHostInfo::uhiUsb
		| CDspHostInfo::uhiNet
		);

/*
* Class default constructor
*/
CDspHwMonitorNotifier::CDspHwMonitorNotifier ()
	: m_mtxUsbExcludeDev(QMutex::Recursive)
	, m_delayedRefreshFlags(0)
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=444188
	//Init USB authentic data
	{
		CDispUsbPreferences _usb_prefs( CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->getUsbPreferences() );
		CDspService::instance()->getHostInfo()->getUsbAuthentic()->SetUsbPreferences( _usb_prefs );
	}
	//Update host info with USB devices detection
	{
		CDspLockedPointer<CDspHostInfo> spHostInfo = CDspService::instance()->getHostInfo();
		spHostInfo->refresh(HI_UPDATE_ALL);
		// Copy usb device list
		QList<CHwUsbDevice*> &lstUsbDev = spHostInfo->data()->m_lstUsbDevices;
		foreach (CHwUsbDevice *dev, lstUsbDev)
			m_lstUsbDev.append(new CHwUsbDevice(dev));

		QList<CHwPrinter*> &lstPrinters = spHostInfo->data()->m_lstPrinters;
		foreach (CHwPrinter* pDev, lstPrinters )
		{
			if( m_hashPrinters.contains( pDev->getDeviceId() ) )
			{
				PRL_ASSERT( "Printer Id already exists" == NULL );
				continue;
			}

			m_hashPrinters[ pDev->getDeviceId() ]
				= SmartPtr<CHwPrinter>( new CHwPrinter(pDev) );
		}
	}
	//Update dispatcher permanent USB settings
	{
		CDispUsbPreferences _usb_prefs( CDspService::instance()->getHostInfo()->getUsbAuthentic()->GetUsbPreferences() );
		CDspLockedPointer<CDispCommonPreferences> pCommonPrefs = CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
		pCommonPrefs->getUsbPreferences()->fromString( _usb_prefs.toString() );
		CDspService::instance()->getDispConfigGuard().saveConfig();
	}
}

CDspHwMonitorNotifier::~CDspHwMonitorNotifier()
{
	foreach( CHwUsbDevice *dev, m_lstUsbDev)
		delete dev;
}

/** Process timeout() signal */
void CDspHwMonitorNotifier::onCheckHwChanges()
{
	// #PDFM-26528 - do not refresh host info
	// if there are no clients and no running VMs
	if ( ! hasConnectedClientsOrRunningVMs() )
	{
		WRITE_TRACE(DBG_DEBUG, "Check HW changes: skip refresh (no VMs running)");
		return;
	}

	// prevent unsafe update and disable "idle" host sleep
	CPowerWatcher::CNoSleepLocker nosleep;
	if ( CPowerWatcher::isSleeping() )
	{
		m_delayedRefreshFlags |= (~HI_ALL_EVENT_DEVICES);
		WRITE_TRACE(DBG_FATAL,
			"Check HW changes: skip refresh (system sleeping), mask = 0x%llx",
			m_delayedRefreshFlags);
		return;
	}

	bool bChanged = false;
	{
		CDspLockedPointer<CDspHostInfo> p_lockedHostInfo = CDspService::instance()->getHostInfo();
		SmartPtr< CHostHardwareInfo > pOldHostInfo( new CHostHardwareInfo( p_lockedHostInfo->data() ) );

		p_lockedHostInfo->refresh( m_delayedRefreshFlags | (~HI_ALL_EVENT_DEVICES) );
		m_delayedRefreshFlags = 0;

		SmartPtr< CHostHardwareInfo > pNewHostInfo( new CHostHardwareInfo( p_lockedHostInfo->data() ) );

		// https://jira.sw.ru/browse/PDFM-20092
		// Exclude non-hardware information
		pOldHostInfo->getMemorySettings()->setAdvancedMemoryInfo(0);
		pNewHostInfo->getMemorySettings()->setAdvancedMemoryInfo(0);

		// copy to prevent deadlock ( notifyVms() / notifyClients() locks its mutex )
		if( pNewHostInfo->toString() != pOldHostInfo->toString() )
		{
			WRITE_TRACE(DBG_FATAL, "Hardware Configuration was changed !" );
			bChanged = true;
		}
	}

	if( bChanged )
		onDeviceChange();
}

/*
* Process deviceChanged signal
*/
void CDspHwMonitorNotifier::onDeviceChange(PRL_DEVICE_TYPE dev_type, QString dev_name, unsigned int event_code )
{
	WRITE_TRACE(DBG_FATAL, "On enter : dev_type = %d  dev_name = %s  event_code = %u"
		, (int)dev_type, dev_name.toUtf8().data(), event_code);
	CUsbAuthenticNameList authList;

	// prevent unsafe update and disable "idle" host sleep
	CPowerWatcher::CNoSleepLocker nosleep;
	if ( CPowerWatcher::isSleeping() )
	{
		m_delayedRefreshFlags |= HI_MAKE_UPDATE_DEVICE_MASK(dev_type);
		WRITE_TRACE(DBG_FATAL,
			"Device change: skip refresh (system sleeping), mask = 0x%llx",
			m_delayedRefreshFlags);
		return;
	}

	// #PDFM-26528
	bool bRefreshHostInfo =
			dev_type == PDE_USB_DEVICE
		||
			// skip double refresh host info after onCheckHwChanges() call
			// and do not refresh host info if there are no clients and no running VMs
			( dev_type != PDE_GENERIC_DEVICE && hasConnectedClientsOrRunningVMs() );

	SmartPtr< CHostHardwareInfo > pHostInfo;
	{
		CDspLockedPointer<CDspHostInfo> p_lockedHostInfo = CDspService::instance()->getHostInfo();

		if ( bRefreshHostInfo )
			p_lockedHostInfo->refresh(HI_MAKE_UPDATE_DEVICE_MASK(dev_type));
		else
			WRITE_TRACE(DBG_FATAL, "Device change: host hardware info refreshing was skipped !");

		// copy to prevent deadlock ( notifyVms() / notifyClients() locks its mutex )
		pHostInfo = SmartPtr<CHostHardwareInfo>(new CHostHardwareInfo( p_lockedHostInfo->data() ));
		authList.SetUsbPreferences( p_lockedHostInfo->getUsbAuthentic()->GetUsbPreferences() );
	}

#if 0
	WRITE_TRACE( DBG_FATAL, "===================  HW INFO TRACE ============" );
	PUT_RAW_MESSAGE( QSTR2UTF8( pHostInfo->toString() ) );
	WRITE_TRACE( DBG_FATAL, "END" );
#endif

	bool bClientsWereNotified = false;
	switch( dev_type )
	{
	case PDE_SOUND_DEVICE:
	case PDE_MIXER_DEVICE:
		//
		// Simply forward notification to VM now
		//
		notifyVMs(pHostInfo, dev_type, dev_name, event_code, "");
		break;
	case PDE_USB_DEVICE:
	case PDE_HARD_DISK:
		// NOTE: HDD may have USB alias, process USB changes for both USB and HDD
		processUsbDeviceChange( pHostInfo, authList, bClientsWereNotified );
		break;
	case PDE_PRINTER:
		processPrinterChange( pHostInfo );
		break;
	default: ;
	}


	if ( !bClientsWereNotified )
		notifyClients();
	//
	WRITE_TRACE(DBG_FATAL, "On exit  : dev_type = %d  dev_name = %s  event_code = %u"
		, (int)dev_type, dev_name.toUtf8().data(), event_code);
}

void CDspHwMonitorNotifier::processUsbDeviceChange(
	const SmartPtr< CHostHardwareInfo >& pHostInfo
	, const CUsbAuthenticNameList& authList
	, bool& )
{
	// Usb device removal -> search for removed device name

	// After this cycle:
	//   lstUsbDevDisconnected will contain the list of disconnected devices (need to be deleted)
	//   lstUsbDevConnected will contain list of newly connected devices
	//   m_lstUsbDev will connect list of all currently connected devices
	QList<CHwUsbDevice*> lstUsbDevConnected;
	QList<CHwUsbDevice*> lstUsbDevDisconnected = m_lstUsbDev;
	m_lstUsbDev.clear();

	foreach(CHwUsbDevice* pDev, pHostInfo->m_lstUsbDevices )
		m_lstUsbDev << new CHwUsbDevice(pDev);

	/* On enter we have 2 lists of usb devices and it's states - old & new.
	* on exit we need to create 2 lists - connected & disconnected devices.
	* There are 3 device states:
	*  H - device connected to host (tag "--")
	*  C - device connecting to guest (tag "PW", for windows host only)
	*  G - device connected to guest (tag "PR")
	*
	* List of connected & disconnected devices created using table:
	*
	* OLD NEW CON DIS
	*  -   -   -   -
	*  -   H   H   -
	*  -   C   C   -	<-- not possible in real life
	*  -   G   G   -
	*  H   -   -   H
	*  H   H   -   -
	*  H   C   C   H
	*  H   G   G   H
	*  C   -   -   C
	*  C   H   H   C	<-- not possible in real life
	*  C   C   -   -
	*  C   G   G   -	<-- skeep C device disconnect if G device connected
	*  G   -   -   G
	*  G   H   H   G
	*  G   C   C   G	<-- not possible in real life
	*  G   G   -   -
	*/
	foreach(CHwUsbDevice *dev0, m_lstUsbDev)
	{
		const QString &id00 = dev0->getDeviceId();
		// We need ignore disconnect of usb device with "PW" tag if the
		// same usb device whith "PR" tag was connected, but we still
		// need to report device connect in this case (#PWE-4629)
		QString id01 = "";
		if (id00.section('|', 4, 4) == "PR")
			id01 = QString("%1|PW|%2").arg(id00.section('|', 0, 3)).arg(id00.section('|', 5));

		bool connected = true;
		foreach(CHwUsbDevice *dev1, lstUsbDevDisconnected) {
			const QString &id1 = dev1->getDeviceId();
			if (id1 != id00 && id1 != id01)
				continue;
			if (id1 == id00)
				connected = false;
			// Device found in old list -> delete it
			lstUsbDevDisconnected.removeAll(dev1);
			delete dev1;
			break;
		}
		if (connected)
			lstUsbDevConnected.append(dev0);
	}

	// Process device disconnection & clear list
	foreach(CHwUsbDevice *dev, lstUsbDevDisconnected)
	{
		const QString &sys_name = dev->getDeviceId();
		const QString &frn_name = dev->getDeviceName();
		PRL_USB_DEVICE_TYPE pudt = dev->getUsbType();
		WRITE_TRACE(DBG_FATAL, "USB disconnected <%s> <%s> <%s>",
			QSTR2UTF8(sys_name), QSTR2UTF8(frn_name), QSTR2UTF8(PRL_USB_DEVICE_TYPE_to_QString(pudt)));
		notifyVMs(pHostInfo, PDE_USB_DEVICE, sys_name, 0 /*disconect*/, frn_name, pudt);
		delete dev;
	}
	lstUsbDevDisconnected.clear();

	// Process device connection
	foreach(CHwUsbDevice *dev, lstUsbDevConnected)
	{
		const QString &sys_name = dev->getDeviceId();
		const QString &frn_name = dev->getDeviceName();
		PRL_USB_DEVICE_TYPE pudt = dev->getUsbType();

		bool bFound = false;
		bool autoconnect = isNeedAutoconnect(sys_name, pudt, authList);

		WRITE_TRACE(DBG_FATAL, "USB connected %s <%s> <%s> <%s>",
			autoconnect ? "(autoconnect)" : "",
			QSTR2UTF8(sys_name), QSTR2UTF8(frn_name),
			QSTR2UTF8(PRL_USB_DEVICE_TYPE_to_QString(pudt)));

		if (autoconnect) {
			CDispUsbPreferences _usb_prefs(
				CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->getUsbPreferences()
			);
			foreach(CDispUsbIdentity * pUi, _usb_prefs.m_lstAuthenticUsbMap)
			{	// Look for the device being connected
				if( !authList.IsTheSameDevice( pUi->getSystemName(),
					sys_name ) )
					continue;
				// Found
				foreach(CDispUsbAssociation * pUa, pUi->m_lstAssociations)
				{	// Look for the connect to VM actions
					if( pUa->getAction() != PUD_CONNECT_TO_GUEST_OS )
						continue;

					SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid(pUa->getVmUuid(),
						pUa->getDirUuid());

					if( !pVm.isValid() )
						continue;

					if ( pVm->getVmState() != VMS_RUNNING )
						continue;

					CDspTaskHelper * pTask = Task_AutoconnectUsbDevice::createTask(pVm, sys_name, frn_name);

					if ( pTask == 0 )
						continue;

					// Send notify before autoconnect task started, to update VM's HwInfo first
					// autoconnect  - reconnect device from Hw to VM
					notifyVMs(pHostInfo, PDE_USB_DEVICE, sys_name, 1 /*connect*/, frn_name, pudt);

					CDspService::instance()->getTaskManager().schedule(pTask);
					bFound = true;
					break;
				}
				if(bFound)
					break;
			}
		}
		// Notify VMs about newly connected usb devices
		if(!bFound)
			notifyVMs(pHostInfo, PDE_USB_DEVICE, sys_name, 1 /*connect*/, frn_name, pudt);

		// Do not remove virtual usb device until it disconnected manually
		if (sys_name.section('|', 4, 4) != "PW" && !sys_name.startsWith("VIRTUAL@"))
			removeUsbDevExcludeAutoconnect(sys_name);
	}

	//https://bugzilla.sw.ru/show_bug.cgi?id=444188
	//Update dispatcher permanent USB settings
	updateUsbPreferences( authList );
}

void CDspHwMonitorNotifier::updateUsbPreferences(const CUsbAuthenticNameList& lstAuth)
{
	// Re-integrate USB Associations with new data from HostInfo
	// Save existing associations
	CDspLockedPointer<CDispCommonPreferences> pCommonPrefs =
		CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
	CDispUsbPreferences _old_usb_prefs( pCommonPrefs->getUsbPreferences() );
	QHash<QString,QList<CDispUsbAssociation*>*> savedAssocs;
	foreach(CDispUsbIdentity * pUi, _old_usb_prefs.m_lstAuthenticUsbMap)
	{
		if( pUi->m_lstAssociations.isEmpty() )
			continue;
		QList<CDispUsbAssociation*> * pSavedList = new QList<CDispUsbAssociation*>;
		while ( !pUi->m_lstAssociations.isEmpty() )
			pSavedList->append( pUi->m_lstAssociations.takeFirst() );
		savedAssocs.insert(pUi->getSystemName(), pSavedList );
	}
	CDispUsbPreferences _usb_prefs( lstAuth.GetUsbPreferences() );
	foreach(CDispUsbIdentity * pUi, _usb_prefs.m_lstAuthenticUsbMap)
	{
		// Check if we have stored any associations for pUi->getSystemName;
		bool found = false;
		foreach( QString up_name, savedAssocs.keys() )
		{
			if( !lstAuth.IsTheSameDevice( pUi->getSystemName(), up_name ) )
				continue;
			found = true; // Have to update associations
			while( !pUi->m_lstAssociations.isEmpty() )
				delete pUi->m_lstAssociations.takeFirst();
			QList<CDispUsbAssociation*> * pSavedList = savedAssocs.take(up_name);
			while( !pSavedList->isEmpty() )
				pUi->m_lstAssociations.append( pSavedList->takeFirst() );
			delete pSavedList;
			break;
		}
		if( !found )	// No associations for the device in CP - remove
			while( !pUi->m_lstAssociations.isEmpty() )
				delete pUi->m_lstAssociations.takeFirst();
	}
	// savedAssocs should be empty here.
	PRL_ASSERT( savedAssocs.isEmpty() );
	pCommonPrefs->getUsbPreferences()->fromString( _usb_prefs.toString()	);
	CDspService::instance()->getDispConfigGuard().saveConfig();
	/**
	 * Notify users that common server prefs were changed
	 */

	CVmEvent event( PET_DSP_EVT_COMMON_PREFS_CHANGED,
					QString(),
					PIE_DISPATCHER);

	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspVmEvent, event );

	CDspService::instance()->getClientManager().sendPackageToAllClients( p );
}

/*
* Notify connected clients
*/
void CDspHwMonitorNotifier::notifyClients()
{
	//////////////////////////////////////////////////////////////////////////
	// Notify all users that host hardware configuration was changed
	//////////////////////////////////////////////////////////////////////////
	CVmEvent event( PET_DSP_EVT_HW_CONFIG_CHANGED,
					Uuid().toString(),
					PIE_DISPATCHER );

	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspVmEvent, event );
	CDspService::instance()->getClientManager().sendPackageToAllClients(p);
}

/*
* Generate event-string to be send to clients
*/
static QString makeEventString(PRL_DEVICE_TYPE dev_type,
						const QString &strDevImage,
						unsigned int uiDevState,
						PRL_USB_DEVICE_AUTO_CONNECT_OPTION connectType,
						const QString &s_UserFriendlyName,
						PRL_USB_DEVICE_TYPE usbType,
						const SmartPtr< CHostHardwareInfo >& pHostInfo,
						quint32 changeFlags = 0,
						const QString& sDevConfig = "")
{
	CVmEvent pEvent( PET_DSP_EVT_HW_CONFIG_CHANGED,
		Uuid().toString(),
		PIE_DISPATCHER,
		PVE::EventRespNotRequired );

	pEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt,
		QString::number( (uint) dev_type),
		EVT_PARAM_DEVICE_TYPE) );

	pEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt,
		QString::number( (uint) uiDevState ),
		EVT_PARAM_VM_CONFIG_DEV_STATE) );

	pEvent.addEventParameter( new CVmEventParameter(
		PVE::String,
		strDevImage,
		EVT_PARAM_VM_CONFIG_DEV_IMAGE) );

	pEvent.addEventParameter( new CVmEventParameter(
		PVE::String,
		s_UserFriendlyName,
		EVT_PARAM_VM_CONFIG_DEV_NAME) );

	pEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt,
		QString::number( (uint) connectType ),
		EVT_PARAM_USB_CONNECTION_TYPE) );

	pEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt,
		QString::number(usbType),
		EVT_PARAM_USB_DEVICE_TYPE) );

	pEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt,
		QString::number(changeFlags),
		EVT_PARAM_HW_DEVICE_CHANGED_FLAGS) );

	pEvent.addEventParameter( new CVmEventParameter(PVE::String,
		sDevConfig,
		EVT_PARAM_HW_DEVICE_CONFIG) );

	PRL_ASSERT( pHostInfo );
	pEvent.addEventParameter( new CVmEventParameter(PVE::String,
		pHostInfo->toString(),
		EVT_PARAM_HOST_HARDWARE_INFO) );

	WRITE_TRACE(DBG_FATAL, "make post to autoconnect event from thread notifier! strDevImage == %s uiDevState==%d",
					QSTR2UTF8( strDevImage), uiDevState );

	return pEvent.toString();
}

/*
* Notify running VMs
*/
void CDspHwMonitorNotifier::notifyVMs( const SmartPtr< CHostHardwareInfo >& pHostInfo,
		PRL_DEVICE_TYPE dev_type, const QString &dev_name,
		unsigned int uiDevState, const QString &userFriendlyName,
		PRL_USB_DEVICE_TYPE usbType,
		quint32 changeFlags,
		const QString& sDevConfig)
{
	WRITE_TRACE(DBG_FATAL, "USB VM notify '%sconnect' <%s> <%s> <%s>",
				uiDevState ? "" : "dis",
				QSTR2UTF8(dev_name), QSTR2UTF8(userFriendlyName),
				QSTR2UTF8(PRL_USB_DEVICE_TYPE_to_QString(usbType)));

	//////////////////////////////////////////////////////////////////////////
	// Notify all running VMs that host hardware configuration was changed
	//////////////////////////////////////////////////////////////////////////

	QString event_string = makeEventString(dev_type, dev_name, uiDevState
		,PUD_CONNECT_TO_PRIMARY_OS, userFriendlyName, usbType, pHostInfo
		, changeFlags, sDevConfig );
	SmartPtr<IOPackage> p =	DispatcherPackage::createInstance( PVE::DspEvtHwChanged, event_string );

	CDspService::instance()->getVmManager().sendPackageToAllVMs(p);
}

bool isAutoconnectBlacklisted(USHORT vid, USHORT pid)
{
	// List of device's VID+PID excluded from autoconnect
	static const struct USB_EXCLUDE_ELEMENT { USHORT vid, pid_min, pid_max; }
	excl_dev_lst[] =
	{
		// vid, pid_min, pid_max
		{ 0x0FCA, 0x0004, 0x0004 },	// BlackBerry Perl on second stage
		{ 0x0FCA, 0x0001, 0x0001 },	// BlackBerry Perl on third stage
		{ 0x10D5, 0x000D, 0x000D },	// KVM TENDNet TK-407
		// This list was originaly created based on
		// IOUSBFamily-378.4.3\AppleUSBHub\Classes\AppleUSBHubPort.cpp
		// and modified according to apple's hardware found in parallels office:
		// 205, 20C, 221 - external keyboard
		// 304 - external mouse
		// 21a, 21b, 22a, 237 - internal keyboard/trackpad
		// 8xxx - internal bt and ishight
		{ 0x05AC, 0x8000, 0x8FFF },
		{ 0x05AC, 0x020E, 0x021C },
		{ 0x05AC, 0x0223, 0x0226 },
		{ 0x05AC, 0x0229, 0x022B },
		{ 0x05AC, 0x022F, 0x022F },
		{ 0x05AC, 0x0230, 0x0238 },
		{ 0x05AC, 0x0307, 0x0307 },
		{ 0x05AC, 0x9212, 0x9217 },
		{ 0x05AC, 0x1000, 0x1000 }
	};

	for (UINT i = 0; i < sizeof(excl_dev_lst)/sizeof(excl_dev_lst[0]); i++)
		if (excl_dev_lst[i].vid == vid &&
			pid <= excl_dev_lst[i].pid_max && pid >= excl_dev_lst[i].pid_min)
			return true;
	return false;
}

// check is need notify client about autoconnect
bool CDspHwMonitorNotifier::isNeedAutoconnect(const QString&				strDeviceId,
											  PRL_USB_DEVICE_TYPE			pudt,
											  const CUsbAuthenticNameList&	authList)
{
	bool bConvertionResult;
	USHORT vid = (USHORT)strDeviceId.section('|', 1, 1).toUInt(&bConvertionResult, 16);
	USHORT pid = (USHORT)strDeviceId.section('|', 2, 2).toUInt(&bConvertionResult, 16);

	if(	isAutoconnectBlacklisted(vid, pid) ||
		// Exclude devices serviced by prl_usb_connect
		( strDeviceId.section('|', 4, 4) == "PR" ) ||
		( strDeviceId.section('|', 4, 4) == "PW" ) ||
		// Exclude "shared" virtual devices (all except virtual disks)
		( strDeviceId.startsWith("VIRTUAL@") && (!strDeviceId.startsWith("VIRTUAL@MSC@")) ) ||
		// Exclude real USB disks, if they replaced by virtual disk aliases
		( pudt == PUDT_DISK_STORAGE && (!strDeviceId.startsWith("VIRTUAL@MSC@"))
		  && authList.GetUsbPreferences().getUsbVirtualDisks()->isUsb()) ||
		// Exclude marked devices
		 findUsbDevExcludeAutoconnect(strDeviceId) ||
		 // has vms on deinitialization
		 !m_lstVmsOnDeinitialization.isEmpty())
		return false;

	return true;
}

/** add/remove USB device ID to list of Usb Id to exclude auto connect*/
void CDspHwMonitorNotifier::setUsbDeviceManualConnected(const QString & strDevId, bool bConnect)
{
	if (!strDevId.startsWith("VIRTUAL@")) {
		addUsbDevExcludeAutoconnect(strDevId);
		return;
	}

	// Virtual devices is not reconnected to host during connect/disconnect to VM.
	// But we need to exclude it from autoconnection logic until it disconnected
	// from VM.
	if (bConnect)
		addUsbDevExcludeAutoconnect(strDevId);
	else
		removeUsbDevExcludeAutoconnect(strDevId);
}

/** add USB device ID to list of Usb Id to exclude auto connect*/
void CDspHwMonitorNotifier::addUsbDevExcludeAutoconnect( const QString & strDevId )
{
	QMutexLocker locker(&m_mtxUsbExcludeDev);
	WRITE_TRACE(DBG_FATAL, "usb device with id == %s added to exclude list of autoconnect",
					QSTR2UTF8( strDevId));
	m_lstUsbDevExcludeAutoconnect.append( strDevId );
}

/** remove USB device ID from list of Usb Id to exclude auto connect*/
void CDspHwMonitorNotifier::removeUsbDevExcludeAutoconnect( const QString & strDevId )
{
	QMutexLocker locker(&m_mtxUsbExcludeDev);
	WRITE_TRACE(DBG_FATAL, "usb device with id == %s removed from list excluded of autoconnect",
		QSTR2UTF8( strDevId));
	foreach( QString strDevId2, m_lstUsbDevExcludeAutoconnect )
		// Compare all but exclude field 4
		if( strDevId.section('|', 0, 3) == strDevId2.section('|', 0, 3) &&
			strDevId.section('|', 5) == strDevId2.section('|', 5) )
			m_lstUsbDevExcludeAutoconnect.removeAll( strDevId2 );
}

/** find USB device ID from list of Usb Id to exclude auto connect*/
bool CDspHwMonitorNotifier::findUsbDevExcludeAutoconnect( const QString & strDevId )
{
	QMutexLocker locker(&m_mtxUsbExcludeDev);
	foreach( QString strDevId2, m_lstUsbDevExcludeAutoconnect )
		// Compare all but exclude field 4
		if( strDevId.section('|', 0, 3) == strDevId2.section('|', 0, 3) &&
			strDevId.section('|', 5) == strDevId2.section('|', 5) )
			return true;
	//
	return false;
}

/** add vm to list of vms which on deinitialization state in prl_vm_app*/
void CDspHwMonitorNotifier::addVmOnDeinitialization( const CVmIdent & ident )
{
	QMutexLocker locker(&m_mtxUsbExcludeDev);
	m_lstVmsOnDeinitialization.append( ident );
}

/** remove vm to list of vms which on deinitialization state in prl_vm_app*/
void CDspHwMonitorNotifier::removeVmOnDeinitialization( const CVmIdent & ident )
{
	QMutexLocker locker(&m_mtxUsbExcludeDev);
	m_lstVmsOnDeinitialization.removeAll( ident );
}

/** process vm stop */
void CDspHwMonitorNotifier::processVmStop( const CVmIdent & ident, CDspLockedPointer<CDspHostInfo> spHostInfo )
{
	QList<CHwUsbDevice*> &lstUsbDevices = spHostInfo->data()->m_lstUsbDevices;

	foreach( CHwUsbDevice* pUsbDevice, lstUsbDevices )
	{
		QStringList lstVmUuids = pUsbDevice->getVmUuids();

		// check that we own that device
		if ( !lstVmUuids.contains( ident.first ) )
			continue;

		// NOTE: we also release ownership for PGS_NON_CONTROLLED_USB devices:
		// USB-to-COM, USB-HDD or USB-CD/DVD connecteded as COM, HDD or CD/DVD
		// After VM stopped we should allow to connect this devices as USB to
		// other VMs

		// sanity check: device state
		PRL_ASSERT( pUsbDevice->getDeviceState() == PGS_CONNECTED_TO_VM
				    || pUsbDevice->getDeviceState() == PGS_CONNECTING_TO_VM
					|| pUsbDevice->getDeviceState() == PGS_NON_CONTROLLED_USB);

		// release ownership
		lstVmUuids.removeAll( ident.first );
		pUsbDevice->setVmUuids( lstVmUuids );

		if ( lstVmUuids.isEmpty() ) {
			// return device to host
			pUsbDevice->setDeviceState( PGS_CONNECTED_TO_HOST );
			removeUsbDevExcludeAutoconnect( pUsbDevice->getDeviceId() );
		} else {
			// sanity check: only CCID device can be shared between VMs now,
			//   others should have only one owner (released now)
			PRL_ASSERT( lstVmUuids.isEmpty()
						|| pUsbDevice->getDeviceId().startsWith(PRL_VIRTUAL_CCID_PATH0) );
		}

		WRITE_TRACE(DBG_FATAL,
					"Release usb device <%s> due to vm stop, owners %d",
					QSTR2UTF8( pUsbDevice->getDeviceId() ),
					lstVmUuids.size() );
	}
}

void CDspHwMonitorNotifier::processPrinterChange( const SmartPtr< CHostHardwareInfo >& pHostInfo )
{
/*	QString devId;
	unsigned int event_code;
*/
	typedef QHash< QString, SmartPtr<CHwPrinter> >
		PrintersHash;

	PrintersHash nowPrinters;
	foreach(CHwPrinter* pHw, pHostInfo->m_lstPrinters )
	{
		SmartPtr<CHwPrinter> pCopy( new CHwPrinter(pHw) );

		PRL_ASSERT( !nowPrinters.contains(pHw->getDeviceId()) );
		nowPrinters.insert( pHw->getDeviceId(), pCopy );
	}

	bool bEventWasSent = false;
	// 1. check for add
	// 2. check for changes
	QList<QString> nowPrinterIds = nowPrinters.keys();
	foreach( QString id, nowPrinterIds )
	{
		// check for added printer
		if( !m_hashPrinters.contains(id) )
		{
			sendPrinterChangeNotifyToVms( nowPrinters.value(id), PVE::DeviceConnected, 0, pHostInfo );
			bEventWasSent = true;
			continue;
		}

		// checks for changed perferences
		quint32 changed_flags = 0;
		SmartPtr<CHwPrinter> pOldDev = m_hashPrinters.value( id );
		SmartPtr<CHwPrinter> pDevNow = nowPrinters.value(id);

		if( pOldDev->getDeviceName() !=  pDevNow->getDeviceName() )
				changed_flags |= PVE::DevParamChangeFriendlyName;

		if( !changed_flags )
		{
			WRITE_TRACE(DBG_DEBUG, "Printer event was skipped - nothing changed: "
				"<sysName: %s> <frName: %s>"
				, QSTR2UTF8(pDevNow->getDeviceId()), QSTR2UTF8(pDevNow->getDeviceName()) );
			continue;
		}

		sendPrinterChangeNotifyToVms( pDevNow, PVE::DeviceConnecting, changed_flags, pHostInfo );
		bEventWasSent = true;
	}

	// 3. check for del
	QList<QString> oldPrinterIds = m_hashPrinters.keys();
	foreach( QString id, oldPrinterIds )
	{
		if( nowPrinters.contains(id) )
			continue;
		sendPrinterChangeNotifyToVms( m_hashPrinters.value(id), PVE::DeviceDisconnected, 0, pHostInfo );
		bEventWasSent = true;
	}

	// if something happens with printers but dispatcher doesn't detect it
	//	it may be any printers change in user session - Default Printer change for example.
	if( !bEventWasSent )
		sendPrinterChangeNotifyToVms( SmartPtr<CHwPrinter>( new CHwPrinter ), PVE::DeviceConnecting
			, PVE::DevParamChangeUnknown, pHostInfo );

	// REPLACE TO NEW VALUES
	m_hashPrinters = nowPrinters;
}

void CDspHwMonitorNotifier::sendPrinterChangeNotifyToVms( const SmartPtr<CHwPrinter>& pDev
	, quint32 event_code, quint32 changed_flags
	, const SmartPtr< CHostHardwareInfo >& pHostInfo )
{
	PRL_ASSERT( pDev );
	const QString sys_name = pDev->getDeviceId();
	const QString frn_name = pDev->getDeviceName();

	WRITE_TRACE(DBG_INFO, "Printer event: <%s> <%s> <event_code %d>, <changed_flags %d> ",
		QSTR2UTF8(sys_name), QSTR2UTF8(frn_name), event_code, changed_flags );

	notifyVMs( pHostInfo, PDE_PRINTER, sys_name, event_code, frn_name
		, PUDT_OTHER, changed_flags, pDev->toString() );
}

bool CDspHwMonitorNotifier::hasConnectedClientsOrRunningVMs() const
{
	return ! CDspService::instance()->getClientManager().getSessionsListSnapshot().isEmpty()
			|| ! CDspService::instance()->getVmManager().getAllRunningVms().isEmpty();
}
