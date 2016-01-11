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
///	CDspHostInfo.h
///
/// @brief
///	Definition of the class CDspHostInfo
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

#ifndef __CDSPHOSTINFO_H__
#define __CDSPHOSTINFO_H__
#include <QStringList>
#include <QList>
#include <map>

#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlcommon/Interfaces/ParallelsTypes.h>
#include <prlcommon/Interfaces/ParallelsQt.h>

#include <prlsdk/PrlOses.h>

//#include "Libraries/DiskImage/PartitionTable.h"  // DiskImage commented out by request from CP team
#include "XmlModel/DispConfig/CDispUsbPreferences.h"

class CHostHardwareInfo;
class CHwHddPartition;
class CHwGenericPciDevice;
class CHwGenericDevice;
class CHwUsbDevice;
class CHwHardDisk;
class CHwOsVersion;

#define HYPSTATUS_BADCODE 0xbaadc0de

// Define strings for usb device's default names
#define USB_MAN_UNKNOWN		"Unknown Manufacturer"
#define USB_DEV_UNKNOWN		"Unknown Device"
#define USB_NUM_EMPTY		"Empty"

// Define string for usb device's bus speed
#define USB_SPD_LOW			"low"
#define USB_SPD_FULL		"full"
#define USB_SPD_HIGH		"high"
#define USB_SPD_SUPER		"super"
#define USB_SPD_UNKNOWN		"unknown"

// Define macro to extract fields from system name
#define USB_SYS_PATH(str)	(str.section('|', 0, 0))
#define USB_VID(str)		(str.section('|', 1, 1))
#define USB_PID(str)		(str.section('|', 2, 2))
#define USB_VID_PID(str)	(str.section('|', 1, 2))
#define USB_SPEED(str)		(str.section('|', 3, 3))
#define USB_STATE(str)		(str.section('|', 4, 4))
#define USB_SERIAL(str)		(str.section('|', 5, 5))

// Virtual devices vid & pid
#define PRL_USB_VID				0x203A
#define PRL_USB_HUB_PID			0xFFFE
#define PRL_USB_CCID_PID		0xFFFD
#define PRL_USB_MOUSE_PID		0xFFFC
#define PRL_USB_KBD_PID			0xFFFB
#define PRL_USB_PRN_PID			0xFFFA
#define PRL_USB_UVC_PID			0xFFF9
#define PRL_USB_BT_PID			0xFFF8
#define PRL_USB_MSC_PID			0xFFF7
#define PRL_USB_MOUSE2_PID		0xFFF6
#define APPLE_USB_VID			0x05AC
#define APPLE_USB_KBD_PID		0x0221	// ISO Aluminium keyboard (GE)
#define MS_USB_VID				0x045E
#define MS_USB_BT_PID			0x007E

// Virtual devices names
#define PRL_VIRTUAL_HUB_PATH	"VIRTUAL@HUB@|203a|fffe|full|--|PW3.0"
#define PRL_VIRTUAL_HUB_NAME	"Virtual USB1.1 HUB"

#define PRL_VIRTUAL_CCID_PATH0	"VIRTUAL@CCID"
#define PRL_VIRTUAL_CCID_PATH1	"203a|fffa|full|--"
#define PRL_VIRTUAL_CCID_NAME	"Proxy CCID"

#define PRL_VIRTUAL_MOUSE		"VIRTUAL@MOUSE@"
#define PRL_VIRTUAL_MOUSE_PATH	PRL_VIRTUAL_MOUSE "|203a|fffc|full|--|PW3.0"
#define PRL_VIRTUAL_MOUSE_NAME	"Virtual Mouse"

#define PRL_VIRTUAL_KBD			"VIRTUAL@KEYBOARD@"
#define PRL_VIRTUAL_KBD_PATH	PRL_VIRTUAL_KBD "|203a|fffb|full|--|PW3.0"
#define PRL_VIRTUAL_KBD_NAME	"Virtual Keyboard"

#define PRL_VIRTUAL_PRN_PATH0	"VIRTUAL@PRINTER@|203a|fffa|full|--"
#define PRL_VIRTUAL_PRN_PATH	PRL_VIRTUAL_PRN_PATH0 "|PW3.0"
#define PRL_VIRTUAL_PRN_NAME	"Virtual Printer"

// PRL_VIRTUAL_PRN_TAG - current shared printer serial number prefix (tag)
// PRL_VIRTUAL_PRN_TAG[3] - tag version. it will allow easily
// and automatically (by means of guest tools) deleting all
// printers in guest if (for some reason) printer model need
// to be changed.
#define PRL_VIRTUAL_PRN_TAG_BASE	"TAG"
#define PRL_VIRTUAL_PRN_TAG_V1		PRL_VIRTUAL_PRN_TAG_BASE "1"
#define PRL_VIRTUAL_PRN_TAG_V2		PRL_VIRTUAL_PRN_TAG_BASE "2"

#define PRL_USB_UVC_HIGH_SPEED
#ifdef PRL_USB_UVC_HIGH_SPEED
#define PRL_VIRTUAL_UVC_PATH0	"VIRTUAL@UVC@|203a|fff9|high|--"
#else
#define PRL_VIRTUAL_UVC_PATH0	"VIRTUAL@UVC@|203a|fff9|full|--"
#endif
#define PRL_VIRTUAL_UVC_PATH	PRL_VIRTUAL_UVC_PATH0 "|PW3.0"
#define PRL_VIRTUAL_UVC_NAME	"Virtual Video Camera"

#define PRL_VIRTUAL_BT_PATH		"VIRTUAL@BT@|203a|fff8|full|--|PW3.0"
#define PRL_VIRTUAL_BT_NAME		"Virtual Bluetooth Controller"

#define PRL_VIRTUAL_MSC_PATH0	"VIRTUAL@MSC"
#define PRL_VIRTUAL_MSC_PATH1	"203a|fff7|unknown|--"
#define PRL_VIRTUAL_MSC_NAME	"Virtual Disk"

// Needed by common code too
#define UDEV_HDD_BASE "/dev/disk/by-id"

#define HI_MAKE_UPDATE_DEVICE_MASK(dev_type)	(1LL << dev_type)
#define HI_UPDATE_ALL							(~(0LL))
#define HI_UPDATE_ALL_WITHOUT_USB				(~(1LL << PDE_USB_DEVICE))

/**
 * @brif This class help to create authentic friendly names for usb (bugfix for #439207)
 */
class CUsbAuthenticNameList
{
public:
	CUsbAuthenticNameList() {}
	QString LookupName(const QString &sSystemName, const QString &sFriendlyName, PRL_USB_DEVICE_TYPE &pudt);
	BOOL IsTheSameDevice(const QString &sys_name1, const QString &sys_name2) const;
	BOOL AddToBlackList(const QString &sys_name);
	BOOL CheckBlackList(const QString &sys_name) const;
	BOOL IsDevicesIdEqual(const QString &sys_name1, const QString &sys_name2) const;
	void SetUsbPreferences(const CDispUsbPreferences &_usb_prefs);
	const CDispUsbPreferences &GetUsbPreferences() const;

private:
	CDispUsbPreferences m_UsbPreferences;
};

/**
 * @brief This class implements Dispatcher's host environment info helper.
 * @author SergeyM
 */
class CDspHostInfo
{
	static const unsigned int VM_MAX_MEM_32BIT;

public:

	enum UpdateHardwareInfo
	{
		uhiFloppy			= 1 << PDE_FLOPPY_DISK,
		uhiCd				= 1 << PDE_OPTICAL_DISK,
		uhiHdd				= 1 << PDE_HARD_DISK,
		uhiNet				= 1 << PDE_GENERIC_NETWORK_ADAPTER,
		uhiSerial			= 1 << PDE_SERIAL_PORT,
		uhiParallel			= 1 << PDE_PARALLEL_PORT,
		uhiSound			= 1 << PDE_SOUND_DEVICE,
		uhiUsb				= 1 << PDE_USB_DEVICE,
		uhiPrinter			= 1 << PDE_PRINTER,
		uhiPci				= 1 << PDE_GENERIC_PCI_DEVICE,
		uhiScsi				= 1 << PDE_GENERIC_SCSI_DEVICE,
		uhiRasterizer		= 1 << PDE_MAX,
		uhiMemory			= 1 << (PDE_MAX + 1),
		uhiCpu				= 1 << (PDE_MAX + 2),
		uhiOsVersion		= 1 << (PDE_MAX + 3),
		uhiMobile			= 1 << (PDE_MAX + 4),

		HI_ALL_CONST_HW = uhiOsVersion | uhiMobile,
	};

	// special modes to make data in updateData() /refresh() calls.
	enum RefreshFlags{ rfNone = 0, rfShowPrimaryPciVga = 1};
	void setRefreshFlags( PRL_UINT32 nFlags );
	PRL_UINT32 getRefreshFlags(){ return m_nRefreshFlags; }

public:
	// Standard class constructor
	CDspHostInfo();

	// Class destructor
	~CDspHostInfo();

	// Get host hardware info
	CHostHardwareInfo* data() { return p_HostHwInfo; };

	// Return string representation of Os Version
	static QString GetOsVersionStringRepresentation();

	// Update memory settings
	void updateMemSettings();

	// Update networking adapters info
	void updateNetworkInfo();

	// Update host info
	void updateData(quint64 nFlags = HI_UPDATE_ALL);

	// Retrieve memory total
	static unsigned int getMemTotal();

	/*return maximum guest RAM possible*/
	static unsigned int getMaxVmMem();

	static BOOL IsProcessorSupportIA32e();

	/* Returns maximum number of displays in host */
	static unsigned int getMaxDisplays();

	// Refresh data, which may dynamically change
	void refresh(quint64 nFlags = HI_UPDATE_ALL_WITHOUT_USB);

	/* returns CPU virtualization technology features (SVM or VTx, see enum HvtFeaturesMask)*/
	static ULONG64 GetHvtFeatures();

	/* Returns CPU vendor ID: 0 - unknown, 1 - Intel, 2 - AMD.
	 * If pVendorName is not null, it will contain CPU vendor name (12 characters + null terminator)
	 */
	static UINT GetCpuVendor(UCHAR* pVendorName = NULL);

	/**
	 * Returns pointer to the USB authentic object
	 */
	CUsbAuthenticNameList *getUsbAuthentic();

	/* Lockup or create usb device */
	void LookupOrCreateUSBDevice(QList<CHwUsbDevice*> &old_dev_lst,
								QList<CHwUsbDevice*> &new_dev_lst,
								const QString &_sys_name, const QString &_frn_name,
								PRL_USB_DEVICE_TYPE pudt = PUDT_OTHER);

	/* Create system name for usb device */
	static QString CreateUsbSystemName( const QString &sLocationId,
								UINT uVID, UINT uPID,
								const QString &sDeviceSpeed,
								const QString &sSuffix,
								const QString &sSerialNumber);

	/* Check usb friendly name for readability */
	static bool CheckUsbFriendlyName(const QString &sFriendlyName);

	/* Create friendly name for usb device */
	static QString CreateUsbFriendlyName( UINT uClassID, UINT uVendorID, UINT uProductID );

	// used to decode sound system name
	static bool ParseSoundSystemName(bool is_input, const QString& sys_name,
									int* id, bool* is_default, bool* is_null);

	static QString GetDefaultSoundInputSystemName();
	static QString GetDefaultSoundOutputSystemName();

#ifdef _WIN_
	// Search for rasterizer path
	static bool GetRasterizerPath(QString& rastExec);
#endif

	static PRL_USB_DEVICE_TYPE UsbDEV2PUDT(UINT, UINT);
	static PRL_USB_DEVICE_TYPE UsbIFC2PUDT(UINT, UINT, UINT);

private:
	/* temporary directory listing list */
	QStringList m_strListTmp;

    /* directory filtering mask */
    char *dmask;

	// Host hardware info
	CHostHardwareInfo* p_HostHwInfo;

	PRL_UINT32 m_nRefreshFlags;

private:
    // Common constructor
    void CommonConstructor();

    // Common destructor
    void CommonDestructor();

	// Clean up class-specific properties
	void cleanupClassProperties();

    /* Detection Functions */

    /* build list of floppy devices */
    void GetFDDList();

    /* build list of hard disks (ide/scsi)*/
    void GetHDDList();

    /* build list of cdroms (ide/scsi)*/
    void GetCDList();

    /* build list of serial ports */
    void GetCOMList();

    /* build list of parallel ports */
    void GetLPTList();

    /* build list of network devices */
    void GetNETList();

    /* build list of USB devices */
    void GetUSBList();

    /* build list of Scsi devices */
    void GetScsiList();

    /* build list of printers*/
	void GetPrinterList();

	/*
	 * Switch old list to new one and clear old
	 */
	template <class T>
	void SafeUpdateList(QList<T*>& NewLst, QList<T*>& Curr)
	{
		QList<T*> Old = Curr;
		Curr = NewLst;

		while (!Old.isEmpty())
			delete Old.takeFirst();

		Old.clear();
	};

// PortAudioWrap commented out by request from CP team
//    /* build lists of sound devices, both: palyback and capture*/
//	void GetSoundLists();

#ifdef _WIN_
	// Search for rasterizer
	void GetRasterizer();
#endif

	/* update Os Version*/
	void GetOsVersion();

	// Fill OsVersion to  object and to string
	// Note: Setup NULL to pOsVer or pOutOsVersionAsString to skip they to fill.
	static void GetOsVersion( CHwOsVersion /*out*/ *pOsVer, QString* pOutOsVersionAsString=0 );

	/* update Cpu Info*/
	void GetCpu();

	/* update Cpu Info*/
	void GetCpuCommon();

	// Get is it a laptop or not
	void GetMobileDevice();

	/*
	 * Add specified HDD to map if it not exist, mark it processed and
	 * return is it in list or not
	 */
	bool CheckAddHdd(const QString& name);

	/*
	 * Drop unprocessed HDD from map (it means that they are removed)
	 */
	PRL_UINT32 DropUnprocessedHdd(QList<CHwHardDisk*>&);

	// Add specified HDD to the hardware list
	bool HddAddToList(const QString& friendlyName,
					const QString& systemName,
					PRL_UINT64 uiSize,
					PRL_UINT32 uiRate,
					bool Removable,
					bool External,
					QList<CHwHardDisk*>& List);

	// Returns "64" if the system can run 64-bit apps, "32" otherwise
	static QString getSystemBitness();

	// Returns kernel bitness ("32" or "64")
	static QString getKernelBitness();

	/* update platform specific memory info*/
	void getAdvancedMemoryInfo();

private:
	// Checks whether system can run 64-bit apps
	static bool Get64BitStatus();

#ifdef _LIN_
	void BuildCOMList(const char* Mask, QList<CHwGenericDevice*>& List);
	void GetHddDevList(QList<CHwHardDisk*>& List);
	void GetHddUdevList(QList<CHwHardDisk*>& List);
#endif

	/* Determine supported generic PCI device or not */
	void GenericPciDeviceSupport(CHwGenericPciDevice* pPciDevice);

#ifndef _WIN_
	/* function for build file list from directory */
	void BuildDirList(const char *dir, const char *mask = NULL);

	/* name filter callback */
	int NameFilter(char *name);
#endif // !_WIN_

	// Already added hard disks
	typedef std::map<QString, bool> AddedHDDMap;
	typedef std::pair<QString, bool> AddedHDDPair;
	AddedHDDMap m_HddMap;
	// List of authentic names for usb devices
	CUsbAuthenticNameList m_UsbAuthenticList;

// DiskImage commented out by request from CP team
//	// Convert to HardwareInfo
//	static PRL_RESULT ConvertToHardwareInfo(QList<CHwHddPartition*>& pHddParts,
//											const CPartition::InfoMap& Parts,
//											const QString& hddName);
	// Dump list of partitions
	static void DumpParts(int iLevel, QList<CHwHddPartition*>& lst);

	void GetDiskUsbAliases(QList<CHwUsbDevice*> &old_dev_lst, QList<CHwUsbDevice*> &new_dev_lst);

	bool isDiskUsbAliasNeeded(CHwHardDisk* disk);
};

#endif // __CDSPHOSTINFO_H__
