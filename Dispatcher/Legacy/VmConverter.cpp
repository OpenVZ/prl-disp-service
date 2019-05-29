///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "VmConverter.h"
#include "CDspService.h"
#include "CDspLibvirt.h"
#include "CDspVmManager_p.h"
#include "CDspVmConfigManager.h"
#include "Libraries/PrlNetworking/netconfig.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include "prlcommon/Interfaces/ApiDevNums.h"
#include <prlsdk/PrlOses.h>
#include <boost/range/irange.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>
#include <boost/bind.hpp>
#include "Tasks/Task_EditVm.h"
#include "Tasks/Task_EditVm_p.h"

using namespace Legacy::Vm;

namespace
{

enum {V2V_RUN_TIMEOUT = 60 * 60 * 1000};
enum {MAX_IDE = PRL_MAX_IDE_DEVICES_NUM - 1}; // 1 for tools cd

const char VIRT_V2V[] = "/usr/bin/virt-v2v";

QString getHostOnlyBridge()
{
	Libvirt::Instrument::Agent::Network::Unit u;
	// Get Host-Only network.
	Libvirt::Result r = Libvirt::Kit.networks().find(
			PrlNet::GetDefaultVirtualNetworkID(0, false), &u);
	if (r.isFailed())
		return QString();

	CVirtualNetwork n;
	r = u.getConfig(n);
	if (r.isFailed())
		return QString();

	CParallelsAdapter* a;
	if (NULL == n.getHostOnlyNetwork() ||
		(a = n.getHostOnlyNetwork()->getParallelsAdapter()) == NULL)
		return QString();

	return a->getName();
}

///////////////////////////////////////////////////////////////////////////////
// struct Helper

struct Helper
{
	Helper(PRL_MASS_STORAGE_INTERFACE_TYPE hddType,
	       PRL_CLUSTERED_DEVICE_SUBTYPE hddSubType,
	       CVmHardware &hardware);

	result_type do_();
	result_type insertTools(const QString &path, CVmStartupOptions *options);

	static result_type buildResult(PRL_RESULT code, const QString &param);

private:
	template <typename T> result_type do_(T *pDevice);
	template <typename T> bool isConverted(const T &device) const;

	CVmHardware &m_hardware;

	PRL_MASS_STORAGE_INTERFACE_TYPE m_hddType;
	PRL_CLUSTERED_DEVICE_SUBTYPE m_hddSubType;
	QMap<PRL_MASS_STORAGE_INTERFACE_TYPE, QList<unsigned> > m_filled;
};

result_type Helper::buildResult(PRL_RESULT code, const QString &param)
{
	SmartPtr<CVmEvent> e(new CVmEvent);
	e->setEventCode(code);
	e->addEventParameter(new CVmEventParameter(PVE::String,
				param, EVT_PARAM_MESSAGE_PARAM_0));
	return e;
}

template<> bool Helper::isConverted(const CVmHardDisk &device) const
{
	return device.getEmulatedType() == PVE::HardDiskImage &&
	       (device.getInterfaceType() != m_hddType
		|| device.getSubType() != m_hddSubType);
}

template<> bool Helper::isConverted(const CVmOpticalDisk &device) const
{
	return device.getEmulatedType() == PVE::CdRomImage &&
	       device.getInterfaceType() != PMS_IDE_DEVICE;
}

template<> result_type Helper::do_(CVmOpticalDisk *pDevice)
{
	// Convert Cdrom devices to IDE since SATA is unsupported.
	if (pDevice == NULL || !isConverted(*pDevice))
		return result_type();
	if (m_filled[PMS_IDE_DEVICE].isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Too many IDE devices after config conversion");
		return buildResult(PRL_ERR_VMCONF_IDE_DEVICES_COUNT_OUT_OF_RANGE,
		                   QString::number(MAX_IDE));
	}
	pDevice->setInterfaceType(PMS_IDE_DEVICE);
	pDevice->setStackIndex(m_filled[PMS_IDE_DEVICE].takeFirst());
	return result_type();
}

template<> result_type Helper::do_(CVmHardDisk *pDevice)
{
	// Convert disks to virtio-scsi
	// There's no virtio-scsi drivers for win2003-, use virtio-block.
	if (NULL == pDevice || !isConverted(*pDevice))
		return result_type();
	if (m_hddType == PMS_SCSI_DEVICE)
	{
		if (m_filled[m_hddType].isEmpty())
		{
			WRITE_TRACE(DBG_FATAL, "Too many SCSI devices after config conversion");
			return buildResult(PRL_ERR_VMCONF_SCSI_DEVICES_COUNT_OUT_OF_RANGE,
			                   QString::number(PRL_MAX_SCSI_DEVICES_NUM));
		}
	}
	pDevice->setInterfaceType(m_hddType);
	pDevice->setSubType(m_hddSubType);
	pDevice->setStackIndex(m_filled[m_hddType].takeFirst());

	return result_type();
}

template<> result_type Helper::do_(CVmGenericNetworkAdapter *pDevice)
{
	if (pDevice == NULL)
		return result_type();
	pDevice->setAdapterType(PNT_VIRTIO);
	pDevice->setHostInterfaceName(HostUtils::generateHostInterfaceName(pDevice->getMacAddress()));
	return result_type();
}

Helper::Helper(PRL_MASS_STORAGE_INTERFACE_TYPE hddType,
               PRL_CLUSTERED_DEVICE_SUBTYPE hddSubType,
               CVmHardware &hardware):
	m_hardware(hardware), m_hddType(hddType), m_hddSubType(hddSubType)
{
	boost::copy(boost::irange(0, (int)PRL_MAX_SCSI_DEVICES_NUM),
	            std::back_inserter(m_filled[PMS_SCSI_DEVICE]));
	boost::copy(boost::irange(0, (int)PRL_MAX_IDE_DEVICES_NUM),
	            std::back_inserter(m_filled[PMS_IDE_DEVICE]));
	if (hddType != PMS_SCSI_DEVICE)
	{
		boost::copy(boost::irange(0, (int)PRL_MAX_GENERIC_PCI_DEVICES),
			    std::back_inserter(m_filled[PMS_VIRTIO_BLOCK_DEVICE]));
	}

	foreach(CVmHardDisk *pDevice, m_hardware.m_lstHardDisks) {
		if (NULL != pDevice && !isConverted(*pDevice))
			m_filled[pDevice->getInterfaceType()].removeOne(pDevice->getStackIndex());
	}

	foreach(CVmOpticalDisk *pDevice, m_hardware.m_lstOpticalDisks) {
		if (NULL != pDevice && !isConverted(*pDevice))
			m_filled[pDevice->getInterfaceType()].removeOne(pDevice->getStackIndex());
	}
}

result_type Helper::insertTools(const QString &path, CVmStartupOptions *options)
{
	if (m_filled[PMS_IDE_DEVICE].isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot insert guest tools iso");
		return buildResult(PRL_ERR_VMCONF_IDE_DEVICES_COUNT_OUT_OF_RANGE,
		                   QString::number(MAX_IDE));
	}

	QList<CVmStartupOptions::CVmBootDevice*> b = options->m_lstBootDeviceList;
	Vm::Config::Index::Device<CVmOpticalDisk, PDE_OPTICAL_DISK> fixIndex(
			m_hardware.m_lstOpticalDisks, b);
	CVmOpticalDisk *d = new CVmOpticalDisk;
	d->setEmulatedType(PVE::CdRomImage);
	d->setSubType(PCD_BUSLOGIC);
	d->setInterfaceType(PMS_IDE_DEVICE);
	d->setEnabled(PVE::DeviceEnabled);
	d->setSystemName(path);
	d->setUserFriendlyName(path);
	d->setStackIndex(m_filled[PMS_IDE_DEVICE].takeFirst());
	m_hardware.m_lstOpticalDisks << d;
	fixIndex(m_hardware.m_lstOpticalDisks);

	return result_type();
}

result_type Helper::do_()
{
	result_type res;

	// Convert Cdrom devices to IDE since SATA is unsupported.
	foreach(CVmOpticalDisk* pDevice, m_hardware.m_lstOpticalDisks) {
		if ((res = do_(pDevice)).isFailed())
			return res;
	}

	foreach(CVmHardDisk *pDevice, m_hardware.m_lstHardDisks) {
		if ((res = do_(pDevice)).isFailed())
			return res;
	}

	// Convert network interfaces to virtio-net
	foreach(CVmGenericNetworkAdapter *pDevice, m_hardware.m_lstNetworkAdapters) {
		if ((res = do_(pDevice)).isFailed())
			return res;
	}

	// 3d acceleration is disabled
	if (m_hardware.getVideo())
		m_hardware.getVideo()->setEnable3DAcceleration(P3D_DISABLED);

	// Parallel ports are not supported anymore
	m_hardware.m_lstParallelPortOlds.clear();
	m_hardware.m_lstParallelPorts.clear();
	return result_type();
}

///////////////////////////////////////////////////////////////////////////////
// struct Killer

struct Killer: Command::Vm::Shutdown::Fallback
{
	Killer(const QString& uuid_, Libvirt::Result& sink_)
		: Command::Vm::Shutdown::Fallback(uuid_, sink_), m_sink(&sink_)
	{
	}

	static Killer* craft(const CVmConfiguration& config_, Libvirt::Result& sink_)
	{
		return new Killer(config_.getVmIdentification()->getVmUuid(), sink_);
	}
	static quint32 getTimeout()
	{
		// Scientifically calculated timeout of 28 mins;
		return 28*60;
	}
	void react()
	{
		if (CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()
			->getDebug()->isLegacyVmUpgrade())
		{
			WRITE_TRACE(DBG_FATAL, "VM has not been stopped before timeout (%u secs)", getTimeout());
			*m_sink = Error::Simple(PRL_ERR_FIXME,
				"The converted VM could not shut down on its own after conversion and was preserved for investigation");
			emit finish();
			return;

		}

		Command::Vm::Shutdown::Fallback::react();
		WRITE_TRACE(DBG_FATAL, "VM was killed after timeout (%u secs)", getTimeout());
		*m_sink = Error::Simple(PRL_ERR_TIMEOUT,
			"VM had not stopped automatically and was forcibly shut down after timeout.");
	}

private:
	Libvirt::Result* m_sink;
};

typedef Command::Tag::Timeout<Command::Tag::State
		<Command::Vm::Starter, Command::Vm::Fork::State::Strict<VMS_STOPPED> >, Killer>
	firstStart_type;

} // namespace

namespace Legacy
{
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Converter

result_type Converter::convertHardware(SmartPtr<CVmConfiguration> &cfg) const
{
	CVmStartupBios* b = cfg->getVmSettings()->getVmStartupOptions()->getBios();
	if (b->isEfiEnabled())
	{
		b->setNVRAM(PRL_VM_NVRAM_FILE_NAME);
	}
	else
		b->setNVRAM(QString());

	CVmHardware* pVmHardware = cfg->getVmHardwareList();
	if (NULL == pVmHardware) {
		WRITE_TRACE(DBG_FATAL, "Can not get Vm hardware list");
		return result_type();
	}
	// NB. we have an unused structure chipset inside vm configurations.
	// its attributes are type: 1, version: 3 be default. it was decided
	// to employ the structure for holding QEMU machine type values.
	// type 1 is treated as a discriminator for the i440fx machine types
	// family and 3 denotes 7.6 machine type. it was decided to assign
	// minimum 7.8 machine type for new VMs thus the chipset version value
	// should be not less than 5 for the i440fx machine type.
	::Chipset* c = pVmHardware->getChipset();
	if (NULL != c && c->getType() == CDspVm::CHIP_PCI440FX)
		c->setVersion(qMax<quint32>(c->getVersion(), CDspVm::PCI440FX_DEFAULT));

	unsigned os = cfg->getVmSettings()->getVmCommonOptions()->getOsVersion();
	bool isWin = cfg->getVmSettings()->getVmCommonOptions()->getOsType() ==
				PVS_GUEST_TYPE_WINDOWS;

	CVmUsbController *usb = cfg->getVmSettings()->getUsbController();
	if (isWin && IS_WIN_VER_BELOW(os, PVS_GUEST_VER_WIN_WINDOWS8))
	{
		usb->setUhcEnabled(true);
		usb->setEhcEnabled(true);
		usb->setXhcEnabled(false);
	}
	else
	{
		usb->setUhcEnabled(false);
		usb->setEhcEnabled(false);
		usb->setXhcEnabled(true);
	}

	// Create USB controller if it is missing
	// It's necessary to ensure we have USB tablet device after
	// conversion so pointer works properly in VNC.
	const QList<CVmUsbDevice*>& usbList = pVmHardware->m_lstUsbDevices;
	if (std::find_if(usbList.begin(), usbList.end(),
		boost::bind(&CVmUsbDevice::getUsbType, _1) == PUDT_OTHER)
		== usbList.end())
	{
		CVmUsbDevice* usbDev = new(CVmUsbDevice);
		usbDev->setEnabled(PVE::DeviceEnabled);
		usbDev->setUsbType(PUDT_OTHER);
		pVmHardware->addUsbDevice(usbDev);
	}

	cfg->getVmSettings()->getVmRuntimeOptions()->getOnCrash()->setMode(POCA_PAUSE);

	bool noSCSI = isWin && IS_WIN_VER_BELOW(os, PVS_GUEST_VER_WIN_VISTA);
	Helper h(noSCSI ? PMS_VIRTIO_BLOCK_DEVICE : PMS_SCSI_DEVICE,
	         noSCSI ? PCD_BUSLOGIC : PCD_VIRTIO_SCSI,
			 *pVmHardware);
	result_type res;
	if ((res = h.do_()).isFailed())
		return res;

	return h.insertTools(ParallelsDirs::getToolsImage(PAM_SERVER, os),
	                     cfg->getVmSettings()->getVmStartupOptions());
}


result_type Converter::convertBios(const SmartPtr<CVmConfiguration> &cfg) const
{
	CVmStartupBios* b = cfg->getVmSettings()->getVmStartupOptions()->getBios();

	QString n = b->getNVRAM();
	if (!n.isEmpty())
	{
		if (QFileInfo(n).isRelative())
			n = QDir(cfg->getVmIdentification()->getHomePath()).absoluteFilePath(n);
		QFile::remove(n);

		PRL_RESULT e = Edit::Vm::Create::Action<CVmStartupBios>(*b, *cfg).execute();
		if (!PRL_SUCCEEDED(e))
		{
			WRITE_TRACE(DBG_FATAL, "Unable to generate new NVRAM image.");
			return Helper::buildResult(e, n);
		}
	}
	return result_type();
}


boost::optional<V2V> Converter::getV2V(const CVmConfiguration &cfg) const
{
	CVmHardware *pVmHardware;
	if ((pVmHardware = cfg.getVmHardwareList()) == NULL)
		return boost::none;

	// Allow restoration of VMs w/o disks.
	if (std::find_if(pVmHardware->m_lstHardDisks.begin(),
	                 pVmHardware->m_lstHardDisks.end(),
	                 boost::bind(std::not_equal_to<CVmHardDisk*>(), _1, (CVmHardDisk*)NULL) &&
	                 boost::bind(&CVmHardDisk::getEnabled, _1) == PVE::DeviceEnabled &&
	                 boost::bind(&CVmHardDisk::getConnected, _1) == PVE::DeviceConnected &&
	                 !boost::bind(&QString::isEmpty, boost::bind(&CVmHardDisk::getSystemName, _1)))
		== pVmHardware->m_lstHardDisks.end())
		return boost::none;

	return V2V(cfg);
}

///////////////////////////////////////////////////////////////////////////////
// struct V2V

PRL_RESULT V2V::do_() const
{
	// Get windows driver ISO.
	QString winDriver = ParallelsDirs::getToolsImage(PAM_SERVER, PVS_GUEST_VER_WIN_2K);
	if (!QFile(winDriver).exists())
	{
		WRITE_TRACE(DBG_WARNING, "Windows drivers image does not exist: %s\n"
								 "Restoring Windows will fail.",
								 qPrintable(winDriver));
	}
	::setenv("VIRTIO_WIN", qPrintable(winDriver), 1);
	WRITE_TRACE(DBG_DEBUG, "setenv: %s=%s", "VIRTIO_WIN", qPrintable(winDriver));
	// If default bridge is not 'virbr0', virt-v2v fails.
	// So we try to find actual bridge name.
	QString bridge = getHostOnlyBridge();
	if (!bridge.isEmpty())
	{
		::setenv("LIBGUESTFS_BACKEND_SETTINGS",
				 qPrintable(QString("network_bridge=%1").arg(bridge)), 1);
		WRITE_TRACE(DBG_FATAL, "setenv: %s=%s",
					"LIBGUESTFS_BACKEND_SETTINGS",
					qPrintable(QString("network_bridge=%1").arg(bridge)));
	}

	QStringList cmdline = QStringList()
		<< VIRT_V2V
		<< "-i" << "libvirt"
		<< "--in-place" << "-v" << "-x";

	cmdline << ::Uuid(m_cfg.getVmIdentification()->getVmUuid()).toStringWithoutBrackets();

	QProcess process;
	HostUtils::RunCmdLineUtilityEx(cmdline.join(" "), process, V2V_RUN_TIMEOUT, NULL);

	WRITE_TRACE(DBG_FATAL, "virt-v2v output:");
	foreach (const QString& s, QString(process.readAllStandardOutput()).split("\n"))
		WRITE_TRACE(DBG_FATAL, "%s", qPrintable(s));
	if (QProcess::NormalExit != process.exitStatus() || process.exitCode() != 0)
	{
		WRITE_TRACE(DBG_FATAL, "Cannot convert VM to vz7 format: virt-v2v failed");
		foreach (const QString& s, QString(process.readAllStandardError()).split("\n"))
			WRITE_TRACE(DBG_FATAL, "%s", qPrintable(s));
		return PRL_ERR_CANT_RECONFIG_GUEST_OS;
	}
	return PRL_ERR_SUCCESS;
}

Prl::Expected<void, Error::Simple> V2V::start() const
{
	CVmConfiguration cfg = m_cfg;

	foreach (CVmGenericNetworkAdapter* l, cfg.getVmHardwareList()->m_lstNetworkAdapters)
		l->setConnected(PVE::DeviceDisconnected);

	Prl::Expected<void, Error::Simple> e = Command::Vm::Gear<firstStart_type>::run(cfg);

	// store config with disabled network
	if (e.isFailed())
	{
		if (e.error().code() == PRL_ERR_FIXME)
			Libvirt::Kit.vms().getGrub(cfg).spawnPersistent();
	}
	else
	{
		Libvirt::Kit.vms().at(m_cfg.getVmIdentification()->getVmUuid())
			.getMaintenance().emitDefined();
	}
	return e;
}

} // namespace Vm
} // namespace Legacy

