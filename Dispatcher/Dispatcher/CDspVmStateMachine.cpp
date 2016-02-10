/*
 * Copyright (c) 2016 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <QString>
#include <cxxabi.h>
#include "CDspVmStateMachine.h"

namespace Vm
{
namespace State
{

QString demangle(const char* name_)
{
	const char* x = name_;
	int status = -1;
	char* y = abi::__cxa_demangle(x, NULL, NULL, &status);
	if (0 != status)
		return x;

	QString output = y;
	free(y);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

void Frontend::setName(const QString& value_)
{
	// NB. there is no home in a libvirt VM config. it is still required
	// by different activities. now we put it into the default VM folder
	// from a user profile. later this behaviour would be re-designed.
	m_name = value_;
	QString h = QDir(m_user->getUserDefaultVmDirPath())
		.absoluteFilePath(QString(m_name).append(VMDIR_DEFAULT_BUNDLE_SUFFIX));
	m_home = QFileInfo(QDir(h), VMDIR_DEFAULT_VM_CONFIG_FILE);
}

void Frontend::updateDirectory(PRL_VM_TYPE type_)
{
	typedef CVmDirectory::TemporaryCatalogueItem item_type;

	CDspVmDirManager& m = m_service->getVmDirManager();
	QScopedPointer<item_type> t(new item_type(getUuid(), getHome(), m_name));
	PRL_RESULT e = m.checkAndLockNotExistsExclusiveVmParameters
				(QStringList(), t.data());
	if (PRL_FAILED(e))
		return;

	QScopedPointer<CVmDirectoryItem> x(new CVmDirectoryItem());
	x->setVmUuid(getUuid());
	x->setVmName(m_name);
	x->setVmHome(getHome());
	x->setVmType(type_);
	x->setValid(PVE::VmValid);
	x->setRegistered(PVE::VmRegistered);
	e = m_service->getVmDirHelper().insertVmDirectoryItem(m_user->getVmDirectoryUuid(), x.data());
	if (PRL_SUCCEEDED(e))
		x.take();

	m.unlockExclusiveVmParameters(t.data());
}

void Frontend::setConfig(CVmConfiguration& value_)
{
	m_service->getVmConfigManager().saveConfig(SmartPtr<CVmConfiguration>
		(&value_, SmartPtrPolicy::DoNotReleasePointee),
		getHome(), m_user, true, true);
}

boost::optional<CVmConfiguration> Frontend::getConfig() const
{
	PRL_RESULT e = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration> x = CDspService::instance()->getVmDirHelper()
		.getVmConfigByUuid(m_user, getUuid(), e);
	if (PRL_FAILED(e) || !x.isValid())
		return boost::none;

	return *x;
}

} // namespace State
} // namespace Vm
