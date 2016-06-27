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
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include "CDspLibvirt.h"
#include "CDspLibvirtExec.h"
#include "CDspVmGuest.h"
#include "CDspVmNetworkHelper.h"

namespace Vm {

namespace Guest {

///////////////////////////////////////////////////////////////////////////////
//class Actor

void Actor::setToolsVersion(CVmConfiguration& c_, const QString& v_)
{
	QString o = c_.getVmSettings()->getVmTools()->getAgentVersion();

	WRITE_TRACE(DBG_INFO, "agent version config %s runtime %s",
			qPrintable(o), qPrintable(v_));

	if (o != v_)
		c_.getVmSettings()->getVmTools()->setAgentVersion(v_);
}

void Actor::configureNetwork(CVmConfiguration& c_, QString& uuid,
	QScopedPointer<Libvirt::Instrument::Agent::Vm::Exec::Request>& r)
{
	Network::Difference::Vm v(c_);
	int type = c_.getVmSettings()->getVmCommonOptions()->getOsType();
	QStringList cmd = v.calculate(CVmConfiguration(), type);
	if (!cmd.isEmpty()) {
		QString c = cmd.takeFirst();
		r.reset(new Libvirt::Instrument::Agent::Vm::Exec::Request(c, cmd));
		r->setRunInShell(type == PVS_GUEST_TYPE_WINDOWS);
		uuid = c_.getVmIdentification()->getVmUuid();
	}
}

void Actor::setToolsVersionSlot(const QString v_)
{
	m_editor(boost::bind(&setToolsVersion, _1, boost::cref(v_)));
}


void Actor::configureNetworkSlot(const QString v_)
{
	Q_UNUSED(v_);
	QString uuid;
	QScopedPointer<Libvirt::Instrument::Agent::Vm::Exec::Request> req;
	m_editor(boost::bind(&configureNetwork, _1, boost::ref(uuid), boost::ref(req)));
	if (!req.isNull())
		 runProgram(Libvirt::Kit.vms().at(uuid).getGuest(), uuid, *req);
}

Libvirt::Result Actor::runProgram(
	Libvirt::Instrument::Agent::Vm::Guest guest_,
	const QString& uuid_,
	const Libvirt::Instrument::Agent::Vm::Exec::Request& request_)
{
	QString cmdline = request_.getPath() + " " + request_.getArgs().join(" ");

	WRITE_TRACE(DBG_INFO, "vm: %s running: %s",
			qPrintable(uuid_),
			qPrintable(cmdline));

	Prl::Expected <Libvirt::Instrument::Agent::Vm::Exec::Result,
		Error::Simple> e = guest_.runProgram(request_);

	if (e.isFailed()) {
		WRITE_TRACE(DBG_FATAL, "vm: %s cannot run: %s error: 0x%08x", 
			qPrintable(uuid_),
			qPrintable(request_.getPath()),
			e.error().code());
		return e.error();
	} else {
		WRITE_TRACE(DBG_INFO, "vm: %s completed: %s exitcode: %d output: %s",
				qPrintable(uuid_),
				qPrintable(request_.getPath()),
				e.value().exitcode,
				qPrintable(QString::fromUtf8(e.value().stdOut) + "\n" +
					   QString::fromUtf8(e.value().stdErr)));
		return Libvirt::Result();
	}
}

///////////////////////////////////////////////////////////////////////////////
//class Watcher

void Watcher::timerEvent(QTimerEvent *ev_)
{
	killTimer(ev_->timerId());

	Prl::Expected<QString, Libvirt::Agent::Failure> r =
		Libvirt::Kit.vms().at(m_uuid).getGuest().getAgentVersion(0);
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

	m_state.set_value(PTS_INSTALLED);

	emit guestToolsStarted(r.value());

	// tools ready, stop checking
	deleteLater();
	return;
}

} // namespace Guest

} // namespace Vm
