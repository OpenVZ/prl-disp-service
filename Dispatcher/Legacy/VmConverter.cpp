///////////////////////////////////////////////////////////////////////////////
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

#include "VmConverter.h"
#include "CDspLibvirt.h"
#include "Libraries/PrlNetworking/netconfig.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include "prlcommon/Interfaces/ApiDevNums.h"
#include <prlsdk/PrlOses.h>
#include <boost/range/irange.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace
{

enum {V2V_RUN_TIMEOUT = 60 * 60 * 1000};

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

	PRL_RESULT do_();

private:
	template <typename T> PRL_RESULT do_(T *pDevice);
	template <typename T> bool isConverted(const T &device) const;

	CVmHardware &m_hardware;

	PRL_MASS_STORAGE_INTERFACE_TYPE m_hddType;
	PRL_CLUSTERED_DEVICE_SUBTYPE m_hddSubType;
	QMap<PRL_MASS_STORAGE_INTERFACE_TYPE, QList<unsigned> > m_filled;
};

template<> bool Helper::isConverted(const CVmHardDisk &device) const
{
	return device.getEmulatedType() == PVE::HardDiskImage &&
	       device.getInterfaceType() != m_hddType;
}

template<> bool Helper::isConverted(const CVmOpticalDisk &device) const
{
	return device.getEmulatedType() == PVE::CdRomImage &&
	       device.getInterfaceType() != PMS_IDE_DEVICE;
}

template<> PRL_RESULT Helper::do_(CVmCpu *pDevice)
{
	// No PMU in guest with Parallels Tools causes BSOD.
	if (pDevice)
		pDevice->setVirtualizePMU(true);
	return PRL_ERR_SUCCESS;
}

template<> PRL_RESULT Helper::do_(CVmOpticalDisk *pDevice)
{
	// Convert Cdrom devices to IDE since SATA is unsupported.
	if (pDevice == NULL || !isConverted(*pDevice))
		return PRL_ERR_SUCCESS;
	if (m_filled[PMS_IDE_DEVICE].isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Too many IDE devices after config conversion");
		return PRL_ERR_VMCONF_IDE_DEVICES_COUNT_OUT_OF_RANGE;
	}
	pDevice->setInterfaceType(PMS_IDE_DEVICE);
	pDevice->setStackIndex(m_filled[PMS_IDE_DEVICE].takeFirst());
	return PRL_ERR_SUCCESS;
}

template<> PRL_RESULT Helper::do_(CVmHardDisk *pDevice)
{
	// Convert disks to virtio-scsi
	// There's no virtio-scsi drivers for win2003-, use virtio-block.
	if (NULL == pDevice || !isConverted(*pDevice))
		return PRL_ERR_SUCCESS;
	if (m_hddType == PMS_SCSI_DEVICE)
	{
		if (m_filled[m_hddType].isEmpty())
		{
			WRITE_TRACE(DBG_FATAL, "Too many SCSI devices after config conversion");
			return PRL_ERR_VMCONF_SCSI_DEVICES_COUNT_OUT_OF_RANGE;
		}
		pDevice->setStackIndex(m_filled[m_hddType].takeFirst());
	}
	pDevice->setInterfaceType(m_hddType);
	pDevice->setSubType(m_hddSubType);
	return PRL_ERR_SUCCESS;
}

template<> PRL_RESULT Helper::do_(CVmGenericNetworkAdapter *pDevice)
{
	if (pDevice == NULL)
		return PRL_ERR_SUCCESS;
	pDevice->setAdapterType(PNT_VIRTIO);
	pDevice->setHostInterfaceName(HostUtils::generateHostInterfaceName(pDevice->getMacAddress()));
	return PRL_ERR_SUCCESS;
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

	foreach(CVmHardDisk *pDevice, m_hardware.m_lstHardDisks) {
		if (NULL != pDevice && !isConverted(*pDevice))
			m_filled[pDevice->getInterfaceType()].removeOne(pDevice->getStackIndex());
	}

	foreach(CVmOpticalDisk *pDevice, m_hardware.m_lstOpticalDisks) {
		if (NULL != pDevice && !isConverted(*pDevice))
			m_filled[pDevice->getInterfaceType()].removeOne(pDevice->getStackIndex());
	}
}

PRL_RESULT Helper::do_()
{
	PRL_RESULT res;
	if (PRL_FAILED(res = do_(m_hardware.getCpu())))
		return res;

	// Convert Cdrom devices to IDE since SATA is unsupported.
	foreach(CVmOpticalDisk* pDevice, m_hardware.m_lstOpticalDisks) {
		if (PRL_FAILED(res = do_(pDevice)))
			return res;
	}

	foreach(CVmHardDisk *pDevice, m_hardware.m_lstHardDisks) {
		if (PRL_FAILED(res = do_(pDevice)))
			return res;
	}

	// Convert network interfaces to virtio-net
	foreach(CVmGenericNetworkAdapter *pDevice, m_hardware.m_lstNetworkAdapters) {
		if (PRL_FAILED(res = do_(pDevice)))
			return res;
	}

	// Parallel ports are not supported anymore
	m_hardware.m_lstParallelPortOlds.clear();
	m_hardware.m_lstParallelPorts.clear();
	return PRL_ERR_SUCCESS;
}

} // namespace

namespace Legacy
{
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Converter

PRL_RESULT Converter::convertHardware(SmartPtr<CVmConfiguration> &cfg) const
{
	CVmHardware *pVmHardware;
	if ((pVmHardware = cfg->getVmHardwareList()) == NULL) {
		WRITE_TRACE(DBG_FATAL, "[%s] Can not get Vm hardware list", __FUNCTION__);
		return PRL_ERR_SUCCESS;
	}

	bool noSCSI = cfg->getVmSettings()->getVmCommonOptions()->getOsType() ==
				PVS_GUEST_TYPE_WINDOWS &&
			IS_WIN_VER_BELOW(cfg->getVmSettings()->getVmCommonOptions()->getOsVersion(),
			                 PVS_GUEST_VER_WIN_VISTA);
	return Helper(noSCSI ? PMS_VIRTIO_BLOCK_DEVICE : PMS_SCSI_DEVICE,
	              noSCSI ? PCD_BUSLOGIC : PCD_VIRTIO_SCSI,
				  *pVmHardware).do_();
}

PRL_RESULT Converter::convertVm(const QString &vmUuid) const
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
		WRITE_TRACE(DBG_DEBUG, "setenv: %s=%s",
					"LIBGUESTFS_BACKEND_SETTINGS",
					qPrintable(QString("network_bridge=%1").arg(bridge)));
	}

	QStringList cmdline = QStringList()
		<< VIRT_V2V
		<< "-i" << "libvirt"
		<< "--in-place";

	if (DBG_DEBUG >= GetLogLevel())
		cmdline << "-v" << "-x";

	cmdline << ::Uuid(vmUuid).toStringWithoutBrackets();

	QProcess process;
	HostUtils::RunCmdLineUtilityEx(cmdline.join(" "), process, V2V_RUN_TIMEOUT, NULL);

	WRITE_TRACE(DBG_DEBUG, "virt-v2v output:");
	foreach (const QString& s, QString(process.readAllStandardOutput()).split("\n"))
		WRITE_TRACE(DBG_DEBUG, "%s", qPrintable(s));
	if (QProcess::NormalExit != process.exitStatus() || process.exitCode() != 0)
	{
		WRITE_TRACE(DBG_FATAL, "Cannot convert VM to vz7 format: virt-v2v failed");
		foreach (const QString& s, QString(process.readAllStandardError()).split("\n"))
			WRITE_TRACE(DBG_DEBUG, "%s", qPrintable(s));
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

} // namespace Vm
} // namespace Legacy

