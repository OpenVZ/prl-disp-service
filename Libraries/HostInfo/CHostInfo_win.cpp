////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	CDspHostInfo.cpp
///
/// @brief
///	Implementation of the class CDspHostInfo
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
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QSettings>
#include <QList>

#include "CHostInfo.h"
#include <Libraries/VmDrv/VmDrv.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/HostHardwareInfo/CHwGenericPciDevice.h>
#include <prlxmlmodel/HostHardwareInfo/CHwHardDisk.h>
#include <prlxmlmodel/HostHardwareInfo/CHwNetAdapter.h>
#include <prlxmlmodel/HostHardwareInfo/CHwMemorySettings.h>
#include "Config.h"

#include <prlcommon/Interfaces/ParallelsPlatform.h>

#include <prlcommon/Logging/Logging.h>
#include "Libraries/VtdSetup/VtdSetup.h"

#include <prlcommon/HostUtils/SetupApiUtils_win.h>
#include <prlcommon/HostUtils/CfgmgrUtils_win.h>
#include <prlcommon/HostUtils/HostUtils.h>

#include <prlcommon/Std/PrlAssert.h>

// getenv() wrapper
#include <prlcommon/PrlCommonUtilsBase/Common.h>

// --- Windows specific ---
#include <InitGuid.h>
#include <devguid.h>
//#include <Devices/Usb/UsbHostDevInfo_Win.h>  // Devices/Usb commented out by request from CP team

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#define DRIVE_PATH_PATRN 			"%1:\\"
#define DRIVE_NAME_PATRN 			"%1: "
#define FLOPPY_DRIVE_FRIENDLY_PATRN			"Floppy Drive (%1:)"
#define DRIVE_SYSTEM_PATH_PATRN 			"\\\\.\\%1:"

const TCHAR REG_PARALL_ENUM[] = 	L"Hardware\\DeviceMap\\PARALLEL PORTS";

// Program files directories
static const QString s_PFDir("PROGRAMFILES");
static const QString s_PFDirX86("PROGRAMFILES(X86)");
// Ghostscript from sourceforge uses the next directory
static const QString s_gsDir("gs");

/**
 * @brief Default class constructor.
 * @return
 */
CDspHostInfo::CDspHostInfo()
{
	CommonConstructor();
} // CDspHostInfo::CDspHostInfo()


/**
 * @brief Class destructor.
 * @return
 */
CDspHostInfo::~CDspHostInfo()
{
    CommonDestructor();
} // CDspHostInfo::CDspHostInfo()


// build list of floppy devices
void CDspHostInfo::GetFDDList()
{
	if (p_HostHwInfo->m_lstFloppyDisks.isEmpty())
	{
		for (char c = 'A'; c <= 'Z'; c++)
		{
			QString	pszDrivePath = QString( DRIVE_PATH_PATRN ).arg( c );
			if (GetDriveType( QSTR2UTF16(pszDrivePath) ) == DRIVE_REMOVABLE)
			{
				QString	strDrive	= QString(DRIVE_SYSTEM_PATH_PATRN ).arg( c );
				HANDLE	hDrive		= CreateFile(QSTR2UTF16(strDrive),
												0,
												FILE_SHARE_READ | FILE_SHARE_WRITE,
												NULL,
												OPEN_EXISTING,
												0,
												NULL);
				if (hDrive != INVALID_HANDLE_VALUE)
				{
					DWORD dwAll, dwReturn = 0;
					DISK_GEOMETRY SupportedGeometry[20];

					if (DeviceIoControl(hDrive,
										IOCTL_DISK_GET_MEDIA_TYPES,
										NULL, 0,
										SupportedGeometry, sizeof(SupportedGeometry),
										&dwReturn,
										NULL))
					{
						dwAll = dwReturn / sizeof(DISK_GEOMETRY);
						for (DWORD i = 0; i < dwAll; i++)
						{
							if (SupportedGeometry[i].MediaType == F3_1Pt44_512)
							{
								QString strFriendlyName	= QString( FLOPPY_DRIVE_FRIENDLY_PATRN ).arg( c );
								QString strSystemName	= QString( DRIVE_SYSTEM_PATH_PATRN ).arg( c );

								p_HostHwInfo->addFloppyDisk(
									new CHwGenericDevice( PDE_FLOPPY_DISK,
										strFriendlyName,
										strSystemName ) );

								break;
							}
						}
					}
					//
					CloseHandle(hDrive);
				}
			}
		}
	}
} // CDspHostInfo::GetFDDList()

// build list of hard disks (ide/scsi)
void CDspHostInfo::GetHDDList()
{
	QString strHddFriendlyName;
	QString strHddDeviceName;
#if defined(ONLY_SCSI_DISKS)
	QString strEnumeratorName;
#endif

	m_HddMap.clear();
	QList<CHwHardDisk*> Disks;

	for ( InterfaceInfo iterDevice(&GUID_DEVINTERFACE_DISK); iterDevice; iterDevice++ )
	{
#if defined(ONLY_SCSI_DISKS)
		strEnumeratorName = iterDevice.GetRegistryPropertyString(SPDRP_ENUMERATOR_NAME);

		if (strEnumeratorName != QString("SCSI"))
			continue;
#endif

#if 0
		// Debug code, do not touch it please!
		for(int i = 0; i < SPDRP_MAXIMUM_PROPERTY; i++)
		{
			QString tmpStr = QString("Property 0x%1 String [%2]")
								.arg(i, 0, 16)
								.arg(iterDevice.GetRegistryPropertyString(i));

			WRITE_TRACE(DBG_FATAL, "Disk property %u is %s", i, tmpStr.toUtf8().data() );
		}
#endif

		strHddFriendlyName = iterDevice.GetFriendlyName();

		// Original logic - do not add devices without name
		if (strHddFriendlyName.isEmpty())
			continue;

		strHddDeviceName = iterDevice.GetSystemName();
		strHddFriendlyName += QString(" (%1)").arg(
			iterDevice.GetRegistryPropertyString(SPDRP_PHYSICAL_DEVICE_OBJECT_NAME));

		/*
		 * We can't use names like MyHDD(\Device0\Drive1) in case of backslashes.
		 * Replace them with #
		 */
		strHddFriendlyName.replace(QChar('\\'), QChar('.'));
		strHddFriendlyName.replace(QChar('/'), QChar('.'));

		if (CheckAddHdd(strHddDeviceName))
			continue;

		unsigned int rate;
		if (PRL_FAILED(HostUtils::GetAtaDriveRate(strHddDeviceName, rate)))
			rate=0;
		HddAddToList(strHddFriendlyName, strHddDeviceName, 0, rate,	false, false, Disks);
	}

	// Cleanup list
	DropUnprocessedHdd(Disks);
	SafeUpdateList<CHwHardDisk>(Disks, p_HostHwInfo->m_lstHardDisks);
} // CDspHostInfo::GetHDDList()


// build list of cdroms (ide/scsi)
void CDspHostInfo::GetCDList()
{
	/**
	* This code detects host cdrom drives by enumerating driver letters,
	* instead of numerating devices.
	* Device with this name opens at high level cdrom driver, that counts
	* referenses as all user proccesses accessed to this drive
	* In this case we can open device in exclusive access
	* If we open device through low level device name, we can not obtain
	* exclusive access, because there is two different names that open by different drivers
	* Another advantage of this code is determining drive letter, that is usefull for users
	* And another reson of this code is avoiding problems with windows automounter.
	* For some reason if we open device throug low level driver we will occure with
	* at least two peroblems while media is changing. We can not get access to new
	* media until it will accessed throug high level driver (by accessing through
	* windows explorer etc.)
	*
	*/

	for (char c = 'A'; c <= 'Z'; c++)
	{
		QString strDrivePath = QString( DRIVE_PATH_PATRN ).arg( c );
		QString strDriveLetter = QString( DRIVE_NAME_PATRN ).arg( c );
		QString strDevSysName = QString( DRIVE_SYSTEM_PATH_PATRN ).arg( c );

		if (GetDriveType( (const wchar_t*)strDrivePath.utf16() ) == DRIVE_CDROM)
		{
			p_HostHwInfo->addOpticalDisk( new CHwGenericDevice( PDE_OPTICAL_DISK,
																strDriveLetter,
																strDevSysName ) );
		}
	}
} // CDspHostInfo::GetCDList()


// build list of serial ports
void CDspHostInfo::GetCOMList()
{
	if (p_HostHwInfo->m_lstSerialPorts.isEmpty())
	{
		for (InterfaceInfo iterDevice(&GUID_DEVINTERFACE_COMPORT); iterDevice; iterDevice++)
		{
			p_HostHwInfo->addSerialPort(
				new CHwGenericDevice(PDE_SERIAL_PORT,
									iterDevice.GetFriendlyName(),
									iterDevice.GetSystemName()));
		}
	}
} // CDspHostInfo::GetCOMList()


// build list of parallel ports
void CDspHostInfo::GetLPTList()
{
	static const DWORD ValSize = 255;
	HKEY	hKey;
	wchar_t	caDevName[ValSize];
	wchar_t caPortName[ValSize];
	DWORD	dwDevNameSize, dwPortNameSize, dwType;
	DWORD	dwError = ERROR_SUCCESS;
	UINT	uiIndex = 0;
	UINT	uFoundPorts = 0;
	QString strPortName;
	QString strPortFriendlyName;

	if( !p_HostHwInfo->m_lstParallelPorts.isEmpty() )
		return;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PARALL_ENUM, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return;

	do
	{
		// Size in TCHAR required, i.e. symbols count
		dwDevNameSize = ValSize;
		dwPortNameSize = ValSize * sizeof(wchar_t);

		dwError = RegEnumValueW(hKey,
			uiIndex++,
			caDevName,
			&dwDevNameSize,
			NULL,
			&dwType,
			(LPBYTE)caPortName,
			&dwPortNameSize);

		if (dwError != ERROR_SUCCESS)
			continue;

		strPortName = UTF16_2QSTR(caPortName);

		//is this a daisy chain device registry entry?
		if (strPortName.contains('.'))
			continue;

		//if not - found a physical parallel port
		++ uFoundPorts;
		strPortFriendlyName = QString( "Printer Port %1 (%2)" )
									.arg(uFoundPorts)
									.arg(strPortName.mid( strPortName.lastIndexOf( QString("LPT") ), 4 ));

		p_HostHwInfo->addParallelPort(
				new CHwGenericDevice( PDE_PARALLEL_PORT,
							strPortFriendlyName,
							strPortName ));
	} while (dwError != ERROR_NO_MORE_ITEMS);

	RegCloseKey(hKey);

	/*
	if (p_HostHwInfo->m_lstParallelPorts.isEmpty())
	{
		for (IntefaceInfo iterDevice(&GUID_PARCLASS_DEVICE); iterDevice; iterDevice++)
		{
			p_HostHwInfo->addParallelPort(
				new CHwGenericDevice(PDE_PARALLEL_PORT,
									iterDevice.GetFriendlyName(),
									iterDevice.GetSystemName()));
		}
	}
	*/

} // CDspHostInfo::GetLPTList()

// build list of USB devices
void CDspHostInfo::GetUSBList()
{
	// Create list of usb keybord/mouse
	QStringList lstUsbKeybordMouse;
	for (DeviceInfo iterHid(&GUID_DEVCLASS_HIDCLASS); iterHid; iterHid++)
	{
		QString sCompatibleIDs = iterHid.GetRegistryPropertyString(SPDRP_COMPATIBLEIDS);
		// We interested only in usb hid devices which support keybord or mouse protocol
		if (sCompatibleIDs.isEmpty() ||
			!sCompatibleIDs.contains("usb\\class_03&subclass_01&prot_01", Qt::CaseInsensitive) &&
			!sCompatibleIDs.contains("usb\\class_03&subclass_01&prot_02", Qt::CaseInsensitive))
			continue;

		QString sHardwareID	= iterHid.GetRegistryPropertyString(SPDRP_HARDWAREID);
		if (sHardwareID.isEmpty())
			continue;

		lstUsbKeybordMouse.append(sHardwareID);
	}
	// Copy current list to old_dev_lst and clear it
	QList<CHwUsbDevice*> old_dev_lst(p_HostHwInfo->m_lstUsbDevices);
	p_HostHwInfo->m_lstUsbDevices.clear();
	QString strHubName;
	// Enumerate through all USB hub devices in set.
	for ( InterfaceInfo iterDevice(&GUID_DEVINTERFACE_USB_HUB); iterDevice; iterDevice++ )
	{
		/*
		* Dev id == USB\ROOT_HUB20\4&1ECEB5E9&0D&REV00A3
		* CM_DRP_HARDWAREID == USB\ROOT_HUB20&VID10DE&PID026E&REV00A3
		* call QString strHwId = iterDevCfg.GetRegistryPropertyString(CM_DRP_HARDWAREID) to get it
		* Get HUB identification string
		* Also we can get hub ID through CM_Get_DevNode_Registry_Property() with CM_DRP_HARDWAREID flag
		*/
		strHubName = iterDevice.GetSystemName();

		for (DeviceCfg iterDevCfg(iterDevice.GetDeviceInstance()); iterDevCfg; iterDevCfg++)
		{
			ULONG	addr = ~0;
			if (!iterDevCfg.GetRegistryPropertyUlong(CM_DRP_ADDRESS, addr)) {
				WRITE_TRACE(DBG_FATAL, "[HOSTINFO] Error: can not detect port for device");
				continue;
			}

			CUsbHostDevInfo UsbDev(strHubName, addr);
			if (!UsbDev.isValid())
				continue;

			ULONG vid	= UsbDev.GetDeviceVendor();
			ULONG pid	= UsbDev.GetDeviceProduct();
			UINT8 deviceClass	= UsbDev.GetDeviceClass();
			QString sSuffix("--");

			// Check if device is served by parallels driver
			QString sDevVidPid = iterDevCfg.GetDeviceId().mid(4, 17);
			if (sDevVidPid.contains("vid_fffe&pid_0001", Qt::CaseInsensitive)) {
				// Check is device started
				if (isUsbDeviceStared(iterDevCfg.GetRegistryPropertyString(CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME)))
					sSuffix = "PR"; // device serviced by prl_usb_dev and started
				else
					sSuffix = "PW";	// device serviced by prl_usb_dev but not started
			} else {
				// Check if device's vid&pid contained in usb keyboard/mouse list
				foreach (QString sTmpId, lstUsbKeybordMouse)
					if (sTmpId.contains(sDevVidPid, Qt::CaseInsensitive)) {
						sSuffix = "KM";
						break;
					}
			}

			// Get device's bus speed
			QString sDevSpeed;
			switch (UsbDev.GetDeviceSpeed()) {
				case  0 : sDevSpeed = USB_SPD_LOW;	break;
				case  1 : sDevSpeed = USB_SPD_FULL;	break;
				case  2 : sDevSpeed = USB_SPD_HIGH; break;
				default : sDevSpeed = USB_SPD_UNKNOWN;
			}

			// Get device's serial number
			QString sNumber(USB_NUM_EMPTY);

			// Make a platform dependent system name. All this information should be stored as system name,
			// but with different separators used on cross platform code
			QString strSysSeparator('@'); // See UsbHostDevInfo_Win.h
			QString sSystemName =
					strHubName				+
					strSysSeparator			+
					QString::number(addr)	+
					strSysSeparator;	// Do not forget last separator, otherwise
										// PDO name will contain whole string with additional info!
			sSystemName = CreateUsbSystemName(sSystemName, vid, pid, sDevSpeed, sSuffix, sNumber);

			// Get device's friendly name logic:
			//  1. Get name from registry (CM_DRP_FRIENDLYNAME).
			//  2. If name not valid -> get vendor & device string desriptor from device.
			//  3. If name not valid -> get name from registry (CM_DRP_DEVICEDESC)
			//  4. If name not valid -> get name from our list
			QString strSpace(" ");
			QString sFriendlyName = iterDevCfg.GetRegistryPropertyString(CM_DRP_FRIENDLYNAME);
			if (CheckUsbFriendlyName(sFriendlyName))
				goto create_device_object;

			if (!UsbDev.GetManufacturerString().isEmpty())
				sFriendlyName = UsbDev.GetManufacturerString().trimmed() + strSpace;
			if (!UsbDev.GetProductString().isEmpty())
				sFriendlyName += UsbDev.GetProductString().trimmed();
			if (CheckUsbFriendlyName(sFriendlyName))
				goto create_device_object;

			sFriendlyName = iterDevCfg.GetRegistryPropertyString(CM_DRP_DEVICEDESC).trimmed();
			if (CheckUsbFriendlyName(sFriendlyName))
				goto create_device_object;

			sFriendlyName = CreateUsbFriendlyName(deviceClass, vid, pid).trimmed();

create_device_object:
			// Loockup or create device element, and append it to new list
			LookupOrCreateUSBDevice(old_dev_lst, p_HostHwInfo->m_lstUsbDevices, sSystemName, sFriendlyName);
		}
	}

	GetDiskUsbAliases(old_dev_lst, p_HostHwInfo->m_lstUsbDevices);

	// Clear list of disconnected devices & release memory used for CHwGenericDevice
	foreach(CHwUsbDevice *old_dev,  old_dev_lst) {
		old_dev_lst.removeAll(old_dev);
		delete old_dev;
	}
} // CDspHostInfo::GetUSBList()


/**
* Creates device name by replacing "\" with "#", adding prefix "\\?\" and class guid
*
* @author mihailm
*
* @return string containing generated name
*/
QString MakeDosDeviceName(const QString& sDeviceId, const QUuid& guid)
{
    QString sIdLow = sDeviceId.toLower();
    QString sDosName("\\\\?\\");
    sDosName += sIdLow.replace(QString("\\"),QString("#"),Qt::CaseInsensitive);
    sDosName += QString("#")+guid.toString();
    return sDosName;
}

/**
* builds a list of Scsi devices
*
* @author mihailm
*
* @return none
*/
typedef  QMap<QString, QUuid> WinGuids;
void CDspHostInfo::GetScsiList()
{
	if (!p_HostHwInfo->m_lstGenericScsiDevices.isEmpty())
		return;

	/**
	* Here some unclear actions.
	* It seems it is impossible to get a sybolic link to device from DosDevices section
	* And we can not get proper device class GUID, that used at creation of this symlink
	* So, we creates it itself.
	* At first we get device ID - it is a path to device
	* At second, we translate it to symbolic link notations (i.e. replacing "\" with "#")
	* and add prefix "\\?\" to it.
	* At thirds, we concatenate it with device class GUID, that we gets from DDK headers
	* To choose appropriate GUID we get device CLASS,
	* and take GUID from DDK that matching to this class.
	*
	*/
	WinGuids  GuidsList;
	GuidsList.insert(QString("DiskDrive"),(QUuid)DiskClassGuid);
	GuidsList.insert(QString("Volume"),(QUuid)VolumeClassGuid);
	GuidsList.insert(QString("CDROM"),(QUuid)CdRomClassGuid);
	GuidsList.insert(QString("TapeDrive"),(QUuid)TapeClassGuid);

	/*
	// Add another GUIDs if necessary
	GuidsList.insert(QString(""),(QUuid)PartitionClassGuid);
	GuidsList.insert(QString(""),(QUuid)WriteOnceDiskClassGuid);
	GuidsList.insert(QString(""),(QUuid)MediumChangerClassGuid);
	GuidsList.insert(QString(""),(QUuid)FloppyClassGuid);
	GuidsList.insert(QString(""),(QUuid)CdChangerClassGuid);
	GuidsList.insert(QString(""),(QUuid)StoragePortClassGuid);
	*/

	/**
	* Here we filter founded devices, because we do not include optical drives
	* and any kinds of hard disks. They are included into OpticalDisk and HardDisk lists
	* Also we eliminate "System" class
	*/
	QString sClassDisk("DiskDrive");
	QString sClassVolume("Volume");
	QString sClassCdrom("CDROM");
	QString sClassSystem("System");

	QString sClassName;
	QString sSystemName;
	QString sFriendlyName;

	QUuid FoundUid;

	// Enumerate through all SCSI adapters
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	hDevInfo = SetupDiGetClassDevs((LPGUID) &GUID_DEVCLASS_SCSIADAPTER,
									0,
									0,
									DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
		return;
	/**
	* Enumerate through all SCSI Adapter devices in set.
	*/
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (int i=0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
	{

		for (DeviceCfg iterDevCfg(DeviceInfoData.DevInst); iterDevCfg; iterDevCfg++) {
			sClassName = iterDevCfg.GetRegistryPropertyString(CM_DRP_CLASS);
			/**
			 * Ok. We have got device class, so we need to skip
			 * DiskDrive, Volume and CDROM classes
			 * But after CM_Get_DevNode_Registry_Property was called
			 * BufferLen == length of a returned buffer,
			 * set it again at initial value
			 */
#if 1
			if(	(sClassName == sClassDisk) || (sClassName == sClassCdrom) ||
				(sClassName == sClassVolume) || (sClassName == sClassSystem))
				continue;
#else
			Q_UNUSED(sClassDisk);
			Q_UNUSED(sClassCdrom);
			Q_UNUSED(sClassVolume);
			Q_UNUSED(sClassSystem);
#endif
			sFriendlyName = iterDevCfg.GetRegistryPropertyString(CM_DRP_FRIENDLYNAME);

			if (sFriendlyName.isEmpty()) {
				/**
				* This device has no friendly name, so we try to get
				* its device description
				*/
				sFriendlyName = iterDevCfg.GetRegistryPropertyString(CM_DRP_DEVICEDESC);
			}

			// Get appropriate GUID for a known class
			FoundUid = GuidsList.value(sClassName);

			// Generate device name from device ID and GUID
			sSystemName = MakeDosDeviceName( iterDevCfg.GetDeviceId(), FoundUid);

			if (sSystemName.isEmpty())
				continue;

			p_HostHwInfo->addGenericScsiDevice(	new CHwGenericDevice(
													PDE_GENERIC_SCSI_DEVICE,
													sFriendlyName,
													sSystemName)
												);
		}
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
} // CDspHostInfo::GetScsiList()

// Check rasterizer directory existance
static QString GetRastDir(const QString& env)
{
	QString pf = Prl::getenvU(env);

	if (pf.isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Error getting directory specified by %s",
					qPrintable(env));
		return QString();
	}

	pf += QString("\\%1").arg(s_gsDir);

	if (QFile::exists(pf))
		return pf;

	return QString();
}

// Check and add rasterizer to list
static void CheckAddRastDir(QStringList& lst, const QString& pfdir)
{
	QString rastDir;

	rastDir = GetRastDir(pfdir);

	if (!rastDir.isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Rasterizer dir found at %s",
					qPrintable(rastDir));
		lst.append(rastDir);
	}
}

// Get rasterizer information
void CDspHostInfo::GetRasterizer()
{
	QString rastExec;

	if (!GetRasterizerPath(rastExec))
		return;

	// Get information from rasterizer itself
	QProcess rsProc;

	// Get version
	rsProc.start(rastExec + QString(" -v"));
	rsProc.waitForFinished();

	// Normal exit code for known rasterizer with -v is 0
	if (rsProc.exitCode() != 0)
	{
		WRITE_TRACE(DBG_FATAL, "Can't get version of rasterizer at %s",
					qPrintable(rastExec));
		return;
	}

	CPSRasterizer* pRS = new (std::nothrow) CPSRasterizer;

	if (!pRS)
	{
		WRITE_TRACE(DBG_FATAL, "Error allocating memory for rasterizer object");
		return;
	}

	QString tmp = QString::fromAscii(rsProc.readAllStandardOutput());
	WRITE_TRACE(DBG_FATAL, "Rasterizer own info: %s", qPrintable(tmp));

	pRS->setDescription(tmp);
	pRS->setExecutable(rastExec);

	p_HostHwInfo->setPSRasterizer(pRS);
}

// Search for rasterizer path
bool CDspHostInfo::GetRasterizerPath(QString& rastExec)
{
	// No difference in name for x86 and x64 version
	static const QString gsBin("\\bin\\gswin32c.exe");
	QRegExp gsVer("gs(\\d+)\\.(\\d+)");
	// List of main directories, where the rasterizer present
	QStringList rastLst;
	QString tmp;
	PRL_UINT32 Major = 0, curMajor;
	PRL_UINT32 Minor = 0, curMinor;

	CheckAddRastDir(rastLst, s_PFDir);

	// Check x86 dir also.
	if (Get64BitStatus())
		CheckAddRastDir(rastLst, s_PFDirX86);

	// No ay dir found. May be nothing installed.
	if (rastLst.isEmpty())
		return false;

	// Search for the executables
	QStringList::const_iterator it;

	for (it = rastLst.constBegin(); it != rastLst.constEnd(); it++)
	{
		QDirIterator dit(*it);

		while (dit.hasNext())
		{
			bool ok1, ok2;

			/*
			 * After construction, the iterator is located
			 * before the first directory entry.
			 */
			dit.next();

			// Not a directory? Try next one
			if (!dit.fileInfo().isDir())
				continue;

			if (gsVer.indexIn(dit.fileName()) == -1)
				continue;

			curMajor = gsVer.cap(1).toUInt(&ok1);
			curMinor = gsVer.cap(2).toUInt(&ok2);

			if (!ok1 || !ok2)
			{
				WRITE_TRACE(DBG_FATAL, "Error converting rasterizer version string %s and %s to ints",
							qPrintable(gsVer.cap(1)),
							qPrintable(gsVer.cap(2)));
				continue;
			}

			if (curMajor < Major)
				continue;

			if ((curMajor == Major) && curMinor < Minor)
				continue;

			tmp = dit.filePath() + gsBin;

			// Newer version, check binaries presence
			if (!QFile::exists(tmp))
			{
				WRITE_TRACE(DBG_FATAL, "Found rasterizer dir without executable %s",
							qPrintable(dit.filePath()));
				continue;
			}

			// Ok, all nice, store
			rastExec = tmp;
			Major = curMajor;
			Minor = curMinor;
		}
	}

	// It's enough to check that
	if (!Major)
		return false;

	// Long path with spaces should be quoted
	rastExec = QString("\"%1\"").arg(rastExec);

	WRITE_TRACE(DBG_FATAL, "Rasterizer found at %s", qPrintable(rastExec));
	WRITE_TRACE(DBG_FATAL, "Rasterizer version %u:%u", Major, Minor);

	return true;
}

// build list of printers
void CDspHostInfo::GetPrinterList()
{
	// List of printers controllers
	QList<CHwPrinter* > Printers;

	const DWORD		flags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
	const DWORD		level = 4;
	PRINTER_INFO_4	*Info = NULL;
	DWORD			sz = 0;
	DWORD			cnt = 0;
	BOOL			res = FALSE;

	for (DWORD attempt=0; attempt<4; attempt++) {

		res = EnumPrinters(flags, NULL, level, (LPBYTE)Info, sz, &sz, &cnt);
		if (res)
			break;

		/*
		* "sz" cointains actual required buffer size
		* realloc in nessesary
		*/
		if (Info)
			free(Info);

		const DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
				WRITE_TRACE(DBG_FATAL, "Printers enumeration failed: %d",err);
				return;
		}

		Info = (PRINTER_INFO_4*)malloc(sz);
		if (NULL == Info) {
			WRITE_TRACE(DBG_FATAL, "ENOMEM for printer device props");
			return;
		}
	}

	if (!res || !cnt) {
		WRITE_TRACE(DBG_FATAL, "ENOENT for printers %d, %d", res, cnt);
		return;
	}

	// storing printers names
	for (DWORD i = 0; i < cnt; i++) {
		CHwPrinter* pDev = new(std::nothrow) CHwPrinter;
		// Currently memory allocation failed. May be we be lucky next time.
		if (!pDev) {
			WRITE_TRACE(DBG_FATAL, "ENOMEM for printer device");
			continue;
		}

		const QString name = UTF16_2QSTR(Info[i].pPrinterName);
		pDev->setDeviceName(name);
		pDev->setDeviceId(name);

		Printers.append(pDev);
	}

	// Switch
	SafeUpdateList<CHwPrinter>(Printers, p_HostHwInfo->m_lstPrinters);
	free(Info);
} // CDspHostInfo::GetPrinterList()


/**
 * @brief get Os Version.
 * @return
 */
void CDspHostInfo::GetOsVersion()
{
	CHwOsVersion* pOsVer = p_HostHwInfo->getOsVersion();
	if ( !pOsVer )
	{
		WRITE_TRACE(DBG_FATAL, "CDspHostInfo::GetOsVersion() : CHwOsVersion is NULL!" );
		return;
	}

	GetOsVersion( pOsVer, 0 );
}

static QString GetOsVersionAsString(const OSVERSIONINFO& os);

void CDspHostInfo::GetOsVersion( CHwOsVersion /*out*/ *pOs, QString* pOutOsVersionAsString )
{
	PRL_ASSERT( pOs || pOutOsVersionAsString );
	if ( !pOs && !pOutOsVersionAsString )
		return;

	OSVERSIONINFO os;
	memset( &os, 0, sizeof( OSVERSIONINFO ) );
	os.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

	if ( !GetVersionEx(&os) )
	{
		WRITE_TRACE(DBG_FATAL, "CDspHostInfo::GetOsVersion() : GetVersionEx() failed. Err=%d", GetLastError() );
		return;
	}

	QString sVersionAsString = GetOsVersionAsString(os);

	if( pOutOsVersionAsString )
		*pOutOsVersionAsString = sVersionAsString;

	if( !pOs )
		return;

	pOs->setStringPresentation( sVersionAsString );
	pOs->setOsType( PHO_WIN );
	pOs->setMajor( os.dwMajorVersion );
	pOs->setMinor( os.dwMinorVersion );
	pOs->setSubMinor( os.dwBuildNumber );
	pOs->setOsArchitecture( getSystemBitness() );
	pOs->setKernelArchitecture( getKernelBitness() );

}

static QString GetOsVersionAsString(const OSVERSIONINFO& os)
{
	QString qsOs = "Unknown";
	// Test for the Windows NT product family.
	if  ( os.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		// Test for the specific product.
		QString qsOsVerKey = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
		QSettings qsOsVerSettings ( qsOsVerKey, QSettings::NativeFormat );
		qsOs = qsOsVerSettings.value("ProductName").toString();
	}

	QString sVersionAsString = "Windows Unknown OS";
	if ( qsOs.contains("Unknown") )
		WRITE_TRACE(DBG_FATAL, "Windows Unknown OS." );
	else
		sVersionAsString =  QString("%1 Version %2.%3.%4").
			arg(qsOs).
			arg(os.dwMajorVersion).
			arg(os.dwMinorVersion).
			arg(os.dwBuildNumber);

	return sVersionAsString;
}

/**
 * @brief get Cpu Info.
 * @return
 */
void CDspHostInfo::GetCpu()
{
	SYSTEM_INFO sys;
	memset( &sys, 0, sizeof(SYSTEM_INFO) );
	GetSystemInfo( &sys );

	// FIXME: Need try to get cpuMode from registry

	PRL_CPU_MODE cpuMode;

	if (IsProcessorSupportIA32e())
	{
		cpuMode = PCM_CPU_MODE_64;
	}
	else
	{
		cpuMode = PCM_CPU_MODE_32;
	}

	QString cpuKey = "HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor";
	QSettings cpuSettings ( cpuKey, QSettings::NativeFormat );
	QStringList	lstCpu = cpuSettings.childGroups();
	int cpuCount = lstCpu.size();
	if 	( cpuCount == 0 )
	{
		WRITE_TRACE(DBG_FATAL, "CDspHostInfo::GetCpu() : There is returned 0 processors!" );
		return;
	}

	cpuSettings.beginReadArray( lstCpu[0] );

	CHwCpu* pCpu = p_HostHwInfo->getCpu();
	if ( pCpu )
	{
		pCpu->setNumber( cpuCount );
		pCpu->setModel( cpuSettings.value( "ProcessorNameString" ).toString().simplified() );
		pCpu->setMode( cpuMode );
		pCpu->setSpeed( cpuSettings.value( "~MHz" ).toInt() );
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "CDspHostInfo::GetCpu() : CHwCpu is NULL!" );
		return;
	}
}


/**
 * @brief Retrieve memory total.
 * @return host total physical memory in megabytes
 */
unsigned int CDspHostInfo::getMemTotal()
{
	// Host memory total
	unsigned int uiMemTotal = 256;
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx( &statex ))
		uiMemTotal = statex.ullTotalPhys / 1024 / 1024;
	else
		LOG_MESSAGE( DBG_FATAL, "GlobalMemoryStatusEx failed" );

	return uiMemTotal;
} // CDspHostInfo::getMemTotal()

/*
 * Determine is computer supports power capabilities
 */
void CDspHostInfo::GetMobileDevice()
{
    // Undeterminable for windows
	p_HostHwInfo->setHostNotebookFlag( false );
}

/*
 Returns maximum number of displays in host.
 Returns 0 if error occurred.
 Note that displays in session of calling process are counted only.
 */
unsigned int CDspHostInfo::getMaxDisplays()
{
	DISPLAY_DEVICEW dspDev;
	DWORD iDevNum = 0;
	unsigned int dspCount = 0;
	INT displayIndex = 0;
	WCHAR endLineSymb = 0;

	dspDev.cb = sizeof(dspDev);
	while (EnumDisplayDevicesW(NULL, iDevNum++, &dspDev, 0) )
	{
		if (!(dspDev.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) &&
			(swscanf_s(dspDev.DeviceName, L"\\\\.\\DISPLAY%d%c",
				&displayIndex, &endLineSymb, sizeof(endLineSymb)) == 1))
			dspCount++;
		dspDev.cb = sizeof(dspDev);
	}

	int systemDspCount = GetSystemMetrics(SM_CMONITORS);

	if (dspCount == 0 || (unsigned int)systemDspCount > dspCount)
		return (unsigned int)systemDspCount;
	return dspCount;
}

void CDspHostInfo::getAdvancedMemoryInfo()
{
	//TODO fill advincedmemoryinfo here for windows
}
