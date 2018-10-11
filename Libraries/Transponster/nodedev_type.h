/*
 * Copyright (c) 2015-2017, Parallels International GmbH
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
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __NODEDEV_TYPE_H__
#define __NODEDEV_TYPE_H__
#include "base.h"
#include "nodedev_data.h"
#include "nodedev_enum.h"
#include "patterns.h"
#include <boost/any.hpp>

namespace Libvirt
{

///////////////////////////////////////////////////////////////////////////////
// struct Parent3467

namespace Nodedev
{
namespace Xml
{
struct Parent3467
{
	const PWwn::value_type& getWwnn() const
	{
		return m_wwnn;
	}
	void setWwnn(const PWwn::value_type& value_)
	{
		m_wwnn = value_;
	}
	const PWwn::value_type& getWwpn() const
	{
		return m_wwpn;
	}
	void setWwpn(const PWwn::value_type& value_)
	{
		m_wwpn = value_;
	}

private:
	PWwn::value_type m_wwnn;
	PWwn::value_type m_wwpn;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct VParent

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<Parent3467, Attribute<Nodedev::Xml::PWwn, Name::Strict<3469> >, Text<QString > > > VParentImpl;
typedef VParentImpl::value_type VParent;

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Hardware

namespace Nodedev
{
namespace Xml
{
struct Hardware
{
	const boost::optional<QString >& getVendor() const
	{
		return m_vendor;
	}
	void setVendor(const boost::optional<QString >& value_)
	{
		m_vendor = value_;
	}
	const boost::optional<QString >& getVersion() const
	{
		return m_version;
	}
	void setVersion(const boost::optional<QString >& value_)
	{
		m_version = value_;
	}
	const boost::optional<QString >& getSerial() const
	{
		return m_serial;
	}
	void setSerial(const boost::optional<QString >& value_)
	{
		m_serial = value_;
	}
	const VUUID& getUuid() const
	{
		return m_uuid;
	}
	void setUuid(const VUUID& value_)
	{
		m_uuid = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QString > m_vendor;
	boost::optional<QString > m_version;
	boost::optional<QString > m_serial;
	VUUID m_uuid;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Firmware

namespace Nodedev
{
namespace Xml
{
struct Firmware
{
	const boost::optional<QString >& getVendor() const
	{
		return m_vendor;
	}
	void setVendor(const boost::optional<QString >& value_)
	{
		m_vendor = value_;
	}
	const boost::optional<QString >& getVersion() const
	{
		return m_version;
	}
	void setVersion(const boost::optional<QString >& value_)
	{
		m_version = value_;
	}
	const boost::optional<QString >& getReleaseDate() const
	{
		return m_releaseDate;
	}
	void setReleaseDate(const boost::optional<QString >& value_)
	{
		m_releaseDate = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QString > m_vendor;
	boost::optional<QString > m_version;
	boost::optional<QString > m_releaseDate;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capsystem

namespace Nodedev
{
namespace Xml
{
struct Capsystem
{
	const boost::optional<QString >& getProduct() const
	{
		return m_product;
	}
	void setProduct(const boost::optional<QString >& value_)
	{
		m_product = value_;
	}
	const Hardware& getHardware() const
	{
		return m_hardware;
	}
	void setHardware(const Hardware& value_)
	{
		m_hardware = value_;
	}
	const Firmware& getFirmware() const
	{
		return m_firmware;
	}
	void setFirmware(const Firmware& value_)
	{
		m_firmware = value_;
	}

private:
	boost::optional<QString > m_product;
	Hardware m_hardware;
	Firmware m_firmware;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Product

namespace Nodedev
{
namespace Xml
{
struct Product
{
	const PHexuint::value_type& getId() const
	{
		return m_id;
	}
	void setId(const PHexuint::value_type& value_)
	{
		m_id = value_;
	}
	const QString& getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(const QString& value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PHexuint::value_type m_id;
	QString m_ownValue;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Vendor

namespace Nodedev
{
namespace Xml
{
struct Vendor
{
	const PHexuint::value_type& getId() const
	{
		return m_id;
	}
	void setId(const PHexuint::value_type& value_)
	{
		m_id = value_;
	}
	const QString& getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(const QString& value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PHexuint::value_type m_id;
	QString m_ownValue;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Address

namespace Nodedev
{
namespace Xml
{
struct Address
{
	const PHexuint::value_type& getDomain() const
	{
		return m_domain;
	}
	void setDomain(const PHexuint::value_type& value_)
	{
		m_domain = value_;
	}
	const PHexuint::value_type& getBus() const
	{
		return m_bus;
	}
	void setBus(const PHexuint::value_type& value_)
	{
		m_bus = value_;
	}
	const PHexuint::value_type& getSlot() const
	{
		return m_slot;
	}
	void setSlot(const PHexuint::value_type& value_)
	{
		m_slot = value_;
	}
	const PHexuint::value_type& getFunction() const
	{
		return m_function;
	}
	void setFunction(const PHexuint::value_type& value_)
	{
		m_function = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PHexuint::value_type m_domain;
	PHexuint::value_type m_bus;
	PHexuint::value_type m_slot;
	PHexuint::value_type m_function;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capability

namespace Nodedev
{
namespace Xml
{
struct Capability
{
	const boost::optional<PUnsignedInt::value_type >& getMaxCount() const
	{
		return m_maxCount;
	}
	void setMaxCount(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_maxCount = value_;
	}
	const QList<Address >& getAddressList() const
	{
		return m_addressList;
	}
	void setAddressList(const QList<Address >& value_)
	{
		m_addressList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PUnsignedInt::value_type > m_maxCount;
	QList<Address > m_addressList;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Type

namespace Nodedev
{
namespace Xml
{
struct Type
{
	Type();

	const PId::value_type& getId() const
	{
		return m_id;
	}
	void setId(const PId::value_type& value_)
	{
		m_id = value_;
	}
	const boost::optional<QString >& getName() const
	{
		return m_name;
	}
	void setName(const boost::optional<QString >& value_)
	{
		m_name = value_;
	}
	PUnsignedInt::value_type getAvailableInstances() const
	{
		return m_availableInstances;
	}
	void setAvailableInstances(PUnsignedInt::value_type value_)
	{
		m_availableInstances = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PId::value_type m_id;
	boost::optional<QString > m_name;
	PUnsignedInt::value_type m_availableInstances;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct IommuGroup

namespace Nodedev
{
namespace Xml
{
struct IommuGroup
{
	IommuGroup();

	PUnsignedInt::value_type getNumber() const
	{
		return m_number;
	}
	void setNumber(PUnsignedInt::value_type value_)
	{
		m_number = value_;
	}
	const QList<Address >& getAddressList() const
	{
		return m_addressList;
	}
	void setAddressList(const QList<Address >& value_)
	{
		m_addressList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_number;
	QList<Address > m_addressList;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Numa

namespace Nodedev
{
namespace Xml
{
struct Numa
{
	const boost::optional<PNode::value_type >& getNode() const
	{
		return m_node;
	}
	void setNode(const boost::optional<PNode::value_type >& value_)
	{
		m_node = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PNode::value_type > m_node;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Link

namespace Nodedev
{
namespace Xml
{
struct Link
{
	Link();

	EValidity getValidity() const
	{
		return m_validity;
	}
	void setValidity(EValidity value_)
	{
		m_validity = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_port = value_;
	}
	const boost::optional<PSpeed::value_type >& getSpeed() const
	{
		return m_speed;
	}
	void setSpeed(const boost::optional<PSpeed::value_type >& value_)
	{
		m_speed = value_;
	}
	PUnsignedInt::value_type getWidth() const
	{
		return m_width;
	}
	void setWidth(PUnsignedInt::value_type value_)
	{
		m_width = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EValidity m_validity;
	boost::optional<PUnsignedInt::value_type > m_port;
	boost::optional<PSpeed::value_type > m_speed;
	PUnsignedInt::value_type m_width;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Cappcidev

namespace Nodedev
{
namespace Xml
{
struct Cappcidev
{
	Cappcidev();

	PUnsignedLong::value_type getDomain() const
	{
		return m_domain;
	}
	void setDomain(PUnsignedLong::value_type value_)
	{
		m_domain = value_;
	}
	PUnsignedLong::value_type getBus() const
	{
		return m_bus;
	}
	void setBus(PUnsignedLong::value_type value_)
	{
		m_bus = value_;
	}
	PUnsignedLong::value_type getSlot() const
	{
		return m_slot;
	}
	void setSlot(PUnsignedLong::value_type value_)
	{
		m_slot = value_;
	}
	PUnsignedLong::value_type getFunction() const
	{
		return m_function;
	}
	void setFunction(PUnsignedLong::value_type value_)
	{
		m_function = value_;
	}
	const Product& getProduct() const
	{
		return m_product;
	}
	void setProduct(const Product& value_)
	{
		m_product = value_;
	}
	const Vendor& getVendor() const
	{
		return m_vendor;
	}
	void setVendor(const Vendor& value_)
	{
		m_vendor = value_;
	}
	const boost::optional<Address >& getCapability() const
	{
		return m_capability;
	}
	void setCapability(const boost::optional<Address >& value_)
	{
		m_capability = value_;
	}
	const boost::optional<Capability >& getCapability2() const
	{
		return m_capability2;
	}
	void setCapability2(const boost::optional<Capability >& value_)
	{
		m_capability2 = value_;
	}
	const boost::optional<EType >& getCapability3() const
	{
		return m_capability3;
	}
	void setCapability3(const boost::optional<EType >& value_)
	{
		m_capability3 = value_;
	}
	const boost::optional<QList<Type > >& getCapability4() const
	{
		return m_capability4;
	}
	void setCapability4(const boost::optional<QList<Type > >& value_)
	{
		m_capability4 = value_;
	}
	const boost::optional<IommuGroup >& getIommuGroup() const
	{
		return m_iommuGroup;
	}
	void setIommuGroup(const boost::optional<IommuGroup >& value_)
	{
		m_iommuGroup = value_;
	}
	const boost::optional<Numa >& getNuma() const
	{
		return m_numa;
	}
	void setNuma(const boost::optional<Numa >& value_)
	{
		m_numa = value_;
	}
	const boost::optional<QList<Link > >& getPciExpress() const
	{
		return m_pciExpress;
	}
	void setPciExpress(const boost::optional<QList<Link > >& value_)
	{
		m_pciExpress = value_;
	}

private:
	PUnsignedLong::value_type m_domain;
	PUnsignedLong::value_type m_bus;
	PUnsignedLong::value_type m_slot;
	PUnsignedLong::value_type m_function;
	Product m_product;
	Vendor m_vendor;
	boost::optional<Address > m_capability;
	boost::optional<Capability > m_capability2;
	boost::optional<EType > m_capability3;
	boost::optional<QList<Type > > m_capability4;
	boost::optional<IommuGroup > m_iommuGroup;
	boost::optional<Numa > m_numa;
	boost::optional<QList<Link > > m_pciExpress;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Product1

namespace Nodedev
{
namespace Xml
{
struct Product1
{
	const PHexuint::value_type& getId() const
	{
		return m_id;
	}
	void setId(const PHexuint::value_type& value_)
	{
		m_id = value_;
	}
	const QString& getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(const QString& value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PHexuint::value_type m_id;
	QString m_ownValue;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Vendor1

namespace Nodedev
{
namespace Xml
{
struct Vendor1
{
	const PHexuint::value_type& getId() const
	{
		return m_id;
	}
	void setId(const PHexuint::value_type& value_)
	{
		m_id = value_;
	}
	const QString& getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(const QString& value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PHexuint::value_type m_id;
	QString m_ownValue;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capusbdev

namespace Nodedev
{
namespace Xml
{
struct Capusbdev
{
	Capusbdev();

	PUnsignedLong::value_type getBus() const
	{
		return m_bus;
	}
	void setBus(PUnsignedLong::value_type value_)
	{
		m_bus = value_;
	}
	PUnsignedLong::value_type getDevice() const
	{
		return m_device;
	}
	void setDevice(PUnsignedLong::value_type value_)
	{
		m_device = value_;
	}
	const Product1& getProduct() const
	{
		return m_product;
	}
	void setProduct(const Product1& value_)
	{
		m_product = value_;
	}
	const Vendor1& getVendor() const
	{
		return m_vendor;
	}
	void setVendor(const Vendor1& value_)
	{
		m_vendor = value_;
	}

private:
	PUnsignedLong::value_type m_bus;
	PUnsignedLong::value_type m_device;
	Product1 m_product;
	Vendor1 m_vendor;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capusbinterface

namespace Nodedev
{
namespace Xml
{
struct Capusbinterface
{
	Capusbinterface();

	PUnsignedLong::value_type getNumber() const
	{
		return m_number;
	}
	void setNumber(PUnsignedLong::value_type value_)
	{
		m_number = value_;
	}
	PUnsignedLong::value_type getClass() const
	{
		return m_class;
	}
	void setClass(PUnsignedLong::value_type value_)
	{
		m_class = value_;
	}
	PUnsignedLong::value_type getSubclass() const
	{
		return m_subclass;
	}
	void setSubclass(PUnsignedLong::value_type value_)
	{
		m_subclass = value_;
	}
	PUnsignedLong::value_type getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(PUnsignedLong::value_type value_)
	{
		m_protocol = value_;
	}
	const boost::optional<QString >& getDescription() const
	{
		return m_description;
	}
	void setDescription(const boost::optional<QString >& value_)
	{
		m_description = value_;
	}

private:
	PUnsignedLong::value_type m_number;
	PUnsignedLong::value_type m_class;
	PUnsignedLong::value_type m_subclass;
	PUnsignedLong::value_type m_protocol;
	boost::optional<QString > m_description;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Link1

namespace Nodedev
{
namespace Xml
{
struct Link1
{
	const boost::optional<PUnsignedInt::value_type >& getSpeed() const
	{
		return m_speed;
	}
	void setSpeed(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_speed = value_;
	}
	const boost::optional<EState >& getState() const
	{
		return m_state;
	}
	void setState(const boost::optional<EState >& value_)
	{
		m_state = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PUnsignedInt::value_type > m_speed;
	boost::optional<EState > m_state;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capability3550

namespace Nodedev
{
namespace Xml
{
struct Capability3550
{
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capability3551

namespace Nodedev
{
namespace Xml
{
struct Capability3551
{
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct VCapability1

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<Capability3550, Capability3551 > > VCapability1Impl;
typedef VCapability1Impl::value_type VCapability1;

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capnet

namespace Nodedev
{
namespace Xml
{
struct Capnet
{
	const QString& getInterface() const
	{
		return m_interface;
	}
	void setInterface(const QString& value_)
	{
		m_interface = value_;
	}
	const boost::optional<PMac::value_type >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<PMac::value_type >& value_)
	{
		m_address = value_;
	}
	const boost::optional<Link1 >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<Link1 >& value_)
	{
		m_link = value_;
	}
	const QList<PNetfeaturename::value_type >& getFeatureList() const
	{
		return m_featureList;
	}
	void setFeatureList(const QList<PNetfeaturename::value_type >& value_)
	{
		m_featureList = value_;
	}
	const QList<VCapability1 >& getCapabilityList() const
	{
		return m_capabilityList;
	}
	void setCapabilityList(const QList<VCapability1 >& value_)
	{
		m_capabilityList = value_;
	}

private:
	QString m_interface;
	boost::optional<PMac::value_type > m_address;
	boost::optional<Link1 > m_link;
	QList<PNetfeaturename::value_type > m_featureList;
	QList<VCapability1 > m_capabilityList;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capsfchost

namespace Nodedev
{
namespace Xml
{
struct Capsfchost
{
	const PWwn::value_type& getWwnn() const
	{
		return m_wwnn;
	}
	void setWwnn(const PWwn::value_type& value_)
	{
		m_wwnn = value_;
	}
	const PWwn::value_type& getWwpn() const
	{
		return m_wwpn;
	}
	void setWwpn(const PWwn::value_type& value_)
	{
		m_wwpn = value_;
	}
	const boost::optional<PWwn::value_type >& getFabricWwn() const
	{
		return m_fabricWwn;
	}
	void setFabricWwn(const boost::optional<PWwn::value_type >& value_)
	{
		m_fabricWwn = value_;
	}

private:
	PWwn::value_type m_wwnn;
	PWwn::value_type m_wwpn;
	boost::optional<PWwn::value_type > m_fabricWwn;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capsvports

namespace Nodedev
{
namespace Xml
{
struct Capsvports
{
	Capsvports();

	PUnsignedInt::value_type getMaxVports() const
	{
		return m_maxVports;
	}
	void setMaxVports(PUnsignedInt::value_type value_)
	{
		m_maxVports = value_;
	}
	PUnsignedInt::value_type getVports() const
	{
		return m_vports;
	}
	void setVports(PUnsignedInt::value_type value_)
	{
		m_vports = value_;
	}

private:
	PUnsignedInt::value_type m_maxVports;
	PUnsignedInt::value_type m_vports;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct VCapability2

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<Capsfchost, Capsvports > > VCapability2Impl;
typedef VCapability2Impl::value_type VCapability2;

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capscsihost

namespace Nodedev
{
namespace Xml
{
struct Capscsihost
{
	Capscsihost();

	PUnsignedLong::value_type getHost() const
	{
		return m_host;
	}
	void setHost(PUnsignedLong::value_type value_)
	{
		m_host = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getUniqueId() const
	{
		return m_uniqueId;
	}
	void setUniqueId(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_uniqueId = value_;
	}
	const boost::optional<QList<VCapability2 > >& getCapabilityList() const
	{
		return m_capabilityList;
	}
	void setCapabilityList(const boost::optional<QList<VCapability2 > >& value_)
	{
		m_capabilityList = value_;
	}

private:
	PUnsignedLong::value_type m_host;
	boost::optional<PPositiveInteger::value_type > m_uniqueId;
	boost::optional<QList<VCapability2 > > m_capabilityList;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capsfcrport

namespace Nodedev
{
namespace Xml
{
struct Capsfcrport
{
	const QString& getRport() const
	{
		return m_rport;
	}
	void setRport(const QString& value_)
	{
		m_rport = value_;
	}
	const PWwn::value_type& getWwpn() const
	{
		return m_wwpn;
	}
	void setWwpn(const PWwn::value_type& value_)
	{
		m_wwpn = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_rport;
	PWwn::value_type m_wwpn;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capscsitarget

namespace Nodedev
{
namespace Xml
{
struct Capscsitarget
{
	const QString& getTarget() const
	{
		return m_target;
	}
	void setTarget(const QString& value_)
	{
		m_target = value_;
	}
	const boost::optional<Capsfcrport >& getCapability() const
	{
		return m_capability;
	}
	void setCapability(const boost::optional<Capsfcrport >& value_)
	{
		m_capability = value_;
	}

private:
	QString m_target;
	boost::optional<Capsfcrport > m_capability;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capscsi

namespace Nodedev
{
namespace Xml
{
struct Capscsi
{
	Capscsi();

	PUnsignedLong::value_type getHost() const
	{
		return m_host;
	}
	void setHost(PUnsignedLong::value_type value_)
	{
		m_host = value_;
	}
	PUnsignedLong::value_type getBus() const
	{
		return m_bus;
	}
	void setBus(PUnsignedLong::value_type value_)
	{
		m_bus = value_;
	}
	PUnsignedLong::value_type getTarget() const
	{
		return m_target;
	}
	void setTarget(PUnsignedLong::value_type value_)
	{
		m_target = value_;
	}
	PUnsignedLong::value_type getLun() const
	{
		return m_lun;
	}
	void setLun(PUnsignedLong::value_type value_)
	{
		m_lun = value_;
	}
	const QString& getType() const
	{
		return m_type;
	}
	void setType(const QString& value_)
	{
		m_type = value_;
	}

private:
	PUnsignedLong::value_type m_host;
	PUnsignedLong::value_type m_bus;
	PUnsignedLong::value_type m_target;
	PUnsignedLong::value_type m_lun;
	QString m_type;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capability1

namespace Nodedev
{
namespace Xml
{
struct Capability1
{
	Capability1();

	EMediaAvailable getMediaAvailable() const
	{
		return m_mediaAvailable;
	}
	void setMediaAvailable(EMediaAvailable value_)
	{
		m_mediaAvailable = value_;
	}
	PUnsignedLong::value_type getMediaSize() const
	{
		return m_mediaSize;
	}
	void setMediaSize(PUnsignedLong::value_type value_)
	{
		m_mediaSize = value_;
	}
	const boost::optional<QString >& getMediaLabel() const
	{
		return m_mediaLabel;
	}
	void setMediaLabel(const boost::optional<QString >& value_)
	{
		m_mediaLabel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EMediaAvailable m_mediaAvailable;
	PUnsignedLong::value_type m_mediaSize;
	boost::optional<QString > m_mediaLabel;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct VCapstorage

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Nodedev::Xml::Capability1, Name::Strict<3467> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<334> > > > VCapstorageImpl;
typedef VCapstorageImpl::value_type VCapstorage;

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capstorage

namespace Nodedev
{
namespace Xml
{
struct Capstorage
{
	const PPath::value_type& getBlock() const
	{
		return m_block;
	}
	void setBlock(const PPath::value_type& value_)
	{
		m_block = value_;
	}
	const boost::optional<QString >& getBus() const
	{
		return m_bus;
	}
	void setBus(const boost::optional<QString >& value_)
	{
		m_bus = value_;
	}
	const boost::optional<QString >& getDriveType() const
	{
		return m_driveType;
	}
	void setDriveType(const boost::optional<QString >& value_)
	{
		m_driveType = value_;
	}
	const boost::optional<QString >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<QString >& value_)
	{
		m_model = value_;
	}
	const boost::optional<QString >& getVendor() const
	{
		return m_vendor;
	}
	void setVendor(const boost::optional<QString >& value_)
	{
		m_vendor = value_;
	}
	const boost::optional<QString >& getSerial() const
	{
		return m_serial;
	}
	void setSerial(const boost::optional<QString >& value_)
	{
		m_serial = value_;
	}
	const VCapstorage& getCapstorage() const
	{
		return m_capstorage;
	}
	void setCapstorage(const VCapstorage& value_)
	{
		m_capstorage = value_;
	}

private:
	PPath::value_type m_block;
	boost::optional<QString > m_bus;
	boost::optional<QString > m_driveType;
	boost::optional<QString > m_model;
	boost::optional<QString > m_vendor;
	boost::optional<QString > m_serial;
	VCapstorage m_capstorage;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capmdev

namespace Nodedev
{
namespace Xml
{
struct Capmdev
{
	Capmdev();

	const PId::value_type& getType() const
	{
		return m_type;
	}
	void setType(const PId::value_type& value_)
	{
		m_type = value_;
	}
	PUnsignedInt::value_type getIommuGroup() const
	{
		return m_iommuGroup;
	}
	void setIommuGroup(PUnsignedInt::value_type value_)
	{
		m_iommuGroup = value_;
	}

private:
	PId::value_type m_type;
	PUnsignedInt::value_type m_iommuGroup;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Capccwdev

namespace Nodedev
{
namespace Xml
{
struct Capccwdev
{
	const VCcwCssidRange& getCssid() const
	{
		return m_cssid;
	}
	void setCssid(const VCcwCssidRange& value_)
	{
		m_cssid = value_;
	}
	const PCcwSsidRange::value_type& getSsid() const
	{
		return m_ssid;
	}
	void setSsid(const PCcwSsidRange::value_type& value_)
	{
		m_ssid = value_;
	}
	const VCcwDevnoRange& getDevno() const
	{
		return m_devno;
	}
	void setDevno(const VCcwDevnoRange& value_)
	{
		m_devno = value_;
	}

private:
	VCcwCssidRange m_cssid;
	PCcwSsidRange::value_type m_ssid;
	VCcwDevnoRange m_devno;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct VCapability

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<Capsystem, Cappcidev, Capusbdev, Capusbinterface, Capnet, Capscsihost, Capscsitarget, Capscsi, Capstorage, Ordered<mpl::vector<Attribute<mpl::int_<3547>, Name::Strict<105> >, Element<Text<Nodedev::Xml::EType1 >, Name::Strict<105> > > >, Capmdev, Capccwdev > > VCapabilityImpl;
typedef VCapabilityImpl::value_type VCapability;

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Device

namespace Nodedev
{
namespace Xml
{
struct Device
{
	const QString& getName() const
	{
		return m_name;
	}
	void setName(const QString& value_)
	{
		m_name = value_;
	}
	const boost::optional<QString >& getPath() const
	{
		return m_path;
	}
	void setPath(const boost::optional<QString >& value_)
	{
		m_path = value_;
	}
	const boost::optional<QString >& getDevnode() const
	{
		return m_devnode;
	}
	void setDevnode(const boost::optional<QString >& value_)
	{
		m_devnode = value_;
	}
	const QList<QString >& getDevnodeList() const
	{
		return m_devnodeList;
	}
	void setDevnodeList(const QList<QString >& value_)
	{
		m_devnodeList = value_;
	}
	const boost::optional<VParent >& getParent() const
	{
		return m_parent;
	}
	void setParent(const boost::optional<VParent >& value_)
	{
		m_parent = value_;
	}
	const boost::optional<QString >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<QString >& value_)
	{
		m_driver = value_;
	}
	const QList<VCapability >& getCapabilityList() const
	{
		return m_capabilityList;
	}
	void setCapabilityList(const QList<VCapability >& value_)
	{
		m_capabilityList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_name;
	boost::optional<QString > m_path;
	boost::optional<QString > m_devnode;
	QList<QString > m_devnodeList;
	boost::optional<VParent > m_parent;
	boost::optional<QString > m_driver;
	QList<VCapability > m_capabilityList;
};

} // namespace Xml
} // namespace Nodedev

///////////////////////////////////////////////////////////////////////////////
// struct Parent3467 traits

template<>
struct Traits<Nodedev::Xml::Parent3467>
{
	typedef Ordered<mpl::vector<Attribute<Nodedev::Xml::PWwn, Name::Strict<117> >, Attribute<Nodedev::Xml::PWwn, Name::Strict<118> > > > marshal_type;

	static int parse(Nodedev::Xml::Parent3467& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Parent3467& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hardware traits

template<>
struct Traits<Nodedev::Xml::Hardware>
{
	typedef Ordered<mpl::vector<Optional<Element<Text<QString >, Name::Strict<459> > >, Optional<Element<Text<QString >, Name::Strict<852> > >, Optional<Element<Text<QString >, Name::Strict<453> > >, Element<Text<Nodedev::Xml::VUUID >, Name::Strict<151> > > > marshal_type;

	static int parse(Nodedev::Xml::Hardware& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Hardware& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Firmware traits

template<>
struct Traits<Nodedev::Xml::Firmware>
{
	typedef Ordered<mpl::vector<Optional<Element<Text<QString >, Name::Strict<459> > >, Optional<Element<Text<QString >, Name::Strict<852> > >, Optional<Element<Text<QString >, Name::Strict<3498> > > > > marshal_type;

	static int parse(Nodedev::Xml::Firmware& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Firmware& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capsystem traits

template<>
struct Traits<Nodedev::Xml::Capsystem>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<1042>, Name::Strict<105> >, Optional<Element<Text<QString >, Name::Strict<460> > >, Element<Nodedev::Xml::Hardware, Name::Strict<3496> >, Element<Nodedev::Xml::Firmware, Name::Strict<3497> > > > marshal_type;

	static int parse(Nodedev::Xml::Capsystem& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capsystem& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Product traits

template<>
struct Traits<Nodedev::Xml::Product>
{
	typedef Ordered<mpl::vector<Attribute<Nodedev::Xml::PHexuint, Name::Strict<208> >, Text<QString > > > marshal_type;

	static int parse(Nodedev::Xml::Product& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Product& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Vendor traits

template<>
struct Traits<Nodedev::Xml::Vendor>
{
	typedef Ordered<mpl::vector<Attribute<Nodedev::Xml::PHexuint, Name::Strict<208> >, Text<QString > > > marshal_type;

	static int parse(Nodedev::Xml::Vendor& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Vendor& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Address traits

template<>
struct Traits<Nodedev::Xml::Address>
{
	typedef Ordered<mpl::vector<Attribute<Nodedev::Xml::PHexuint, Name::Strict<1> >, Attribute<Nodedev::Xml::PHexuint, Name::Strict<29> >, Attribute<Nodedev::Xml::PHexuint, Name::Strict<31> >, Attribute<Nodedev::Xml::PHexuint, Name::Strict<33> > > > marshal_type;

	static int parse(Nodedev::Xml::Address& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Address& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capability traits

template<>
struct Traits<Nodedev::Xml::Capability>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<3502>, Name::Strict<105> >, Optional<Attribute<Nodedev::Xml::PUnsignedInt, Name::Strict<3503> > >, ZeroOrMore<Element<Nodedev::Xml::Address, Name::Strict<111> > > > > marshal_type;

	static int parse(Nodedev::Xml::Capability& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capability& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Type traits

template<>
struct Traits<Nodedev::Xml::Type>
{
	typedef Ordered<mpl::vector<Attribute<Nodedev::Xml::PId, Name::Strict<208> >, Optional<Element<Text<QString >, Name::Strict<107> > >, Element<Text<mpl::int_<3507> >, Name::Strict<3506> >, Element<Text<Nodedev::Xml::PUnsignedInt >, Name::Strict<3508> > > > marshal_type;

	static int parse(Nodedev::Xml::Type& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Type& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct IommuGroup traits

template<>
struct Traits<Nodedev::Xml::IommuGroup>
{
	typedef Ordered<mpl::vector<Attribute<Nodedev::Xml::PUnsignedInt, Name::Strict<3510> >, OneOrMore<Element<Nodedev::Xml::Address, Name::Strict<111> > > > > marshal_type;

	static int parse(Nodedev::Xml::IommuGroup& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::IommuGroup& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Numa traits

template<>
struct Traits<Nodedev::Xml::Numa>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Nodedev::Xml::PNode, Name::Strict<609> > > > > marshal_type;

	static int parse(Nodedev::Xml::Numa& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Numa& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Link traits

template<>
struct Traits<Nodedev::Xml::Link>
{
	typedef Ordered<mpl::vector<Attribute<Nodedev::Xml::EValidity, Name::Strict<3512> >, Optional<Attribute<Nodedev::Xml::PUnsignedInt, Name::Strict<212> > >, Optional<Attribute<Nodedev::Xml::PSpeed, Name::Strict<125> > >, Attribute<Nodedev::Xml::PUnsignedInt, Name::Strict<3515> > > > marshal_type;

	static int parse(Nodedev::Xml::Link& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Link& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cappcidev traits

template<>
struct Traits<Nodedev::Xml::Cappcidev>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<596>, Name::Strict<105> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<1> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<29> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<31> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<33> >, Element<Nodedev::Xml::Product, Name::Strict<460> >, Element<Nodedev::Xml::Vendor, Name::Strict<459> >, Optional<Element<Ordered<mpl::vector<Attribute<mpl::int_<3501>, Name::Strict<105> >, Optional<Element<Nodedev::Xml::Address, Name::Strict<111> > > > >, Name::Strict<3467> > >, Optional<Element<Nodedev::Xml::Capability, Name::Strict<3467> > >, Optional<Element<Attribute<Nodedev::Xml::EType, Name::Strict<105> >, Name::Strict<3467> > >, Optional<Element<Ordered<mpl::vector<Attribute<mpl::int_<3505>, Name::Strict<105> >, OneOrMore<Element<Nodedev::Xml::Type, Name::Strict<105> > > > >, Name::Strict<3467> > >, Optional<Element<Nodedev::Xml::IommuGroup, Name::Strict<3509> > >, Optional<Element<Nodedev::Xml::Numa, Name::Strict<1031> > >, Optional<Element<ZeroOrMore<Element<Nodedev::Xml::Link, Name::Strict<124> > >, Name::Strict<3511> > > > > marshal_type;

	static int parse(Nodedev::Xml::Cappcidev& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Cappcidev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Product1 traits

template<>
struct Traits<Nodedev::Xml::Product1>
{
	typedef Ordered<mpl::vector<Attribute<Nodedev::Xml::PHexuint, Name::Strict<208> >, Text<QString > > > marshal_type;

	static int parse(Nodedev::Xml::Product1& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Product1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Vendor1 traits

template<>
struct Traits<Nodedev::Xml::Vendor1>
{
	typedef Ordered<mpl::vector<Attribute<Nodedev::Xml::PHexuint, Name::Strict<208> >, Text<QString > > > marshal_type;

	static int parse(Nodedev::Xml::Vendor1& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Vendor1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capusbdev traits

template<>
struct Traits<Nodedev::Xml::Capusbdev>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<3516>, Name::Strict<105> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<29> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<354> >, Element<Nodedev::Xml::Product1, Name::Strict<460> >, Element<Nodedev::Xml::Vendor1, Name::Strict<459> > > > marshal_type;

	static int parse(Nodedev::Xml::Capusbdev& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capusbdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capusbinterface traits

template<>
struct Traits<Nodedev::Xml::Capusbinterface>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<531>, Name::Strict<105> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<3510> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<842> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<3519> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<203> >, Optional<Element<Text<QString >, Name::Strict<216> > > > > marshal_type;

	static int parse(Nodedev::Xml::Capusbinterface& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capusbinterface& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Link1 traits

template<>
struct Traits<Nodedev::Xml::Link1>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Nodedev::Xml::PUnsignedInt, Name::Strict<125> > >, Optional<Attribute<Nodedev::Xml::EState, Name::Strict<126> > > > > marshal_type;

	static int parse(Nodedev::Xml::Link1& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Link1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capability3550 traits

template<>
struct Traits<Nodedev::Xml::Capability3550>
{
	typedef Attribute<mpl::int_<3526>, Name::Strict<105> > marshal_type;

	static int parse(Nodedev::Xml::Capability3550& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capability3550& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capability3551 traits

template<>
struct Traits<Nodedev::Xml::Capability3551>
{
	typedef Attribute<mpl::int_<3527>, Name::Strict<105> > marshal_type;

	static int parse(Nodedev::Xml::Capability3551& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capability3551& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capnet traits

template<>
struct Traits<Nodedev::Xml::Capnet>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<941>, Name::Strict<105> >, Element<Text<QString >, Name::Strict<657> >, Optional<Element<Text<Nodedev::Xml::PMac >, Name::Strict<111> > >, Optional<Element<Nodedev::Xml::Link1, Name::Strict<124> > >, ZeroOrMore<Element<Attribute<Nodedev::Xml::PNetfeaturename, Name::Strict<107> >, Name::Strict<1022> > >, ZeroOrMore<Element<Nodedev::Xml::VCapability1Impl, Name::Strict<3467> > > > > marshal_type;

	static int parse(Nodedev::Xml::Capnet& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capnet& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capsfchost traits

template<>
struct Traits<Nodedev::Xml::Capsfchost>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<115>, Name::Strict<105> >, Element<Text<Nodedev::Xml::PWwn >, Name::Strict<117> >, Element<Text<Nodedev::Xml::PWwn >, Name::Strict<118> >, Optional<Element<Text<Nodedev::Xml::PWwn >, Name::Strict<3469> > > > > marshal_type;

	static int parse(Nodedev::Xml::Capsfchost& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capsfchost& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capsvports traits

template<>
struct Traits<Nodedev::Xml::Capsvports>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<3530>, Name::Strict<105> >, Element<Text<Nodedev::Xml::PUnsignedInt >, Name::Strict<3531> >, Element<Text<Nodedev::Xml::PUnsignedInt >, Name::Strict<3532> > > > marshal_type;

	static int parse(Nodedev::Xml::Capsvports& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capsvports& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capscsihost traits

template<>
struct Traits<Nodedev::Xml::Capscsihost>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<106>, Name::Strict<105> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<513> >, Optional<Element<Text<Nodedev::Xml::PPositiveInteger >, Name::Strict<110> > >, Optional<ZeroOrMore<Element<Nodedev::Xml::VCapability2Impl, Name::Strict<3467> > > > > > marshal_type;

	static int parse(Nodedev::Xml::Capscsihost& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capscsihost& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capsfcrport traits

template<>
struct Traits<Nodedev::Xml::Capsfcrport>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<3536>, Name::Strict<105> >, Element<Text<QString >, Name::Strict<3537> >, Element<Text<Nodedev::Xml::PWwn >, Name::Strict<118> > > > marshal_type;

	static int parse(Nodedev::Xml::Capsfcrport& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capsfcrport& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capscsitarget traits

template<>
struct Traits<Nodedev::Xml::Capscsitarget>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<3538>, Name::Strict<105> >, Element<Text<QString >, Name::Strict<323> >, Optional<Element<Nodedev::Xml::Capsfcrport, Name::Strict<3467> > > > > marshal_type;

	static int parse(Nodedev::Xml::Capscsitarget& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capscsitarget& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capscsi traits

template<>
struct Traits<Nodedev::Xml::Capscsi>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<529>, Name::Strict<105> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<513> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<29> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<323> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<475> >, Element<Text<QString >, Name::Strict<105> > > > marshal_type;

	static int parse(Nodedev::Xml::Capscsi& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capscsi& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capability1 traits

template<>
struct Traits<Nodedev::Xml::Capability1>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<537>, Name::Strict<105> >, Element<Text<Nodedev::Xml::EMediaAvailable >, Name::Strict<3544> >, Element<Text<Nodedev::Xml::PUnsignedLong >, Name::Strict<3545> >, Optional<Element<Text<QString >, Name::Strict<3546> > > > > marshal_type;

	static int parse(Nodedev::Xml::Capability1& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capability1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capstorage traits

template<>
struct Traits<Nodedev::Xml::Capstorage>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<938>, Name::Strict<105> >, Element<Text<Nodedev::Xml::PPath >, Name::Strict<502> >, Optional<Element<Text<QString >, Name::Strict<29> > >, Optional<Element<Text<QString >, Name::Strict<3539> > >, Optional<Element<Text<QString >, Name::Strict<231> > >, Optional<Element<Text<QString >, Name::Strict<459> > >, Optional<Element<Text<QString >, Name::Strict<453> > >, Nodedev::Xml::VCapstorageImpl, Optional<Element<Attribute<mpl::int_<345>, Name::Strict<105> >, Name::Strict<3467> > > > > marshal_type;

	static int parse(Nodedev::Xml::Capstorage& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capstorage& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capmdev traits

template<>
struct Traits<Nodedev::Xml::Capmdev>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<3550>, Name::Strict<105> >, Element<Attribute<Nodedev::Xml::PId, Name::Strict<208> >, Name::Strict<105> >, Element<Attribute<Nodedev::Xml::PUnsignedInt, Name::Strict<3510> >, Name::Strict<3509> > > > marshal_type;

	static int parse(Nodedev::Xml::Capmdev& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capmdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capccwdev traits

template<>
struct Traits<Nodedev::Xml::Capccwdev>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<1059>, Name::Strict<105> >, Element<Text<Nodedev::Xml::VCcwCssidRange >, Name::Strict<950> >, Element<Text<Nodedev::Xml::PCcwSsidRange >, Name::Strict<952> >, Element<Text<Nodedev::Xml::VCcwDevnoRange >, Name::Strict<954> > > > marshal_type;

	static int parse(Nodedev::Xml::Capccwdev& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Capccwdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Device traits

template<>
struct Traits<Nodedev::Xml::Device>
{
	typedef Ordered<mpl::vector<Element<Text<QString >, Name::Strict<107> >, Optional<Element<Text<QString >, Name::Strict<355> > >, Optional<Element<Ordered<mpl::vector<Attribute<mpl::int_<441>, Name::Strict<105> >, Text<QString > > >, Name::Strict<3466> > >, ZeroOrMore<Element<Ordered<mpl::vector<Attribute<mpl::int_<124>, Name::Strict<105> >, Text<QString > > >, Name::Strict<3466> > >, Optional<Element<Nodedev::Xml::VParentImpl, Name::Strict<116> > >, Optional<Element<Element<Text<QString >, Name::Strict<107> >, Name::Strict<546> > >, ZeroOrMore<Element<Nodedev::Xml::VCapabilityImpl, Name::Strict<3467> > > > > marshal_type;

	static int parse(Nodedev::Xml::Device& , QStack<QDomElement>& );
	static int generate(const Nodedev::Xml::Device& , QDomElement& );
};

} // namespace Libvirt

#endif // __NODEDEV_TYPE_H__
