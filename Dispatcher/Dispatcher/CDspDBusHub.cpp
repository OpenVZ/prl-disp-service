////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2008-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
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

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct SetFeatures

struct SetFeatures: std::unary_function<CDispCommonPreferences& , PRL_RESULT>
{
	result_type operator()(argument_type config_)
	{
		std::auto_ptr<CDispCpuPreferences> cpuMask(CCpuHelper::get_cpu_mask());
		if (!cpuMask.get()) {
			WRITE_TRACE(DBG_FATAL, "Unable to get new CPU features");
			return PRL_ERR_FAILURE;
		}
		config_.getCpuPreferences()->setFeatures(*cpuMask);

		return PRL_ERR_SUCCESS;
	};
};

} // namespace

///////////////////////////////////////////////////////////////////////////////
// struct CDspDBusHub

CDspDBusHub::CDspDBusHub()
	: m_bus(QDBusConnection::systemBus())
{
	if (!m_bus.isConnected())
	{
		WRITE_TRACE(DBG_FATAL, "Failed to get D-Bus connection");
		// TODO: retry on timer
		return;
	}

	if (!m_bus.connect("com.virtuozzo.cpufeatures", "/com/virtuozzo/cpufeatures",
		"com.virtuozzo.cpufeatures", "Sync", this, SLOT(slotCpuFeaturesSync())))
	{
		WRITE_TRACE(DBG_FATAL, "Unable to subscribe on Sync event");
	}
}

void CDspDBusHub::createDetached()
{
	QScopedPointer<CDspDBusHub> p(new CDspDBusHub());

	if (!p->connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), SLOT(deleteLater())))
		return;

	// make it fly (detach)
	p.take()->moveToThread(QCoreApplication::instance()->thread());
}

void CDspDBusHub::slotCpuFeaturesSync()
{
	WRITE_TRACE(DBG_INFO, "CPU features were synced");
	CDspService::instance()->updateCommonPreferences(SetFeatures());
}
