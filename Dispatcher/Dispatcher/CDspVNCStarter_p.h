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
///	CDspVNCStarter_p.h
///
/// @brief
///	Private VNC management related stuff
///
/// @author
///	shrike@
///
///////////////////////////////////////////////////////////////////////////////

#ifndef CDSPVNCSTARTER_P_H
#define CDSPVNCSTARTER_P_H

#include <QObject>
#include <QProcess>
#include <QTcpSocket>
#include <QHostAddress>
#include <boost/mpl/at.hpp>
#include <CDspVmConfigManager.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

class CDspVNCStarter;

namespace Vnc
{
struct Guard;

///////////////////////////////////////////////////////////////////////////////
// struct Traits

struct Traits
{
	static CVmRemoteDisplay* purify(const CVmConfiguration* config_);
	static PRL_RESULT configure(const quint16 value_, CVmConfiguration& dst_);
	static PRL_RESULT configureWS(const quint16 value_, CVmConfiguration& dst_);
};

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

} // namespace Api

namespace Component
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T>
struct Unit
{
	~Unit();

	PRL_RESULT operator()(const CDspVNCStarter& observer_);
private:
	QMutex m_mutex;
	std::auto_ptr<Guard> m_guard;
};

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

///////////////////////////////////////////////////////////////////////////////
// struct Stunnel

struct Stunnel: Unit<Stunnel>
{
	Stunnel(quint16 accept_, quint16 connect_, const Api::Stunnel& api_);
	Stunnel(quint16 accept_, const QString& connect_, const Api::Stunnel& api_);

	PRL_RESULT do_(QProcess& process_);
private:
	quint16 m_accept;
	QString m_connect;
	Api::Stunnel m_api;
};

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


} // namespace Starter

namespace Socat
{
///////////////////////////////////////////////////////////////////////////////
// struct Encryption

struct Encryption: private Api::Stunnel
{
	typedef QPair<QString, QString> value_type;
	typedef Prl::Expected<value_type, PRL_RESULT> result_type;

	explicit Encryption(const Api::Stunnel& ssl_): Api::Stunnel(ssl_)
	{
	}

	result_type operator()();

private:
	QSharedPointer<QTemporaryFile> m_key;
	QSharedPointer<QTemporaryFile> m_certificate;
};

///////////////////////////////////////////////////////////////////////////////
// struct Launcher

struct Launcher
{
	typedef boost::variant<QString, QPair<QString, Encryption> > mode_type;

	explicit Launcher(const mode_type& mode_): m_mode(mode_)
	{
	}

	PRL_RESULT operator()(quint16 accept_, QProcess& process_);
	Launcher& setTarget(const QHostAddress& address_, quint16 port_);
	const QPair<QHostAddress, quint16>& getServer() const
	{
		return m_server;
	}

private:
	mode_type m_mode;
	QStringList m_target;
	QPair<QHostAddress, quint16> m_server;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor: boost::static_visitor<Prl::Expected<QString, PRL_RESULT> >
{
	explicit Visitor(quint16 port_): m_port(port_)
	{
	}

	result_type operator()
		(const boost::mpl::at_c<Launcher::mode_type::types, 0>::type& insecure_) const;
	result_type operator()
		(boost::mpl::at_c<Launcher::mode_type::types, 1>::type& secure_) const;

private:
	QString bind(const char* spell_, const QHostAddress& name_) const;

	const quint16 m_port;
};

} // namespace Socat

namespace Secure
{
typedef ::Vm::Config::Edit::Atomic commit_type;

///////////////////////////////////////////////////////////////////////////////
// struct Liquidator

struct Liquidator: QObject
{
	explicit Liquidator(QObject& parent_): QObject(&parent_)
	{
	}

public slots:
	void execute()
	{
		parent()->deleteLater();
	}

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Tunnel

struct Tunnel: QObject
{
	Tunnel(quint16 accept_, QTcpSocket* keepAlive_, QProcess* stunnel_);

	quint16 getPeer() const
	{
		return NULL == m_keepAlive ? 0 : m_keepAlive->peerPort();
	}
	quint16 getPort() const
	{
		return m_port;
	}

signals:
	void ripped();
	void closed();

public slots:
	void reactFinish(int, QProcess::ExitStatus);
	void reactDisconnect();

private:
	Q_OBJECT

	quint16 m_port;
	QProcess* m_stunnel;
	QTcpSocket* m_keepAlive;
};

namespace Launch
{
///////////////////////////////////////////////////////////////////////////////
// struct Sweeper

struct Sweeper: QObject
{
	typedef boost::function<PRL_RESULT(const quint16)> commit_type;

	explicit Sweeper(const commit_type& commit_): m_commit(commit_)
	{
	}

	static void cleanup(QProcess* victim_);
	static void cleanup(QTcpSocket* victim_);

public slots:
	void reactRipped()
	{
		WRITE_TRACE(DBG_DEBUG, "a VNC tunnel has been ripped");
		QtConcurrent::run(m_commit, 0);
	}

private:
	Q_OBJECT

	commit_type m_commit;
};

///////////////////////////////////////////////////////////////////////////////
// struct Subject

struct Subject
{
	explicit Subject(const Socat::Launcher& launcher_);

	PRL_RESULT bringUpKeepAlive();
	PRL_RESULT startStunnel(quint16 begin_, quint16 end_);
	Tunnel* getResult();

private:
	quint16 m_accept;
	Socat::Launcher m_launcher;
	QScopedPointer<QProcess, Sweeper> m_stunnel;
	QScopedPointer<QTcpSocket, Sweeper> m_keepAlive;
};

///////////////////////////////////////////////////////////////////////////////
// struct SetPort

struct SetPort
{
	typedef boost::function<PRL_RESULT(const quint16, CVmConfiguration&)>
		configure_type;

	SetPort(const commit_type &commit_, const configure_type &configure_,
		CDspService& service_):
		m_commit(commit_), m_service(&service_), m_configure(configure_)
	{
	}

	PRL_RESULT operator()(quint16 port_);

private:
	commit_type m_commit;
	CDspService* m_service;
	configure_type m_configure;
};

///////////////////////////////////////////////////////////////////////////////
// struct Feedback

struct Feedback: QObject
{
	void generate(Tunnel* result_);

signals:
	void abort();
	void adopt(Tunnel* orphan_);

private:
	Q_OBJECT
};

///////////////////////////////////////////////////////////////////////////////
// struct Script

struct Script
{
	typedef void result_type;
	typedef boost::function0<Subject* > subject_type;
	typedef QPair<quint16, quint16> range_type;
	typedef boost::function<PRL_RESULT(const quint16)> configure_type;

	Script(const subject_type& subject_, const range_type& input_,
		const configure_type& commit_):
		m_commit(commit_), m_subject(subject_), m_setup(input_),
		m_sweepMode()
	{
	}

	void operator()(Feedback& feedback_);
	void setSweepMode(PRL_VM_REMOTE_DISPLAY_MODE value_)
	{
		m_sweepMode = value_;
	}

private:
	configure_type m_commit;
	subject_type m_subject;
	range_type m_setup;
	PRL_VM_REMOTE_DISPLAY_MODE m_sweepMode;
};

} // namespace Launch

///////////////////////////////////////////////////////////////////////////////
// struct Driver

struct Driver: QObject
{
	Driver();

	Q_INVOKABLE void stop(Liquidator* liquidator_);
	Q_INVOKABLE void start(Launch::Script* script_);

public slots:
	void abort();
	void adopt(Tunnel* orphan_);
	void evict();
	void collapse();

private:
	Q_OBJECT
	
	void launch();

	Tunnel* m_active;
	Liquidator* m_liquidator;
	Launch::Script* m_starting;
	QScopedPointer<Launch::Script> m_pending;
};

} // namespace Secure

///////////////////////////////////////////////////////////////////////////////
// struct Scope

struct Scope
{
	typedef Secure::Launch::Script::range_type range_type;

	explicit Scope(CDspService& service_): m_service(&service_)
	{
	}

	range_type getAutoRange() const;
	boost::optional<Api::Stunnel> getEncryption() const;

private:
	CDspService* m_service;
};

} // namespace Vnc

#endif //CDSPVNCSTARTER_P_H

