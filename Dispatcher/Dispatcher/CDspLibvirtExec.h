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
	Request (const QString& path, const QList<QString>& args)
		: m_path(path), m_args(args), m_runInShell(false)
	{
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
	QList< QPair<int, int> > m_channels;
	bool m_runInShell;
};

///////////////////////////////////////////////////////////////////////////////
// struct Exec

struct Exec {
	explicit Exec (const  QSharedPointer<virDomain>& domain_)
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
// struct ExecMessage - a container for data chunk to/from guest program

struct ExecMessage : QObject {
	ExecMessage() : m_size(0), m_client(0) {}
	ExecMessage(const QByteArray &d_, const int client_);

	QByteArray getData();
	const QByteArray & getRawData() const
	{
		return m_data;
	}

	void appendData(const QByteArray &d_);
	void resetData();

	int getClient()
	{
		return m_client;
	}

signals:
	void DataReady();
	void Eof();

private:
	Q_OBJECT
	void processData();

	QByteArray m_data;
	QMutex m_lock;
	int m_size;
	int m_client;
};

///////////////////////////////////////////////////////////////////////////////
// struct AsyncExecDevice - subclass of QIODevice representing asynchronous
// communications with a program running in guest over aux channel

struct AuxChannel;

struct AsyncExecDevice: QIODevice {
	AsyncExecDevice() : m_client(0), m_finished(false) {}
	~AsyncExecDevice();

	int getClient()
	{
		return m_client;
	}
	void setClient(const int client_)
	{
		m_client = client_;
	}


	void bindChannel(const QSharedPointer<AuxChannel> & aux_);
	void appendData(const QByteArray &d_);

	// QIODevice interface
	virtual bool open(QIODevice::OpenMode mode_);
	virtual void close();
	virtual qint64 bytesAvailable() const;
	virtual qint64 readData(char *data_, qint64 maxSize_);
	virtual qint64 writeData(const char *data_, qint64 len_);

private:
	int m_client;
	QMutex m_lock;
	bool m_finished;
	QSharedPointer<AuxChannel> m_channel;
	QByteArray m_data;
};

///////////////////////////////////////////////////////////////////////////////
// struct AuxChannel - wrapper over libvirt's Stream interface over aux channel
// for communication with guest's programs

struct AuxChannel : QObject {
	AuxChannel(QSharedPointer<virDomain>& domain_, QWeakPointer<virConnect>& link_)
		: m_domain(domain_), m_link(link_), m_stream(NULL)
		, m_ioChannelCounter(0)
	{
		QObject::connect(&m_readMsg, SIGNAL(DataReady()),
				SLOT(readMessage()), Qt::DirectConnection);
		QObject::connect(&m_readMsg, SIGNAL(Eof()),
				SLOT(readMessage()), Qt::DirectConnection);
	}
	~AuxChannel();

	bool open();
	bool isOpen();

	static void reactEvent(virStreamPtr st_, int events_, void *opaque_);
	void processEvent(int events_);

	int addIoChannel(AsyncExecDevice &d_);
	void removeIoChannel(int id_);

	int writeMessage(const ExecMessage & msg_);

public slots:
	void readMessage();

private:
	Q_OBJECT
	QSharedPointer<virDomain> m_domain;
	QWeakPointer<virConnect> m_link;
	virStreamPtr m_stream;
	ExecMessage m_readMsg;
	int m_ioChannelCounter;
	QMap<int, AsyncExecDevice *> m_ioChannels;
};

} //namespace Exec

} //namespace Vm
} //namespace Agent
} //namespace Instrument
} //namespace Libvirt

#endif // __CDSPLIBVIRTEXEC_H__
