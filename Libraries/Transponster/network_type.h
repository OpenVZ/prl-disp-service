/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __NETWORK_TYPE_H__
#define __NETWORK_TYPE_H__
#include "base.h"
#include "network_data.h"
#include "network_enum.h"
#include "patterns.h"
#include <boost/any.hpp>

namespace Libvirt
{

///////////////////////////////////////////////////////////////////////////////
// struct Bridge

namespace Network
{
namespace Xml
{
struct Bridge
{
	const boost::optional<PDeviceName::value_type >& getName() const
	{
		return m_name;
	}
	void setName(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_name = value_;
	}
	const boost::optional<EVirOnOff >& getStp() const
	{
		return m_stp;
	}
	void setStp(const boost::optional<EVirOnOff >& value_)
	{
		m_stp = value_;
	}
	const boost::optional<PDelay::value_type >& getDelay() const
	{
		return m_delay;
	}
	void setDelay(const boost::optional<PDelay::value_type >& value_)
	{
		m_delay = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PDeviceName::value_type > m_name;
	boost::optional<EVirOnOff > m_stp;
	boost::optional<PDelay::value_type > m_delay;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Interface

namespace Network
{
namespace Xml
{
struct Interface
{
	const PDeviceName::value_type& getDev() const
	{
		return m_dev;
	}
	void setDev(const PDeviceName::value_type& value_)
	{
		m_dev = value_;
	}
	const boost::optional<PConnections::value_type >& getConnections() const
	{
		return m_connections;
	}
	void setConnections(const boost::optional<PConnections::value_type >& value_)
	{
		m_connections = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PDeviceName::value_type m_dev;
	boost::optional<PConnections::value_type > m_connections;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Pciaddress

namespace Network
{
namespace Xml
{
struct Pciaddress
{
	const boost::optional<PPciDomain::value_type >& getDomain() const
	{
		return m_domain;
	}
	void setDomain(const boost::optional<PPciDomain::value_type >& value_)
	{
		m_domain = value_;
	}
	const PPciBus::value_type& getBus() const
	{
		return m_bus;
	}
	void setBus(const PPciBus::value_type& value_)
	{
		m_bus = value_;
	}
	const PPciSlot::value_type& getSlot() const
	{
		return m_slot;
	}
	void setSlot(const PPciSlot::value_type& value_)
	{
		m_slot = value_;
	}
	const PPciFunc::value_type& getFunction() const
	{
		return m_function;
	}
	void setFunction(const PPciFunc::value_type& value_)
	{
		m_function = value_;
	}
	const boost::optional<EVirOnOff >& getMultifunction() const
	{
		return m_multifunction;
	}
	void setMultifunction(const boost::optional<EVirOnOff >& value_)
	{
		m_multifunction = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<PPciDomain::value_type > m_domain;
	PPciBus::value_type m_bus;
	PPciSlot::value_type m_slot;
	PPciFunc::value_type m_function;
	boost::optional<EVirOnOff > m_multifunction;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Address

namespace Network
{
namespace Xml
{
struct Address
{
	const Pciaddress& getPciaddress() const
	{
		return m_pciaddress;
	}
	void setPciaddress(const Pciaddress& value_)
	{
		m_pciaddress = value_;
	}
	const boost::optional<PConnections::value_type >& getConnections() const
	{
		return m_connections;
	}
	void setConnections(const boost::optional<PConnections::value_type >& value_)
	{
		m_connections = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	Pciaddress m_pciaddress;
	boost::optional<PConnections::value_type > m_connections;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1161

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<ZeroOrMore<Element<Network::Xml::Interface, Name::Strict<617> > >, ZeroOrMore<Element<Network::Xml::Address, Name::Strict<105> > > > > VChoice1161Impl;
typedef VChoice1161Impl::value_type VChoice1161;

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Address1

namespace Network
{
namespace Xml
{
struct Address1
{
	const PIpv4Addr::value_type& getStart() const
	{
		return m_start;
	}
	void setStart(const PIpv4Addr::value_type& value_)
	{
		m_start = value_;
	}
	const PIpv4Addr::value_type& getEnd() const
	{
		return m_end;
	}
	void setEnd(const PIpv4Addr::value_type& value_)
	{
		m_end = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PIpv4Addr::value_type m_start;
	PIpv4Addr::value_type m_end;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Port

namespace Network
{
namespace Xml
{
struct Port
{
	Port();

	PPort::value_type getStart() const
	{
		return m_start;
	}
	void setStart(PPort::value_type value_)
	{
		m_start = value_;
	}
	PPort::value_type getEnd() const
	{
		return m_end;
	}
	void setEnd(PPort::value_type value_)
	{
		m_end = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PPort::value_type m_start;
	PPort::value_type m_end;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Nat

namespace Network
{
namespace Xml
{
struct Nat
{
	const boost::optional<Address1 >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<Address1 >& value_)
	{
		m_address = value_;
	}
	const boost::optional<Port >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<Port >& value_)
	{
		m_port = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<Address1 > m_address;
	boost::optional<Port > m_port;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Forward

namespace Network
{
namespace Xml
{
struct Forward
{
	const boost::optional<PDeviceName::value_type >& getDev() const
	{
		return m_dev;
	}
	void setDev(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_dev = value_;
	}
	const boost::optional<EMode >& getMode() const
	{
		return m_mode;
	}
	void setMode(const boost::optional<EMode >& value_)
	{
		m_mode = value_;
	}
	const boost::optional<EVirYesNo >& getManaged() const
	{
		return m_managed;
	}
	void setManaged(const boost::optional<EVirYesNo >& value_)
	{
		m_managed = value_;
	}
	const VChoice1161& getChoice1161() const
	{
		return m_choice1161;
	}
	void setChoice1161(const VChoice1161& value_)
	{
		m_choice1161 = value_;
	}
	const boost::optional<PDeviceName::value_type >& getPf() const
	{
		return m_pf;
	}
	void setPf(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_pf = value_;
	}
	const boost::optional<EName >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<EName >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<Nat >& getNat() const
	{
		return m_nat;
	}
	void setNat(const boost::optional<Nat >& value_)
	{
		m_nat = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PDeviceName::value_type > m_dev;
	boost::optional<EMode > m_mode;
	boost::optional<EVirYesNo > m_managed;
	VChoice1161 m_choice1161;
	boost::optional<PDeviceName::value_type > m_pf;
	boost::optional<EName > m_driver;
	boost::optional<Nat > m_nat;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Parameters

namespace Network
{
namespace Xml
{
struct Parameters
{
	const boost::optional<VUint8range >& getManagerid() const
	{
		return m_managerid;
	}
	void setManagerid(const boost::optional<VUint8range >& value_)
	{
		m_managerid = value_;
	}
	const boost::optional<VUint24range >& getTypeid() const
	{
		return m_typeid;
	}
	void setTypeid(const boost::optional<VUint24range >& value_)
	{
		m_typeid = value_;
	}
	const boost::optional<VUint8range >& getTypeidversion() const
	{
		return m_typeidversion;
	}
	void setTypeidversion(const boost::optional<VUint8range >& value_)
	{
		m_typeidversion = value_;
	}
	const boost::optional<VUUID >& getInstanceid() const
	{
		return m_instanceid;
	}
	void setInstanceid(const boost::optional<VUUID >& value_)
	{
		m_instanceid = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<VUint8range > m_managerid;
	boost::optional<VUint24range > m_typeid;
	boost::optional<VUint8range > m_typeidversion;
	boost::optional<VUUID > m_instanceid;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Parameters1

namespace Network
{
namespace Xml
{
struct Parameters1
{
	const boost::optional<PVirtualPortProfileID::value_type >& getProfileid() const
	{
		return m_profileid;
	}
	void setProfileid(const boost::optional<PVirtualPortProfileID::value_type >& value_)
	{
		m_profileid = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PVirtualPortProfileID::value_type > m_profileid;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Parameters2

namespace Network
{
namespace Xml
{
struct Parameters2
{
	const boost::optional<PVirtualPortProfileID::value_type >& getProfileid() const
	{
		return m_profileid;
	}
	void setProfileid(const boost::optional<PVirtualPortProfileID::value_type >& value_)
	{
		m_profileid = value_;
	}
	const boost::optional<VUUID >& getInterfaceid() const
	{
		return m_interfaceid;
	}
	void setInterfaceid(const boost::optional<VUUID >& value_)
	{
		m_interfaceid = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PVirtualPortProfileID::value_type > m_profileid;
	boost::optional<VUUID > m_interfaceid;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Parameters3

namespace Network
{
namespace Xml
{
struct Parameters3
{
	const boost::optional<VUint8range >& getManagerid() const
	{
		return m_managerid;
	}
	void setManagerid(const boost::optional<VUint8range >& value_)
	{
		m_managerid = value_;
	}
	const boost::optional<VUint24range >& getTypeid() const
	{
		return m_typeid;
	}
	void setTypeid(const boost::optional<VUint24range >& value_)
	{
		m_typeid = value_;
	}
	const boost::optional<VUint8range >& getTypeidversion() const
	{
		return m_typeidversion;
	}
	void setTypeidversion(const boost::optional<VUint8range >& value_)
	{
		m_typeidversion = value_;
	}
	const boost::optional<VUUID >& getInstanceid() const
	{
		return m_instanceid;
	}
	void setInstanceid(const boost::optional<VUUID >& value_)
	{
		m_instanceid = value_;
	}
	const boost::optional<PVirtualPortProfileID::value_type >& getProfileid() const
	{
		return m_profileid;
	}
	void setProfileid(const boost::optional<PVirtualPortProfileID::value_type >& value_)
	{
		m_profileid = value_;
	}
	const boost::optional<VUUID >& getInterfaceid() const
	{
		return m_interfaceid;
	}
	void setInterfaceid(const boost::optional<VUUID >& value_)
	{
		m_interfaceid = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<VUint8range > m_managerid;
	boost::optional<VUint24range > m_typeid;
	boost::optional<VUint8range > m_typeidversion;
	boost::optional<VUUID > m_instanceid;
	boost::optional<PVirtualPortProfileID::value_type > m_profileid;
	boost::optional<VUUID > m_interfaceid;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Virtualport

namespace Network
{
namespace Xml
{
struct Virtualport
{
	const boost::optional<Parameters3 >& getParameters() const
	{
		return m_parameters;
	}
	void setParameters(const boost::optional<Parameters3 >& value_)
	{
		m_parameters = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<Parameters3 > m_parameters;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct VVirtualPortProfile

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Ordered<mpl::vector<Attribute<mpl::int_<167>, Name::Strict<99> >, Optional<Element<Network::Xml::Parameters, Name::Strict<168> > > > >, Name::Strict<166> >, Element<Ordered<mpl::vector<Attribute<mpl::int_<174>, Name::Strict<99> >, Optional<Element<Network::Xml::Parameters1, Name::Strict<168> > > > >, Name::Strict<166> >, Element<Ordered<mpl::vector<Attribute<mpl::int_<177>, Name::Strict<99> >, Optional<Element<Network::Xml::Parameters2, Name::Strict<168> > > > >, Name::Strict<166> >, Element<Network::Xml::Virtualport, Name::Strict<166> > > > VVirtualPortProfileImpl;
typedef VVirtualPortProfileImpl::value_type VVirtualPortProfile;

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct BandwidthAttributes

namespace Network
{
namespace Xml
{
struct BandwidthAttributes
{
	BandwidthAttributes();

	PSpeed::value_type getAverage() const
	{
		return m_average;
	}
	void setAverage(PSpeed::value_type value_)
	{
		m_average = value_;
	}
	const boost::optional<PSpeed::value_type >& getPeak() const
	{
		return m_peak;
	}
	void setPeak(const boost::optional<PSpeed::value_type >& value_)
	{
		m_peak = value_;
	}
	const boost::optional<PSpeed::value_type >& getFloor() const
	{
		return m_floor;
	}
	void setFloor(const boost::optional<PSpeed::value_type >& value_)
	{
		m_floor = value_;
	}
	const boost::optional<PBurstSize::value_type >& getBurst() const
	{
		return m_burst;
	}
	void setBurst(const boost::optional<PBurstSize::value_type >& value_)
	{
		m_burst = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PSpeed::value_type m_average;
	boost::optional<PSpeed::value_type > m_peak;
	boost::optional<PSpeed::value_type > m_floor;
	boost::optional<PBurstSize::value_type > m_burst;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Bandwidth

namespace Network
{
namespace Xml
{
struct Bandwidth
{
	const boost::optional<BandwidthAttributes >& getInbound() const
	{
		return m_inbound;
	}
	void setInbound(const boost::optional<BandwidthAttributes >& value_)
	{
		m_inbound = value_;
	}
	const boost::optional<BandwidthAttributes >& getOutbound() const
	{
		return m_outbound;
	}
	void setOutbound(const boost::optional<BandwidthAttributes >& value_)
	{
		m_outbound = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<BandwidthAttributes > m_inbound;
	boost::optional<BandwidthAttributes > m_outbound;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Tag

namespace Network
{
namespace Xml
{
struct Tag
{
	Tag();

	PId::value_type getId() const
	{
		return m_id;
	}
	void setId(PId::value_type value_)
	{
		m_id = value_;
	}
	const boost::optional<ENativeMode >& getNativeMode() const
	{
		return m_nativeMode;
	}
	void setNativeMode(const boost::optional<ENativeMode >& value_)
	{
		m_nativeMode = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PId::value_type m_id;
	boost::optional<ENativeMode > m_nativeMode;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Portgroup

namespace Network
{
namespace Xml
{
struct Portgroup
{
	const PDeviceName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PDeviceName::value_type& value_)
	{
		m_name = value_;
	}
	const boost::optional<EVirYesNo >& getDefault() const
	{
		return m_default;
	}
	void setDefault(const boost::optional<EVirYesNo >& value_)
	{
		m_default = value_;
	}
	const boost::optional<VVirtualPortProfile >& getVirtualPortProfile() const
	{
		return m_virtualPortProfile;
	}
	void setVirtualPortProfile(const boost::optional<VVirtualPortProfile >& value_)
	{
		m_virtualPortProfile = value_;
	}
	const boost::optional<Bandwidth >& getBandwidth() const
	{
		return m_bandwidth;
	}
	void setBandwidth(const boost::optional<Bandwidth >& value_)
	{
		m_bandwidth = value_;
	}
	const boost::optional<QList<Tag > >& getVlan() const
	{
		return m_vlan;
	}
	void setVlan(const boost::optional<QList<Tag > >& value_)
	{
		m_vlan = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PDeviceName::value_type m_name;
	boost::optional<EVirYesNo > m_default;
	boost::optional<VVirtualPortProfile > m_virtualPortProfile;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Txt

namespace Network
{
namespace Xml
{
struct Txt
{
	const PDnsName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PDnsName::value_type& value_)
	{
		m_name = value_;
	}
	const QString& getValue() const
	{
		return m_value;
	}
	void setValue(const QString& value_)
	{
		m_value = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PDnsName::value_type m_name;
	QString m_value;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1189

namespace Network
{
namespace Xml
{
struct Anonymous1189
{
	Anonymous1189();

	const PDnsName::value_type& getDomain() const
	{
		return m_domain;
	}
	void setDomain(const PDnsName::value_type& value_)
	{
		m_domain = value_;
	}
	const QString& getTarget() const
	{
		return m_target;
	}
	void setTarget(const QString& value_)
	{
		m_target = value_;
	}
	PUnsignedShort::value_type getPort() const
	{
		return m_port;
	}
	void setPort(PUnsignedShort::value_type value_)
	{
		m_port = value_;
	}
	PUnsignedShort::value_type getPriority() const
	{
		return m_priority;
	}
	void setPriority(PUnsignedShort::value_type value_)
	{
		m_priority = value_;
	}
	PUnsignedShort::value_type getWeight() const
	{
		return m_weight;
	}
	void setWeight(PUnsignedShort::value_type value_)
	{
		m_weight = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PDnsName::value_type m_domain;
	QString m_target;
	PUnsignedShort::value_type m_port;
	PUnsignedShort::value_type m_priority;
	PUnsignedShort::value_type m_weight;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Srv

namespace Network
{
namespace Xml
{
struct Srv
{
	const QString& getService() const
	{
		return m_service;
	}
	void setService(const QString& value_)
	{
		m_service = value_;
	}
	const PProtocol::value_type& getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(const PProtocol::value_type& value_)
	{
		m_protocol = value_;
	}
	const boost::optional<Anonymous1189 >& getAnonymous1189() const
	{
		return m_anonymous1189;
	}
	void setAnonymous1189(const boost::optional<Anonymous1189 >& value_)
	{
		m_anonymous1189 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_service;
	PProtocol::value_type m_protocol;
	boost::optional<Anonymous1189 > m_anonymous1189;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Host

namespace Network
{
namespace Xml
{
struct Host
{
	const VIpAddr& getIp() const
	{
		return m_ip;
	}
	void setIp(const VIpAddr& value_)
	{
		m_ip = value_;
	}
	const QList<PDnsName::value_type >& getHostnameList() const
	{
		return m_hostnameList;
	}
	void setHostnameList(const QList<PDnsName::value_type >& value_)
	{
		m_hostnameList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VIpAddr m_ip;
	QList<PDnsName::value_type > m_hostnameList;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Dns

namespace Network
{
namespace Xml
{
struct Dns
{
	const boost::optional<EVirYesNo >& getForwardPlainNames() const
	{
		return m_forwardPlainNames;
	}
	void setForwardPlainNames(const boost::optional<EVirYesNo >& value_)
	{
		m_forwardPlainNames = value_;
	}
	const QList<VIpAddr >& getForwarderList() const
	{
		return m_forwarderList;
	}
	void setForwarderList(const QList<VIpAddr >& value_)
	{
		m_forwarderList = value_;
	}
	const QList<Txt >& getTxtList() const
	{
		return m_txtList;
	}
	void setTxtList(const QList<Txt >& value_)
	{
		m_txtList = value_;
	}
	const QList<Srv >& getSrvList() const
	{
		return m_srvList;
	}
	void setSrvList(const QList<Srv >& value_)
	{
		m_srvList = value_;
	}
	const QList<Host >& getHostList() const
	{
		return m_hostList;
	}
	void setHostList(const QList<Host >& value_)
	{
		m_hostList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_forwardPlainNames;
	QList<VIpAddr > m_forwarderList;
	QList<Txt > m_txtList;
	QList<Srv > m_srvList;
	QList<Host > m_hostList;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1175

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Network::Xml::PIpv4Addr, Name::Strict<1173> >, Attribute<Network::Xml::VIpPrefix, Name::Strict<653> > > > VChoice1175Impl;
typedef VChoice1175Impl::value_type VChoice1175;

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Range

namespace Network
{
namespace Xml
{
struct Range
{
	const VIpAddr& getStart() const
	{
		return m_start;
	}
	void setStart(const VIpAddr& value_)
	{
		m_start = value_;
	}
	const VIpAddr& getEnd() const
	{
		return m_end;
	}
	void setEnd(const VIpAddr& value_)
	{
		m_end = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VIpAddr m_start;
	VIpAddr m_end;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1180

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Network::Xml::PUniMacAddr, Name::Strict<635> >, Attribute<Network::Xml::VDUID, Name::Strict<196> > > > VChoice1180Impl;
typedef VChoice1180Impl::value_type VChoice1180;

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Host1181

namespace Network
{
namespace Xml
{
struct Host1181
{
	const VChoice1180& getChoice1180() const
	{
		return m_choice1180;
	}
	void setChoice1180(const VChoice1180& value_)
	{
		m_choice1180 = value_;
	}
	const boost::optional<QString >& getName() const
	{
		return m_name;
	}
	void setName(const boost::optional<QString >& value_)
	{
		m_name = value_;
	}

private:
	VChoice1180 m_choice1180;
	boost::optional<QString > m_name;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct VHost

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<Host1181, Attribute<QString, Name::Strict<101> > > > VHostImpl;
typedef VHostImpl::value_type VHost;

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Host1

namespace Network
{
namespace Xml
{
struct Host1
{
	const VHost& getHost() const
	{
		return m_host;
	}
	void setHost(const VHost& value_)
	{
		m_host = value_;
	}
	const VIpAddr& getIp() const
	{
		return m_ip;
	}
	void setIp(const VIpAddr& value_)
	{
		m_ip = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VHost m_host;
	VIpAddr m_ip;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Bootp

namespace Network
{
namespace Xml
{
struct Bootp
{
	const PFilePath::value_type& getFile() const
	{
		return m_file;
	}
	void setFile(const PFilePath::value_type& value_)
	{
		m_file = value_;
	}
	const boost::optional<PDnsName::value_type >& getServer() const
	{
		return m_server;
	}
	void setServer(const boost::optional<PDnsName::value_type >& value_)
	{
		m_server = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PFilePath::value_type m_file;
	boost::optional<PDnsName::value_type > m_server;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Dhcp

namespace Network
{
namespace Xml
{
struct Dhcp
{
	const QList<Range >& getRangeList() const
	{
		return m_rangeList;
	}
	void setRangeList(const QList<Range >& value_)
	{
		m_rangeList = value_;
	}
	const QList<Host1 >& getHostList() const
	{
		return m_hostList;
	}
	void setHostList(const QList<Host1 >& value_)
	{
		m_hostList = value_;
	}
	const boost::optional<Bootp >& getBootp() const
	{
		return m_bootp;
	}
	void setBootp(const boost::optional<Bootp >& value_)
	{
		m_bootp = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QList<Range > m_rangeList;
	QList<Host1 > m_hostList;
	boost::optional<Bootp > m_bootp;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Ip

namespace Network
{
namespace Xml
{
struct Ip
{
	const boost::optional<VIpAddr >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VIpAddr >& value_)
	{
		m_address = value_;
	}
	const boost::optional<VChoice1175 >& getChoice1175() const
	{
		return m_choice1175;
	}
	void setChoice1175(const boost::optional<VChoice1175 >& value_)
	{
		m_choice1175 = value_;
	}
	const boost::optional<PAddrFamily::value_type >& getFamily() const
	{
		return m_family;
	}
	void setFamily(const boost::optional<PAddrFamily::value_type >& value_)
	{
		m_family = value_;
	}
	const boost::optional<QString >& getTftp() const
	{
		return m_tftp;
	}
	void setTftp(const boost::optional<QString >& value_)
	{
		m_tftp = value_;
	}
	const boost::optional<Dhcp >& getDhcp() const
	{
		return m_dhcp;
	}
	void setDhcp(const boost::optional<Dhcp >& value_)
	{
		m_dhcp = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<VIpAddr > m_address;
	boost::optional<VChoice1175 > m_choice1175;
	boost::optional<PAddrFamily::value_type > m_family;
	boost::optional<QString > m_tftp;
	boost::optional<Dhcp > m_dhcp;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1186

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Network::Xml::PIpv4Addr, Name::Strict<1173> >, Attribute<Network::Xml::VIpPrefix, Name::Strict<653> > > > VChoice1186Impl;
typedef VChoice1186Impl::value_type VChoice1186;

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Route

namespace Network
{
namespace Xml
{
struct Route
{
	const boost::optional<PAddrFamily::value_type >& getFamily() const
	{
		return m_family;
	}
	void setFamily(const boost::optional<PAddrFamily::value_type >& value_)
	{
		m_family = value_;
	}
	const VIpAddr& getAddress() const
	{
		return m_address;
	}
	void setAddress(const VIpAddr& value_)
	{
		m_address = value_;
	}
	const boost::optional<VChoice1186 >& getChoice1186() const
	{
		return m_choice1186;
	}
	void setChoice1186(const boost::optional<VChoice1186 >& value_)
	{
		m_choice1186 = value_;
	}
	const VIpAddr& getGateway() const
	{
		return m_gateway;
	}
	void setGateway(const VIpAddr& value_)
	{
		m_gateway = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getMetric() const
	{
		return m_metric;
	}
	void setMetric(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_metric = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PAddrFamily::value_type > m_family;
	VIpAddr m_address;
	boost::optional<VChoice1186 > m_choice1186;
	VIpAddr m_gateway;
	boost::optional<PUnsignedInt::value_type > m_metric;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Network

namespace Network
{
namespace Xml
{
struct Network
{
	const boost::optional<PConnections::value_type >& getConnections() const
	{
		return m_connections;
	}
	void setConnections(const boost::optional<PConnections::value_type >& value_)
	{
		m_connections = value_;
	}
	const boost::optional<EVirYesNo >& getIpv6() const
	{
		return m_ipv6;
	}
	void setIpv6(const boost::optional<EVirYesNo >& value_)
	{
		m_ipv6 = value_;
	}
	const QString& getName() const
	{
		return m_name;
	}
	void setName(const QString& value_)
	{
		m_name = value_;
	}
	const boost::optional<VUUID >& getUuid() const
	{
		return m_uuid;
	}
	void setUuid(const boost::optional<VUUID >& value_)
	{
		m_uuid = value_;
	}
	const boost::optional<Bridge >& getBridge() const
	{
		return m_bridge;
	}
	void setBridge(const boost::optional<Bridge >& value_)
	{
		m_bridge = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const boost::optional<Forward >& getForward() const
	{
		return m_forward;
	}
	void setForward(const boost::optional<Forward >& value_)
	{
		m_forward = value_;
	}
	const boost::optional<VVirtualPortProfile >& getVirtualPortProfile() const
	{
		return m_virtualPortProfile;
	}
	void setVirtualPortProfile(const boost::optional<VVirtualPortProfile >& value_)
	{
		m_virtualPortProfile = value_;
	}
	const QList<Portgroup >& getPortgroupList() const
	{
		return m_portgroupList;
	}
	void setPortgroupList(const QList<Portgroup >& value_)
	{
		m_portgroupList = value_;
	}
	const boost::optional<PDnsName::value_type >& getDomain() const
	{
		return m_domain;
	}
	void setDomain(const boost::optional<PDnsName::value_type >& value_)
	{
		m_domain = value_;
	}
	const boost::optional<Dns >& getDns() const
	{
		return m_dns;
	}
	void setDns(const boost::optional<Dns >& value_)
	{
		m_dns = value_;
	}
	const boost::optional<Bandwidth >& getBandwidth() const
	{
		return m_bandwidth;
	}
	void setBandwidth(const boost::optional<Bandwidth >& value_)
	{
		m_bandwidth = value_;
	}
	const boost::optional<QList<Tag > >& getVlan() const
	{
		return m_vlan;
	}
	void setVlan(const boost::optional<QList<Tag > >& value_)
	{
		m_vlan = value_;
	}
	const boost::optional<EState >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<EState >& value_)
	{
		m_link = value_;
	}
	const QList<Ip >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip >& value_)
	{
		m_ipList = value_;
	}
	const QList<Route >& getRouteList() const
	{
		return m_routeList;
	}
	void setRouteList(const QList<Route >& value_)
	{
		m_routeList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PConnections::value_type > m_connections;
	boost::optional<EVirYesNo > m_ipv6;
	QString m_name;
	boost::optional<VUUID > m_uuid;
	boost::optional<Bridge > m_bridge;
	boost::optional<PUniMacAddr::value_type > m_mac;
	boost::optional<Forward > m_forward;
	boost::optional<VVirtualPortProfile > m_virtualPortProfile;
	QList<Portgroup > m_portgroupList;
	boost::optional<PDnsName::value_type > m_domain;
	boost::optional<Dns > m_dns;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
	boost::optional<EState > m_link;
	QList<Ip > m_ipList;
	QList<Route > m_routeList;
};

} // namespace Xml
} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Bridge traits

template<>
struct Traits<Network::Xml::Bridge>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::PDeviceName, Name::Strict<101> > >, Optional<Attribute<Network::Xml::EVirOnOff, Name::Strict<1155> > >, Optional<Attribute<Network::Xml::PDelay, Name::Strict<407> > > > > marshal_type;

	static int parse(Network::Xml::Bridge& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Bridge& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface traits

template<>
struct Traits<Network::Xml::Interface>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::PDeviceName, Name::Strict<423> >, Optional<Attribute<Network::Xml::PConnections, Name::Strict<1153> > > > > marshal_type;

	static int parse(Network::Xml::Interface& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Interface& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Pciaddress traits

template<>
struct Traits<Network::Xml::Pciaddress>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::PPciDomain, Name::Strict<1> > >, Attribute<Network::Xml::PPciBus, Name::Strict<23> >, Attribute<Network::Xml::PPciSlot, Name::Strict<25> >, Attribute<Network::Xml::PPciFunc, Name::Strict<27> >, Optional<Attribute<Network::Xml::EVirOnOff, Name::Strict<29> > > > > marshal_type;

	static int parse(Network::Xml::Pciaddress& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Pciaddress& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Address traits

template<>
struct Traits<Network::Xml::Address>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<576>, Name::Strict<99> >, Fragment<Network::Xml::Pciaddress >, Optional<Attribute<Network::Xml::PConnections, Name::Strict<1153> > > > > marshal_type;

	static int parse(Network::Xml::Address& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Address& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Address1 traits

template<>
struct Traits<Network::Xml::Address1>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::PIpv4Addr, Name::Strict<307> >, Attribute<Network::Xml::PIpv4Addr, Name::Strict<1164> > > > marshal_type;

	static int parse(Network::Xml::Address1& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Address1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Port traits

template<>
struct Traits<Network::Xml::Port>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::PPort, Name::Strict<307> >, Attribute<Network::Xml::PPort, Name::Strict<1164> > > > marshal_type;

	static int parse(Network::Xml::Port& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Port& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Nat traits

template<>
struct Traits<Network::Xml::Nat>
{
	typedef Unordered<mpl::vector<Optional<Element<Network::Xml::Address1, Name::Strict<105> > >, Optional<Element<Network::Xml::Port, Name::Strict<200> > > > > marshal_type;

	static int parse(Network::Xml::Nat& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Nat& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Forward traits

template<>
struct Traits<Network::Xml::Forward>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::PDeviceName, Name::Strict<423> > >, Optional<Attribute<Network::Xml::EMode, Name::Strict<360> > >, Optional<Attribute<Network::Xml::EVirYesNo, Name::Strict<639> > >, Unordered<mpl::vector<Network::Xml::VChoice1161Impl, Optional<Element<Attribute<Network::Xml::PDeviceName, Name::Strict<423> >, Name::Strict<1163> > >, Optional<Element<Attribute<Network::Xml::EName, Name::Strict<101> >, Name::Strict<527> > >, Optional<Element<Network::Xml::Nat, Name::Strict<1157> > > > > > > marshal_type;

	static int parse(Network::Xml::Forward& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Forward& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameters traits

template<>
struct Traits<Network::Xml::Parameters>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::VUint8range, Name::Strict<169> > >, Optional<Attribute<Network::Xml::VUint24range, Name::Strict<170> > >, Optional<Attribute<Network::Xml::VUint8range, Name::Strict<171> > >, Optional<Attribute<Network::Xml::VUUID, Name::Strict<172> > > > > marshal_type;

	static int parse(Network::Xml::Parameters& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Parameters& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameters1 traits

template<>
struct Traits<Network::Xml::Parameters1>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::PVirtualPortProfileID, Name::Strict<175> > > > > marshal_type;

	static int parse(Network::Xml::Parameters1& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Parameters1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameters2 traits

template<>
struct Traits<Network::Xml::Parameters2>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::PVirtualPortProfileID, Name::Strict<175> > >, Optional<Attribute<Network::Xml::VUUID, Name::Strict<178> > > > > marshal_type;

	static int parse(Network::Xml::Parameters2& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Parameters2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameters3 traits

template<>
struct Traits<Network::Xml::Parameters3>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::VUint8range, Name::Strict<169> > >, Optional<Attribute<Network::Xml::VUint24range, Name::Strict<170> > >, Optional<Attribute<Network::Xml::VUint8range, Name::Strict<171> > >, Optional<Attribute<Network::Xml::VUUID, Name::Strict<172> > >, Optional<Attribute<Network::Xml::PVirtualPortProfileID, Name::Strict<175> > >, Optional<Attribute<Network::Xml::VUUID, Name::Strict<178> > > > > marshal_type;

	static int parse(Network::Xml::Parameters3& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Parameters3& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Virtualport traits

template<>
struct Traits<Network::Xml::Virtualport>
{
	typedef Ordered<mpl::vector<Optional<Element<Network::Xml::Parameters3, Name::Strict<168> > > > > marshal_type;

	static int parse(Network::Xml::Virtualport& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Virtualport& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct BandwidthAttributes traits

template<>
struct Traits<Network::Xml::BandwidthAttributes>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::PSpeed, Name::Strict<185> >, Optional<Attribute<Network::Xml::PSpeed, Name::Strict<186> > >, Optional<Attribute<Network::Xml::PSpeed, Name::Strict<187> > >, Optional<Attribute<Network::Xml::PBurstSize, Name::Strict<188> > > > > marshal_type;

	static int parse(Network::Xml::BandwidthAttributes& , QStack<QDomElement>& );
	static int generate(const Network::Xml::BandwidthAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Bandwidth traits

template<>
struct Traits<Network::Xml::Bandwidth>
{
	typedef Unordered<mpl::vector<Optional<Element<Network::Xml::BandwidthAttributes, Name::Strict<182> > >, Optional<Element<Network::Xml::BandwidthAttributes, Name::Strict<184> > > > > marshal_type;

	static int parse(Network::Xml::Bandwidth& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Bandwidth& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Tag traits

template<>
struct Traits<Network::Xml::Tag>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::PId, Name::Strict<196> >, Optional<Attribute<Network::Xml::ENativeMode, Name::Strict<197> > > > > marshal_type;

	static int parse(Network::Xml::Tag& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Tag& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Portgroup traits

template<>
struct Traits<Network::Xml::Portgroup>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::PDeviceName, Name::Strict<101> >, Optional<Attribute<Network::Xml::EVirYesNo, Name::Strict<135> > >, Unordered<mpl::vector<Optional<Network::Xml::VVirtualPortProfileImpl >, Optional<Element<Network::Xml::Bandwidth, Name::Strict<181> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<129>, Name::Strict<194> > >, OneOrMore<Element<Network::Xml::Tag, Name::Strict<195> > > > >, Name::Strict<193> > > > > > > marshal_type;

	static int parse(Network::Xml::Portgroup& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Portgroup& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Txt traits

template<>
struct Traits<Network::Xml::Txt>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::PDnsName, Name::Strict<101> >, Attribute<QString, Name::Strict<1029> > > > marshal_type;

	static int parse(Network::Xml::Txt& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Txt& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1189 traits

template<>
struct Traits<Network::Xml::Anonymous1189>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::PDnsName, Name::Strict<1> >, Attribute<QString, Name::Strict<308> >, Attribute<Network::Xml::PUnsignedShort, Name::Strict<200> >, Attribute<Network::Xml::PUnsignedShort, Name::Strict<1171> >, Attribute<Network::Xml::PUnsignedShort, Name::Strict<334> > > > marshal_type;

	static int parse(Network::Xml::Anonymous1189& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Anonymous1189& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Srv traits

template<>
struct Traits<Network::Xml::Srv>
{
	typedef Ordered<mpl::vector<Attribute<QString, Name::Strict<813> >, Attribute<Network::Xml::PProtocol, Name::Strict<191> >, Optional<Fragment<Network::Xml::Anonymous1189 > > > > marshal_type;

	static int parse(Network::Xml::Srv& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Srv& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Host traits

template<>
struct Traits<Network::Xml::Host>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::VIpAddr, Name::Strict<651> >, OneOrMore<Element<Text<Network::Xml::PDnsName >, Name::Strict<1172> > > > > marshal_type;

	static int parse(Network::Xml::Host& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Host& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Dns traits

template<>
struct Traits<Network::Xml::Dns>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::EVirYesNo, Name::Strict<1166> > >, Unordered<mpl::vector<ZeroOrMore<Element<Attribute<Network::Xml::VIpAddr, Name::Strict<1168> >, Name::Strict<1167> > >, ZeroOrMore<Element<Network::Xml::Txt, Name::Strict<1169> > >, ZeroOrMore<Element<Network::Xml::Srv, Name::Strict<1170> > >, ZeroOrMore<Element<Network::Xml::Host, Name::Strict<494> > > > > > > marshal_type;

	static int parse(Network::Xml::Dns& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Dns& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Range traits

template<>
struct Traits<Network::Xml::Range>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::VIpAddr, Name::Strict<307> >, Attribute<Network::Xml::VIpAddr, Name::Strict<1164> > > > marshal_type;

	static int parse(Network::Xml::Range& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Range& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Host1181 traits

template<>
struct Traits<Network::Xml::Host1181>
{
	typedef Ordered<mpl::vector<Network::Xml::VChoice1180Impl, Optional<Attribute<QString, Name::Strict<101> > > > > marshal_type;

	static int parse(Network::Xml::Host1181& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Host1181& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Host1 traits

template<>
struct Traits<Network::Xml::Host1>
{
	typedef Ordered<mpl::vector<Network::Xml::VHostImpl, Attribute<Network::Xml::VIpAddr, Name::Strict<651> > > > marshal_type;

	static int parse(Network::Xml::Host1& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Host1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Bootp traits

template<>
struct Traits<Network::Xml::Bootp>
{
	typedef Ordered<mpl::vector<Attribute<Network::Xml::PFilePath, Name::Strict<481> >, Optional<Attribute<Network::Xml::PDnsName, Name::Strict<624> > > > > marshal_type;

	static int parse(Network::Xml::Bootp& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Bootp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Dhcp traits

template<>
struct Traits<Network::Xml::Dhcp>
{
	typedef Ordered<mpl::vector<ZeroOrMore<Element<Network::Xml::Range, Name::Strict<1178> > >, ZeroOrMore<Element<Network::Xml::Host1, Name::Strict<494> > >, Optional<Element<Network::Xml::Bootp, Name::Strict<1184> > > > > marshal_type;

	static int parse(Network::Xml::Dhcp& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Dhcp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Ip traits

template<>
struct Traits<Network::Xml::Ip>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::VIpAddr, Name::Strict<105> > >, Optional<Network::Xml::VChoice1175Impl >, Optional<Attribute<Network::Xml::PAddrFamily, Name::Strict<652> > >, Optional<Element<Attribute<QString, Name::Strict<420> >, Name::Strict<493> > >, Optional<Element<Network::Xml::Dhcp, Name::Strict<1177> > > > > marshal_type;

	static int parse(Network::Xml::Ip& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Ip& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Route traits

template<>
struct Traits<Network::Xml::Route>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::PAddrFamily, Name::Strict<652> > >, Attribute<Network::Xml::VIpAddr, Name::Strict<105> >, Optional<Network::Xml::VChoice1186Impl >, Attribute<Network::Xml::VIpAddr, Name::Strict<1188> >, Optional<Attribute<Network::Xml::PUnsignedInt, Name::Strict<1189> > > > > marshal_type;

	static int parse(Network::Xml::Route& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Route& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Network traits

template<>
struct Traits<Network::Xml::Network>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Network::Xml::PConnections, Name::Strict<1153> > >, Optional<Attribute<Network::Xml::EVirYesNo, Name::Strict<1154> > >, Unordered<mpl::vector<Element<Text<QString >, Name::Strict<101> >, Optional<Element<Text<Network::Xml::VUUID >, Name::Strict<139> > >, Optional<Element<Network::Xml::Bridge, Name::Strict<618> > >, Optional<Element<Attribute<Network::Xml::PUniMacAddr, Name::Strict<105> >, Name::Strict<635> > >, Optional<Element<Network::Xml::Forward, Name::Strict<1156> > >, Optional<Network::Xml::VVirtualPortProfileImpl >, ZeroOrMore<Element<Network::Xml::Portgroup, Name::Strict<627> > >, Optional<Element<Attribute<Network::Xml::PDnsName, Name::Strict<101> >, Name::Strict<1> > >, Optional<Element<Network::Xml::Dns, Name::Strict<1165> > >, Optional<Element<Network::Xml::Bandwidth, Name::Strict<181> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<129>, Name::Strict<194> > >, OneOrMore<Element<Network::Xml::Tag, Name::Strict<195> > > > >, Name::Strict<193> > >, Optional<Element<Attribute<Network::Xml::EState, Name::Strict<120> >, Name::Strict<118> > >, ZeroOrMore<Element<Network::Xml::Ip, Name::Strict<651> > >, ZeroOrMore<Element<Network::Xml::Route, Name::Strict<1158> > > > > > > marshal_type;

	static int parse(Network::Xml::Network& , QStack<QDomElement>& );
	static int generate(const Network::Xml::Network& , QDomElement& );
};

} // namespace Libvirt

#endif // __NETWORK_TYPE_H__
