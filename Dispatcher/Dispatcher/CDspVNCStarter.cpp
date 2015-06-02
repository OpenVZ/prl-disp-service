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

#include "Interfaces/ParallelsQt.h"
#include "Interfaces/ParallelsSdkPrivate.h"
#include "CDspVNCStarter.h"
#include "CDspService.h"
#include "Libraries/Logging/Logging.h"

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

struct Server
{
	Server(const QString& binary_, const QString& vm_,
		const CVmRemoteDisplay& display_);

	const QString& binary() const
	{
		return m_binary;
	}
	const QString& password() const
	{
		return m_password;
	}
	const QProcessEnvironment& envp() const
	{
		return m_envp;
	}
	void hostname(const QHostAddress& value_)
	{
		m_hostname = value_.toString();
	}
	QStringList operator()(quint16 port_) const;
	QStringList operator()(quint16 start_, quint16 end_) const;
private:
	QStringList do_(const QStringList& aux_) const;

	QString m_vm;
	QString m_binary;
	QString m_hostname;
	QString m_password;
	QProcessEnvironment m_envp;
};

Server::Server(const QString& binary_, const QString& vm_, const CVmRemoteDisplay& display_):
	m_vm(vm_), m_binary(binary_), m_hostname(display_.getHostName()),
	m_password(display_.getPassword()), m_envp(QProcessEnvironment::systemEnvironment())
{
	QString a = QCoreApplication::applicationDirPath();
	if (!CDspService::instance()->isServerModePSBM())
		m_binary = a  + "/" + m_binary;
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

struct Stunnel
{
	Stunnel(const QByteArray& key_, const QByteArray& certificate_);

	PRL_RESULT key(QTemporaryFile& dst_) const;
	PRL_RESULT certificate(QTemporaryFile& dst_) const;
private:
	static PRL_RESULT prepare(QTemporaryFile& dst_, const QByteArray data_);

	QByteArray m_key;
	QByteArray m_certificate;
};

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
	QObject::event(event_);
	if (QEvent::User == event_->type())
	{
		if (NULL == m_process.get())
			return true;

		if (QProcess::NotRunning == m_process->state())
			quit();

		return true;
	}
	if (QEvent::Close != event_->type())
		return false;
	QProcess* p = m_process.release();
	if (NULL == p)
		return true;

#ifdef _WIN_
	p->kill();
#else // _WIN_
	p->terminate();
#endif // _WIN_
	if (!p->waitForFinished(WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS))
	{
#ifndef _WIN_
		WRITE_TRACE(DBG_FATAL, "Try to stop task by sending the KILL signal");
		p->kill();
		if (!p->waitForFinished(WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS))
#endif // _WIN_
		{
			WRITE_TRACE(DBG_FATAL, "Can't stop task");
			p->deleteLater();
		}
	}
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
struct Unit
{
	~Unit()
	{
		QMutexLocker g(&m_mutex);
		Guard* x = m_guard.release();
		if (NULL == x)
			return;

		QCoreApplication::postEvent(x, new QEvent(QEvent::Close));
		QCoreApplication::postEvent(x, new QEvent(QEvent::DeferredDelete));
	}

	PRL_RESULT operator()(const CDspVNCStarter& observer_);
private:
	QMutex m_mutex;
	std::auto_ptr<Guard> m_guard;
};

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

struct Server: Unit<Server>
{
	Server(quint16 port_, const Api::Server& api_);
	Server(quint16 start_, quint16 end_, const Api::Server& api_);

	quint16 port() const
	{
		return m_port;
	}
	PRL_RESULT do_(QProcess& process_);
private:
	PRL_RESULT port(QProcess& process_);

	quint16 m_port;
	QString m_marker;
	Api::Server m_api;
	QStringList m_argv;
};

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
			!process_.waitForReadyRead(WAIT_TO_EXIT_VNC_SERVER_AFTER_START))
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

struct Stunnel: Unit<Stunnel>
{
	Stunnel(quint16 accept_, quint16 connect_, const Api::Stunnel& api_);

	PRL_RESULT do_(QProcess& process_);
private:
	quint16 m_accept;
	quint16 m_connect;
	Api::Stunnel m_api;
};

Stunnel::Stunnel(quint16 accept_, quint16 connect_, const Api::Stunnel& api_):
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
// struct Unit

struct Unit
{
	enum
	{
		DEFAULT_START = 5800,
		DEFAULT_END = 65535
	};
	virtual ~Unit()
	{
	}

	quint16 port() const
	{
		return m_port;
	}
	virtual PRL_RESULT start(quint16 port_) = 0;
	virtual PRL_RESULT start(quint16 start_, quint16 end_) = 0;
protected:
	Unit(CDspVNCStarter& q_): m_port(0), m_q(&q_)
	{
	}
	CDspVNCStarter& q() const
	{
		return *m_q;
	}
	void port(quint16 value_)
	{
		m_port = value_;
	}
private:
	quint16 m_port;
	CDspVNCStarter* m_q;
};

///////////////////////////////////////////////////////////////////////////////
// struct Raw

struct Raw: Unit
{
	Raw(const Api::Server& api_, CDspVNCStarter& q_);

	PRL_RESULT start(quint16 port_);
	PRL_RESULT start(quint16 start_, quint16 end_);
private:
	PRL_RESULT start(Component::Server* server_);

	const Api::Server m_api;
	std::auto_ptr<Component::Server> m_server;
};

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

struct Secure: Unit
{
	Secure(const Api::Server& raw_, const Api::Stunnel& secure_, CDspVNCStarter& q_);

	PRL_RESULT start(quint16 port_);
	PRL_RESULT start(quint16 start_, quint16 end_);
private:
	const Api::Server m_raw;
	const Api::Stunnel m_secure;
	std::auto_ptr<Component::Server> m_server;
	std::auto_ptr<Component::Stunnel> m_stunnel;
};

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
} // namespace Vnc

///////////////////////////////////////////////////////////////////////////////
// class CDspVNCStarter

CDspVNCStarter::CDspVNCStarter(): m_impl(NULL), m_ticket(NULL)
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
		PRL_UINT32 minPort )
{
	QMutexLocker g(&m_mutex);
	if (NULL != m_impl)
		return PRL_ERR_VNC_SERVER_ALREADY_STARTED;

	PRL_RESULT e;
	Vnc::Api::Server a(app, sID, *remDisplay);
	std::auto_ptr<Vnc::Starter::Unit> x(new Vnc::Starter::Raw(a, *this));
	if (CDspService::instance()->isServerModePSBM())
	{
		QByteArray c, k;
		if (Vnc::Encryption(*(CDspService::instance()->getQSettings().getPtr())).state(k, c))
		{
			a.hostname(QHostAddress(QHostAddress::LocalHost));
			Vnc::Api::Stunnel b(k, c);
			x.reset(new Vnc::Starter::Secure(a, b, *this));
		}
	}
	m_ticket = x.get();
	g.unlock();

	if (PRD_AUTO == remDisplay->getMode())
		e = x->start(minPort, Vnc::Starter::Unit::DEFAULT_END);
	else
		e = x->start(remDisplay->getPortNumber());

	if (PRL_FAILED(e))
		return e;

	g.relock();
	if (m_ticket != x.get())
		return PRL_ERR_FAILED_TO_START_VNC_SERVER;

	m_impl = x.release();
	m_ticket = NULL;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspVNCStarter::Start (
		const QString& sVmUuid,
		CVmRemoteDisplay *remDisplay,
		PRL_UINT32 minPort )
{
	return Start(VNCServerApp, sVmUuid, remDisplay, minPort);
}

PRL_RESULT CDspVNCStarter::Terminate ()
{
	LOG_MESSAGE(DBG_DEBUG, "Terminate");
	QMutexLocker g(&m_mutex);
	if (NULL == m_impl)
		return PRL_ERR_VNC_SERVER_NOT_STARTED;

	Vnc::Starter::Unit* x = m_impl;
	m_impl = NULL;
	m_ticket = NULL;
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
	m_ticket = NULL;
	m_mutex.unlock();
	delete x;
}

/*****************************************************************************/
