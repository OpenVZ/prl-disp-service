///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmGuest.cpp
///
/// Class for VM activity running inside guest agent
///
/// @author yur
///
/// Copyright (c) 2015-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#include "CDspVmStateMachine.h"
#include <prlcommon/PrlCommonUtilsBase/CFileHelper.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "CDspVmNetworkHelper.h"
#include <libvirt/virterror.h>

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

	WRITE_TRACE(DBG_INFO, "configureNetwork %s", qPrintable(cmd.join(" ")));
	if (!cmd.isEmpty()) {
		QString c = cmd.takeFirst();
		r.reset(new Libvirt::Instrument::Agent::Vm::Exec::Request(c, cmd));
		if (type == PVS_GUEST_TYPE_WINDOWS)
			r->setRunInShell();
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
	if (m_retries == 0)
	{
		deleteLater();
		return;
	}
	--m_retries;
	QRunnable* q = new Spin(m_ident, *this);
	q->setAutoDelete(true);
	QThreadPool::globalInstance()->start(q);
}

void Watcher::adopt(PRL_VM_TOOLS_STATE state_, const QString& version_)
{
	QSharedPointer<QAtomicInt> ref = m_incarnation.toStrongRef();
	if (!ref || m_ourIncarnation != *ref) {
		WRITE_TRACE(DBG_INFO, "watcher::adopt wrong incarnation, our %d, caller %d",
			m_ourIncarnation, ref ? int(*ref) : -1);
		return;
	}

	const QString& u = m_ident.first;
	Libvirt::Kit.vms().at(u).getMaintenance().emitAgentCorollary(state_);
	if (PTS_INSTALLED == state_)
	{
		CVmEvent e(PET_DSP_EVT_VM_TOOLS_STATE_CHANGED, u, PIE_DISPATCHER);
		e.addEventParameter(new CVmEventParameter(PVE::Integer, QString::number(state_),
				EVT_PARAM_VM_TOOLS_STATE));
		e.addEventParameter(new CVmEventParameter(PVE::String, version_, EVT_PARAM_VM_TOOLS_VERSION));
		SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, e);
		CDspService::instance()->getClientManager().sendPackageToVmClients(p, m_ident.second, u);

		emit guestToolsStarted(version_);
	}
}

void Watcher::respin()
{
	if (thread() == QThread::currentThread())
		startTimer(60000);
	else
		WRITE_TRACE(DBG_FATAL, "Bad thread for the call");
}

///////////////////////////////////////////////////////////////////////////////
// struct Spin

void Spin::run()
{
	Prl::Expected<QString, Libvirt::Agent::Failure> r =
		Libvirt::Kit.vms().at(m_ident.first).getGuest().getAgentVersion(0);
	if (r.isSucceed())
	{
		// emit or smth.
		WRITE_TRACE(DBG_INFO, "%s spin tools installed %s",
			qPrintable(m_ident.first), qPrintable(r.value()));
		m_watcher->adopt(PTS_INSTALLED, r.value());
	}
	else
	{
		PRL_VM_TOOLS_STATE s = PTS_NOT_INSTALLED;
		switch (r.error().getMainCode())
		{
		case VIR_ERR_OPERATION_INVALID:
			// domain is not running
			WRITE_TRACE(DBG_INFO, "%s Spin::run domain is not running",
				qPrintable(m_ident.first));
			break;
		case VIR_ERR_AGENT_UNRESPONSIVE:
			// agent is not started - retry 10 minutes with 20 secs interval
			WRITE_TRACE(DBG_INFO, "%s Spin::run agent not responed",
				qPrintable(m_ident.first));
			s = PTS_POSSIBLY_INSTALLED;
		default:
			WRITE_TRACE(DBG_INFO, "%s Spin::run retry %d errcode %d",
				qPrintable(m_ident.first), m_watcher->getRetries(), r.error().getMainCode());
			if (0 >= m_watcher->getRetries())
			{
				m_watcher->adopt(s);
				break;
			}
			// agent is not ready, retry
			return (void)QMetaObject::invokeMethod(m_watcher, "respin",
				Qt::QueuedConnection);
		}
	}
	m_watcher->deleteLater();
}

///////////////////////////////////////////////////////////////////////////////
// struct Connector

Connector::Connector(const QString& directory_, State::Frontend& frontend_,
		QWeakPointer<QAtomicInt> incarnation_) :
	m_retries(30), m_directory(directory_), m_frontend(&frontend_),
	m_incarnation(incarnation_)
{
}

Connector& Connector::setNetwork(Actor* value_)
{
	m_network.reset(value_);
	return *this;
}
Connector& Connector::setRetries(quint32 value_)
{
	m_retries = value_;
	return *this;
}
void Connector::operator()()
{
	Actor *a = new Actor(m_frontend->getConfigEditor());
	Watcher *p = new Watcher(MakeVmIdent(m_frontend->getUuid(), m_directory),
		m_incarnation);

	a->setParent(p);
	a->connect(p, SIGNAL(guestToolsStarted(const QString)),
		SLOT(setToolsVersionSlot(const QString)), Qt::DirectConnection);
	if (!m_network.isNull())
	{
		m_network->setParent(p);
		m_network->connect(p, SIGNAL(guestToolsStarted(const QString)),
			SLOT(configureNetworkSlot(const QString)), Qt::DirectConnection);
		m_network.take();
	}
	// usually guest agent is ready within 2..3 seconds after event
	// starting watcher earlier results in connect error logged
	p->setRetries(m_retries);
	p->startTimer(5000);
	p->moveToThread(QCoreApplication::instance()->thread());
}

} // namespace Guest
} // namespace Vm
