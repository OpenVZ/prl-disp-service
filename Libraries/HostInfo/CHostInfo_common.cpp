////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	CHostInfo_common.cpp
///
/// @brief
///	Implementation common members of the class CDspHostInfo
///
/// @brief
///	This class implements Dispatcher's host environment info helper
///
/// @author
///	SergeyM
///
/// @date
///	2006-01-05
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#include "CHostInfo.h"

#include <prlcommon/Interfaces/ParallelsQt.h>

#include <stack>
#include <QMutexLocker>
#include <QMutex>
#include <QCryptographicHash>

#include <prlxmlmodel/HostHardwareInfo/CHwNetAdapter.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

#include "Libraries/PrlNetworking/PrlNetLibrary.h"
#include "Libraries/PrlNetworking/netconfig.h"
#include <prlcommon/HostUtils/EFI_GPT_Types.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/OsInfo.h>
#include "Libraries/PrlNetworking/IpStatistics.h"
#include <prlcommon/PrlCommonUtilsBase/EnumToString.h>
#include "Libraries/PowerWatcher/PowerWatcher.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

// Atomaticaly generated header needed only for CreateUsbFriendlyName
#include "UsbFriendlyNames.h"
#include <prlcommon/Logging/Logging.h>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"
#include <prlcommon/Interfaces/ApiDevNums.h>
#include "Interfaces/Config.h"

#include "Build/Current.ver"

#include "cpufeatures.h"
#include "Libraries/CpuFeatures/CCpuHelper.h"

// Supported video cards
#define SVC_NVIDIA_QUADRO_FX_3800	"nVidia Corporation Quadro FX 3800"
#define SVC_NVIDIA_QUADRO_FX_4800	"nVidia Corporation Quadro FX 4800"
#define SVC_NVIDIA_QUADRO_FX_5800	"nVidia Corporation Quadro FX 5800"

/* This struct help convert usb device devndor_id+device_id
   to parallels usb device type */
struct USB_DEV2PUDT {
	UINT ven_id;		// [0x0000, 0xffff] -> exect match, ~0 -> any
	UINT dev_id;		// [0x0000, 0xffff] -> exect match, ~0 -> any
	PRL_USB_DEVICE_TYPE pudt;
};

/* This struct help convert usb interface class+subclass+protocol
   to parallels usb device type */
struct USB_IFC2PUDT {
	UINT ifc_class;			// [0x00, 0xff] -> exect match, ~0 -> any
	UINT ifc_subclass;		// [0x00, 0xff] -> exect match, ~0 -> any
	UINT ifc_protocol;		// [0x00, 0xff] -> exect match, ~0 -> any
	PRL_USB_DEVICE_TYPE pudt;
};

// Table to covert usb device vendor_id+product_id to
// parallels usb device type.
static const USB_DEV2PUDT usb_dev2pudt[] = {
	{ 0x05ac,   0x1000, PUDT_BLUETOOTH},	// Apple bluetooth controller
	{ 0x05ac,   0x8203, PUDT_BLUETOOTH},	// Apple bluetooth controller
	{ 0x05ac,   0x8204, PUDT_BLUETOOTH},	// Apple bluetooth controller
	{ 0x05ac,   0x8205, PUDT_BLUETOOTH},	// Apple bluetooth controller
	{ 0x05ac,   0x8206, PUDT_BLUETOOTH},	// Apple bluetooth controller
	{ 0x05ac,   0x8210, PUDT_BLUETOOTH},	// Apple bluetooth controller
	{ 0x05ac,   0x8213, PUDT_BLUETOOTH},	// Apple bluetooth controller
	{ 0x05ac,   0x821b, PUDT_BLUETOOTH},	// Apple bluetooth controller
	{ 0x05ac,   0x821f, PUDT_BLUETOOTH},	// Apple bluetooth controller
	{ 0x05ac,   0x8240, PUDT_COMMUNICATION},// Apple ir controller
	{ 0x05ac,   0x8242, PUDT_COMMUNICATION},// Apple ir controller

	{ 0x05ac,   0x1201, PUDT_APPLE_IPOD},	// Apple 3G iPod
	{ 0x05ac,   0x1202, PUDT_APPLE_IPOD},   // Apple iPod 2G
	{ 0x05ac,   0x1203, PUDT_APPLE_IPOD},   // Apple iPod 4.Gen Grayscale 40G
	{ 0x05ac,   0x1204, PUDT_APPLE_IPOD},   // Apple iPod [Photo]
	{ 0x05ac,   0x1205, PUDT_APPLE_IPOD},   // Apple iPod Mini 1.Gen/2.Gen
	{ 0x05ac,   0x1206, PUDT_APPLE_IPOD},   // Apple iPod '06'
	{ 0x05ac,   0x1207, PUDT_APPLE_IPOD},   // Apple iPod '07'
	{ 0x05ac,   0x1208, PUDT_APPLE_IPOD},   // Apple iPod '08'
	{ 0x05ac,   0x1209, PUDT_APPLE_IPOD},   // Apple iPod Video
	{ 0x05ac,   0x120a, PUDT_APPLE_IPOD},   // Apple iPod Nano
	{ 0x05ac,   0x1223, PUDT_APPLE_IPOD},   // Apple iPod Classic/Nano 3.Gen (DFU mode)
	{ 0x05ac,   0x1224, PUDT_APPLE_IPOD},   // Apple iPod Nano 3.Gen (DFU mode)
	{ 0x05ac,   0x1225, PUDT_APPLE_IPOD},   // Apple iPod Nano 4.Gen (DFU mode)
	{ 0x05ac,   0x1231, PUDT_APPLE_IPOD},   // Apple iPod Nano 5.Gen (DFU mode)
	{ 0x05ac,   0x1240, PUDT_APPLE_IPOD},   // Apple iPod Nano 2.Gen (DFU mode)
	{ 0x05ac,   0x1242, PUDT_APPLE_IPOD},   // Apple iPod Nano 3.Gen (WTF mode)
	{ 0x05ac,   0x1243, PUDT_APPLE_IPOD},   // Apple iPod Nano 4.Gen (WTF mode)
	{ 0x05ac,   0x1245, PUDT_APPLE_IPOD},   // Apple iPod Classic 3.Gen (WTF mode)
	{ 0x05ac,   0x1246, PUDT_APPLE_IPOD},   // Apple iPod Nano 5.Gen (WTF mode)
	{ 0x05ac,   0x1255, PUDT_APPLE_IPOD},   // Apple iPod Nano 4.Gen (DFU mode)
	{ 0x05ac,   0x1260, PUDT_APPLE_IPOD},   // Apple iPod Nano 2.Gen
	{ 0x05ac,   0x1261, PUDT_APPLE_IPOD},   // Apple iPod Classic
	{ 0x05ac,   0x1262, PUDT_APPLE_IPOD},   // Apple iPod Nano 3.Gen
	{ 0x05ac,   0x1263, PUDT_APPLE_IPOD},   // Apple iPod Nano 4.Gen
	{ 0x05ac,   0x1265, PUDT_APPLE_IPOD},   // Apple iPod Nano 5.Gen
	{ 0x05ac,   0x1266, PUDT_APPLE_IPOD},   // Apple iPod Nano 6.Gen
	{ 0x05ac,   0x1291, PUDT_APPLE_IPOD},   // Apple iPod Touch 1.Gen
	{ 0x05ac,   0x1293, PUDT_APPLE_IPOD},   // Apple iPod Touch 2.Gen
	{ 0x05ac,   0x1296, PUDT_APPLE_IPOD},   // Apple iPod Touch 3.Gen (8GB)
	{ 0x05ac,   0x1299, PUDT_APPLE_IPOD},   // Apple iPod Touch 3.Gen
	{ 0x05ac,   0x129e, PUDT_APPLE_IPOD},   // Apple iPod Touch 4.Gen
	{ 0x05ac,   0x12aa, PUDT_APPLE_IPOD},   // Apple iPod Touch 5.Gen [A1421]
	{ 0x05ac,   0x1300, PUDT_APPLE_IPOD},   // Apple iPod Shuffle
	{ 0x05ac,   0x1301, PUDT_APPLE_IPOD},   // Apple iPod Shuffle 2.Gen
	{ 0x05ac,   0x1302, PUDT_APPLE_IPOD},   // Apple iPod Shuffle 3.Gen
	{ 0x05ac,   0x1303, PUDT_APPLE_IPOD},   // Apple iPod Shuffle 4.Gen
	{ 0x05ac,   0x1290, PUDT_APPLE_IPHONE}, // Apple iPhone
	{ 0x05ac,   0x1292, PUDT_APPLE_IPHONE}, // Apple iPhone 3G
	{ 0x05ac,   0x1294, PUDT_APPLE_IPHONE}, // Apple iPhone 3GS
	{ 0x05ac,   0x1297, PUDT_APPLE_IPHONE}, // Apple iPhone 4
	{ 0x05ac,   0x129c, PUDT_APPLE_IPHONE}, // Apple iPhone 4
	{ 0x05ac,   0x12a0, PUDT_APPLE_IPHONE}, // Apple iPhone 4S
	{ 0x05ac,   0x12a8, PUDT_APPLE_IPHONE}, // Apple iPhone 5 C/S
	{ 0x05ac,   0x129a, PUDT_APPLE_IPAD},   // Apple iPad
	{ 0x05ac,   0x129f, PUDT_APPLE_IPAD},   // Apple iPad 2
	{ 0x05ac,   0x12a2, PUDT_APPLE_IPAD},   // Apple iPad 2 (3G; 64GB)
	{ 0x05ac,   0x12a3, PUDT_APPLE_IPAD},   // Apple iPad 2
	{ 0x05ac,   0x12a4, PUDT_APPLE_IPAD},   // Apple iPad 3
	{ 0x05ac,   0x12a5, PUDT_APPLE_IPAD},   // Apple iPad 3
	{ 0x05ac,   0x12a6, PUDT_APPLE_IPAD},   // Apple iPad 3 (3G; 16GB)
	{ 0x05ac,   0x12a9, PUDT_APPLE_IPAD},   // Apple iPad 2
	{ 0x05ac,   0x12ab, PUDT_APPLE_IPAD},	// Apple iPad 2
	// /S/L/E/IONetworkingFamily.kext/Contents/PlugIns/AppleUSBEthernet.kext/Contents/Info.plist
	{ 0x05ac,   0x1402, PUDT_APPLE_IETH},   // Apple iEthernet (Chip AX88772A)
	{ 0x0b95,   0x7720, PUDT_APPLE_IETH},   // Apple iEthernet (Chip AX88772)
	{ 0x0b95,   0x772a, PUDT_APPLE_IETH},   // Apple iEthernet (Chip AX88772A)
	{ 0x13b1,   0x0018, PUDT_APPLE_IETH},   // Apple iEthernet (Chip AX88772)
	{ 0x2001,   0x3c05, PUDT_APPLE_IETH},   // Apple iEthernet (Chip AX88772)

	{ 0x05ac,   0x8300, PUDT_VIDEO},		// Apple iSight
	{ 0x05ac,   0x8501, PUDT_VIDEO},		// Apple iSight
	{ 0x05ac,   0x8502, PUDT_VIDEO},		// Apple iSight
	{ 0x05ac,   0x8505, PUDT_VIDEO},		// Apple iSight
	{ 0x05ac,   0x850a, PUDT_VIDEO},		// Apple FaceTime Camera

	{ 0x0fca,     ~0U, PUDT_RIM_BLACKBERRY},// BlackBerry

	{ 0x091e,	0x0003, PUDT_GARMIN_GPS},	// Garmin GPS

	{ 0x0bb4,	0x0b04, PUDT_COMMUNICATION},// HTC

	{ 0x07d1,   0x3c03, PUDT_WIRELESS},		// 802.11 bg WLAN
	{ 0x0a5c,   0x200a, PUDT_BLUETOOTH},	// Bluetooth controller 1
	{ 0x0a5c,   0x3502, PUDT_BLUETOOTH},	// Bluetooth controller 2
	{ 0x0a5c,   0x3503, PUDT_BLUETOOTH},	// Bluetooth controller 3
	{ 0x1310,   0x0001, PUDT_BLUETOOTH},	// Bluetooth controller 4
	{ 0x045e,   0x0710, PUDT_DISK_STORAGE}, // MS Zune
	{ 0x0dc3,   0x0802, PUDT_SMART_CARD},	// ASEDrive III
	{ 0x04a9,   0x2225, PUDT_SCANNER}		// CanoScan
};

// Table to converting usb interface class+subcluss+protocol to
// parallels usb device type.
static const USB_IFC2PUDT usb_ifc2pudt[] = {
	{ 0x0e,  ~0U,  ~0U, PUDT_VIDEO},		// Detect video device
	{ 0x06,  ~0U,  ~0U, PUDT_FOTO},			// Detect still image device
	{ 0x01,  ~0U,  ~0U, PUDT_AUDIO},		// Detect audio device
	{ 0x07,  ~0U,  ~0U, PUDT_PRINTER},		// Detect printer
	{ 0xe0, 0x01, 0x01, PUDT_BLUETOOTH},	// Detect bluetooth controller
	{ 0xe0, 0x01, 0x04, PUDT_BLUETOOTH},	// Detect bluetooth controller
	{ 0xe0,  ~0U,  ~0U, PUDT_WIRELESS},		// Detect wireless controller
	{ 0x02,  ~0U,  ~0U, PUDT_COMMUNICATION},// Detect communication device
	{ 0x0a,  ~0U,  ~0U, PUDT_COMMUNICATION},// Detect communication device
	{ 0x03, 0x00, 0x01, PUDT_KEYBOARD},		// Detect keyboard
	{ 0x03, 0x01, 0x01, PUDT_KEYBOARD},		// Detect keyboard
	{ 0x03, 0x00, 0x02, PUDT_MOUSE},		// Detect mouse
	{ 0x03, 0x01, 0x02, PUDT_MOUSE},		// Detect mouse
	{ 0x0b,  ~0U,  ~0U, PUDT_SMART_CARD},	// Detect smart-card reader
	{ 0x08, 0x02,  ~0U, PUDT_ATAPI_STORAGE},	// Detect CD/DVD-ROM
	{ 0x08,  ~0U,  ~0U, PUDT_DISK_STORAGE},		// Detect mass storage
};

static CHwNetAdapter* convertAndCreateNetAdapter( const PrlNet::EthernetAdapter& a );

namespace
{

template<class T>
void CleanupGenericDevicesList( QList<T*> &ptrsList, CBaseNode* parent )
{
	for( int i = 0; i<ptrsList.count(); ++i )
	{
		delete ptrsList.at(i);
	}
	ptrsList.clear();
	parent->clearMaxDynListIds();
}

} // end of namespace

const unsigned int CDspHostInfo::VM_MAX_MEM_32BIT = 8 * 1024;

// Common constructor
void CDspHostInfo::CommonConstructor()
{
	dmask = NULL;
	p_HostHwInfo = NULL;
	m_nRefreshFlags = 0;
}

// Common destructor
void CDspHostInfo::CommonDestructor()
{
	if( p_HostHwInfo )
		delete p_HostHwInfo;
	p_HostHwInfo = NULL;

	m_strListTmp.clear();

	// Cleanup added HDD map
	m_HddMap.clear();
}

void CDspHostInfo::setRefreshFlags( PRL_UINT32 nFlags )
{
	m_nRefreshFlags = nFlags;
}

/**
 * @brief Clean up class-specific properties.
 * @return
 */
void CDspHostInfo::cleanupClassProperties()
{
	// recreate data container
	delete p_HostHwInfo;
	p_HostHwInfo = new CHostHardwareInfo();

	// default RAM size value (avoid to initialize this value with zero
	// since it is used in some important calculations (oevrall memory limit, etc.) )
	p_HostHwInfo->getMemorySettings()->setHostRamSize( 256 );

	// clear temporary list
	m_strListTmp.clear();

} // CDspHostInfo::cleanupClassProperties()


// Update networking adapters info
void CDspHostInfo::updateNetworkInfo()
{
	if (NULL == p_HostHwInfo)
		return;

	CleanupGenericDevicesList( p_HostHwInfo->m_lstNetworkAdapters,
								p_HostHwInfo->getNetworkAdapters() );

	p_HostHwInfo->getNetworkSettings()->setMaxVmNetAdapters(MAX_NET_DEVICES);
	p_HostHwInfo->getNetworkSettings()->setMaxHostNetAdapters(PrlNet::getMaximumAdapterIndex()+1);

	GetNETList();
} //CDspHostInfo::updateNetworkInfo()


void CDspHostInfo::GetNETList()
{
	using namespace PrlNet;

	PRL_RESULT ret = PRL_ERR_FIXME;

	if( !p_HostHwInfo->m_lstNetworkAdapters.isEmpty() )
		return;

	//==================== get eth adapters
	EthAdaptersList ethList;
	ret = makeBindableAdapterList( ethList, true );

	if ( PRL_FAILED( ret ) )
		WRITE_TRACE(DBG_FATAL, "makeBindableAdapterList() return error %#x, [%s]", ret, QSTR2UTF8( getSysErrorText() ) );

	QListIterator<EthernetAdapter> it(ethList);
	while ( it.hasNext() )
	{
		p_HostHwInfo->addNetworkAdapter( convertAndCreateNetAdapter( it.next() ) );
	}//while


	PrlNet::IfIpList ifIpList;

	// Fill ip4 configuration
	PrlNet::getIfaceIpList( ifIpList, true );
	foreach(CHwNetAdapter *netAdapter, p_HostHwInfo->m_lstNetworkAdapters)
	{
		QList<QString> ipList;
		foreach( AddressInfo ai, ifIpList )
		{
			if ( ai.ifaceName == netAdapter->getDeviceId() )
			{
				QString ipMask = ai.address;
				if ( !ai.netmask.isEmpty() )
				{
					ipMask += "/";
					ipMask += ai.netmask;
				}
				ipList += ipMask;
			}
		}
		netAdapter->setNetAddresses( ipList );
	}

	// debug output:
	LOG_MESSAGE (DBG_DEBUG, "hw network adapter list");

	QListIterator< CHwNetAdapter* > it2 = p_HostHwInfo->m_lstNetworkAdapters;
	while ( it2.hasNext() )
	{
		QString	xml = ElementToString< CHwNetAdapter* > ( it2.next(), "debug_hw_adapter_info" );
		LOG_MESSAGE (DBG_DEBUG, "%s", QSTR2UTF8 ( xml ) );
	}//while
}


/**
 * @brief Get memory settings.
 */
void CDspHostInfo::updateMemSettings()
{
	// get host RAM size
	unsigned int mem_total = getMemTotal();
	unsigned int uiMaxMemory = getMaxVmMem();

	// host RAM size
	p_HostHwInfo->getMemorySettings()->setHostRamSize( mem_total );

	// min VM memory
	p_HostHwInfo->getMemorySettings()->setMinVmMemory( VM_MIN_MEM );

	// max VM memory
	p_HostHwInfo->getMemorySettings()->setMaxVmMemory( uiMaxMemory );

	// max reserved memory limit
	// TODO: need to decide what to do with with value
	//       it works only for VMs.
	int max_reserved = mem_total;
	p_HostHwInfo->getMemorySettings()->setMaxReservedMemoryLimit( max_reserved );

	// overall memory limit
	unsigned int overall_limit = max_reserved;
	p_HostHwInfo->getMemorySettings()->setReservedMemoryLimit( overall_limit );

	// recommended max VM memory
	unsigned int rec_mem = qMin( overall_limit,
						   qMin( uiMaxMemory, HostUtils::GetRecommendedMaxVmMemory( mem_total ) ) );
	p_HostHwInfo->getMemorySettings()->setRecommendedMaxVmMemory( rec_mem );

	getAdvancedMemoryInfo();
} // CDspHostInfo::updateMemSettings()


namespace {
/**
 * Hard disks list sorting helpers set
 */
bool compareHardDisks(CHwHardDisk *pHardDisk1, CHwHardDisk *pHardDisk2)
{
	return pHardDisk1->getDeviceId() < pHardDisk2->getDeviceId();
}

void SortHardDisksList(QList<CHwHardDisk *> &lstHardDisks)
{
	qSort(lstHardDisks.begin(), lstHardDisks.end(), compareHardDisks);
}

}

/**
 * @brief Update host data.
 * @return
 */
void CDspHostInfo::updateData(quint64 nFlags)
{
	WRITE_TRACE(DBG_DEBUG, "--- start host info collection ---");

	// cleanup data container
	cleanupClassProperties();

	if (nFlags & uhiOsVersion)
	{
		// get OS version
		GetOsVersion();
		WRITE_TRACE(DBG_DEBUG, "--- OS info collected ---");
	}
	p_HostHwInfo->setVtdInitializationCode(  );
	p_HostHwInfo->setVtdSupported( false );

	if (nFlags & uhiMobile)
	{
		// Detect laptops (it can't be changed dynamically)
		GetMobileDevice();
		WRITE_TRACE(DBG_DEBUG, "--- host info collected ---");
	}

	refresh(nFlags);

} // CDspHostInfo::updateData()


/**
 * @brief Refresh data, which may dynamically change.
 */
void CDspHostInfo::refresh(quint64 nFlags)
{
	WRITE_TRACE(DBG_DEBUG, "--- start host info refreshing ---");

	// prevent system from sleep until we touch hardware
	CPowerWatcher::CNoSleepLocker sleepLocker;
	if (sleepLocker.isSleeping()) {
		WRITE_TRACE(DBG_FATAL,
			"[HostInfo] Unsafe refresh: system is sleeping");
	}

	// clear temporary list
	m_strListTmp.clear();

	if (nFlags & uhiCpu)
	{
		// get CPU info
		GetCpuCommon();
		WRITE_TRACE(DBG_DEBUG, "--- CPU info refreshed ---");
	}

	if (nFlags & uhiMemory)
	{
		// get memory settings
		updateMemSettings();
		WRITE_TRACE(DBG_DEBUG, "--- memory info refreshed ---");
	}

	// NOTE: HDD should be before USB (HDD may have USB alias)
	if (nFlags & uhiHdd)
	{
		// get list of hard disks
		GetHDDList();
		SortHardDisksList(p_HostHwInfo->m_lstHardDisks);

		// force USB enumeration to update USB disk aliases (add or remove them)
		nFlags |= uhiUsb;

		WRITE_TRACE(DBG_DEBUG, "--- HDD info refreshed ---");
	}

	if (nFlags & uhiUsb)
	{
		// Don't clear usb-devices-list. We use old list to get properties:
		// CHwGenericDevice::m_qsVmUuid && CHwGenericDevice::m_ctDeviceState
		// for a new usb-devices-list
		// CleanupGenericDevicesList( p_HostHwInfo->m_lstUsbDevices );

		// get list of USB devices
		GetUSBList();
		WRITE_TRACE(DBG_DEBUG, "--- USB info refreshed ---");
	}

	if (nFlags & uhiPci)
	{
		// clear list of PCI devices
		CleanupGenericDevicesList(p_HostHwInfo->m_lstGenericPciDevices, p_HostHwInfo->getGenericPciDevices());
		// get pci list of PCI devices
		WRITE_TRACE(DBG_DEBUG, "--- PCI info refreshed ---");
	}

	if (nFlags & uhiScsi)
	{
		// clear list of SCSI devices
		CleanupGenericDevicesList(p_HostHwInfo->m_lstGenericScsiDevices, p_HostHwInfo->getGenericScsiDevices());
		// get list of SCSI devices
		GetScsiList();
		WRITE_TRACE(DBG_DEBUG, "--- SCSI info collected ---");
	}

	if (nFlags & uhiPrinter)
	{
		// get list of printers (Windows only))
		GetPrinterList();
		WRITE_TRACE(DBG_DEBUG, "--- printer info refreshed ---");
	}

#ifdef _WIN_
	if (nFlags & uhiRasterizer)
	{
		// Find PS rasterizer
		GetRasterizer();
	}
#endif

	if (nFlags & uhiCd)
	{
		// clear list of CDs
		CleanupGenericDevicesList(p_HostHwInfo->m_lstOpticalDisks, p_HostHwInfo->getCdROMs());
		// get list of CD/DVD-ROM disks
		GetCDList();
		WRITE_TRACE(DBG_DEBUG, "--- CD info refreshed ---");
	}

	if (nFlags & uhiSerial)
	{
		// clear list of serial ports
		CleanupGenericDevicesList(p_HostHwInfo->m_lstSerialPorts, p_HostHwInfo->getSerialPorts());
		// get list of serial ports
		GetCOMList();
		WRITE_TRACE(DBG_DEBUG, "--- COM info refreshed ---");
	}

	if (nFlags & uhiFloppy)
	{
		// clear list of Hdds
		CleanupGenericDevicesList(p_HostHwInfo->m_lstFloppyDisks, p_HostHwInfo->getFloppyDisks());
		// get list of floppy disks
		GetFDDList();
		WRITE_TRACE(DBG_DEBUG, "--- FDD info refreshed ---");
	}

	if (nFlags & uhiParallel)
	{
		// clear list of Lpt
		CleanupGenericDevicesList(p_HostHwInfo->m_lstParallelPorts, p_HostHwInfo->getParallelPorts());
		// get list of parallel ports
		GetLPTList();
		WRITE_TRACE(DBG_DEBUG, "--- LPT info refreshed ---");
	}

	if (nFlags & uhiNet)
	{
		// get Network Info
		updateNetworkInfo();
		WRITE_TRACE(DBG_DEBUG, "--- NET info refreshed ---");
	}

} // CDspHostInfo::refresh()

QString CDspHostInfo::getKernelBitness()
{
	return QString("%1").arg(osInfo_getArchitecture());
}

bool CDspHostInfo::Get64BitStatus()
{
	return osInfo_getArchitecture() == OSINFO_ARCHITECTURE_64_BIT;
}

QString CDspHostInfo::getSystemBitness()
{
	if ( Get64BitStatus() )
		return PRL_HOST_OS_64_BIT;
	return PRL_HOST_OS_32_BIT;
}

/**
 * Return maximum allowed guest RAM size.
 *
 * 1) For any product running on 32-bit hardware maximum guest RAM size is
 *    limited to VM_MAX_MEM_32BIT. This is empiric constant that
 *    reflects current VMM's architectural limitation.
 * 2) For Desktop it's limited to VM_MAX_MEM_32BIT regardless of
 *    host bitness.
 * 3) For non-desktop products on 64-bit hardware it's limited to VM_MAX_MEM.
 *
 * @return guest RAM limit in megabytes
 */
unsigned int CDspHostInfo::getMaxVmMem()
{
	if (IsProcessorSupportIA32e())
		return VM_MAX_MEM;
	return VM_MAX_MEM_32BIT;
}

/**
* Checks whether host processor supports Intel 64 architecture
*
* @return TRUE if process supports Intel 64 architecture
*
* @author lenkor
*/
BOOL CDspHostInfo::IsProcessorSupportIA32e()
{
	return HostUtils::GetCpuidEax(0x80000000) >= 0x80000001 &&
	      (HostUtils::GetCpuidEdx(0x80000001) & (1u << 29));
}

ULONG64 CDspHostInfo::GetHvtFeatures()
{
	static QMutex s_featuresMutex;
	QMutexLocker Locker(&s_featuresMutex);

	static ULONG64 s_hvtFeatures = ~0;
	if (s_hvtFeatures != (ULONG64)~0)
		return s_hvtFeatures;

	//TODO: find a way to determine HVT features
	return s_hvtFeatures;
}

UINT CDspHostInfo::GetCpuVendor(UCHAR* pVendorName)
{
	static UINT l_uCpuVendor = CPU_VENDOR_UNKNOWN;
	static UCHAR aVendor[13] = {0};
	static QMutex	l_VendorMutex;

	QMutexLocker Locker(&l_VendorMutex);
	if ( l_uCpuVendor == CPU_VENDOR_UNKNOWN )
	{
		UINT uEax, uEbx, uEcx, uEdx;
		uEax = 0;
		uEcx = 0;
		HostUtils::GetCpuid(uEax, uEcx, uEdx, uEbx);
		*(UINT *) &aVendor[0] = uEbx;
		*(UINT *) &aVendor[4] = uEdx;
		*(UINT *) &aVendor[8] = uEcx;
		aVendor[12] = 0;

		if (memcmp(aVendor, "GenuineIntel", 12) == 0)
			l_uCpuVendor = CPU_VENDOR_INTEL;
		else if (memcmp(aVendor, "AuthenticAMD", 12) == 0)
			l_uCpuVendor = CPU_VENDOR_AMD;
		else
			l_uCpuVendor= CPU_VENDOR_UNKNOWN;
	}
	if (pVendorName)
		memcpy(pVendorName, aVendor, sizeof(aVendor));
	return l_uCpuVendor;
}

void CDspHostInfo::GenericPciDeviceSupport(CHwGenericPciDevice* pPciDevice)
{
	if ( pPciDevice->getType() != PGD_PCI_DISPLAY )
		return;

	QStringList lstVCards = QStringList()
	<< SVC_NVIDIA_QUADRO_FX_3800 << SVC_NVIDIA_QUADRO_FX_4800 << SVC_NVIDIA_QUADRO_FX_5800;

	pPciDevice->setSupported(lstVCards.contains(pPciDevice->getDeviceName()));
}

/* update Cpu Info*/
void CDspHostInfo::GetCpuCommon()
{
	CHwCpu* pCpu = p_HostHwInfo->getCpu();
	if ( pCpu )
	{
		// Get vtx/svm modes
		ULONG64 hvtFeatures = GetHvtFeatures();
		if ((hvtFeatures & 1) == 1)
		{
			if (GetCpuVendor() == CPU_VENDOR_AMD)
				pCpu->setVtxMode(PCM_CPU_AMD_V);
			else
				pCpu->setVtxMode(PCM_CPU_INTEL_VT_X);
			pCpu->setHvtNptAvail(hvtFeatures & HVF_NPT);
			pCpu->setHvtUnrestrictedAvail(hvtFeatures & HVF_VTX_UNRESTRICTED);
		} else
			pCpu->setVtxMode(PCM_CPU_NONE_HV);
		//FIXME VtdEnable is not set anywhere
		//VtdEnable is erroniously? named Vtx in xml model file
		// Get vtd mode
		GetCpu();

#ifdef _LIN_
		CCpuHelper::fill_cpu_info(*pCpu);
#endif
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "CDspHostInfo::GetCpu() : CHwCpu is NULL!" );
		return;
	}

}


static CHwNetAdapter* convertAndCreateNetAdapter( const PrlNet::EthernetAdapter& a )
{
	CHwNetAdapter*
		ret = new CHwNetAdapter (
			PDE_GENERIC_NETWORK_ADAPTER,
			a._name,
			a._systemName,
			a._adapterIndex,
			PrlNet::ethAddressToString(a._macAddr),
			a._vlanTag,
			a._bEnabled,
			a._adapterGuid
		);

	if ( PrlNet::isWIFIAdapter(a) )
		ret->setNetAdapterType(PHY_WIFI_REAL_NET_ADAPTER);
	else if ( PrlNet::isVirtualAdapter(a) )
		ret->setNetAdapterType(PHI_VIRTUAL_NET_ADAPTER);
	else
		ret->setNetAdapterType(PHI_REAL_NET_ADAPTER);

	return ret;
} //namespace

/**
 * Dump all partitions from the HwPartition info
 * This function is need to debug real disks and
 * partitions support
 *
 * @param Level of receursion
 * @param HDD partitions list
 *
 * @author antonz@
 *
 * @return Error code
 */
void CDspHostInfo::DumpParts(int iLevel, QList<CHwHddPartition*>& lst)
{
	int i = 0;
	CHwHddPartition* pPart;
	QList<CHwHddPartition*>::iterator Start, End;
	WRITE_TRACE(DBG_FATAL, "Level %u\n", iLevel);

	for(Start = lst.begin(), End = lst.end();
		Start != End;
		Start++)
	{
		WRITE_TRACE(DBG_FATAL, "Partition %u", i);
		i++;
		pPart = *Start;

		WRITE_TRACE(DBG_FATAL, "Name: %s", pPart->getName().toUtf8().data());
		WRITE_TRACE(DBG_FATAL, "System Name: %s", pPart->getSystemName().toUtf8().data());
		WRITE_TRACE(DBG_FATAL, "Index %u", pPart->getIndex());
		WRITE_TRACE(DBG_FATAL, "Size %llu", pPart->getSize());
		WRITE_TRACE(DBG_FATAL, "Type %u", pPart->getType());
		WRITE_TRACE(DBG_FATAL, "Active %u", pPart->getIsActive());

		if (pPart->m_lstPartitions.size())
		{
			DumpParts(iLevel + 1, pPart->m_lstPartitions);
			WRITE_TRACE(DBG_FATAL, "Returned to level %u\n", iLevel);
		}
	}
}

/**
 * Create system name for USB devices
 */
QString CDspHostInfo::CreateUsbSystemName(	const QString &sLocationId,
											UINT  uVID, UINT uPID,
											const QString &sDeviceSpeed,
											const QString &sSuffix,
											const QString &sSerialNumber)
{
	// Create usb-device identifier :
	//		<System Name>|<Vendor ID>|<Product ID>|<Device Bus Speed>|
	//      <Keyboard/Mouse or ParallelsDevice>|<Serial Number>
	return QString("%1|%2|%3|%4|%5|%6")
					.arg(sLocationId)
					.arg(uVID , 4, 16, QChar('0'))
					.arg(uPID, 4, 16, QChar('0'))
					.arg(sDeviceSpeed)
					.arg(sSuffix)
					.arg(sSerialNumber);
}


/**
* Check usb frindly name for readability
*/
bool CDspHostInfo::CheckUsbFriendlyName(const QString &sFriendlyName)
{
	return  (!sFriendlyName.isEmpty() &&
			!sFriendlyName.contains("composite device", Qt::CaseInsensitive) &&
			!sFriendlyName.contains("port_#", Qt::CaseInsensitive) &&
			!sFriendlyName.contains("parallels usb device", Qt::CaseInsensitive));
}

/**
 * Create friendly name for USB devices
 *
 * @param USB Device's class id
 * @param USB Device's vendor id
 * @param USB Device's product id
 *
 * @author alexkod
 *
 * @return USB device friendly name
 */
QString CDspHostInfo::CreateUsbFriendlyName( UINT uClassID, UINT uVendorID, UINT uProductID )
{
	// Searching Manufacturer (Vendor)
	PUSB_VENDOR_ID pVID = aVendorList;
	while( !(pVID->uVendorID & 0x80000000) && (pVID->uVendorID != uVendorID) )
		pVID++;

	// Get vendor name
	QString strVendor;
	if( pVID->uVendorID & 0x80000000 )
		strVendor.sprintf("Vendor %04X", uVendorID);
	else
		strVendor = pVID->szVendor;

	// Searching Device (Product)
	PUSB_DEVICE_ID pPID = pVID->pUsbDevice;
	while( !(pPID->uDeviceID & 0x80000000) && (pPID->uDeviceID != uProductID) )
		pPID++;

	// Get product name
	QString strDevice;
	if( pPID->uDeviceID & 0x80000000 )
	{
#ifdef _MAC_
		// Get name of class for USB device
		switch( uClassID )
		{
		case kUSBCompositeClass:
			strDevice = "Composite";
			break;
		case kUSBAudioClass:
			strDevice = "Audio";
			break;
		case kUSBCommunicationClass:
			strDevice = "Communication";
			break;
		case kUSBHIDClass:
			strDevice = "HID";
			break;
		case kUSBPrintingClass:
			strDevice = "Printer";
			break;
		case kUSBMassStorageClass:
			strDevice = "Mass Storage";
			break;
		case kUSBHubClass:
			strDevice = "Hub";
			break;
		case kUSBDataClass:
			strDevice = "Data";
			break;
		case kUSBChipSmartCardInterfaceClass:
			strDevice = "Chip SmartCard";
			break;
		case kUSBContentSecurityInterfaceClass:
			strDevice = "Content Security";
			break;
		case kUSBVideoInterfaceClass:
			strDevice = "Video";
			break;
		case kUSBDiagnosticClass:
			strDevice = "Diagnostic";
			break;
		case kUSBWirelessControllerClass:
			strDevice = "Wireless";
			break;
		case kUSBMiscellaneousClass:
			strDevice = "Miscellaneous";
			break;
		case kUSBApplicationSpecificClass:
			strDevice = "Application Specific";
			break;
		case kUSBVendorSpecificClass:
			strDevice = "Vendor Specific";
			break;
		default:
			strDevice.sprintf("USB Device %04X", uProductID);
		}
#else
		Q_UNUSED(uClassID);
		strDevice.sprintf("USB Device %04X", uProductID);
#endif // _MAC_

	}
	else
		strDevice = pPID->szDevice;
	//
	return QString("%1 - %2").arg(strVendor).arg(strDevice);
}

/**
* Search old_dev_lst for already connected device.
* if found -> detach from old_dev_list, update system name and append to new_dev_lst
* if not found -> create new elemet and append it to new_dev_lst.
*/
void CDspHostInfo::LookupOrCreateUSBDevice(QList<CHwUsbDevice*> &old_dev_lst,
										   QList<CHwUsbDevice*> &new_dev_lst,
										   const QString &_sys_name, const QString &_frn_name,
										   PRL_USB_DEVICE_TYPE pudt)
{
	QString frn_name_auth;
	CHwUsbDevice *new_dev = NULL;
	// Remove zeros from names
	QString sys_name(_sys_name), frn_name(_frn_name);
	sys_name.replace(QChar('\x0'), QChar(' '));
	frn_name.replace(QChar('\x0'), QChar(' '));
	if (sys_name != _sys_name)
		WRITE_TRACE(DBG_FATAL, "LOOKUP0 system   name was patched <%s>", QSTR2UTF8(sys_name));
	if (frn_name != _frn_name)
		WRITE_TRACE(DBG_FATAL, "LOOKUP0 friendly name was patched <%s>", QSTR2UTF8(frn_name));
	// Scan list to find already connected devices
	foreach(CHwUsbDevice *old_dev,  old_dev_lst) {
		QString sys_name2 = old_dev->getDeviceId();
		// Compare full system name
		if (sys_name != sys_name2)
			continue;
		// Remove device from old list
		old_dev_lst.removeAll(old_dev);
		new_dev = old_dev;
		// re-Lookup friendly name for already connected device
		frn_name_auth = m_UsbAuthenticList.LookupName(sys_name, frn_name, pudt);
		break;
	}
	// Scan new_dev_lst to detect invalid serial
	QString vid_pid = USB_VID_PID(sys_name);
	QString sn		= USB_SERIAL(sys_name);
	CHwUsbDevice *conflict_dev = NULL;
	foreach(CHwUsbDevice *cur_dev, new_dev_lst) {
		QString cur_sys_name = cur_dev->getDeviceId();
		if (USB_VID_PID(cur_sys_name) == vid_pid && USB_SERIAL(cur_sys_name) == sn) {
			conflict_dev = cur_dev;
			break;
		}
	}
	// Add vid+pid+serial to black list
	if (conflict_dev) {
		BOOL need_relookup = m_UsbAuthenticList.AddToBlackList(sys_name);
		// re-Lookup friendly name for conflicting device only if vid+pi+serial first added to black list
		if (new_dev && need_relookup) {
			QString frn_name_conf = m_UsbAuthenticList.LookupName(conflict_dev->getDeviceId(), frn_name, pudt);
			conflict_dev->setDeviceName(frn_name_conf);
			WRITE_TRACE(DBG_INFO, "LOOKUP2 <%s> <%s>",
						QSTR2UTF8(conflict_dev->getDeviceId()),
						QSTR2UTF8(frn_name_conf));
		}
	}
	// Create new device and lookup fiendly name
	if (!new_dev) {
		new_dev = new CHwUsbDevice();
		frn_name_auth = m_UsbAuthenticList.LookupName(sys_name, frn_name, pudt);
	}
	WRITE_TRACE(DBG_INFO, "LOOKUP1 <%s> <%s> <%s>",
		QSTR2UTF8(sys_name), QSTR2UTF8(frn_name_auth), QSTR2UTF8(PRL_USB_DEVICE_TYPE_to_QString(pudt)));
	// Rewrite device's system name
	new_dev->setDeviceId(sys_name);
	// Rewrite device's friendly name
	new_dev->setDeviceName(frn_name_auth);
	// Rewrite device's type
	new_dev->setUsbType(pudt);
	// Append device to new_dev_lst
	new_dev_lst.append(new_dev);
	return;
}

/**
 * Returns pointer to the USB authentic object
 */
CUsbAuthenticNameList *CDspHostInfo::getUsbAuthentic()
{
	return (&m_UsbAuthenticList);
}

/*
 * Add specified HDD to map if it not exist, mark it processed and
 * return is it in list or not
 *
 * @param HDD BSD name
 *
 * @author antonz
 *
 * @return True if device already exist in map
 */
bool CDspHostInfo::CheckAddHdd(const QString& name)
{
	std::pair<AddedHDDMap::iterator, bool> Element;

	LOG_MESSAGE(DBG_DEBUG, "Process HDD [%s]", name.toUtf8().constData());

	Element = m_HddMap.insert(AddedHDDPair(name, true));

	/*
	 * Element already exists, so we need to set the processed flag
	 * to prevent dropping it out
	 */
	if (!Element.second)
	{
		LOG_MESSAGE(DBG_DEBUG, "HDD [%s] exists in the map.", name.toUtf8().constData());
		Element.first->second = true;
	} else
		LOG_MESSAGE(DBG_DEBUG, "HDD [%s] not exists in the map.", name.toUtf8().constData());

	return !Element.second;
}

/*
 * Drop unprocessed HDD from map (it means that they are removed)
 *
 * @author antonz
 *
 * @return Dropped HDDs count
 */
PRL_UINT32 CDspHostInfo::DropUnprocessedHdd(QList<CHwHardDisk*>& List)
{
	PRL_UINT32 uiDropped = 0;
	QList<CHwHardDisk*>::iterator itHdd = List.begin();
	AddedHDDMap::iterator itFound;

	while(itHdd != List.end())
	{
		LOG_MESSAGE(DBG_DEBUG, "Search for HDD from list [%s] in the map.",
						(*itHdd)->getDeviceId().toUtf8().constData());

		itFound = m_HddMap.find((*itHdd)->getDeviceId());

#ifdef _DEBUG
		// That situation should not happen.
		if (itFound == m_HddMap.end())
		{
			LOG_MESSAGE(DBG_DEBUG, "Error searching HDD from list [%s] in the map.",
									(*itHdd)->getDeviceId().toUtf8().constData());
			// Remove HDD from list in any case
			itHdd = List.erase(itHdd);
			continue;
		}
#endif

		// That device was not processed, so we need to remove it from list and map
		if (!itFound->second)
		{
			itHdd = List.erase(itHdd);
			m_HddMap.erase(itFound);
			uiDropped++;
		} else {
			itHdd++;
			// Drop processed flag to check for changes next time
			itFound->second = false;
		}
	}

	// i think, that we not need to clear map from "processed" units
#ifdef _DEBUG
	// But better to check
	AddedHDDMap::iterator it;

	for(it = m_HddMap.begin(); it != m_HddMap.end(); it++)
		if (it->second)
		{
			WRITE_TRACE(DBG_FATAL, "Found undeleted \"processed\" flag in the entry [%s]",
							it->first.toUtf8().constData());
			it->second = false;
		}
#endif

	return uiDropped;
}

/*
 * Add HDD and its partitions to the list
 *
 * @param Disk friendly name
 * @param Disk system name
 * @param Disk size, 0 - determine size by disk image itself
 *
 * @author antonz
 *
 * @return True if HDD was added
 */
bool CDspHostInfo::HddAddToList(const QString& friendlyName,
								const QString& systemName,
								PRL_UINT64 uiSize,
								PRL_UINT32 uiRate,
								bool Removable,
								bool External,
								QList<CHwHardDisk*>& List)
{
	PRL_UINT64 uiDiskSize = uiSize;
	PRL_RESULT Err = PRL_ERR_SUCCESS;

	// Now we _MUST_ skip unequal mbr, user should see its bootcamp
	if (PRL_FAILED(Err) && (Err != PRL_ERR_DISK_GPT_MBR_NOT_EQUAL))
	{
		WRITE_TRACE(DBG_FATAL, "Error 0x%x when enumerating partitions", Err);
		return false;
	}

	CHwHardDisk* pHwDisk = new (std::nothrow) CHwHardDisk( friendlyName, uiDiskSize,
															uiRate, systemName);

	if (!pHwDisk)
	{
		WRITE_TRACE(DBG_FATAL, "Error creating CHwHardDisk!");
		return false;
	}

	pHwDisk->setRemovable(Removable);
	pHwDisk->setExternal(External);

	List.append( pHwDisk );

	return true;
}

BOOL CUsbAuthenticNameList::IsTheSameDevice(const QString& sys_name1, const QString& sys_name2) const
{
	// 1. vid&pid doesn't match or
	// 2. serial valid && doesn't match or
	// 3. serial isnt't valid & sys_path doesn't match:
	if( !IsDevicesIdEqual( sys_name1, sys_name2 ) )
		return FALSE;

	BOOL sn_valid = !CheckBlackList( sys_name1 );

	if( sn_valid && USB_SERIAL(sys_name1) != USB_SERIAL(sys_name2) )
		return FALSE;

	if( !sn_valid && USB_SYS_PATH(sys_name1) != USB_SYS_PATH(sys_name2) )
		return FALSE;

	return TRUE;
}

/**
 * Lookup device's friendly name (bugfix for #439207)
 */
QString CUsbAuthenticNameList::LookupName(const QString &sys_name, const QString &frn_name, PRL_USB_DEVICE_TYPE &pudt)
{
	CDispUsbIdentity *elm = NULL;
	PRL_USB_DEVICE_TYPE pudt2 = PUDT_OTHER;
	UINT index = 0;
	foreach(CDispUsbIdentity *elm2, m_UsbPreferences.m_lstAuthenticUsbMap) {
		if (IsTheSameDevice (sys_name, elm2->getSystemName())) {
			elm = elm2;
			goto activate_element;
		}
		// Check friendly name, if equal -> update index
		if (frn_name == elm2->getFriendlyName() && index < elm2->getIndex())
			index = elm2->getIndex();
	}
	// Create & fill element
	elm = new CDispUsbIdentity;
	elm->setFriendlyName(frn_name);
	elm->setIndex(index+1);
	m_UsbPreferences.m_lstAuthenticUsbMap.append(elm);

activate_element:
	elm->setSystemName(sys_name);
	// Create authentic friendly name
	QString auth_frn_name = elm->getFriendlyName();
	// if this is first device -> skip index adding
	if (elm->getIndex() > 1)
		 auth_frn_name += QString(" #%1").arg(elm->getIndex());
	// Update usb device type
	pudt2 = elm->getUsbType();
	if (pudt == PUDT_OTHER) {
		pudt = pudt2;
	} else {
		if (pudt2 != PUDT_OTHER && pudt2 != pudt)
			WRITE_TRACE(DBG_FATAL, "Conflicting usb device type (%d, %d)", pudt, pudt2);
		elm->setUsbType(pudt);
	}
	return auth_frn_name;
}

/**
 * Add sys_name to black-list if not yet added.
 * If vid+pid+serial first time added to black list -> return TRUE
 */
BOOL CUsbAuthenticNameList::AddToBlackList(const QString &sys_name)
{
	if (CheckBlackList(sys_name))
		return FALSE;
	m_UsbPreferences.setUsbBlackList(m_UsbPreferences.getUsbBlackList()<<sys_name);
	return TRUE;
}

/**
 * Check black list and return TRUE if vid+pid+serial found
 */
BOOL CUsbAuthenticNameList::CheckBlackList(const QString &sys_name) const
{
	// Empty sn always in black list
	QString sn = USB_SERIAL(sys_name);
	if (sn == USB_NUM_EMPTY)
		return TRUE;
	// Check other entry
	QString vid_pid = USB_VID_PID(sys_name);
	foreach (const QString &sys_name2, m_UsbPreferences.getUsbBlackList()) {
		if (USB_VID_PID(sys_name2) == vid_pid && USB_SERIAL(sys_name2) == sn)
			return TRUE;
	}
	return FALSE;
}

/**
 * Compare devices' vid&pid.
 */
BOOL CUsbAuthenticNameList::IsDevicesIdEqual(const QString &sys_name1, const QString &sys_name2) const
{
	bool ok1, ok2, ok3, ok4;
	USHORT vid1 = (USHORT)USB_VID(sys_name1).toUInt(&ok1, 16);
	USHORT pid1 = (USHORT)USB_PID(sys_name1).toUInt(&ok2, 16);
	USHORT vid2 = (USHORT)USB_VID(sys_name2).toUInt(&ok3, 16);
	USHORT pid2 = (USHORT)USB_PID(sys_name2).toUInt(&ok4, 16);
	if (!(ok1 && ok2 && ok3 && ok4))
		return FALSE;

	if (vid1 != vid2)
		return FALSE;

	if (pid1 == pid2)
		return TRUE;

	// List of device's VID + PID[] interpret as equal devices
	static const USHORT equal_dev_lst[] =
		{//	count vid	 pid1    pid2	 pid3....
			4, 0x05AC, 0x1000, 0x8205,			// Apple BT
			4, 0x05AC, 0x8501, 0x8503,			// Apple iSight
			5, 0x0FCA, 0x0001, 0x0004, 0x0006	// BlackBerry Perl
		};

	for (UINT i = 0; i < sizeof(equal_dev_lst)/sizeof(equal_dev_lst[0]); i += equal_dev_lst[i]) {
		if (vid1 != equal_dev_lst[i+1])
			continue;

		bool b1 = false, b2 = false;
		for (INT j = 0; j < (equal_dev_lst[i]-2); j++) {
			USHORT pid = equal_dev_lst[i+j+2];
			if (pid == pid1)
				b1 = true;
			else if (pid == pid2)
				b2 = true;

			if (b1 && b2)
				return TRUE;
		}
	}

	return FALSE;
}

void CUsbAuthenticNameList::SetUsbPreferences(const CDispUsbPreferences &_usb_prefs)
{
	m_UsbPreferences.fromString(_usb_prefs.toString());
}

const CDispUsbPreferences &CUsbAuthenticNameList::GetUsbPreferences() const
{
	return (m_UsbPreferences);
}

/**
 * Check device for exect match device_id+vendor_id
 */
PRL_USB_DEVICE_TYPE CDspHostInfo::UsbDEV2PUDT(UINT ven_id, UINT dev_id)
{
	for (UINT i = 0; i < ARRAY_SIZE(usb_dev2pudt); i++) {
		const USB_DEV2PUDT &elem = usb_dev2pudt[i];
		if ((elem.ven_id == ven_id || elem.ven_id == (UINT)~0) &&
			(elem.dev_id == dev_id || elem.dev_id == (UINT)~0))
			return elem.pudt;
	}
	return PUDT_OTHER;
}

/**
 * Check usb interface calss in order of priority
 */
PRL_USB_DEVICE_TYPE CDspHostInfo::UsbIFC2PUDT(UINT ifc_class, UINT ifc_subclass, UINT ifc_protocol)
{
	for (UINT i = 0; i < ARRAY_SIZE(usb_ifc2pudt); i++) {
		const USB_IFC2PUDT &elem = usb_ifc2pudt[i];
		if ((elem.ifc_class == ifc_class || elem.ifc_class == (UINT)~0) &&
			(elem.ifc_subclass == ifc_subclass || elem.ifc_subclass == (UINT)~0) &&
			(elem.ifc_protocol == ifc_protocol || elem.ifc_protocol == (UINT)~0)) {
			return elem.pudt;
		}
	}
	return PUDT_OTHER;
}

QString CDspHostInfo::GetOsVersionStringRepresentation()
{
	QString sOsVersion;
	GetOsVersion( 0, &sOsVersion );
	return sOsVersion;
}

void CDspHostInfo::GetDiskUsbAliases(QList<CHwUsbDevice*> &old_dev_lst, QList<CHwUsbDevice*> &new_dev_lst)
{
	CHwHardDisk* disk;
	foreach(disk, p_HostHwInfo->m_lstHardDisks) {
		if (!isDiskUsbAliasNeeded(disk))
			continue;

		// XXX: keep this in sync with USB emulation code

		QString tag(QByteArray(QCryptographicHash::hash(
							  disk->getDeviceName().toUtf8(),
							  QCryptographicHash::Sha1)).toHex());

		QString sys_name = QString("%1@%2|%3|%4@%5")
								   .arg(PRL_VIRTUAL_MSC_PATH0)
								   .arg(tag)
								   .arg(PRL_VIRTUAL_MSC_PATH1)
								   .arg(tag)
								   .arg(disk->getDeviceId());

		QString frn_name = disk->getDeviceName();

		LookupOrCreateUSBDevice(old_dev_lst, new_dev_lst, sys_name, frn_name, PUDT_DISK_STORAGE);
	}
}

bool CDspHostInfo::isDiskUsbAliasNeeded(CHwHardDisk* disk)
{
	// always skip ROOT (system) disk -- we can't unmount it :) but it may
	// be external/removable (host booted from alternate media)
	if (CFileHelper::isRootDevice(disk->getDeviceId()))
		return false;

	// group rules (and config options) by priority
	// because USB and FireWire devices can be attached via thunderbolt dock
	// and USB always marked as removable device:
	// 1. USB or FireWire
	// 2. Thunderbolt
	// 3. Any device marked as removable

	// 1: USB and FireWire
	if (CFileHelper::isUsbDevice(disk->getDeviceId()))
		return m_UsbAuthenticList.GetUsbPreferences().getUsbVirtualDisks()->isUsb();
	if (CFileHelper::isFireWireDevice(disk->getDeviceId()))
		return m_UsbAuthenticList.GetUsbPreferences().getUsbVirtualDisks()->isFireWire();

	// 2: Thunderbolt device
	if (CFileHelper::isThunderboltDevice(disk->getDeviceId()))
		return m_UsbAuthenticList.GetUsbPreferences().getUsbVirtualDisks()->isThunderbolt();

	// 3: any device marked as external
	if (disk->isRemovable())
		return m_UsbAuthenticList.GetUsbPreferences().getUsbVirtualDisks()->isRemovable();

	return false;
}
