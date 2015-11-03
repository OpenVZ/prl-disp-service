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
#include <XmlModel/VmConfig/CVmConfiguration.h>
#include <XmlModel/NetworkConfig/CVirtualNetwork.h>
#include <XmlModel/HostHardwareInfo/CHwNetAdapter.h>

class CSavedStateTree;

namespace Libvirt
{
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

	PRL_RESULT getCpu(quint64& nanoseconds_) const;
	PRL_RESULT getDisk() const;
	PRL_RESULT getMemory() const;
	PRL_RESULT getNetwork() const;

private:
	QSharedPointer<virDomain> m_domain;
};

///////////////////////////////////////////////////////////////////////////////
// struct Guest

struct Guest
{
	explicit Guest(const  QSharedPointer<virDomain>& domain_): m_domain(domain_)
	{
	}

	PRL_RESULT dumpMemory(const QString& path, QString& reply);
	PRL_RESULT dumpState(const QString& path, QString& reply);

private:
	PRL_RESULT execute(const QString& cmd, QString& reply);

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
	PRL_RESULT getUuid(QString& dst_) const;
	PRL_RESULT getState(CSavedStateTree& dst_) const;
	PRL_RESULT revert();
	PRL_RESULT undefine();
	PRL_RESULT undefineRecursive();

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
	PRL_RESULT all(QList<Unit>& dst_) const;
	PRL_RESULT define(const QString& uuid_, Unit* dst_ = NULL);
	PRL_RESULT defineConsistent(const QString& uuid_, Unit* dst_ = NULL);

private:
	PRL_RESULT define(const QString& uuid_, quint32 flags_, Unit* dst_);

	QSharedPointer<virDomain> m_domain;
};

} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	explicit Unit(virDomainPtr domain_ = NULL);

	PRL_RESULT getUuid(QString& dst_) const;
	PRL_RESULT kill();
	PRL_RESULT shutdown();
	PRL_RESULT start();
	PRL_RESULT reboot();
	PRL_RESULT pause();
	PRL_RESULT unpause();
	PRL_RESULT resume(const QString& sav_);
	PRL_RESULT suspend(const QString& sav_);
	PRL_RESULT undefine();
	PRL_RESULT changeMedia(const CVmOpticalDisk& device_);
	PRL_RESULT getState(VIRTUAL_MACHINE_STATE& dst_) const;
	PRL_RESULT getConfig(CVmConfiguration& dst_, bool runtime_ = false) const;
	PRL_RESULT getConfig(QString& dst_, bool runtime_ = false) const;
	PRL_RESULT setConfig(const CVmConfiguration& value_);
	PRL_RESULT completeConfig(CVmConfiguration& config_);
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

private:
	char* getConfig(bool runtime_ = false) const
	{
		return virDomainGetXMLDesc(m_domain.data(), VIR_DOMAIN_XML_SECURE
			| runtime_ ? 0 : VIR_DOMAIN_XML_INACTIVE);
	}

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
	PRL_RESULT all(QList<Unit>& dst_);
	PRL_RESULT define(const CVmConfiguration& config_, Unit* dst_ = NULL);

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

	PRL_RESULT stop();
	PRL_RESULT start();
	PRL_RESULT undefine();
	PRL_RESULT getConfig(CVirtualNetwork& dst_) const;

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
	PRL_RESULT all(QList<Unit>& dst_) const;
	PRL_RESULT find(const QString& name_, Unit* dst_ = NULL) const;
	PRL_RESULT define(const CVirtualNetwork& config_, Unit* dst_ = NULL);

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
	PRL_RESULT stop();
	PRL_RESULT start();
	PRL_RESULT undefine();

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

	PRL_RESULT all(QList<Bridge>& dst_) const;
	PRL_RESULT find(const QString& mac_, CHwNetAdapter& dst_) const;
	PRL_RESULT find(const QString& name_, Bridge& dst_) const;
	PRL_RESULT find(const CHwNetAdapter& eth_, Bridge& dst_) const;
	PRL_RESULT define(const CHwNetAdapter& eth_, Bridge& dst_);

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

