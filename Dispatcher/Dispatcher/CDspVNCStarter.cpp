///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
///	CDspVNCStarter.cpp
///
/// @brief
///	Implementation of the class CDspVNCStarter
///
/// @brief
///	Manages VNC server process: starts, terminates.
///
/// @author sergeyt
///	romanp@
///
/// @history
///
///////////////////////////////////////////////////////////////////////////////

#include <QCoreApplication>
#include <QStringList>

#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Interfaces/ParallelsSdkPrivate.h>
#include "CDspVNCStarter.h"
#include "CDspService.h"
#include "CDspVNCStarter_p.h"
#include <prlcommon/Logging/Logging.h>
#include <boost/functional/factory.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/statement.hpp>
#include <boost/phoenix/object/delete.hpp>
#include <boost/phoenix/core/reference.hpp>
#include <boost/phoenix/bind/bind_function_object.hpp>
#include <boost/phoenix/bind/bind_member_function.hpp>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


/*****************************************************************************/

#ifdef _WIN_
    const QString VNCServerApp("prl_vncserver_app.exe");
#else
    const QString VNCServerApp("prl_vncserver_app");
#endif

/*****************************************************************************/

namespace
{
const char PRL_KEY_TO_STORE_VNC_SSL_PRIVATE_KEY[] = "{FF71877B-B0B2-4EB3-9061-56BC43ED2713}";
const char PRL_KEY_TO_STORE_VNC_SSL_CERTIFICATE[] = "{76473A08-BE1D-4930-9900-FAA313314E8E}";
} // namespace

namespace Vnc
{
namespace Api
{
///////////////////////////////////////////////////////////////////////////////
// struct Server

Server::Server(const QString& binary_, const QString& vm_, const CVmRemoteDisplay& display_):
	m_vm(vm_), m_binary(binary_), m_hostname(display_.getHostName()),
	m_password(display_.getPassword()), m_envp(QProcessEnvironment::systemEnvironment())
{
	QString a = QCoreApplication::applicationDirPath();
#if _LIN_
	m_envp.insert("LD_LIBRARY_PATH", a);
#else
	m_envp.insert("PATH", a);
#endif
}

QStringList Server::operator()(quint16 port_) const
{
	return do_(QStringList() << "--port" << QString("%1").arg(port_));
}

QStringList Server::operator()(quint16 start_, quint16 end_) const
{
	QStringList x;
	x << "--auto-port" << "--min-port" << QString("%1").arg(start_)
		<< "--max-port" << QString("%1").arg(end_);

	return do_(x);
}

QStringList Server::do_(const QStringList& aux_) const
{
	QStringList output;
	if (!m_password.isEmpty())
		output << "--passwd";

	return output << "--listen" << m_hostname << aux_ << m_vm;
}

///////////////////////////////////////////////////////////////////////////////
// struct Stunnel

Stunnel::Stunnel(const QByteArray& key_, const QByteArray& certificate_):
	m_key(key_), m_certificate(certificate_)
{
}

PRL_RESULT Stunnel::prepare(QTemporaryFile& dst_, const QByteArray data_)
{
	if (!dst_.open())
	{
		WRITE_TRACE(DBG_FATAL, "Error: cannot open a temporary file");
		return PRL_ERR_OPEN_FAILED;
	}
	dst_.setAutoRemove(true);
	if (-1 == dst_.write(data_) || !dst_.flush())
	{
		WRITE_TRACE(DBG_FATAL, "Error: cannot write to a temporary file");
		return PRL_ERR_WRITE_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Stunnel::key(QTemporaryFile& dst_) const
{
	dst_.setFileTemplate(QString("%1/key.XXXXXX.pem").arg(QDir::tempPath()));
	return prepare(dst_, m_key);
}

PRL_RESULT Stunnel::certificate(QTemporaryFile& dst_) const
{
	dst_.setFileTemplate(QString("%1/crt.XXXXXX.pem").arg(QDir::tempPath()));
	return prepare(dst_, m_certificate);
}

} // namespace Api

///////////////////////////////////////////////////////////////////////////////
// struct Scope

Scope::range_type Scope::getAutoRange() const
{
	CDspLockedPointer<CDispCommonPreferences> p = m_service->getDispConfigGuard()
                        .getDispCommonPrefs();

	return range_type(p->getRemoteDisplayPreferences()->getMinPort(),
			p->getRemoteDisplayPreferences()->getMaxPort());
}

boost::optional<Api::Stunnel> Scope::getEncryption() const
{
	QByteArray c, k;
	if (Vnc::Encryption(*(m_service->getQSettings().getPtr())).state(k, c))
		return Api::Stunnel(k, c);

	return boost::none;
}

namespace Socat
{
///////////////////////////////////////////////////////////////////////////////
// struct Encryption

Encryption::result_type Encryption::operator()()
{
	if (m_key.isNull())
	{
		PRL_RESULT e;
		QSharedPointer<QTemporaryFile> k(new QTemporaryFile());
		if (PRL_FAILED(e = key(*k)))
			return e;

		QSharedPointer<QTemporaryFile> c(new QTemporaryFile());
		if (PRL_FAILED(e = certificate(*c)))
			return e;

		m_key = k;
		m_certificate = c;
	}

	return qMakePair(m_key->fileName(), m_certificate->fileName());
}

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

Visitor::result_type Visitor::operator()
	(const boost::mpl::at_c<Launcher::mode_type::types, 0>::type& insecure_) const
{
	return bind("TCP4-LISTEN", QHostAddress(insecure_));
}

Visitor::result_type Visitor::operator()
	(boost::mpl::at_c<Launcher::mode_type::types, 1>::type& secure_) const
{
	Encryption::result_type e = secure_.second();
	if (e.isFailed())
		return e.error();

	return bind("OPENSSL-LISTEN", QHostAddress(secure_.first))
		.append(",verify=0,pf=ip4")
		.append(",key=").append(e.value().first)
		.append(",cert=").append(e.value().second);
}

QString Visitor::bind(const char* spell_, const QHostAddress& name_) const
{
	QString output = QString(spell_).append(":")
		.append(QString::number(m_port))
		.append(",reuseaddr,fork,linger=0");
	if (!name_.isNull() && !(name_ == QHostAddress::Any || name_ == QHostAddress::AnyIPv6))
		output.append(",bind=").append(name_.toString());

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Launcher

Launcher& Launcher::setTarget(const QHostAddress& address_, quint16 port_)
{
	switch (address_.protocol())
	{
	case QAbstractSocket::IPv4Protocol:
		m_server = qMakePair(QHostAddress(QHostAddress::LocalHost), port_);
		m_target = QStringList() << QString("TCP4:")
			.append(m_server.first.toString())
			.append(":").append(QString::number(port_));
		break;
	case QAbstractSocket::IPv6Protocol:
		m_server = qMakePair(QHostAddress(QHostAddress::LocalHostIPv6), port_);
		m_target = QStringList() << QString("TCP6:[")
			.append(m_server.first.toString())
			.append("]:").append(QString::number(port_));
		break;
	default:
		m_target.clear();
		m_server = qMakePair(QHostAddress(QHostAddress::Null), port_);
	}
	return *this;
}

PRL_RESULT Launcher::operator()(quint16 accept_, QProcess& process_)
{
	if (m_target.isEmpty())
		return PRL_ERR_UNINITIALIZED;

	Visitor::result_type a = boost::apply_visitor(Visitor(accept_), m_mode);
	if (a.isFailed())
		return a.error();

	process_.start("/usr/bin/socat", QStringList() << "-d" << "-d"
		<< "-lyuser" << a.value() << m_target);
	if (!process_.waitForStarted(WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS))
	{
		WRITE_TRACE(DBG_FATAL, "Error: can't start socat");
		return PRL_ERR_FAILED_TO_START_VNC_SERVER;
	}
	if (process_.waitForFinished(WAIT_TO_EXIT_VNC_SERVER_AFTER_START))
	{
		WRITE_TRACE(DBG_FATAL, "Error: the socat has quitted unexectedly");
		return PRL_ERR_UNEXPECTED;
	}
	return PRL_ERR_SUCCESS;
}

} // namespace Socat

namespace Secure
{
namespace Launch
{
///////////////////////////////////////////////////////////////////////////////
// struct Sweeper

void Sweeper::cleanup(QProcess* victim_)
{
	if (NULL == victim_)
		return;

	WRITE_TRACE(DBG_FATAL, "Try to stop a task by sending KILL signal");
	victim_->kill();
	if (!victim_->waitForFinished(WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS))
		WRITE_TRACE(DBG_FATAL, "Can't stop the task");

	delete victim_;
}

void Sweeper::cleanup(QTcpSocket* victim_)
{
	if (NULL != victim_)
	{
		victim_->disconnectFromHost();
		delete victim_;
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct SetPort

PRL_RESULT SetPort::operator()(quint16 port_)
{
	PRL_RESULT e = m_commit(boost::bind(m_configure, port_, _1));
	if (PRL_FAILED(e))
		return e;

	m_service->getVmDirHelper()
		.sendVmConfigChangedEvent(m_commit.getObject(), SmartPtr<IOPackage>());

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Subject

Subject::Subject(const Socat::Launcher& launcher_):
	m_accept(), m_launcher(launcher_)
{
}

PRL_RESULT Subject::startStunnel(quint16 begin_, quint16 end_)
{
	if (begin_ > end_)
		return PRL_ERR_INVALID_ARG;

	QSet<quint16> u;
	boost::mt19937 g(time(NULL));
	boost::uniform_int<quint16> d(begin_, end_);
	quint16 z = d.max() - d.min() + 1;
	while (u.size() != z)
	{
		quint16 p = d(g);
		if (u.constEnd() == u.insert(p))
			continue;

		m_stunnel.reset(new QProcess());
		if (PRL_SUCCEEDED(m_launcher(p, *m_stunnel)))
		{
			m_accept = p;
			return PRL_ERR_SUCCESS;
		}
	}
	m_stunnel.reset();
	return PRL_ERR_FAILURE;
}

PRL_RESULT Subject::bringUpKeepAlive()
{
	m_keepAlive.reset(new QTcpSocket());
	m_keepAlive->connectToHost(m_launcher.getServer().first,
		m_launcher.getServer().second);
	if (!m_keepAlive->waitForConnected(WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS))
		return PRL_ERR_CANT_CONNECT_TO_DISPATCHER;

	return PRL_ERR_SUCCESS;
}

Tunnel* Subject::getResult()
{
	QScopedPointer<Tunnel> y(new Tunnel(m_accept, m_keepAlive.data(), m_stunnel.data()));
	bool x = y->connect(m_stunnel.take(), SIGNAL(finished(int, QProcess::ExitStatus)),
			SLOT(reactFinish(int, QProcess::ExitStatus))) &&
			y->connect(m_keepAlive.take(), SIGNAL(disconnected()),
				SLOT(reactDisconnect())) &&
			y->connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
				SLOT(reactDisconnect()));
	if (!x) 
		y.reset();

	return y.take();
}

///////////////////////////////////////////////////////////////////////////////
// struct Feedback

void Feedback::generate(Tunnel* result_)
{
	if (NULL == result_)
		emit abort();
	else
	{
		result_->connect(result_, SIGNAL(ripped()), SLOT(deleteLater()));
		result_->moveToThread(thread());
		emit adopt(result_);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Script

void Script::operator()(Feedback& feedback_)
{
	QScopedPointer<Subject> s(m_subject());
	quint16 p = m_setup.first, e = m_setup.second;

	if (PRL_FAILED(s->startStunnel(p, e)))
		return feedback_.generate(NULL);

	if (PRL_FAILED(s->bringUpKeepAlive()))
		return feedback_.generate(NULL);

	Tunnel* t = s->getResult();
	if (NULL == t)
		return feedback_.generate(NULL);

	if (PRD_AUTO == m_sweepMode)
	{
		Sweeper* w = new Sweeper(m_commit);
		w->setParent(t);
		w->connect(t, SIGNAL(closed()), SLOT(reactRipped()));
	}
	p = t->getPort();
	feedback_.generate(t);
	m_commit(p);
}

} // namespace Launch

///////////////////////////////////////////////////////////////////////////////
// struct Tunnel

Tunnel::Tunnel(quint16 accept_, QTcpSocket* keepAlive_, QProcess* stunnel_):
	m_port(accept_), m_stunnel(stunnel_), m_keepAlive(keepAlive_)
{
	if (NULL != m_stunnel)
		m_stunnel->setParent(this);

	if (NULL != m_keepAlive)
		m_keepAlive->setParent(this);
}

void Tunnel::reactFinish(int, QProcess::ExitStatus)
{
	m_stunnel = NULL;
	if (NULL == m_keepAlive)
		emit ripped();
	else
	{
		emit closed();
		m_keepAlive->disconnectFromHost();
	}
}

void Tunnel::reactDisconnect()
{
	m_keepAlive = NULL;
	if (NULL == m_stunnel)
		emit ripped();
	else
	{
		emit closed();
		m_stunnel->terminate();
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Driver

Driver::Driver(): m_active(), m_liquidator(), m_starting()
{
	qRegisterMetaType<Tunnel* >("Tunnel*");
	qRegisterMetaType<Liquidator* >("Liquidator*");
	qRegisterMetaType<Launch::Script* >("Launch::Script*");
}

void Driver::stop(Liquidator* liquidator_)
{
	PRL_ASSERT(NULL != liquidator_);
	PRL_ASSERT(NULL == m_liquidator);

	m_liquidator = liquidator_;
	if (NULL != m_active)
	{
		m_liquidator->connect(m_active, SIGNAL(ripped()), SLOT(execute()));
		return;
	}
	if (NULL == m_starting)
		m_liquidator->execute();
}

void Driver::start(Launch::Script* script_)
{
	PRL_ASSERT(NULL != script_);

	if (NULL != m_liquidator)
	{
		delete script_;
		return;
	}
	m_pending.reset(script_);
	if (NULL != m_active)
		return;

	if (NULL == m_starting)
		launch();
}

void Driver::abort()
{
	PRL_ASSERT(NULL != m_starting);

	m_starting = NULL;
	if (NULL != m_liquidator)
		m_liquidator->execute();
	else if (!m_pending.isNull())
		launch();
}

void Driver::adopt(Tunnel* orphan_)
{
	PRL_ASSERT(NULL != orphan_);
	PRL_ASSERT(NULL != m_starting);

	m_active = orphan_;
	m_starting = NULL;

	connect(orphan_, SIGNAL(ripped()), SLOT(evict()));
	if (NULL != m_liquidator)
		m_liquidator->connect(orphan_, SIGNAL(ripped()), SLOT(execute()));
}

void Driver::evict()
{
	if (NULL == m_active)
	{
		WRITE_TRACE(DBG_FATAL, "second evict. it is anxiously");
		return;
	}
	PRL_ASSERT(static_cast<Tunnel* >(sender()) == m_active);

	disconnect(m_active);
	m_active = NULL;

	if (!m_pending.isNull())
		launch();
}

void Driver::launch()
{
	PRL_ASSERT(!m_pending.isNull());
	PRL_ASSERT(NULL == m_starting);

	Launch::Feedback* f = new Launch::Feedback();
	connect(f, SIGNAL(abort()), SLOT(abort()));
	connect(f, SIGNAL(adopt(Tunnel*)), SLOT(adopt(Tunnel*)));

	m_starting = m_pending.take();
	namespace bp = boost::phoenix;
	boost::function<void ()> l =
		(
			bp::bind(bp::ref(*m_starting), bp::ref(*f)),
			bp::delete_(m_starting),
			bp::bind(&Launch::Feedback::deleteLater, f)
		);
	QtConcurrent::run(l);
}

void Driver::collapse()
{
	connect(m_active, SIGNAL(closed()), SLOT(evict()));
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

Frontend::Frontend(const ::Vm::Config::Edit::Atomic& commit_, CDspService& service_):
	m_rfb(new Driver()), m_websocket(new Driver()), m_service(&service_),
	m_commit(commit_)
{
	m_rfb->moveToThread(QCoreApplication::instance()->thread());
	m_rfb->connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
		SLOT(collapse()));
	m_websocket->moveToThread(QCoreApplication::instance()->thread());
	m_websocket->connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
		SLOT(collapse()));
}

Frontend::~Frontend()
{
	QMetaObject::invokeMethod(m_rfb, "stop", Qt::AutoConnection,
		Q_ARG(Liquidator* , new Liquidator(*m_rfb)));
	QMetaObject::invokeMethod(m_websocket, "stop", Qt::AutoConnection,
		Q_ARG(Liquidator* , new Liquidator(*m_websocket)));
}

void Frontend::draw(CVmRemoteDisplay& object_, const CVmRemoteDisplay* runtime_,
	const range_type& playground_) const
{
	object_.setEncrypted(false);
	if (PRD_DISABLED == object_.getMode())
		return;

	if (NULL == runtime_)
		return;

	Scope s(*m_service);
	Socat::Launcher u(object_.getHostName());
	boost::optional<Vnc::Api::Stunnel> e = s.getEncryption();
	if (e)
	{
		u = Socat::Launcher(qMakePair(object_.getHostName(), Socat::Encryption(e.get())));
		object_.setEncrypted(true);
	}
	QHostAddress a(runtime_->getHostName());
	u.setTarget(a, runtime_->getPortNumber());
	Launch::Script::range_type d(playground_.first, playground_.second);
	Launch::Script* q = new Launch::Script(boost::bind(
			boost::factory<Launch::Subject* >(), u), d,
				Launch::SetPort(m_commit, &Traits::configure,
					*m_service));
	q->setSweepMode(object_.getMode());
	QMetaObject::invokeMethod(m_rfb, "start", Qt::AutoConnection,
				Q_ARG(Launch::Script* , q));

	if (runtime_->getWebSocketPortNumber() == runtime_->getPortNumber())
		return;

	u.setTarget(a, runtime_->getWebSocketPortNumber());
	// Start from auto-assigned and to max.
	d.first = d.second = runtime_->getWebSocketPortNumber();
	if (QAbstractSocket::IPv4Protocol == a.protocol())
		d = s.getAutoRange();

	q = new Launch::Script(boost::bind(
			boost::factory<Launch::Subject* >(), u), d,
				Launch::SetPort(m_commit, &Traits::configureWS,
					*m_service));
	q->setSweepMode(PRD_AUTO);
	QMetaObject::invokeMethod(m_websocket, "start", Qt::AutoConnection,
				Q_ARG(Launch::Script* , q));
}

void Frontend::setup(CVmConfiguration& object_, const CVmConfiguration& runtime_) const
{
	CVmRemoteDisplay* a = Traits::purify(&object_);
	if (NULL == a)
		return;

	range_type d;
	CVmRemoteDisplay* b = Traits::purify(&runtime_);
	if (PRD_AUTO == a->getMode())
	{
		Scope::range_type x = Scope(*m_service).getAutoRange();
		d.first = x.first;
		d.second = x.second; 
	}
	else if (NULL != b)
	{
		d.first = b->getPortNumber();
		d.second = b->getPortNumber();
	}
	draw(*a, b, d);
}

void Frontend::restore(CVmConfiguration& object_, const CVmConfiguration& runtime_) const
{
	CVmRemoteDisplay* a = Traits::purify(&object_);
	if (NULL == a)
		return;

	draw(*a, Traits::purify(&runtime_), range_type(a->getPortNumber(), a->getPortNumber()));
}

} // namespace Secure

///////////////////////////////////////////////////////////////////////////////
// class Guard

bool Guard::attach(QProcess* process_)
{
	if (NULL == process_ || m_process.get() != NULL ||
		thread() != QThread::currentThread())
		return false;

	m_process.reset(process_);
	moveToThread(process_->thread());
	connect(m_process.get(),
		SIGNAL(finished(int, QProcess::ExitStatus)),
		SLOT(finish(int, QProcess::ExitStatus)));
	QCoreApplication::postEvent(this, new QEvent(QEvent::User));
	return true;
}

void Guard::quit()
{
	QProcess* p = m_process.release();
	if (NULL != p)
	{
		p->deleteLater();
		emit stop();
	}
}

bool Guard::event(QEvent* event_)
{
	switch (event_->type())
	{
	case QEvent::User:
		if (NULL == m_process.get())
			break;

		if (QProcess::NotRunning == m_process->state())
			quit();
		break;
	case QEvent::DeferredDelete:
	{
		QProcess* p = m_process.release();
		if (NULL == p)
			break;

		p->terminate();
		if (!p->waitForFinished(WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS))
		{
			WRITE_TRACE(DBG_FATAL, "Try to stop task by sending the KILL signal");
			p->kill();
			if (!p->waitForFinished(WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS))
			{
				WRITE_TRACE(DBG_FATAL, "Can't stop task");
			}
		}
		p->deleteLater();
		break;
	}
	default:
		QObject::event(event_);	
		return false;
	}
	
	QObject::event(event_);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct Encryption

// m_storage(CDspService::instance()->getQSettings())
Encryption::Encryption(QSettings& storage_): m_storage(&storage_)
{
}

bool Encryption::enabled() const
{
	if (NULL == m_storage)
		return false;
	return m_storage->contains(PRL_KEY_TO_STORE_VNC_SSL_PRIVATE_KEY)
	    && m_storage->contains(PRL_KEY_TO_STORE_VNC_SSL_CERTIFICATE);
}

bool Encryption::state(QByteArray& key_, QByteArray& certificate_) const
{
	if (NULL == m_storage)
		return false;
	QVariant k = m_storage->value(PRL_KEY_TO_STORE_VNC_SSL_PRIVATE_KEY);
	QVariant c = m_storage->value(PRL_KEY_TO_STORE_VNC_SSL_CERTIFICATE);
	if (!(k.isValid() && c.isValid()))
		return false;

	key_ = k.toByteArray();
	certificate_ = c.toByteArray();
	return true;
}

void Encryption::set(const char* name_, const QString& value_)
{
	if (value_.isEmpty())
		m_storage->remove(name_);
	else
		m_storage->setValue(name_, QVariant(value_));

	m_storage->sync();
}

PRL_RESULT Encryption::setKey(const QString& value_)
{
	if (NULL == m_storage)
		return PRL_ERR_UNINITIALIZED;

	set(PRL_KEY_TO_STORE_VNC_SSL_PRIVATE_KEY, value_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Encryption::setCertificate(const QString& value_)
{
	if (NULL == m_storage)
		return PRL_ERR_UNINITIALIZED;

	set(PRL_KEY_TO_STORE_VNC_SSL_CERTIFICATE, value_);
	return PRL_ERR_SUCCESS;
}

namespace Component
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T>
Unit<T>::~Unit()
{
	QMutexLocker g(&m_mutex);
	Guard* x = m_guard.release();
	if (NULL == x)
		return;

	QCoreApplication::postEvent(x, new QEvent(QEvent::DeferredDelete));
}

template<class T>
PRL_RESULT Unit<T>::operator()(const CDspVNCStarter& observer_)
{
	QMutexLocker g(&m_mutex);
	if (NULL != m_guard.get())
		return PRL_ERR_VNC_SERVER_ALREADY_STARTED;

	g.unlock();
	QProcess* p = new QProcess;
	PRL_RESULT e = static_cast<T&>(*this).do_(*p);
	if (PRL_FAILED(e))
	{
		p->kill();
		p->deleteLater();
		return e;
	}
	// NB. we must move to the target thread after start not before
	// the process exec because the QProcess startProcess function
	// creates a QSocketNotifier object implicitly owned by this
	// not the target thread due to the design of Qt. that 
	// notifier won't be able to handle events from the monitor
	// when this thread passes away. thus we'll be unable to catch
	// the process exit for instance.
	p->moveToThread(QCoreApplication::instance()->thread());
	g.relock();
	if (QProcess::NotRunning == p->state() || m_guard.get() != NULL)
	{
		p->kill();
		p->deleteLater();
		return PRL_ERR_FAILED_TO_START_VNC_SERVER;
	}
	m_guard.reset(new Guard);
	QObject::connect(m_guard.get(), SIGNAL(stop()), &observer_, SLOT(doOnExit()));
	m_guard->attach(p);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Server

Server::Server(quint16 port_, const Api::Server& api_):
	m_port(port_), m_marker("Listening for VNC connections on TCP port "),
	m_api(api_), m_argv(api_(port_))
{
}

Server::Server(quint16 start_, quint16 end_, const Api::Server& api_):
	m_port(0), m_marker("Autoprobing selected TCP port "),
	m_api(api_), m_argv(api_(start_, end_))
{
}

PRL_RESULT Server::do_(QProcess& process_)
{
	WRITE_TRACE(DBG_FATAL, "the VNC server starts : %s %s",
			QSTR2UTF8(m_api.binary()), QSTR2UTF8(m_argv.join(" ")));

	process_.setProcessEnvironment(m_api.envp());
	process_.setReadChannel(QProcess::StandardError);

	// the QProcess creates QSocketNotifier in start method, which will be
	// owned by the current thread.
	process_.start(m_api.binary(), m_argv);
	if (!process_.waitForStarted(WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS))
	{
		WRITE_TRACE(DBG_FATAL, "Error: can't start the VNC");
		return PRL_ERR_FAILED_TO_START_VNC_SERVER;
	}
	// Write passwd to stdin of VNC process if passwd is not empty
	if (!m_api.password().isEmpty())
	{
		process_.write(QSTR2UTF8(m_api.password()));
		process_.waitForBytesWritten();
		process_.closeWriteChannel();
	}
	return port(process_);
}

PRL_RESULT Server::port(QProcess& process_)
{
	m_port = 0;
	for (bool ok = false; !ok;)
	{
		if (!process_.canReadLine () &&
			!process_.waitForReadyRead(WAIT_VNC_SERVER_TO_WRITE_AFTER_START))
		{
			process_.waitForFinished(1);
			if (QProcess::NotRunning == process_.state())
			{
				WRITE_TRACE(DBG_FATAL, "can't start VNC: It finished after "
					"process start by error %d %s",
					process_.exitCode(),
					QSTR2UTF8(process_.errorString()));
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Timeout reached while waiting for "
							"a reply from the VNC server.");
			}
			return PRL_ERR_FAILED_TO_START_VNC_SERVER;
		}
		QByteArray b = process_.readLine(1024).trimmed();
		if (b.isEmpty())
			continue;

		int i = b.indexOf(m_marker);
		if (-1 != i)
			m_port = b.remove(0, i + m_marker.size()).toUInt(&ok);
	}
	if (0 == m_port)
	{
		WRITE_TRACE(DBG_FATAL, "the VNC server start error: "
					"can't get port from the VNC server");
		return PRL_ERR_FAILED_TO_START_VNC_SERVER;
	}
	WRITE_TRACE(DBG_INFO, "the VNC server is listening on port %d", m_port);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Stunnel

Stunnel::Stunnel(quint16 accept_, quint16 connect_, const Api::Stunnel& api_):
	m_accept(accept_), m_connect(QString::number(connect_)), m_api(api_)
{
}

Stunnel::Stunnel(quint16 accept_, const QString& connect_, const Api::Stunnel& api_):
	m_accept(accept_), m_connect(connect_), m_api(api_)
{
}

PRL_RESULT Stunnel::do_(QProcess& process_)
{
	PRL_RESULT e;
	QTemporaryFile p;
	if (PRL_FAILED(e = m_api.key(p)))
		return e;
	QTemporaryFile c;
	if (PRL_FAILED(e = m_api.certificate(c)))
		return e;
	process_.start("/usr/bin/stunnel", QStringList() << "-fd" << "0");
	if (!process_.waitForStarted(WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS))
	{
		WRITE_TRACE(DBG_FATAL, "Error: can't start the stunnel");
		return PRL_ERR_FAILED_TO_START_VNC_SERVER;
	}
	process_.write(QSTR2UTF8(QString("accept=%1\ndebug=0\nforeground=yes\n"
			"[vncserver]\nclient=no\naccept=%1\nconnect=%2\n"
			"cert=%3\nkey=%4\n").arg(m_accept).arg(m_connect)
			.arg(c.fileName()).arg(p.fileName())));
	process_.waitForBytesWritten();
	process_.closeWriteChannel();
	if (process_.waitForFinished(WAIT_TO_EXIT_VNC_SERVER_AFTER_START))
	{
		WRITE_TRACE(DBG_FATAL, "Error: the stunnel has quitted unexectedly");
		return PRL_ERR_UNEXPECTED;
	}
	return PRL_ERR_SUCCESS;
}

} // namespace Component

namespace Starter
{
///////////////////////////////////////////////////////////////////////////////
// struct Raw

Raw::Raw(const Api::Server& api_, CDspVNCStarter& q_): Unit(q_), m_api(api_)
{
}

PRL_RESULT Raw::start(Component::Server* server_)
{
	std::auto_ptr<Component::Server> a(server_);
	PRL_RESULT e = (*a)(q());
	if (PRL_FAILED(e))
		return e;

	m_server = a;
	port(m_server->port());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Raw::start(quint16 port_)
{
	return start(new Component::Server(port_, m_api));
}

PRL_RESULT Raw::start(quint16 start_, quint16 end_)
{
	return start(new Component::Server(start_, end_, m_api));
}

///////////////////////////////////////////////////////////////////////////////
// struct Secure

Secure::Secure(const Api::Server& raw_, const Api::Stunnel& secure_, CDspVNCStarter& q_):
	Unit(q_), m_raw(raw_), m_secure(secure_)
{
}

PRL_RESULT Secure::start(quint16 port_)
{
	QTcpServer s;
	PRL_RESULT e = PRL_ERR_FAILED_TO_START_VNC_SERVER;
	if (!s.listen(QHostAddress::Any, port_))
		return e;
	std::auto_ptr<Component::Server> a(new Component::Server
					(DEFAULT_START, DEFAULT_END, m_raw));
	e = (*a)(q());
	s.close();
	if (PRL_FAILED(e))
		return e;

	std::auto_ptr<Component::Stunnel> b(new Component::Stunnel
					(port_, a->port(), m_secure));
	e = (*b)(q());
	if (PRL_FAILED(e))
		return e;

	m_server = a;
	m_stunnel = b;
	port(port_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Secure::start(quint16 start_, quint16 end_)
{
	std::auto_ptr<Component::Server> a(new Component::Server
					(DEFAULT_START, DEFAULT_END, m_raw));
	PRL_RESULT e = (*a)(q());
	if (PRL_FAILED(e))
		return e;

	for (; start_ <= end_; ++start_)
	{
		std::auto_ptr<Component::Stunnel> b(new Component::Stunnel
						(start_, a->port(), m_secure));
		if (PRL_SUCCEEDED(e = (*b)(q())))
		{
			m_server = a;
			m_stunnel = b;
			port(start_);
			return PRL_ERR_SUCCESS;
		}
	}
	return PRL_ERR_FAILED_TO_START_VNC_SERVER;
}

} // namespace Starter

///////////////////////////////////////////////////////////////////////////////
// struct Traits

CVmRemoteDisplay* Traits::purify(const CVmConfiguration* config_)
{
	if (NULL == config_)
		return NULL;

	CVmSettings* x = config_->getVmSettings();
	if (NULL == x)
		return NULL;

	return x->getVmRemoteDisplay();
}

PRL_RESULT Traits::configure(const quint16 value_, CVmConfiguration& dst_)
{
	CVmRemoteDisplay* b = purify(&dst_);
	if (NULL == b)
		return PRL_ERR_INVALID_ARG;

	b->setPortNumber(value_);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Traits::configureWS(const quint16 value_, CVmConfiguration& dst_)
{
	CVmRemoteDisplay* b = purify(&dst_);
	if (NULL == b)
		return PRL_ERR_INVALID_ARG;

	b->setWebSocketPortNumber(value_);
	return PRL_ERR_SUCCESS;
}

} // namespace Vnc

///////////////////////////////////////////////////////////////////////////////
// class CDspVNCStarter

CDspVNCStarter::CDspVNCStarter(): m_impl(NULL)
{
	moveToThread(QCoreApplication::instance()->thread());
}

CDspVNCStarter::~CDspVNCStarter ()
{
	Terminate();
}

bool CDspVNCStarter::IsRunning ()
{
	QMutexLocker lock(&m_mutex);
	return NULL != m_impl;
}

PRL_RESULT CDspVNCStarter::Start (
		const QString& app,
		const QString& sID,
		CVmRemoteDisplay *remDisplay,
		Vnc::range_type port )
{
	QMutexLocker g(&m_mutex);
	if (NULL != m_impl)
		return PRL_ERR_VNC_SERVER_ALREADY_STARTED;

	PRL_RESULT e;
	Vnc::Api::Server a(app, sID, *remDisplay);
	std::auto_ptr<Vnc::Starter::Unit> x(new Vnc::Starter::Raw(a, *this));
	{
		QByteArray c, k;
		if (Vnc::Encryption(*(CDspService::instance()->getQSettings().getPtr())).state(k, c))
		{
			a.hostname(QHostAddress(QHostAddress::LocalHost));
			Vnc::Api::Stunnel b(k, c);
			x.reset(new Vnc::Starter::Secure(a, b, *this));
		}
	}
	m_ticket = reinterpret_cast<uintptr_t>(x.get());
	g.unlock();

	if (PRD_AUTO == remDisplay->getMode())
		e = x->start(port.first, port.second);
	else
		e = x->start(remDisplay->getPortNumber());

	if (PRL_FAILED(e))
		return e;

	g.relock();
	if (!m_ticket || m_ticket.get() != reinterpret_cast<uintptr_t>(x.get()))
		return PRL_ERR_FAILED_TO_START_VNC_SERVER;

	m_ticket.reset();
	m_impl = x.release();
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspVNCStarter::Start (
		const QString& sVmUuid,
		CVmRemoteDisplay *remDisplay,
		Vnc::range_type port )
{
	return Start(VNCServerApp, sVmUuid, remDisplay, port);
}

PRL_RESULT CDspVNCStarter::Terminate ()
{
	LOG_MESSAGE(DBG_DEBUG, "Terminate");
	QMutexLocker g(&m_mutex);
	if (NULL == m_impl)
		return PRL_ERR_VNC_SERVER_NOT_STARTED;

	Vnc::Starter::Unit* x = m_impl;
	m_impl = NULL;
	m_ticket.reset();
	g.unlock();
	delete x;
	return PRL_ERR_SUCCESS;
}

PRL_UINT32 CDspVNCStarter::GetPort()
{
	QMutexLocker lock(&m_mutex);
	if (NULL == m_impl)
		return 0;

	return m_impl->port();
}

void CDspVNCStarter::doOnExit()
{
	WRITE_TRACE(DBG_FATAL, "VNC process exited");
	m_mutex.lock();
	Vnc::Starter::Unit* x = m_impl;
	m_impl = NULL;
	m_ticket.reset();
	m_mutex.unlock();
	delete x;
}

/*****************************************************************************/
