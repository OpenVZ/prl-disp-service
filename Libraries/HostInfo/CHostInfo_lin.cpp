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

#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/soundcard.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/hdreg.h>
#include <linux/cdrom.h>
#include <linux/fd.h>
#include <linux/lp.h>
#include <linux/serial.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include <sys/vfs.h>

#include <dlfcn.h>

#include "Libraries/Virtuozzo/CVzHelper.h"
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include <prlcommon/Std/PrlAssert.h>

/* base proc/dev filesystems path */
#define PROC_BASE "/proc/"
#define DEV_BASE "/dev/"

/* base path for ide devices */
#define IDE_PATH   "ide/"
#define IDE_MEDIA  "/media"
#define IDE_DRIVER "/driver"

/* base path for serial ports */
#define COM_SERIAL "tty/driver/serial"

/* base path for lpt */
#define LPT_PARPORT "sys/dev/parport/"

/* sound devices defaults */
#if defined(_BSD_)
#define DEFAULT_DSP_NAME	"dspW"
#endif

#define DEFAULT_DSP_NAME	"dsp"

#define DEFAULT_MIXER_NAME	"mixer"

// --- end of Linux specific ---


#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QString>
#include "CHostInfo.h"
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/HostHardwareInfo/CHwGenericPciDevice.h>
#include <prlxmlmodel/HostHardwareInfo/CHwHardDisk.h>
#include <prlxmlmodel/HostHardwareInfo/CHwMemorySettings.h>
#include "Config.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

/**
 * function that reads scsi inquiry data from a given device
 * @author mihailm@
 *
 * @param strDevName		- device letter in device name format (i.e. "\\\\.\X:")
 * @param pInquiryBuffer	- buffer for data
 * @param usSize			- Size of a data
 *
 * @return 0 success, -1 otherwise
 */
static int InquirySCSIDevice(const QString& strDevName, quint8* pInquiryBuffer, quint16 usSize)
{
	// Open device for inquiry command
	int fd = open(strDevName.toUtf8().constData(),O_NONBLOCK);

	if (fd < 0)
		return -1;

	/*
	* prepare structure of scatter-gather IO to device
	*/
	sg_io_hdr_t io_hdr;
	quint8 cmd[6];

	// create CDB command
	cmd[0] = INQUIRY;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x00;
	cmd[4] = 0x24; // transfer length
	cmd[5] = 0x00;

	// Initialize request structure
	memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
	io_hdr.interface_id = 'S';
	io_hdr.cmd_len = 6;
	io_hdr.mx_sb_len = 0;
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.dxfer_len = usSize;
	io_hdr.dxferp = pInquiryBuffer;
	io_hdr.cmdp = &cmd[0];
	// we don't interested in sense info
	io_hdr.sbp = NULL;
	io_hdr.timeout = 20000;

	// Send command to device
	int ret = ioctl(fd, SG_IO, &io_hdr);
	close(fd);

	return ret;
}

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
	QStringList::iterator p;
	QString ptr;
	int fd;

	struct floppy_drive_struct fds;


	if( !p_HostHwInfo->m_lstFloppyDisks.isEmpty() )
		return;

	BuildDirList(DEV_BASE, "fd?");
	p = m_strListTmp.begin();
	while(p != m_strListTmp.end())
	{
		ptr.truncate(0);
		ptr = DEV_BASE;
		ptr += *p;
		fd = open(QFLN2CH(ptr), O_RDONLY | O_NONBLOCK);
		if (fd >= 0)
		{
			memset(&fds, 0, sizeof(fds));
			if (ioctl(fd, FDGETDRVSTAT, &fds, sizeof(fds)) != -1)
			{
				if (!(fds.flags == FD_VERIFY))
				{

					p_HostHwInfo->addFloppyDisk(
						new CHwGenericDevice( PDE_FLOPPY_DISK, ptr, ptr ) );

				}
			}
			else
			{
				break;
			}

			close(fd);
		}
		else
		{
			break;
		}
		p++;
	}

} // CDspHostInfo::GetFDDList()

/*
 * Check file content.
 */
static bool CheckFileContent(const char *file, const char *cont)
{
	if (!file || !cont)
		return false;

	FILE *fd = fopen(file, "r");
	if (!fd)
		return false;

	char buff[1024];
	char *ret = fgets(buff, sizeof(buff) - 1, fd);

	fclose(fd);

	if (ret == NULL)
		return false;

	if (!strncmp(buff, cont, strlen(cont)))
		return true;

	return false;
}

/*
 * Get ide device path in /dev/
 */
static QString GetIdeDev(const QString& Base)
{
	QString Str = QString(PROC_BASE IDE_PATH "%1" IDE_MEDIA).arg(Base);

	if (CheckFileContent(QFLN2CH(Base), "disk"))
		goto Exit;

	Str = QString(PROC_BASE IDE_PATH "%1" IDE_DRIVER).arg(Base);

	if (CheckFileContent(QFLN2CH(Base), "ide-disk"))
		goto Exit;

	return QString();
Exit:
	return QString(DEV_BASE "%1").arg(Base);
}

/*
 * Get HDD friendly name
 */
static QString GetHDDFriendly(const QString& Dev)
{
	QString FriendlyName;
	char InquiryBuffer[36];
	int Fd = open(QFLN2CH(Dev), O_RDONLY | O_NONBLOCK);

	if (Fd < 0)
	{
		WRITE_TRACE(DBG_INFO, "Unable to open resolved disk name %s",
					QSTR2UTF8(Dev));
		return FriendlyName;
	}

	/*
	 * Device found.
	 * send Inquiry command to obtain friendly name
	 */
	FriendlyName = Dev;

	if (InquirySCSIDevice(Dev, (PRL_UINT8_PTR)InquiryBuffer, sizeof(InquiryBuffer)) < 0)
		goto Exit;

	/*
	 *  we've get info about device
	 */
	FriendlyName = QString("%1 %2 (%3)").arg(QString(QByteArray(&InquiryBuffer[16], 16)).trimmed())
										.arg(QString(QByteArray(&InquiryBuffer[8], 8)).trimmed())
										.arg(Dev);

Exit:
	close(Fd);
	return FriendlyName;
}

/*
 * Check that we able to enumerate HDD by /dev/disks/by-id/
 */
static inline bool IsUdevActive()
{
	return QDir(UDEV_HDD_BASE).exists();
}

static inline bool isCdrom(const QString& dev_)
{
	char InquiryBuffer[36];

	if (InquirySCSIDevice(dev_, (PRL_UINT8_PTR)InquiryBuffer, sizeof(InquiryBuffer)) < 0)
	{
		WRITE_TRACE(DBG_INFO, "Unable to find out device type %s", QSTR2UTF8(dev_));
		return false;
	}

	return (InquiryBuffer[0]&0x1F) == TYPE_ROM;
}

QMap<QString, QString> CDspHostInfo::GetHddMpathList()
{
	QMap<QString, QString> list;
	QDir d("/dev/mapper");
	foreach (const QString &n, d.entryList(QDir::Files, QDir::Name))
	{
		if (!n.contains("mpath"))
			continue;

		QFileInfo i(d, n);
		list.insert(i.canonicalFilePath(), i.absoluteFilePath());
	}

	return list;
}

/*
 * Build HDD list based on udev disk list by id.
 */
void CDspHostInfo::GetHddUdevList(QList<CHwHardDisk*>& List)
{
	QRegExp Filter("^(?!.+part\\d+$)");
	QDir d(UDEV_HDD_BASE);
	QStringList Entries = d.entryList(QDir::Files, QDir::Name);

	Entries = Entries.filter(Filter);

	QMap<QString, QString> mpath = GetHddMpathList();
	foreach (const QString &n, Entries)
	{
		QFileInfo Info(d, n);
		QString File = Info.absoluteFilePath();
		QString c = Info.canonicalFilePath();

		QMap<QString, QString>::const_iterator i = mpath.find(c);
		if (i != mpath.end())
			File = i.value();

		if (isCdrom(c))
			continue;

		QString FriendlyName = GetHDDFriendly(c);

		if (CheckAddHdd(File))
			continue;

		HddAddToList(FriendlyName, File, 0, 0, false, false, List);
	}
}

/*
 * Build HDD list based on ordinary dev tree
 */
void CDspHostInfo::GetHddDevList(QList<CHwHardDisk*>& List)
{
	QStringList::iterator p;
	QString Str;

	/* check for ide disk drives */
	BuildDirList(PROC_BASE IDE_PATH, "hd?");

	for(p = m_strListTmp.begin(); p != m_strListTmp.end(); p++)
	{
		Str = GetIdeDev(*p);

		if (Str.isEmpty())
		{
			WRITE_TRACE(DBG_INFO, "Unable to resolve disk name '%s' to dev",
				QSTR2UTF8(*p));
			continue;
		}

		QString FriendlyName = GetHDDFriendly(Str);

		if (CheckAddHdd(Str))
			continue;

		HddAddToList(FriendlyName, Str, 0, 0, false, false, List);
	}

	/* check for scsi disk drives */
	BuildDirList(DEV_BASE, "sd?");

	for(p = m_strListTmp.begin(); p != m_strListTmp.end(); p++)
	{
		Str = QString(DEV_BASE "%1").arg(*p);

		QString FriendlyName = GetHDDFriendly(Str);

		if (CheckAddHdd(Str))
			continue;

		HddAddToList(FriendlyName, Str, 0, 0, false, false, List);
	}
}

// build list of hard disks (ide/scsi)
void CDspHostInfo::GetHDDList()
{
	m_HddMap.clear();
	QList<CHwHardDisk*> Disks;

	// Check is udev lists available
	if (IsUdevActive())
		GetHddUdevList(Disks);
	else
		GetHddDevList(Disks);

	// Cleanup list
	DropUnprocessedHdd(Disks);
	SafeUpdateList<CHwHardDisk>(Disks, p_HostHwInfo->m_lstHardDisks);

} // CDspHostInfo::GetHDDList()

static void add_dvd_device(const QString& path, CHostHardwareInfo* hw)
{
	QString strDevSysName = path;
	QString strDevFrName = QString("");

	// Device found. Check whether its name is a symbolic link
	char buf[36];
	if (readlink(QSTR2UTF8(path), buf, sizeof(buf)) != -1)
		return;

	// send Inquiry command to obtain friendly name
	int iRetVal = InquirySCSIDevice(strDevSysName, (quint8*)buf, sizeof(buf));


	/**
	* Extract peripheral device type from Inquiry Data
	*/
	char ucDeviceType = buf[0]&0x1F;
	const char LUN_IS_OPTICAL_DRIVE = 0x05; // see inf8090@p.489 for details

	if((iRetVal == 0)&&(ucDeviceType == LUN_IS_OPTICAL_DRIVE))
	{
		/*
		*  we've get info about device
		*/
		QByteArray frbuffer(&buf[8],31-8);
		strDevFrName = QString("%1").arg(QString(frbuffer.trimmed()));

		hw->addOpticalDisk( new CHwGenericDevice( PDE_OPTICAL_DISK,
			strDevFrName,
			strDevSysName ) );

	}
}

static bool match_s(const char* name)
{
	return (name[0] == 's');
}

static bool match_h(const char* name)
{
	return (name[0] == 'h');
}

// build list of cdroms (ide/scsi)
void CDspHostInfo::GetCDList()
{
	DIR *dp;
	struct dirent *ep;

	const char* dev_dir = "/dev";
	const char* sr_match = "sr+([0-9])";
	const char* scd_match = "scd+([0-9])";
	const char* sd_match = "sd[[:alpha:]]";
	const char* hd_match = "hd[[:alpha:]]";

	dp = opendir(dev_dir);
	if (dp == NULL) {
		WRITE_TRACE(DBG_FATAL, "opendir for (%s) failed: %d", dev_dir, errno);
		return;
	}

	while ((ep = readdir(dp))) {
		if (!match_s(ep->d_name) && !match_h(ep->d_name))
			continue;
		if (!fnmatch(sr_match, ep->d_name, FNM_EXTMATCH)  ||
			!fnmatch(sd_match, ep->d_name, FNM_EXTMATCH)  ||
			!fnmatch(scd_match, ep->d_name, FNM_EXTMATCH) ||
			!fnmatch(hd_match, ep->d_name, FNM_EXTMATCH)) {
			QString name = QString(DEV_BASE) + QString(ep->d_name);
			add_dvd_device(name, p_HostHwInfo);
		}
	}
	(void)closedir(dp);
} // CDspHostInfo::GetCDList()

/*
 * Check that opened descriptor is serial one and add to list
 */
static inline void SerialCheckAdd(const QString& Path, int fd,
							QList<CHwGenericDevice*>& List)
{
	struct serial_struct sps;
	memset(&sps, 0, sizeof(sps));

	if (ioctl(fd, TIOCGSERIAL, &sps, sizeof(sps)) < 0)
	{
		WRITE_TRACE(DBG_FATAL, "Ioctl TIOCGSERIAL failed for (%s) with code %d",
					QSTR2UTF8(Path), errno);
		return;
	}

	if (!sps.type)
		return;

	CHwGenericDevice* Dev = new (std::nothrow)
							CHwGenericDevice( PDE_SERIAL_PORT, Path, Path );

	if (!Dev)
	{
		WRITE_TRACE(DBG_FATAL, "Memory allocation failed to store serial port (%s) info",
					QSTR2UTF8(Path));
		return;
	}

	List.append( Dev );
}

/*
 * Build list of serial ports by specified mask
 */
void CDspHostInfo::BuildCOMList(const char* Mask, QList<CHwGenericDevice*>& List)
{
	BuildDirList(DEV_BASE, Mask);

	for(QStringList::iterator p = m_strListTmp.begin();
		p != m_strListTmp.end();
		p++)
	{
		QString Dev = QString(DEV_BASE);
		Dev += *p;

		int fd = open(QFLN2CH(Dev), O_RDWR | O_NONBLOCK);

		if (fd < 0)
			continue;

		SerialCheckAdd(Dev, fd, List);

		close(fd);
	}
}

// build list of serial ports
void CDspHostInfo::GetCOMList()
{
	QList<CHwGenericDevice*> Serial;

	BuildCOMList("ttyS?", Serial);
	BuildCOMList("ttyUSB?", Serial);

	SafeUpdateList<CHwGenericDevice>(Serial, p_HostHwInfo->m_lstSerialPorts);
} // CDspHostInfo::GetCOMList()


// build list of parallel ports
void CDspHostInfo::GetLPTList()
{
	if( !p_HostHwInfo->m_lstParallelPorts.isEmpty() )
		return;

	QStringList::iterator p;
	QString ptr;
	int fd, ret;

	BuildDirList(PROC_BASE LPT_PARPORT, "parport?");
	p = m_strListTmp.begin();
	while(p != m_strListTmp.end())
	{
		ptr.truncate(0);
		ptr = *p;
		ptr.remove(0, 7);
		ptr.insert(0, "/dev/lp");
		fd = open(QFLN2CH(ptr), O_RDWR | O_NONBLOCK);
		if (fd >= 0)
		{
			ret = -1;

			if (ioctl(fd, LPGETSTATUS, &ret, sizeof(int)) != -1)
			{
				p_HostHwInfo->addParallelPort(
					new CHwGenericDevice( PDE_PARALLEL_PORT, ptr, ptr ) );
			}
			close(fd);
		}
		*p++;
	}

} // CDspHostInfo::GetLPTList()


namespace
{
const char PRL_USB_SYSFS_PATH[] = "/sys/bus/usb/devices";

} // namespace

// Devices/Usb commented out by request from CP team
static QString get_qstring_attr(const char * dev, const char * attr)
{
	char buf[256];
	char name[sizeof(PRL_USB_SYSFS_PATH) + strlen(dev) + 1 + strlen(attr) + 1];
	sprintf(name, "%s/%s/%s", PRL_USB_SYSFS_PATH, dev, attr);

	FILE *fp = fopen(name, "r");
	if (fp == NULL)
		return QString("");

	if (fgets(buf, sizeof(buf), fp) != NULL) {
		buf[sizeof(buf)-1] = 0;
		int len = strlen(buf);
		while (len && (isblank(buf[len-1]) || !isprint(buf[len-1])))
			buf[--len] = 0;
		fclose(fp);
		return QString(buf);
	}
	fclose(fp);
	return QString();
}

// Devices/Usb commented out by request from CP team
static int get_int_attr(const char * dev, const char * attr)
{
	int res;
	char name[sizeof(PRL_USB_SYSFS_PATH) + strlen(dev) + 1 + strlen(attr) + 1];
	sprintf(name, "%s/%s/%s", PRL_USB_SYSFS_PATH, dev, attr);

	FILE *fp = fopen(name, "r");
	if (fp == NULL)
		return -1;

	if (fscanf(fp, "%d", &res) != 1)
		res = -1;

	fclose(fp);
	return res;
}

// Devices/Usb commented out by request from CP team
static int get_hex_attr(const char * dev, const char * attr)
{
	int res;
	char name[sizeof(PRL_USB_SYSFS_PATH) + strlen(dev) + 1 + strlen(attr) + 1];
	sprintf(name, "%s/%s/%s", PRL_USB_SYSFS_PATH, dev, attr);

	FILE *fp = fopen(name, "r");
	if (fp == NULL)
		return -1;

	if (fscanf(fp, "%x", &res) != 1)
		res = -1;

	fclose(fp);
	return res;
}

/* Here we try to detect keyboard/mouse.
 * For this perpouse we use value of InterfaceClass, InterfaceSubClass & InterfaceProtocol
 * Such approach work correctly with many keyboarb/mouse but not all.
 * To detect keyboard/mouse for sure need to get UsagePage from HID's Report Descriptor
 * hut = hid usage table (http://www.usb.org/developers/devclass_docs/Hut1_11.pdf)
 */

// Devices/Usb commented out by request from CP team
static BOOL check_km(const char * dev)
{
	int subclass = get_hex_attr(dev, "bDeviceSubClass");
	int protocol = get_hex_attr(dev, "bDeviceProtocol");

	return subclass == 1 && (protocol == 1 || protocol == 2);
}

// Devices/Usb commented out by request from CP team
static BOOL check_km_intf(const char * dev)
{
	int iconfig = get_hex_attr(dev, "bConfigurationValue");
	int inum = get_hex_attr(dev, "bNumInterfaces");

	for (int i = 0; i < inum; i++) {
		char name[256];

		snprintf(name, sizeof(name)-1, "%s:%d.%d", dev, iconfig, i);

		int iclass = get_hex_attr(name, "bInterfaceClass");

		if (iclass == 0x3) {
			int isubclass = get_hex_attr(name, "bInterfaceSubClass");
			int iprotocol = get_hex_attr(name, "bInterfaceProtocol");

			if (isubclass == 1 && (iprotocol == 1 || iprotocol == 2))
				return TRUE;
		}
	}
	return FALSE;
}

// Devices/Usb commented out by request from CP team
static BOOL check_msc_intf(const char * dev)
{
	int iconfig = get_hex_attr(dev, "bConfigurationValue");
	int inum = get_hex_attr(dev, "bNumInterfaces");

	for (int i = 0; i < inum; i++) {
		char name[256];

		snprintf(name, sizeof(name)-1, "%s:%d.%d", dev, iconfig, i);

		const int iclass = get_hex_attr(name, "bInterfaceClass");
		const int iprotocol = get_hex_attr(name, "bInterfaceProtocol");

		PRL_USB_DEVICE_TYPE pudt =
			CDspHostInfo::UsbIFC2PUDT(iclass, (UINT)~0, iprotocol);

		if (pudt == PUDT_DISK_STORAGE)
				return TRUE;
	}
	return FALSE;
}

// Devices/Usb commented out by request from CP team
static unsigned int make_location(const char * dev, int busnum)
{
	const char * p;
	unsigned int location = 0;
	int shift = 20;

	if ((p = strchr(dev, '-')) == NULL)
		return 0;

	do {
		unsigned int port;

		p++;
		if (sscanf(p, "%u", &port) != 1)
			return 0;

		if (shift < 0)
			return 0;

		location |= (port << shift);
		shift -= 4;
	} while ((p = strchr(p, '.')) != NULL);

	return location ? (location | (busnum << 24)) : 0;
}

static QString get_os_type(void)
{
	FILE *fp = NULL;
	char buffer[BUFSIZ] = {0};
	struct
	{
		FILE * (*open)(const char *, const char *);
		int (*close)(FILE *);
		const char * fname;
	} release_info[] =
	{
		{fopen, fclose, "/etc/parallels-release"},
		{fopen, fclose, "/etc/redhat-release"},
		{fopen, fclose, "/etc/SuSE-release"},
		{popen, pclose, "/usr/bin/lsb_release -ds"},
		{fopen, fclose, "/etc/mandrakelinux-release"},
		{fopen, fclose, "/etc/xandros-desktop-version"},
		{0, 0, 0}
	};

	for( int i = 0; release_info[i].open; i++)
		if( (fp = release_info[i].open(release_info[i].fname, "r")) != NULL )
		{
			char * s = fgets(buffer, sizeof(buffer) - 1, fp);
			release_info[i].close( fp );
			if (s != NULL) {
				*strchrnul( buffer, '\n' ) = 0;
				return  UTF8_2QSTR(buffer);
			}
		}

	return QString();
}

// build list of USB devices
void CDspHostInfo::GetUSBList()
{
// Devices/Usb commented out by request from CP team
	LOG_MESSAGE( DBG_INFO, "Recreate USB Devices list in HostHardwareInfo" );

	BOOL rescan_required = FALSE;
	int prl_usb_connector = -1;
	DIR * dir;
	struct dirent * de;
	// Copy current list to old_dev_lst and clear it
	QList<CHwUsbDevice*> old_dev_lst(p_HostHwInfo->m_lstUsbDevices);
	p_HostHwInfo->m_lstUsbDevices.clear();

	dir = opendir(PRL_USB_SYSFS_PATH);

	if (dir == NULL) {
		p_HostHwInfo->setUsbSupported(0);
		goto out;
	}

	p_HostHwInfo->setUsbSupported(1);

//	prl_usb_connector = open(PRL_USB_CONNECT_PATH, O_RDONLY);

	while ((de = readdir(dir)) != NULL) {
		int busnum;
		int devnum;
		int iclass;
		BOOL bDeviceServicedByPrl = FALSE;
		BOOL bKeyboardMouse = FALSE;

		/* Quickly skip irrelevant entries */
		if (de->d_name[0] == '.')
			continue;
		if (strchr(de->d_name, ':') != NULL)
			continue;
		if (memcmp(de->d_name, "usb", 3) == 0)
			continue;

		busnum = get_int_attr(de->d_name, "busnum");
		if (busnum < 0)
			continue;

		devnum = get_int_attr(de->d_name, "devnum");
		if (devnum < 0)
			continue;

		if (0 == make_location(de->d_name, busnum))
			continue;

		/* Skip hubs */
		iclass = get_hex_attr(de->d_name, "bDeviceClass");
//		if (iclass < 0)
//			continue;
		if (iclass < 0 || iclass == 0x09)
			continue;

		quint16 vendor = get_hex_attr(de->d_name, "idVendor");
		quint16 product = get_hex_attr(de->d_name, "idProduct");

		QString sNumber = get_qstring_attr(de->d_name, "serial");
		if (sNumber.isEmpty())
			sNumber = USB_NUM_EMPTY;

		PRL_USB_DEVICE_TYPE pudt = PUDT_OTHER;

		/* Check that this thing is keyboard/mouse */
		if (!bDeviceServicedByPrl) {
			if (iclass == 0x03) {
				if (check_km(de->d_name))
					bKeyboardMouse = TRUE;
			} else if (iclass == 0) {
				if (likely(check_msc_intf(de->d_name))) {
					iclass = 0x8;
					pudt = PUDT_DISK_STORAGE;
				}
				else if (check_km_intf(de->d_name))
					bKeyboardMouse = TRUE;
			}
		}

		/* Create device's suffix */
		QString sSuffix("--");
		if (bDeviceServicedByPrl)
			sSuffix = "PR";
		else if (bKeyboardMouse)
			sSuffix = "KM";

		QString sDevSpeed = USB_SPD_UNKNOWN;
		QString sSpeed = get_qstring_attr(de->d_name, "speed");
		if (sSpeed == "1.5")		sDevSpeed = USB_SPD_LOW;
		else if (sSpeed == "12")	sDevSpeed = USB_SPD_FULL;
		else if (sSpeed == "480")	sDevSpeed = USB_SPD_HIGH;
		else if (sSpeed == "5000")	sDevSpeed = USB_SPD_SUPER;

		QString sManufacturer = get_qstring_attr(de->d_name, "manufacturer");
		QString sProduct = get_qstring_attr(de->d_name, "product");

		QString sSystemName =
			CreateUsbSystemName (QString("%1-%2").arg(busnum).arg(devnum),
				vendor, product, sDevSpeed, sSuffix, sNumber);

		QString sFriendlyName;
		if (sManufacturer.isEmpty() || sProduct.isEmpty()) {
			sFriendlyName = CreateUsbFriendlyName(iclass, vendor, product);
			if (sManufacturer.isEmpty())
				sManufacturer = sFriendlyName.section(" - ", 0, 0);
			if (sProduct.isEmpty())
				sProduct = sFriendlyName.section(" - ", 1, 1);
		}
		sFriendlyName = QString("%1 - %2").arg(sManufacturer).arg(sProduct);

		if (devnum == 0) {
			BOOL was_there = FALSE;

			/* Special case. Linux >= 2.6.27 has a disgusting feature: when
			 * the device is reset, its devnum is set to 0. It is logically
			 * correct, the device really enters default state upon reset,
			 * but we cannot open usbfs handle even though usbfs works
			 * normally and actually the reset is caused by ioctl()
			 * on an open usbfs descriptor. Workaround is: if the device is
			 * not in the old list, we ignore it, but we must
			 * reschedule scan to wait for completion of reset,
			 * linux does not notify about this event.
			 * If the device was on the list we proceed normally.
			 * There is a risk that the device will be passed to VM
			 * right after this and CUsbDev will not able to open usbfs file.
			 * We take this risk.
			 */
			foreach(CHwUsbDevice *old_dev, old_dev_lst)
			{
				if (old_dev->getDeviceId().section('|', 0, 5) == sSystemName) {
					if (prl_usb_connector < 0)
						sSystemName = old_dev->getDeviceId();
					was_there = TRUE;
					break;
				}
			}

			if (!was_there) {
				rescan_required = TRUE;
				continue;
			}
		} else {
			/* Generate unique name for each new reconnection, when
			 * prl_usb_connect is not available. Autoconnect logic
			 * in CDspHwMonitorNotifier.cpp implies that autoconnect
			 * is disabled, when device is connected to VM and reenabled
			 * only on the second reconnection of device with the same name.
			 * Could be repaired there but does not worth it.
			 */
			if (prl_usb_connector < 0)
				sSystemName += QString("|%1").arg(devnum, 0, 10);
		}
		// Loockup or create device element, and append it to new list
		LookupOrCreateUSBDevice(old_dev_lst, p_HostHwInfo->m_lstUsbDevices, sSystemName, sFriendlyName, pudt);
	}

	closedir(dir);

	if (prl_usb_connector >= 0)
		close(prl_usb_connector);

	GetDiskUsbAliases(old_dev_lst, p_HostHwInfo->m_lstUsbDevices);

out:
	// Clear list of disconnected devices & release memory used for CHwGenericDevice
	foreach(CHwUsbDevice *old_dev,  old_dev_lst) {
		old_dev_lst.removeAll(old_dev);
		delete old_dev;
	}

	/* Walking over thin ice. We have no direct way to reschedule scan.
	 * Setting mtime on sysfs works, but it is a hack which can fail
	 * with any newer kernel version.
	 */
	if (rescan_required)
		utimes(PRL_USB_SYSFS_PATH, NULL);
} // CDspHostInfo::GetUSBList()


/**
* Build list of Scsi devices
*/
void CDspHostInfo::GetScsiList()
{
	QString sDevDir("/dev");
	QString sgfiles("sg*");
	QStringList filters;

	filters.append(sgfiles);

	QDir dirDb(sDevDir);

	dirDb.setFilter(QDir::Files | QDir::NoSymLinks  | QDir::Hidden | QDir::System);
	dirDb.setNameFilters(filters);

	QFileInfoList fileList = dirDb.entryInfoList();

	for(int i=0; i< fileList.size(); i++)
	{
		QFileInfo fi = fileList.at(i);
		QString strDevSysName = fi.absoluteFilePath();
		QString strDevFrName = QString("");
		/**
		* Device found.
		* send Inquiry command to obtain friendly name
		*/
		char InquiryBuffer[36];

		int iRetVal = InquirySCSIDevice(strDevSysName, (quint8*)InquiryBuffer, 36);

#if 1
		/**
		* Extract peripheral device type from Inquiry Data
		*/
		char ucDeviceType = InquiryBuffer[0]&0x1F;
		const char LUN_IS_OPTICAL_DRIVE = 0x05; // see inf8090@p.489 for details
		const char LUN_IS_MAGNETIC_DISK = 0x00; // see inf8090@p.489 for details
		const char LUN_IS_RAID 			= 0x0C; // see inf8090@p.489 for details

		if((iRetVal == 0)&&
			(ucDeviceType != LUN_IS_OPTICAL_DRIVE)&&
			(ucDeviceType != LUN_IS_MAGNETIC_DISK)&&
			(ucDeviceType != LUN_IS_RAID)
			)
#else
		if(iRetVal == 0)
#endif
		{
			/*
			*  we've get info about device
			*/
			QByteArray frbuffer(&InquiryBuffer[8],31-8);
			strDevFrName = QString("%1").arg(QString(frbuffer.trimmed()));

			p_HostHwInfo->addGenericScsiDevice( new CHwGenericDevice( PDE_GENERIC_SCSI_DEVICE,
				strDevFrName,
				strDevSysName ) );

		}
	}
}


// build list of printers
void CDspHostInfo::GetPrinterList()
{
	// no printers should be detected on Linux

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

void CDspHostInfo::GetOsVersion( CHwOsVersion /*out*/ *pOsVer, QString* pOutOsVersionAsString )
{
	PRL_ASSERT( pOsVer || pOutOsVersionAsString );
	if ( !pOsVer && !pOutOsVersionAsString )
		return;

	struct utsname name;

	if ( uname (&name) == -1 )
		WRITE_TRACE(DBG_FATAL, "CDspHostInfo::GetOsVersion() : uname() error = %d", errno );

	QString sOsType = get_os_type();
	QString qsRelease(name.release);

	QString sOsVersionAsString = sOsType.isEmpty()
					? QString("Linux Version %1").arg(qsRelease)
					: QString("%1 (%2)").arg(sOsType).arg(qsRelease);

	if ( pOutOsVersionAsString )
		*pOutOsVersionAsString = sOsVersionAsString;

	if ( !pOsVer )
		return;

	PRL_ASSERT( pOsVer );

	pOsVer->setOsType( PHO_LIN );

	QStringList lstRelease = qsRelease.split('.');
	if ( lstRelease.size() > 0 )
		pOsVer->setMajor( lstRelease[0].toUInt() );
	if ( lstRelease.size() > 1 )
		pOsVer->setMinor( lstRelease[1].toUInt() );
	if ( lstRelease.size() > 2 )
	{
		QString qsTmp = lstRelease[2];
		QString qsSubMinorVer;

		for ( int i = 0; i < qsTmp.size(); i++ )
		{
			if (qsTmp[i].isDigit())
				qsSubMinorVer += qsTmp[i];
			else
				break;
		}
		pOsVer->setSubMinor( qsSubMinorVer.toUInt() );
	}

	pOsVer->setStringPresentation( sOsVersionAsString );

	if (CVzHelper::is_vz_running())
		 pOsVer->setVzRunning(true);

	pOsVer->setOsArchitecture( getSystemBitness() );
	pOsVer->setKernelArchitecture( getKernelBitness() );

	/*
	QFile file_version("/proc/version");
	if ( file_version.open(QIODevice::ReadOnly) )
	{
	pOsVer->setStringPresentation( file_version.readAll() );
	file_version.close();
	}
	else
	WRITE_TRACE(DBG_FATAL, "Can not open /proc/version file." );
	*/
	//	WRITE_TRACE(DBG_FATAL, "CDspHostInfo::getOSVersion() : \n%s", QSTR2UTF8(p_HostHwInfo->toString()) );
}


/**
* @brief get Cpu Info.
* @return
*/
void CDspHostInfo::GetCpu()
{
	CHwCpu* pCpu = p_HostHwInfo->getCpu();
	if ( !pCpu )
	{
		WRITE_TRACE(DBG_FATAL, "CDspHostInfo::GetCpu() : CHwCpu is NULL!" );
		return;
	}

	QString qsCpuInfo;
	QFile file_cpuinfo("/proc/cpuinfo");
	if ( file_cpuinfo.open(QIODevice::ReadOnly) )
	{
		qsCpuInfo = file_cpuinfo.readAll();
		file_cpuinfo.close();
	}
	else
		WRITE_TRACE(DBG_FATAL, "Can not open /proc/cpuinfo file." );

	QStringList lstCpuInfo = qsCpuInfo.split('\n');
	int nCpuNumber = 1;

	QString qsFirstKey;
	for ( int i =0; i < lstCpuInfo.size(); i++ )
	{
		QStringList lstTmp = lstCpuInfo[i].split(':');
		QString qsKey = lstTmp[0];
		if ( qsFirstKey == qsKey )
			++nCpuNumber;

		if ( qsFirstKey.isEmpty() )
			qsFirstKey = qsKey;

		if ( nCpuNumber == 1 )
		{
			if ( qsKey.contains("model name") )
				pCpu->setModel( lstTmp[1].simplified() );
			if ( qsKey.contains("cpu MHz") )
				pCpu->setSpeed( (unsigned int)lstTmp[1].simplified().toFloat() );

		}
	}
	// Get Max freq from /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq
	if (ParallelsDirs::getAppExecuteMode() == PAM_SERVER)
	{
		QFile file_max_freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
		if ( file_max_freq.open(QIODevice::ReadOnly) )
		{
			bool bOk;
			unsigned long nMaxFreq;

			QString qsMaxFreq = file_max_freq.readAll();
			file_max_freq.close();

			nMaxFreq = qsMaxFreq.toULong(&bOk);
			if (bOk)
				pCpu->setSpeed( nMaxFreq / 1000 );
		}
	}


	pCpu->setNumber( nCpuNumber );


	if ( IsProcessorSupportIA32e() )
		pCpu->setMode( PCM_CPU_MODE_64 );
	else
		pCpu->setMode( PCM_CPU_MODE_32 );
}


/**
* @brief Build list of directories.
* @param dir
* @param mask
*/
void CDspHostInfo::BuildDirList(const char *dir, const char *mask)
{
	if (!dir)
		return;

	if (mask)
		dmask = strdup(mask);

	m_strListTmp.clear();

	struct dirent **namelist;
	int n, i;
	n = scandir(dir, &namelist, 0, alphasort);
	if (n < 0) {
		free(dmask);
		return;
	} else {
		for(i = 0; i < n; i++) {
			if (NameFilter(namelist[i]->d_name)) {
				m_strListTmp.push_back(namelist[i]->d_name);
			}
			free(namelist[i]);
		}
		free(namelist);
	}
	free(dmask);

} // CDspHostInfo::BuildDirList()


/**
* @brief Helping functions for CDspHostInfo class
* Name filtering in directory, checking file contents and etc.
* @param name
* @return
*/
int CDspHostInfo::NameFilter(char *name)
{
	if (!strcmp(name, "."))
		return 0;

	if (!strcmp(name, ".."))
		return 0;

	char *pptr;
	if (dmask) {
		pptr = dmask;
		while (*name && *pptr) {
			while (*name == *pptr) {
				pptr++;
				name++;
			}

			switch (*pptr) {
			case '\0':
				break;

			case '*':
				pptr++;
				if (!*pptr)
					return 1;
				while (*name && *name != *pptr)
					name++;
				break;

			case '?':
				pptr++;
				if (!*name)
					return 0;
				name++;
				break;

			default:
				return 0;
			}
		}

		if (!*name && !*pptr)
			return 1;
	} else {
		return 1;
	}

	return 0;

} // CDspHostInfo::NameFilter()

unsigned int CDspHostInfo::getMemTotal()
{
	long unsigned int uiMemTotal = 256;

	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen("/proc/meminfo", "r");
	if (fp == NULL)
		return uiMemTotal;

	while ((read = getline(&line, &len, fp)) != -1) {
		len = sscanf(line,"MemTotal:%lu kB\n", &uiMemTotal);
        if (len > 0) {
			uiMemTotal /= 1024;
			break;
		}
	}

	fclose(fp);
	if (line)
		free(line);

	return uiMemTotal;

} // CDspHostInfo::getMemTotal()

/*
 * Determine is computer supports power capabilities
 */
void CDspHostInfo::GetMobileDevice()
{
    // Undeterminable for linux
	p_HostHwInfo->setHostNotebookFlag( false );
}

/*
 Returns maximum number of displays in host.
 Returns 0 if error occuried.
 */
unsigned int CDspHostInfo::getMaxDisplays()
{
	// Not fully implemented for linux hosts yet.
	// XXX Return value > 1, to use new interface (TG_REQUEST_VID_SET_MODE) to manage video modes,
	// otherwise Video Scaner not properly switches when switching guest Win Aero on/off.
	return 2;
}

void CDspHostInfo::getAdvancedMemoryInfo()
{
	//TODO fill advincedmemoryinfo here for windows
}
