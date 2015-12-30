///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmGuestPersonality.cpp
///
/// VM guest has a number of "personal" data (IP address, passwords, user list)
/// That class mounts iso image into VM with predefined cloud-init data
///
/// @author aburluka
///
/// Copyright (c) 2015-2016 Parallels IP Holdings GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#include "CDspVmGuestPersonality.h"
#include "CDspService.h"
#include "Tasks/Task_EditVm.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include <prlcommon/HostUtils/HostUtils.h>

namespace Personalize
{

bool Configurator::setNettool(const QStringList& args_) const
{
	QString iso(ParallelsDirs::getVmCloudConfigIsoPath(m_cfg.getVmIdentification()->getHomePath()));
	QString disp(ParallelsDirs::getVmDispCloudConfigPath(m_cfg.getVmIdentification()->getHomePath()));
	QStringList c;

	c << "/usr/libexec/cloud_config_ctl.py"
		<< QString("--input=") + disp
		<< QString("--output=") + disp
		<< "nettool-command"
		<< QString("\"") + "'" + args_.join("' '") + "'" + "\"";
	if (!execute(c))
		return false;

	c.clear();
	c << "/usr/libexec/cloud_config_ctl.py"
		<< QString("--output-iso=") + iso
		<< "merge"
		<< disp
		<< ParallelsDirs::getVmUserCloudConfigPath(m_cfg.getVmIdentification()->getHomePath())
		<< ParallelsDirs::getVmTemplateCloudConfigPath(m_cfg.getVmSettings()->getVmCommonOptions()->getOsVersion());
	return execute(c);
}

bool Configurator::setUserPassword(const QString& user_, const QString& passwd_, bool encrypted_) const
{
	QString o;
	QStringList c;
	QString iso = CFileHelper::GetFileRoot(m_cfg.getVmIdentification()->getHomePath());
	QString disp(ParallelsDirs::getVmDispCloudConfigPath(m_cfg.getVmIdentification()->getHomePath()));
	QProcess p;
	c << "/usr/libexec/cloud_config_ctl.py"
		<< QString("--input=") + disp
		<< QString("--output=") + disp
		<< "user" << user_
		<< "--password" << passwd_;
	if (!encrypted_)
		c << "--not-encrypted";

	if (!execute(c))
		return false;

	c.clear();
	c << "/usr/libexec/cloud_config_ctl.py"
		<< QString("--output-iso=") + iso
		<< "merge"
		<< disp
		<< ParallelsDirs::getVmUserCloudConfigPath(m_cfg.getVmIdentification()->getHomePath())
		<< ParallelsDirs::getVmTemplateCloudConfigPath(m_cfg.getVmSettings()->getVmCommonOptions()->getOsVersion());

	return execute(c);
}

bool Configurator::execute(const QStringList& args_) const
{
	QProcess p;
	QString cmd(args_.join(" "));
	WRITE_TRACE(DBG_FATAL, "Running cmd: %s", QSTR2UTF8(cmd));

	QString out;
	if (!HostUtils::RunCmdLineUtility(cmd, out, -1, &p)) {
		WRITE_TRACE(DBG_FATAL, "Cmd failed: %d", p.exitCode());
		return false;
	}

	return true;
}

} // namespace Personalize

void CDspVmGuestPersonality::slotVmConfigChanged(QString vmDirUuid_, QString vmUuid_)
{
	PRL_RESULT res;
	SmartPtr<CVmConfiguration> vm = CDspService::instance()->getVmDirHelper()
		.getVmConfigByUuid(vmDirUuid_, vmUuid_, res);
	if (!vm || PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "Unable to get %s config file", QSTR2UTF8(vmUuid_));
		return;
	}

	QString h(getHomeDir(vmDirUuid_, vmUuid_));
	if (h.isEmpty())
		return;

	QString p = ParallelsDirs::getVmCloudConfigIsoPath(h);
	CAuthHelper r;
	if (!CFileHelper::FileExists(p, &r))
	{
		WRITE_TRACE(DBG_FATAL, "Image %p does not exist", QSTR2UTF8(p));
		return;
	}

	quint32 i(0);
	foreach(CVmOpticalDisk* d, vm->getVmHardwareList()->m_lstOpticalDisks)
	{
		if (d->getEmulatedType() == PVE::CdRomImage &&
				d->getSystemName() == p &&
				d->getUserFriendlyName() == p)
			return;

		if (i < d->getIndex())
			i = d->getIndex();
	}

	CVmEvent e;
	e.addEventParameter(new CVmEventParameter(PVE::String,
				prepareNewCdrom(p, i), EVT_PARAM_VMCFG_NEW_DEVICE_CONFIG));
	Task_EditVm::atomicEditVmConfigByVm(vmDirUuid_, vmUuid_,
			e, CDspClient::makeServiceUser(vmDirUuid_));
}

QString CDspVmGuestPersonality::getHomeDir(const QString& dirUuid_, const QString& vmUuid_) const
{
	CDspLockedPointer<CVmDirectoryItem> p =
		CDspService::instance()->getVmDirManager().getVmDirItemByUuid(dirUuid_, vmUuid_);
	if (!p)
	{
		WRITE_TRACE(DBG_FATAL, "Unable to get dir item");
		return "";
	}
	return p->getVmHome();
}

QString CDspVmGuestPersonality::prepareNewCdrom(const QString& image_, quint32 index_) const
{
	CVmOpticalDisk d;
	d.setEnabled(PVE::DeviceEnabled);
	d.setConnected(PVE::DeviceConnected);
	d.setEmulatedType(PVE::CdRomImage);
	d.setSystemName(image_);
	d.setUserFriendlyName(image_);
	d.setIndex(index_ + 1);
	return d.toString();
}
