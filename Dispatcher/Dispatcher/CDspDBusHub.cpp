////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2008-2015 Parallels IP Holdings GmbH
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
/// @file
///  CDspDBusHub.cpp
///
/// @brief
///  Implementation of the class CDspDBusHub
///
/// @brief
///  This class implements dbus handling routines
///
/// @author aburluka
///
/// @date
///  2016-06-28
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////


#define FORCE_LOGGING_PREFIX "DspVmStateSender"

#include "CDspService.h"
#include "CDspDBusHub.h"
#include <QtDBus>
#include <QDBusInterface>
#include <prlcommon/Logging/Logging.h>
#include <Libraries/CpuFeatures/CCpuHelper.h>
#include "Tasks/Task_UpdateCommonPrefs.h"

CDspDBusHub::CDspDBusHub()
	: m_interface(new QDBusInterface("vz.cpufeatures", "/vz/cpufeatures",
				"vz.CpuFeatures", QDBusConnection::systemBus(), this))
{
	if (!m_interface->connection().connect("vz.cpufeatures", "/vz/cpufeatures",
				"vz.CpuFeatures", "Sync", this, SLOT(slotCpuFeaturesSync())))
	{
		WRITE_TRACE(DBG_FATAL, "Unable to subscribe on Sync event");
	}
}

CDspDBusHub::~CDspDBusHub()
{
}

struct SetFeatures: std::unary_function<void, CDispCommonPreferences&>
{
	void operator()(CDispCommonPreferences& config_)
	{
		std::auto_ptr<CDispCpuPreferences> cpuMask(CCpuHelper::get_cpu_mask());
		if (!cpuMask.get()) {
			WRITE_TRACE(DBG_FATAL, "Unable to get new CPU features");
			return;
		}
		config_.getCpuPreferences()->setFeatures(*cpuMask);
	};
};

void CDspDBusHub::slotCpuFeaturesSync()
{
	WRITE_TRACE(DBG_INFO, "CPU features were synced");
	CDspService::instance()->updateCommonPreferences(SetFeatures());
}

