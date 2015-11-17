///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspLibvirt.h
///
/// Public interfaces of the libvirt interaction.
///
/// @author shrike
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __CDSPLIBVIRT_H__
#define __CDSPLIBVIRT_H__

#include <QThread>
#include <QWeakPointer>
#include <prlsdk/PrlTypes.h>
#include <libvirt/libvirt.h>
#include <libvirt/libvirt-qemu.h>
#include <libvirt/virterror.h>
#include <libvirt/libvirt-qemu.h>
#include <XmlModel/VmConfig/CVmConfiguration.h>
#include <XmlModel/NetworkConfig/CVirtualNetwork.h>
#include <XmlModel/HostHardwareInfo/CHwNetAdapter.h>
#include <boost/optional.hpp>
#include <Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h>
#include <Libraries/PrlCommonUtilsBase/SysError.h>
#include <utility>

class CSavedStateTree;

namespace Libvirt
{

namespace Error
{

///////////////////////////////////////////////////////////////////////////////
// struct Simple

struct Simple
{
	Simple(PRL_RESULT result_, const QString& str_ = QString())
		: m_data(std::make_pair(result_, str_))
	{
	}

	CVmEvent convertToEvent() const
	{
		CVmEvent e;
		e.setEventCode(m_data.first);
		e.addEventParameter(new CVmEventParameter(PVE::String, m_data.second, EVT_PARAM_DETAIL_DESCRIPTION));
		return e;
	}

	PRL_RESULT code() const
	{
		return m_data.first;
	}

protected:
	QString& details()
	{
		return m_data.second;
	}

private:
	std::pair<PRL_RESULT, QString> m_data;
};

///////////////////////////////////////////////////////////////////////////////
// struct Detailed

struct Detailed : Simple
{
	Detailed(PRL_RESULT result_)
		: Simple(result_)
	{
		#if (LIBVIR_VERSION_NUMBER > 1000004)
		const char* m = virGetLastErrorMessage();
		WRITE_TRACE(DBG_FATAL, "libvirt error %s", m ? : "unknown");
		details() = m;
		#endif
	}
};

} // namespace Error

typedef Prl::Expected<void, Error::Simple> Result;

namespace Tools
{
namespace Agent
{
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Performance

struct Performance
{
	explicit Performance(const QSharedPointer<virDomain>& domain_):
		m_domain(domain_)
	{
	}

	Result getCpu(quint64& nanoseconds_) const;
	Result getDisk() const;
	Result getMemory() const;
	Result getNetwork() const;

private:
	QSharedPointer<virDomain> m_domain;
};

namespace Exec
{
///////////////////////////////////////////////////////////////////////////////
// struct Waiter

struct Waiter : QObject {
private slots:
	void stop()
	{
		m_loop.exit(0);
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
// struct Exec

struct Exec {
	explicit Exec (const  QSharedPointer<virDomain>& domain_): m_domain(domain_)
	{
	}
	Prl::Expected<int, Error::Simple>
		runCommand(const QString& path, const QList<QString>& args, const QByteArray& stdIn);

	Prl::Expected<boost::optional<Result>, Error::Simple>
		getCommandStatus(int pid);
	::Libvirt::Result executeInAgent(const QString& cmd, QString& reply);
private:
	QSharedPointer<virDomain> m_domain;
};

}; //namespace Exec


///////////////////////////////////////////////////////////////////////////////
// struct Guest

struct Guest
{
	explicit Guest(const  QSharedPointer<virDomain>& domain_): m_domain(domain_)
	{
	}
 
	Result dumpMemory(const QString& path, QString& reply);
	Result dumpState(const QString& path, QString& reply);
	Result setUserPasswd(const QString& user, const QString& passwd);
	Prl::Expected<Exec::Future, Error::Simple>
		startProgram(const QString& path, const QList<QString>& args, const QByteArray& stdIn);
	Prl::Expected<Exec::Result, Error::Simple>
		runProgram(const QString& path, const QList<QString>& args, const QByteArray& stdIn);

private:
	Result execute(const QString& cmd, QString& reply);

	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Runtime

struct Runtime
{
	explicit Runtime(const QSharedPointer<virDomain>& domain_)
		: m_domain(domain_)
	{
	}

	Result setIoLimit(const CVmHardDisk& disk_, quint32 limit_);
	Result setIopsLimit(const CVmHardDisk& disk_, quint32 limit_);
	Result changeMedia(const CVmOpticalDisk& device_);
	Result setIoPriority(quint32 ioprio_);

private:
	Result setBlockIoTune(const CVmHardDisk& disk_, const char* param_, quint32 limit_);
	QSharedPointer<virDomain> m_domain;
};

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	explicit Unit(virDomainSnapshotPtr snapshot_);

	Unit getParent() const;
	Result getUuid(QString& dst_) const;
	Result getState(CSavedStateTree& dst_) const;
	Result revert();
	Result undefine();
	Result undefineRecursive();

private:
	QSharedPointer<virDomainSnapshot> m_snapshot;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(const QSharedPointer<virDomain>& domain_): m_domain(domain_)
	{
	}

	Unit at(const QString& uuid_) const;
	Result all(QList<Unit>& dst_) const;
	Result define(const QString& uuid_, Unit* dst_ = NULL);
	Result defineConsistent(const QString& uuid_, Unit* dst_ = NULL);

private:
	Result define(const QString& uuid_, quint32 flags_, Unit* dst_);

	QSharedPointer<virDomain> m_domain;
};

} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	explicit Unit(virDomainPtr domain_ = NULL);

	Result getUuid(QString& dst_) const;
	Result kill();
	Result shutdown();
	Result start();
	Result reboot();
	Result reset();
	Result pause();
	Result unpause();
	Result resume(const QString& sav_);
	Result suspend(const QString& sav_);
	Result undefine();
	Result getState(VIRTUAL_MACHINE_STATE& dst_) const;
	Result getConfig(CVmConfiguration& dst_, bool runtime_ = false) const;
	Result getConfig(QString& dst_, bool runtime_ = false) const;
	Result setConfig(const CVmConfiguration& value_);
	Result completeConfig(CVmConfiguration& config_);
	Performance getPerformance() const
	{
		return Performance(m_domain);
	}
	Guest getGuest() const
	{
		return Guest(m_domain);
	}
	Snapshot::List getSnapshot() const
	{
		return Snapshot::List(m_domain);
	}

	Runtime getRuntime() const;

private:
	char* getConfig(bool runtime_ = false) const;

	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(QSharedPointer<virConnect> link_): m_link(link_)
	{
	}

	Unit at(const QString& uuid_) const;
	Result all(QList<Unit>& dst_);
	Result define(const CVmConfiguration& config_, Unit* dst_ = NULL);

private:
	QSharedPointer<virConnect> m_link;
};

} // namespace Vm

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	explicit Unit(virNetworkPtr network_ = NULL);

	Result stop();
	Result start();
	Result undefine();
	Result getConfig(CVirtualNetwork& dst_) const;

private:
	QSharedPointer<virNetwork> m_network;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(QSharedPointer<virConnect> link_): m_link(link_)
	{
	}

	Unit at(const QString& uuid_) const;
	Result all(QList<Unit>& dst_) const;
	Result find(const QString& name_, Unit* dst_ = NULL) const;
	Result define(const CVirtualNetwork& config_, Unit* dst_ = NULL);

private:
	QSharedPointer<virConnect> m_link;
};

} // namespace Network

namespace Interface
{
///////////////////////////////////////////////////////////////////////////////
// struct Bridge

struct Bridge
{
	Bridge()
	{
	}
	Bridge(virInterfacePtr interface_, const CHwNetAdapter& master_);

	QString getName() const;
	const CHwNetAdapter& getMaster() const
	{
		return m_master;
	}
	Result stop();
	Result start();
	Result undefine();

private:
	CHwNetAdapter m_master;
	QSharedPointer<virInterface> m_interface;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(QSharedPointer<virConnect> link_): m_link(link_)
	{
	}

	Result all(QList<Bridge>& dst_) const;
	Result find(const QString& mac_, CHwNetAdapter& dst_) const;
	Result find(const QString& name_, Bridge& dst_) const;
	Result find(const CHwNetAdapter& eth_, Bridge& dst_) const;
	Result define(const CHwNetAdapter& eth_, Bridge& dst_);

private:
	QSharedPointer<virConnect> m_link;
};

} // namespace Interface

///////////////////////////////////////////////////////////////////////////////
// struct Hub

struct Hub
{
	Vm::List vms()
	{
		return Vm::List(m_link.toStrongRef());
	}
	Network::List networks()
	{
		return Network::List(m_link.toStrongRef());
	}
	Interface::List interfaces()
	{
		return Interface::List(m_link.toStrongRef());
	}
	void setLink(QSharedPointer<virConnect> value_)
	{
		m_link = value_.toWeakRef();
	}

private:
	QWeakPointer<virConnect> m_link;
};

} // namespace Agent
} // namespace Tools

extern Tools::Agent::Hub Kit;

///////////////////////////////////////////////////////////////////////////////
// struct Host

struct Host: QThread
{
protected:
        void run();

private:
        Q_OBJECT
};

} // namespace Libvirt

#endif // __CDSPLIBVIRT_H__

