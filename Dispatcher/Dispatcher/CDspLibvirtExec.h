///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspLibvirtExec.h
///
/// Interfaces of the libvirt guest agent interaction.
///
/// @author yur
///
/// Copyright (c) 2016-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __CDSPLIBVIRTEXEC_H__
#define __CDSPLIBVIRTEXEC_H__

#include <QObject>
#include <QEventLoop>
#include <prlsdk/PrlTypes.h>
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <boost/tuple/tuple.hpp>
#include "CDspLibvirt.h"
#include <boost/serialization/strong_typedef.hpp>
#include <libvirt/libvirt.h>

struct _virStream;
typedef struct _virStream virStream;
typedef virStream *virStreamPtr;

namespace Libvirt
{
namespace Agent
{

struct Failure;

} // namespace Agent

namespace Instrument
{
namespace Agent
{
namespace Vm
{
namespace Exec
{
struct Callback;

///////////////////////////////////////////////////////////////////////////////
// struct ReadDevice - subclass of QIODevice representing read-only pipe end of
// a program running inside guest. Data is transferred via aux channel.

struct ReadDevice: QIODevice {
	explicit ReadDevice(virStreamPtr stream_);
		
	void setEof();
	void appendData(const QByteArray &data_);
	Libvirt::Result track(Callback* tracker_);

	// QIODevice interface
	virtual bool open(QIODevice::OpenMode mode_);
	virtual void close();
	virtual bool atEnd();
	virtual qint64 bytesAvailable();
	virtual qint64 readData(char *data_, qint64 maxSize_);
	virtual qint64 writeData(const char *data_, qint64 len_);

private:
	QMutex m_lock;
	bool m_finished;
	QByteArray m_data;
	QSharedPointer<virStream> m_stream;
};

///////////////////////////////////////////////////////////////////////////////
// struct WriteDevice - subclass of QIODevice representing write-only pipe end of
// a program running inside guest. Data is transferred via aux channel.

struct WriteDevice: QIODevice {
	explicit WriteDevice(virStreamPtr stream_);

	// QIODevice interface
	virtual bool open(QIODevice::OpenMode mode_);
	virtual void close();
	virtual qint64 readData(char *data_, qint64 maxSize_);
	virtual qint64 writeData(const char *data_, qint64 len_);

private:
	QMutex m_lock;
	QSharedPointer<virStream> m_stream;
};

///////////////////////////////////////////////////////////////////////////////
// struct Waiter

struct Waiter : QObject {
private slots:
	void stop()
	{
		m_loop.quit();
	}

public:
	void wait(int msecs)
	{
		QTimer::singleShot(msecs, this, SLOT(stop()));
		m_loop.exec();
	}

private:
	Q_OBJECT
	QEventLoop m_loop;
};

///////////////////////////////////////////////////////////////////////////////
// struct Result

struct Result {
	int exitcode;
	QProcess::ExitStatus exitStatus;
	QByteArray stdOut;
	QByteArray stdErr;
};

///////////////////////////////////////////////////////////////////////////////
// struct Request

struct Request {
	Request(const QString& path, const QList<QString>& args)
		: m_path(path), m_args(args), m_flags(0)
	{
	}

	void setEnvironment(const QList<QString>& env)
	{
		m_env = env;
	}
	void setRunInShell()
	{
		m_flags |= VIR_DOMAIN_COMMAND_X_EXEC_SHELL;
	}
	void setRunInTerminal()
	{
		m_flags |= VIR_DOMAIN_COMMAND_X_EXEC_TERMINAL;
	}

	int getFlags() const { return m_flags; }

	const QString& getPath()     const { return m_path; }
	const QStringList& getArgs() const { return m_args; }
	const QStringList& getEnv()  const { return m_env; }

private:
	QString m_path;
	QStringList m_args;
	QStringList m_env;
	int m_flags;
};

///////////////////////////////////////////////////////////////////////////////
// struct Workbench

struct Workbench
{
	int calculateTimeout(int index_) const;

	const char** translate(const QStringList& src_, QVector<char* >& dst_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Launcher
// NB: virDomainCommandXExec function has 13 arguments while boost::bind
// supports no more than 9. shame on designer of the libvirt api. thus we have
// to introduce this type to carry part of the function parameters before a
// call

struct Launcher
{
	explicit Launcher(const QSharedPointer<virDomain>& domain_);

	Launcher& setStdin(virStreamPtr value_)
	{
		m_stdin = value_;
		return *this;
	}
	Launcher& setStdout(virStreamPtr value_)
	{
		m_stdout = value_;
		return *this;
	}
	Launcher& setStderr(virStreamPtr value_)
	{
		m_stderr = value_;
		return *this;
	}
	Prl::Expected<int, Libvirt::Agent::Failure>
		operator()(const Request& request_) const;

private:
	virStreamPtr m_stdin;
	virStreamPtr m_stdout;
	virStreamPtr m_stderr;
	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit
// NB: there cannot be 2 copies of an exec thus noncopyable

struct Unit: boost::noncopyable, private Workbench
{
	typedef Prl::Expected<Result, PRL_RESULT> result_type;

	explicit Unit(const QSharedPointer<virDomain>& domain_);

	const result_type& getResult() const
	{
		return m_result;
	}
	ReadDevice* getStderr() const
	{
		return m_stderr.data();
	}
	ReadDevice* getStdout() const
	{
		return m_stdout.data();
	}
	WriteDevice* getStdin() const
	{
		return m_stdin.data();
	}
	Libvirt::Result start(const Request& request_);
	Libvirt::Result wait(int timeout_ = -1);
	void cancel();

private:
	typedef boost::optional<Launcher> launcher_type;

	Prl::Expected<void, ::Libvirt::Agent::Failure> query();

	int m_waitFailures;
	result_type m_result;
	launcher_type m_launcher;
	boost::optional<int> m_pid;
	QSharedPointer<virDomain> m_domain;
	QSharedPointer<ReadDevice> m_stdout;
	QSharedPointer<ReadDevice> m_stderr;
	QSharedPointer<WriteDevice> m_stdin;
};

///////////////////////////////////////////////////////////////////////////////
// struct Callback

struct Callback
{
	typedef QWeakPointer<ReadDevice> target_type;

	explicit Callback(const target_type& target_): m_target(target_)
	{
	}

	void operator()(virStreamPtr stream_, int events_);

	static void react(virStreamPtr stream_, int events_, void *opaque_);

private:
	target_type m_target;
};

} //namespace Exec

namespace Command
{
struct Future;

} // namespace Command

///////////////////////////////////////////////////////////////////////////////
// struct Screenshot

struct Screenshot
{
	explicit Screenshot(const QByteArray& data_) : m_data(data_), m_size(0, 0)
	{
	}

	void setSize(const QSize& size_)
	{
		m_size = size_;
	}
	PRL_RESULT save(const QString& to_, const QString& format_) const;

private:
	QByteArray m_data;
	QSize m_size;
};

///////////////////////////////////////////////////////////////////////////////
// struct Guest

struct Unit;
struct Guest
{
	enum { 
		RETRIES = 30,
		INFINITE = -1
	};
	explicit Guest(const QSharedPointer<virDomain>& domain_): m_domain(domain_)
	{
	}

	Result traceEvents(bool enable_); 
	Prl::Expected<Screenshot, ::Error::Simple> dumpScreen();
	Prl::Expected<QString, ::Error::Simple>
		dumpMemory(const QString& path);
	Prl::Expected<Command::Future, ::Error::Simple>
		dumpState(const QString& path_);
	Result setUserPasswd(const QString& user_, const QString& passwd_, bool crypted_);
	Result freezeFs();
	Result thawFs();
	Prl::Expected< QList<boost::tuple<quint64,quint64,QString,QString,QString> >, ::Error::Simple>
		getFsInfo();
	Result checkAgent();
	Prl::Expected<QString, Libvirt::Agent::Failure> getAgentVersion(int retries = RETRIES);
	Prl::Expected<QString, Error::Simple>
		execute(const QString& cmd, bool isHmp = true);

	typedef Exec::Result result_type;

	Prl::Expected<QSharedPointer<Exec::Unit>, Error::Simple>
		getExec() const;
	Prl::Expected<result_type, Error::Simple> runProgram(const Exec::Request& r);

private:
	Prl::Expected<QString, Libvirt::Agent::Failure>
		executeInAgent(const QString& cmd, int retries = RETRIES);

	QSharedPointer<virDomain> m_domain;
};

namespace Command
{
///////////////////////////////////////////////////////////////////////////////
// struct Future

struct Future
{
	Future(const QSharedPointer<virDomain>& domain_,
		const std::string& status_ = std::string())
	: m_guest(domain_), m_status(status_)
	{
	}

	Result wait(int timeout_);

private:
	Guest m_guest;
	std::string m_status;
};

} // namespace Command

} //namespace Vm
} //namespace Agent
} //namespace Instrument
} //namespace Libvirt

#endif // __CDSPLIBVIRTEXEC_H__
