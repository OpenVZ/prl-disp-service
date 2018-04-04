///////////////////////////////////////////////////////////////////////////////
///
/// @file Direct.h
///
/// Convertor from the libvirt config format into the SDK one.
///
/// @author shrike
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __DIRECT_H__
#define __DIRECT_H__

#include "capability_type.h"
#include "iface_type.h"
#include "domain_type.h"
#include "network_type.h"
#include "snapshot_type.h"
#include <prlxmlmodel/VtInfo/VtInfo.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <Libraries/StatesStore/SavedStateTree.h>
#include <prlxmlmodel/NetworkConfig/CVirtualNetwork.h>
#include <prlxmlmodel/HostHardwareInfo/CHwNetAdapter.h>

namespace Transponster
{
namespace Direct
{
///////////////////////////////////////////////////////////////////////////////
// struct Text

struct Text: Libvirt::Details::Value::Bin<QByteArray>
{
	explicit Text(char* xml_);
	explicit Text(const QString& xml_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Distiller

template<class T>
struct Distiller: Libvirt::Details::Value::Bin<T>
{
	PRL_RESULT operator()(const Text& xml_)
	{
		QDomDocument x;
		if (!x.setContent(xml_.getValue(), true))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot parse XML");
			return PRL_ERR_READ_XML_CONTENT;
		}
		T y;
		if (y.load(x.documentElement()))
		{
			this->setValue(y);
			return PRL_ERR_SUCCESS;
		}
		WRITE_TRACE(DBG_FATAL, "RNG schema validation has failed");

		return PRL_ERR_FAILURE;
	}
};

} // namespace Direct

namespace Vm
{
namespace Direct
{
///////////////////////////////////////////////////////////////////////////////
// struct Cpu

struct Cpu
{
	Cpu(const Libvirt::Domain::Xml::Domain& vm_, CVmCpu* prototype_,
		const VtInfo& vt_);

	PRL_RESULT setMask();
	PRL_RESULT setNode();
	PRL_RESULT setUnits();
	PRL_RESULT setLimit();
	PRL_RESULT setNumber();

	CVmCpu* getResult()
	{
		return m_result.take();
	}

private:
	QScopedPointer<CVmCpu> m_result;
	boost::optional<qint32> m_period;
	boost::optional<Libvirt::Domain::Xml::Vcpu> m_vcpu;
	boost::optional<Libvirt::Domain::Xml::Cputune> m_tune;
	boost::optional<Libvirt::Domain::Xml::Numatune> m_numa;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	explicit Vm(char* xml_);

	PRL_RESULT setBlank();
	PRL_RESULT setIdentification();
	PRL_RESULT setSettings();
	PRL_RESULT setDevices();
	PRL_RESULT setResources(const VtInfo& vt_);

	CVmConfiguration* getResult()
	{
		return m_result.take();
	}

protected:
	Vm()
	{
	}

	QScopedPointer<Libvirt::Domain::Xml::Domain> m_input;

private:
	QScopedPointer<CVmConfiguration> m_result;
};

} // namespace Direct
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

struct Direct: Libvirt::Details::Value::Bin<CHwNetAdapter>
{
	typedef Libvirt::Iface::Xml::Interface object_type;

	explicit Direct(bool enabled_);

	PRL_RESULT operator()(const object_type& object_);
};

} // namespace Physical

namespace Vlan
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

struct Direct: Libvirt::Details::Value::Bin<CHwNetAdapter>
{
	typedef Libvirt::Iface::Xml::Interface4 object_type;

	explicit Direct(bool enabled_);

	PRL_RESULT operator()(const object_type& object_);
};

} // namespace Vlan

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

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

struct Direct
{
	explicit Direct(char* xml_);

	PRL_RESULT setIdentity();
	PRL_RESULT setInstructions();

	const CSavedStateTree& getResult() const
	{
		return m_result;
	}

private:
	CSavedStateTree m_result;
	QScopedPointer<Libvirt::Snapshot::Xml::Domainsnapshot> m_input;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm: ::Transponster::Vm::Direct::Vm
{
	explicit Vm(char* xml_);
};

namespace Export
{
///////////////////////////////////////////////////////////////////////////////
// struct View

struct View: Libvirt::Details::Value::Bin
			<
				QList
				<
					boost::tuple<QString, QString, QUrl>
				>
			>
{
	typedef Libvirt::Domain::Xml::Domain object_type;

	PRL_RESULT operator()(const object_type& object_);
};

} // namespace Export
} // namespace Snapshot

namespace Capabilities
{

///////////////////////////////////////////////////////////////////////////////
// struct Direct

struct Direct
{
	explicit Direct(char* xml_);

	CpuFeatures* getCpuFeatures() const;
	QString getCpuModel() const;

private:
	bool isValid() const;

	QScopedPointer<Libvirt::Capability::Xml::DomainCapabilities> m_input;
};

} // namespace Capabilities

///////////////////////////////////////////////////////////////////////////////
// struct Director

struct Director
{
	template<class T>
	static PRL_RESULT domain(T& builder_, const VtInfo& vt_)
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

		if (PRL_FAILED(e = builder_.setResources(vt_)))
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
	template<class T>
	static PRL_RESULT snapshot(T& builder_)
	{
		PRL_RESULT e;
		if (PRL_FAILED(e = builder_.setIdentity()))
			return e;

		if (PRL_FAILED(e = builder_.setInstructions()))
			return e;

		return PRL_ERR_SUCCESS;
	}
	template<class T>
	static PRL_RESULT cpu(T& builder_)
	{
		PRL_RESULT e;
		if (PRL_FAILED(e = builder_.setNumber()))
			return e;

		if (PRL_FAILED(e = builder_.setMask()))
			return e;

		if (PRL_FAILED(e = builder_.setNode()))
			return e;

		if (PRL_FAILED(e = builder_.setUnits()))
			return e;

		if (PRL_FAILED(e = builder_.setLimit()))
			return e;

		return PRL_ERR_SUCCESS;
	}
	template<class T, class U>
	static PRL_RESULT
		marshalDirect(U xml_, T& builder_)
	{
		Direct::Distiller<typename T::object_type> d;
		PRL_RESULT e = d(Direct::Text(xml_));
		if (PRL_FAILED(e))
			return e;
		
		return builder_(d.getValue());
	}
	template<class T>
	static Prl::Expected<QString, PRL_RESULT>
		marshalReverse(const typename T::object_type& object_, T& builder_)
	{
		PRL_RESULT e = builder_(object_);
		if (PRL_FAILED(e))
			return e;

		QDomDocument x;
		builder_.getValue().save(x);
		return x.toString();
	}
};

} // namespace Transponster

#endif // __DIRECT_H__

