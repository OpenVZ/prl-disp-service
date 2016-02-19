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
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <libvirt/libvirt.h>

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
	Future(const QSharedPointer<virDomain>& domain_, int pid_): m_domain(domain_), m_pid(pid_)
	{
	}

	::Libvirt::Result wait(int timeout = 0);
	boost::optional<Result> getResult()
	{
		return m_status;
	}

private:
	int calculateTimeout(int i) const;

	QSharedPointer<virDomain> m_domain;
	int m_pid;
	boost::optional<Result> m_status;
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

private:
	QString m_path;
	QList<QString> m_args;
	QList<QString> m_env;
	QList< QPair<int, int> > m_channels;
	bool m_runInShell;
};

///////////////////////////////////////////////////////////////////////////////
// struct Exec

struct Exec {
	explicit Exec(const  QSharedPointer<virDomain>& domain_)
		: m_domain(domain_)
	{
	}
	Prl::Expected<int, Libvirt::Agent::Failure>
		runCommand(const Request& r);

	Prl::Expected<boost::optional<Result>, Libvirt::Agent::Failure>
		getCommandStatus(int pid);

	Prl::Expected<QString, Libvirt::Agent::Failure>
		executeInAgent(const QString& cmd);

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

	void setEof()
	{
		m_finished = true;
	}

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

	bool isOpen();
	void close();

	static void reactEvent(virStreamPtr st_, int events_, void *opaque_);
	void processEvent(int events_);

	void addIoChannel(Device& device_);
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
	int m_ioChannelCounter;
	QMap<int, Device *> m_ioChannels;
};

} //namespace Exec

namespace Command
{

struct Future;

} // namespace Command

///////////////////////////////////////////////////////////////////////////////
// struct Guest

struct Unit;
struct Guest
{
	explicit Guest(const QSharedPointer<virDomain>& domain_): m_domain(domain_)
	{
	}

	Result traceEvents(bool enable_); 
	Result dumpScreen(const QString& path);
	Prl::Expected<QString, ::Error::Simple>
		dumpMemory(const QString& path);
	Prl::Expected<Command::Future, ::Error::Simple>
		dumpState(const QString& path_);
	Result setUserPasswd(const QString& user_, const QString& passwd_, bool crypted_);
	Result checkAgent();
	Prl::Expected<QString, Error::Simple> getAgentVersion();
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
