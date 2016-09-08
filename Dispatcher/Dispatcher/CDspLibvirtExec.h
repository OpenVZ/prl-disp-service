///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspLibvirtExec.h
///
/// Interfaces of the libvirt guest agent interaction.
///
/// @author yur
///
/// Copyright (c) 2016 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __CDSPLIBVIRTEXEC_H__
#define __CDSPLIBVIRTEXEC_H__

#include <QObject>
#include <QEventLoop>
#include <prlsdk/PrlTypes.h>
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <libvirt/libvirt.h>
#include <boost/tuple/tuple.hpp>
#include "CDspLibvirt.h"

namespace Libvirt
{

typedef Prl::Expected<void, ::Error::Simple> Result;

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
	bool signaled;
	QByteArray stdOut;
	QByteArray stdErr;
};

///////////////////////////////////////////////////////////////////////////////
// struct Future

struct Future {
	enum { MAX_TRANSIENT_FAILS = 10 };

	typedef boost::optional<Result> result_type;
	Future(const QSharedPointer<virDomain>& domain_, int pid_):
		m_domain(domain_), m_pid(pid_), m_failcnt(0)
	{
	}

	Libvirt::Result wait(int timeout = -1);
	void cancel();
	const result_type& getResult() const
	{
		return m_status;
	}

private:
	int calculateTimeout(int i) const;

	QSharedPointer<virDomain> m_domain;
	int m_pid;
	int m_failcnt;
	result_type m_status;
};

///////////////////////////////////////////////////////////////////////////////
// struct Request

struct Request {
	Request(const QString& path, const QList<QString>& args)
		: m_path(path), m_args(args), m_runInShell(false)
	{
	}
	void setEnvironment(const QList<QString>& env)
	{
		m_env = env;
	}
	void setRunInShell(bool val)
	{
		m_runInShell = val;
	}
	void setChannels(const QList< QPair<int, int> >& chs)
	{
		m_channels = chs;
	}
	QString getJson() const;

	const QString& getPath() const { return m_path; }
	const QStringList& getArgs() const { return m_args; }
	const QStringList& getEnv()  const { return m_env; }

private:
	QString m_path;
	QStringList m_args;
	QStringList m_env;
	QList< QPair<int, int> > m_channels;
	bool m_runInShell;
};

///////////////////////////////////////////////////////////////////////////////
// struct Exec

struct Exec {
	enum { 
		RETRIES = 30,
		INFINITE = -1
	};

	explicit Exec(const  QSharedPointer<virDomain>& domain_)
		: m_domain(domain_)
	{
	}
	Prl::Expected<int, Libvirt::Agent::Failure>
		runCommand(const Request& r);

	Prl::Expected<boost::optional<Result>, Libvirt::Agent::Failure>
		getCommandStatus(int pid);

	Prl::Expected<void, Libvirt::Agent::Failure>
		terminate(int pid);

	Prl::Expected<QString, Libvirt::Agent::Failure>
		executeInAgent(const QString& cmd, int retries = RETRIES);

private:
	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Device - subclass of QIODevice representing pipe end of a program
// running inside guest. Data is transferred via aux channel.

struct AuxChannel;

struct Device: QIODevice {
	explicit Device(QSharedPointer<AuxChannel> aux_)
		: m_client(0), m_channel(aux_) { }
	~Device()
	{
		close();
	}

	int getClient() const
	{
		return m_client;
	}
	void setClient(const int client_)
	{
		m_client = client_;
	}

	// QIODevice interface
	virtual bool open(QIODevice::OpenMode mode_);
	virtual void close();

protected:
	int m_client;
	QSharedPointer<AuxChannel> m_channel;
};

///////////////////////////////////////////////////////////////////////////////
// struct ReadDevice - subclass of QIODevice representing read-only pipe end of
// a program running inside guest. Data is transferred via aux channel.

struct ReadDevice: Device {
	explicit ReadDevice(QSharedPointer<AuxChannel> aux_)
		: Device(aux_), m_finished(false) { }

	void setEof();
	void appendData(const QByteArray &data_);

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
};

///////////////////////////////////////////////////////////////////////////////
// struct WriteDevice - subclass of QIODevice representing write-only pipe end of
// a program running inside guest. Data is transferred via aux channel.

struct WriteDevice: Device {
	explicit WriteDevice(QSharedPointer<AuxChannel> aux_)
		: Device(aux_) { }

	// QIODevice interface
	virtual bool open(QIODevice::OpenMode mode_);
	virtual void close();
	virtual qint64 readData(char *data_, qint64 maxSize_);
	virtual qint64 writeData(const char *data_, qint64 len_);
};

struct CidGenerator
{
	CidGenerator(): m_last(0)
	{
	}

	void release(int id_);
	boost::optional<int> acquire();

private:
	QMutex m_mutex;
	QSet<int> m_set;
	int m_last;
};

///////////////////////////////////////////////////////////////////////////////
// struct AuxChannel - wrapper over libvirt's Stream interface over aux channel
// for communication with guest's programs

struct AuxChannel : QObject {

	typedef struct AuxMessageHeader
	{
		quint32 magic;
		quint32 cid;
		quint32 length;
	} AuxMessageHeader;

	explicit AuxChannel(virStreamPtr stream_);
	~AuxChannel();

	void setGenerator(const QSharedPointer<CidGenerator>& generator_)
	{
		m_cidGenerator = generator_;
	}

	bool isOpen();
	void close();

	static void reactEvent(virStreamPtr st_, int events_, void *opaque_);
	void processEvent(int events_);

	bool addIoChannel(Device& device_);
	void removeIoChannel(int id_);

	int writeMessage(const QByteArray& data_, int client_);

private:
	Q_OBJECT
	void readMessage(const QByteArray& data_);
	void skipTrash(const QByteArray& data_);
	void restartRead();

	QMutex m_lock;
	virStreamPtr m_stream;
	AuxMessageHeader m_readHdr;
	quint32 m_read;
	QSharedPointer<CidGenerator> m_cidGenerator;
	QMap<int, Device *> m_ioChannels;
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
	explicit Screenshot(const QByteArray& data_) : m_data(data_) {}

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
	Prl::Expected< QList<boost::tuple<quint64,quint64,QString> >, ::Error::Simple>
		getFsInfo();
	Result checkAgent();
	Prl::Expected<QString, Libvirt::Agent::Failure> getAgentVersion(int retries = Exec::Exec::RETRIES);
	Prl::Expected<Exec::Future, Error::Simple> startProgram(const Exec::Request& r);
	Prl::Expected<Exec::Result, Error::Simple> runProgram(const Exec::Request& r);
	Prl::Expected<QString, Error::Simple>
		execute(const QString& cmd, bool isHmp = true);

private:
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
