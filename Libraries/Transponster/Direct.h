///////////////////////////////////////////////////////////////////////////////
///
/// @file Direct.h
///
/// Convertor from the libvirt config format into the SDK one.
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

#ifndef __DIRECT_H__
#define __DIRECT_H__

#include "iface_type.h"
#include "domain_type.h"
#include "network_type.h"
#include <XmlModel/VmConfig/CVmConfiguration.h>
#include <XmlModel/NetworkConfig/CVirtualNetwork.h>
#include <XmlModel/HostHardwareInfo/CHwNetAdapter.h>

namespace Transponster
{
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

struct Direct
{
	explicit Direct(char* xml_);

	PRL_RESULT setBlank();
	PRL_RESULT setIdentification();
	PRL_RESULT setSettings();
	PRL_RESULT setDevices();
	PRL_RESULT setResources();

	CVmConfiguration* getResult()
	{
		return m_result.take();
	}

private:
	QScopedPointer<Libvirt::Domain::Xml::Domain> m_input;
	QScopedPointer<CVmConfiguration> m_result;
};

} // namespace Vm

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

struct Direct
{
	Direct(char* xml_, bool enabled_);

	PRL_RESULT setUuid();
	PRL_RESULT setName();
	PRL_RESULT setType();
	PRL_RESULT setBridge();
	PRL_RESULT setVlan();
	PRL_RESULT setHostOnly();

	const CVirtualNetwork& getResult() const
	{
		return m_result;
	}

private:
	CVirtualNetwork m_result;
	QScopedPointer<Libvirt::Network::Xml::Network> m_input;
};

} // namespace Network

namespace Interface
{
namespace Physical
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

struct Direct
{
	Direct(char* xml_, bool enabled_);

	PRL_RESULT operator()();

	const CHwNetAdapter& getResult() const
	{
		return m_result;
	}

private:
	CHwNetAdapter m_result;
	QScopedPointer<Libvirt::Iface::Xml::Interface> m_input;
};

} // namespace Physical

namespace Bridge
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

struct Direct
{
	Direct(char* xml_, bool enabled_);

	PRL_RESULT setMaster();
	PRL_RESULT setBridge();
	PRL_RESULT setInterface();

	bool getStp() const
	{
		return m_stp;
	}
	bool isEnabled() const
	{
		return m_enabled;
	}
	double getDelay() const
	{
		return m_delay;
	}
	const QString& getName() const
	{
		return m_name;
	}
	const CHwNetAdapter& getMaster() const
	{
		return m_master;
	}

private:
	bool m_stp;
	bool m_enabled;
	double m_delay;
	QString m_name;
	CHwNetAdapter m_master;
	QScopedPointer<Libvirt::Iface::Xml::Interface3> m_input;
};

} // namespace Bridge
} // namespace Interface

///////////////////////////////////////////////////////////////////////////////
// struct Director

struct Director
{
	template<class T>
	static PRL_RESULT domain(T& builder_)
	{
		PRL_RESULT e;
		if (PRL_FAILED(e = builder_.setBlank()))
			return e;

		if (PRL_FAILED(e = builder_.setIdentification()))
			return e;

		if (PRL_FAILED(e = builder_.setSettings()))
			return e;

		if (PRL_FAILED(e = builder_.setDevices()))
			return e;

		if (PRL_FAILED(e = builder_.setResources()))
			return e;

		return PRL_ERR_SUCCESS;
	}
	template<class T>
	static PRL_RESULT network(T& builder_)
	{
		PRL_RESULT e;
		if (PRL_FAILED(e = builder_.setUuid()))
			return e;

		if (PRL_FAILED(e = builder_.setName()))
			return e;

		if (PRL_FAILED(e = builder_.setType()))
			return e;

		if (PRL_FAILED(e = builder_.setBridge()))
			return e;

		if (PRL_FAILED(e = builder_.setHostOnly()))
			return e;

		if (PRL_FAILED(e = builder_.setVlan()))
			return e;

		return PRL_ERR_SUCCESS;
	}
	template<class T>
	static PRL_RESULT physical(T& builder_)
	{
		return builder_();
	}
	template<class T>
	static PRL_RESULT bridge(T& builder_)
	{
		PRL_RESULT e;
		if (PRL_FAILED(e = builder_.setMaster()))
			return e;

		if (PRL_FAILED(e = builder_.setBridge()))
			return e;

		if (PRL_FAILED(e = builder_.setInterface()))
			return e;

		return PRL_ERR_SUCCESS;
	}
};

} // namespace Transponster

#endif // __DIRECT_H__

