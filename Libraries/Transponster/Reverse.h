///////////////////////////////////////////////////////////////////////////////
///
/// @file Reverse.h
///
/// Convertor from the SDK config format into the libvirt one.
///
/// @author shrike
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __REVERSE_H__
#define __REVERSE_H__

#include "iface_type.h"
#include "domain_type.h"
#include "network_type.h"
#include "snapshot_type.h"
#include <XmlModel/VmConfig/CVmConfiguration.h>
#include <XmlModel/NetworkConfig/CVirtualNetwork.h>
#include <XmlModel/HostHardwareInfo/CHwNetAdapter.h>

namespace Transponster
{
namespace Vm
{
namespace Reverse
{
///////////////////////////////////////////////////////////////////////////////
// struct Cdrom

struct Cdrom
{
	explicit Cdrom(const CVmOpticalDisk& input_): m_input(input_)
	{
	}

	PRL_RESULT operator()();

	QString getResult();

private:
	CVmOpticalDisk m_input;
	boost::optional<Libvirt::Domain::Xml::Disk> m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	explicit Vm(const CVmConfiguration& input_);

	PRL_RESULT setBlank();
	PRL_RESULT setIdentification();
	PRL_RESULT setSettings();
	PRL_RESULT setDevices();
	PRL_RESULT setResources();

	QString getResult();

private:
	CVmConfiguration m_input;
	boost::optional<Libvirt::Domain::Xml::Domain> m_result;
};

} // namespace Reverse
} // namespace Vm

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

struct Reverse
{
	explicit Reverse(const CVirtualNetwork& input_);

	PRL_RESULT setUuid();
	PRL_RESULT setName();
	PRL_RESULT setType();
	PRL_RESULT setBridge();
	PRL_RESULT setVlan();
	PRL_RESULT setHostOnly();

	QString getResult() const;

private:
	CVirtualNetwork m_input;
	Libvirt::Network::Xml::Network m_result;
};

} // namespace Network

namespace Interface
{
namespace Bridge
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

struct Reverse
{
	Reverse(const QString& name_, const CHwNetAdapter& master_):
		m_name(name_), m_master(master_)
	{
	}

	PRL_RESULT setMaster();
	PRL_RESULT setBridge();
	PRL_RESULT setInterface();

	QString getResult() const;

private:
	QString m_name;
	CHwNetAdapter m_master;
	Libvirt::Iface::Xml::Interface3 m_result;
};

} // namespace Bridge
} // namespace Interface

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

struct Reverse
{
	Reverse(const QString& uuid_, const CVmConfiguration& input_);

	PRL_RESULT setIdentity();
	PRL_RESULT setInstructions();
	void setMemory();

	QString getResult() const;

private:
	QString m_uuid;
	CVmHardware m_hardware;
	Libvirt::Snapshot::Xml::Domainsnapshot m_result;
};

} // namespace Snapshot
} // namespace Transponster

#endif // __REVERSE_H__

