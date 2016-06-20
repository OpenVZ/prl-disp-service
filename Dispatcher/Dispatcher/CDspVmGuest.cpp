///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmGuest.cpp
///
/// Class for VM activity running inside guest agent
///
/// @author yur
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

#include "CDspService.h"
#include "Tasks/Task_EditVm.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include "CDspLibvirt.h"
#include "CDspLibvirtExec.h"
#include "CDspVmGuest.h"

namespace Vm {

///////////////////////////////////////////////////////////////////////////////
//class Guest

void Guest::setToolsVersion(CVmConfiguration& c_, const QString& v_)
{
	QString o = c_.getVmSettings()->getVmTools()->getAgentVersion();

	WRITE_TRACE(DBG_INFO, "agent version config %s runtime %s",
			qPrintable(o), qPrintable(v_));

	if (o != v_)
		c_.getVmSettings()->getVmTools()->setAgentVersion(v_);
}

void Guest::timerEvent(QTimerEvent *ev_)
{
	killTimer(ev_->timerId());

	Prl::Expected<QString, Libvirt::Agent::Failure> r =
		Libvirt::Kit.vms().at(m_uuid)
		.getGuest().getAgentVersion(0);
	if (r.isFailed()) {
		if (r.error().virErrorCode() == VIR_ERR_OPERATION_INVALID) {
			// domain is not running
			deleteLater();
			return;
		}
		// agent is not ready, retry
		startTimer(1000);
		return;
	}

	m_editor(boost::bind(&setToolsVersion, _1, boost::cref(r.value())));
	m_state.set_value(std::make_pair(PTS_INSTALLED, r.value()));

	// tools ready, stop checking
	deleteLater();
	return;
}

} //namespace Vm
