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

namespace Secure
{
typedef ::Vm::Config::Edit::Atomic commit_type;

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

struct Sweeper
{
	static void cleanup(QProcess* victim_);
	static void cleanup(QTcpSocket* victim_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Subject

struct Subject
{
	Subject(quint16 peer_, const Api::Stunnel& api_);

	PRL_RESULT bringUpKeepAlive();
	PRL_RESULT startStunnel(quint16 begin_, quint16 end_);
	Tunnel* getResult();

private:
	quint16 m_peer;
	quint16 m_accept;
	Api::Stunnel m_api;
	QScopedPointer<QProcess, Sweeper> m_stunnel;
	QScopedPointer<QTcpSocket, Sweeper> m_keepAlive;
};

///////////////////////////////////////////////////////////////////////////////
// struct SetPort

struct SetPort
{
	typedef boost::function2<PRL_RESULT, const quint16, CVmConfiguration&> configure_type;

	SetPort(const commit_type &commit_, const configure_type &configure_):
		m_commit(commit_), m_configure(configure_)
	{
	}

	PRL_RESULT operator()(quint16 port_)
	{
		return m_commit(boost::bind(m_configure, port_, _1)); 
	}

private:
	commit_type m_commit;
	configure_type m_configure;
};

///////////////////////////////////////////////////////////////////////////////
// struct Backend

struct Backend: QRunnable
{
	typedef boost::function0<Subject* > subject_type;
	typedef QPair<quint16, quint16> range_type;
	typedef boost::function1<PRL_RESULT, const quint16> configure_type;

	Backend(const subject_type& subject_, const range_type& input_,
		const configure_type& commit_):
		m_commit(commit_), m_subject(subject_), m_setup(input_)
	{
	}

	void run();

private:
	configure_type m_commit;
	subject_type m_subject;
	range_type m_setup;
};

} // namespace Launch
} // namespace Secure
} // namespace Vnc

#endif //CDSPVNCSTARTER_P_H

