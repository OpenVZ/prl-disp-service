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

#ifndef __CAPABILITY_TYPE_H__
#define __CAPABILITY_TYPE_H__
#include "base.h"
#include "capability_data.h"
#include "capability_enum.h"
#include "patterns.h"
#include <boost/any.hpp>

namespace Libvirt
{

///////////////////////////////////////////////////////////////////////////////
// struct Enum

namespace Capability
{
namespace Xml
{
struct Enum
{
	const QString& getName() const
	{
		return m_name;
	}
	void setName(const QString& value_)
	{
		m_name = value_;
	}
	const QList<QString >& getValueList() const
	{
		return m_valueList;
	}
	void setValueList(const QList<QString >& value_)
	{
		m_valueList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_name;
	QList<QString > m_valueList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Loader

namespace Capability
{
namespace Xml
{
struct Loader
{
	Loader();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const boost::optional<QList<QString > >& getValueList() const
	{
		return m_valueList;
	}
	void setValueList(const boost::optional<QList<QString > >& value_)
	{
		m_valueList = value_;
	}
	const QList<Enum >& getEnumList() const
	{
		return m_enumList;
	}
	void setEnumList(const QList<Enum >& value_)
	{
		m_enumList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	boost::optional<QList<QString > > m_valueList;
	QList<Enum > m_enumList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Os

namespace Capability
{
namespace Xml
{
struct Os
{
	Os();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const boost::optional<Loader >& getLoader() const
	{
		return m_loader;
	}
	void setLoader(const boost::optional<Loader >& value_)
	{
		m_loader = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	boost::optional<Loader > m_loader;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Model

namespace Capability
{
namespace Xml
{
struct Model
{
	const boost::optional<EFallback >& getFallback() const
	{
		return m_fallback;
	}
	void setFallback(const boost::optional<EFallback >& value_)
	{
		m_fallback = value_;
	}
	const boost::optional<PVendorId::value_type >& getVendorId() const
	{
		return m_vendorId;
	}
	void setVendorId(const boost::optional<PVendorId::value_type >& value_)
	{
		m_vendorId = value_;
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
	boost::optional<EFallback > m_fallback;
	boost::optional<PVendorId::value_type > m_vendorId;
	QString m_ownValue;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Feature

namespace Capability
{
namespace Xml
{
struct Feature
{
	Feature();

	EPolicy getPolicy() const
	{
		return m_policy;
	}
	void setPolicy(EPolicy value_)
	{
		m_policy = value_;
	}
	const PFeatureName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PFeatureName::value_type& value_)
	{
		m_name = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EPolicy m_policy;
	PFeatureName::value_type m_name;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1853

namespace Capability
{
namespace Xml
{
struct Anonymous1853
{
	const Model& getModel() const
	{
		return m_model;
	}
	void setModel(const Model& value_)
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
	const QList<Feature >& getFeatureList() const
	{
		return m_featureList;
	}
	void setFeatureList(const QList<Feature >& value_)
	{
		m_featureList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	Model m_model;
	boost::optional<QString > m_vendor;
	QList<Feature > m_featureList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Mode

namespace Capability
{
namespace Xml
{
struct Mode
{
	Mode();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const boost::optional<Anonymous1853 >& getAnonymous1853() const
	{
		return m_anonymous1853;
	}
	void setAnonymous1853(const boost::optional<Anonymous1853 >& value_)
	{
		m_anonymous1853 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	boost::optional<Anonymous1853 > m_anonymous1853;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Model1

namespace Capability
{
namespace Xml
{
struct Model1
{
	Model1();

	EUsable getUsable() const
	{
		return m_usable;
	}
	void setUsable(EUsable value_)
	{
		m_usable = value_;
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
	EUsable m_usable;
	QString m_ownValue;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Mode1

namespace Capability
{
namespace Xml
{
struct Mode1
{
	Mode1();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const QList<Model1 >& getModelList() const
	{
		return m_modelList;
	}
	void setModelList(const QList<Model1 >& value_)
	{
		m_modelList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	QList<Model1 > m_modelList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

namespace Capability
{
namespace Xml
{
struct Cpu
{
	Cpu();

	EVirYesNo getMode() const
	{
		return m_mode;
	}
	void setMode(EVirYesNo value_)
	{
		m_mode = value_;
	}
	const Mode& getMode2() const
	{
		return m_mode2;
	}
	void setMode2(const Mode& value_)
	{
		m_mode2 = value_;
	}
	const Mode1& getMode3() const
	{
		return m_mode3;
	}
	void setMode3(const Mode1& value_)
	{
		m_mode3 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_mode;
	Mode m_mode2;
	Mode1 m_mode3;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Capability
{
namespace Xml
{
struct Disk
{
	Disk();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const QList<Enum >& getEnumList() const
	{
		return m_enumList;
	}
	void setEnumList(const QList<Enum >& value_)
	{
		m_enumList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	QList<Enum > m_enumList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Graphics

namespace Capability
{
namespace Xml
{
struct Graphics
{
	Graphics();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const QList<Enum >& getEnumList() const
	{
		return m_enumList;
	}
	void setEnumList(const QList<Enum >& value_)
	{
		m_enumList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	QList<Enum > m_enumList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Video

namespace Capability
{
namespace Xml
{
struct Video
{
	Video();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const QList<Enum >& getEnumList() const
	{
		return m_enumList;
	}
	void setEnumList(const QList<Enum >& value_)
	{
		m_enumList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	QList<Enum > m_enumList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Hostdev

namespace Capability
{
namespace Xml
{
struct Hostdev
{
	Hostdev();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const QList<Enum >& getEnumList() const
	{
		return m_enumList;
	}
	void setEnumList(const QList<Enum >& value_)
	{
		m_enumList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	QList<Enum > m_enumList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Devices

namespace Capability
{
namespace Xml
{
struct Devices
{
	const Disk& getDisk() const
	{
		return m_disk;
	}
	void setDisk(const Disk& value_)
	{
		m_disk = value_;
	}
	const Graphics& getGraphics() const
	{
		return m_graphics;
	}
	void setGraphics(const Graphics& value_)
	{
		m_graphics = value_;
	}
	const Video& getVideo() const
	{
		return m_video;
	}
	void setVideo(const Video& value_)
	{
		m_video = value_;
	}
	const Hostdev& getHostdev() const
	{
		return m_hostdev;
	}
	void setHostdev(const Hostdev& value_)
	{
		m_hostdev = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	Disk m_disk;
	Graphics m_graphics;
	Video m_video;
	Hostdev m_hostdev;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Gic

namespace Capability
{
namespace Xml
{
struct Gic
{
	Gic();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const QList<Enum >& getEnumList() const
	{
		return m_enumList;
	}
	void setEnumList(const QList<Enum >& value_)
	{
		m_enumList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	QList<Enum > m_enumList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct DomainCapabilities

namespace Capability
{
namespace Xml
{
struct DomainCapabilities
{
	const PAbsFilePath::value_type& getPath() const
	{
		return m_path;
	}
	void setPath(const PAbsFilePath::value_type& value_)
	{
		m_path = value_;
	}
	const QString& getDomain() const
	{
		return m_domain;
	}
	void setDomain(const QString& value_)
	{
		m_domain = value_;
	}
	const boost::optional<QString >& getMachine() const
	{
		return m_machine;
	}
	void setMachine(const boost::optional<QString >& value_)
	{
		m_machine = value_;
	}
	const QString& getArch() const
	{
		return m_arch;
	}
	void setArch(const QString& value_)
	{
		m_arch = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getVcpu() const
	{
		return m_vcpu;
	}
	void setVcpu(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_vcpu = value_;
	}
	const boost::optional<Os >& getOs() const
	{
		return m_os;
	}
	void setOs(const boost::optional<Os >& value_)
	{
		m_os = value_;
	}
	const boost::optional<Cpu >& getCpu() const
	{
		return m_cpu;
	}
	void setCpu(const boost::optional<Cpu >& value_)
	{
		m_cpu = value_;
	}
	const boost::optional<Devices >& getDevices() const
	{
		return m_devices;
	}
	void setDevices(const boost::optional<Devices >& value_)
	{
		m_devices = value_;
	}
	const boost::optional<Gic >& getFeatures() const
	{
		return m_features;
	}
	void setFeatures(const boost::optional<Gic >& value_)
	{
		m_features = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PAbsFilePath::value_type m_path;
	QString m_domain;
	boost::optional<QString > m_machine;
	QString m_arch;
	boost::optional<PUnsignedInt::value_type > m_vcpu;
	boost::optional<Os > m_os;
	boost::optional<Cpu > m_cpu;
	boost::optional<Devices > m_devices;
	boost::optional<Gic > m_features;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Enum traits

template<>
struct Traits<Capability::Xml::Enum>
{
	typedef Ordered<mpl::vector<Attribute<QString, Name::Strict<107> >, ZeroOrMore<Element<Text<QString >, Name::Strict<1073> > > > > marshal_type;

	static int parse(Capability::Xml::Enum& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Enum& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Loader traits

template<>
struct Traits<Capability::Xml::Loader>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> >, Optional<ZeroOrMore<Element<Text<QString >, Name::Strict<1073> > > >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1849> > > > > marshal_type;

	static int parse(Capability::Xml::Loader& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Loader& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Os traits

template<>
struct Traits<Capability::Xml::Os>
{
	typedef Unordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> >, Optional<Element<Capability::Xml::Loader, Name::Strict<273> > > > > marshal_type;

	static int parse(Capability::Xml::Os& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Os& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Model traits

template<>
struct Traits<Capability::Xml::Model>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Capability::Xml::EFallback, Name::Strict<1005> > >, Optional<Attribute<Capability::Xml::PVendorId, Name::Strict<1007> > >, Text<QString > > > marshal_type;

	static int parse(Capability::Xml::Model& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Model& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Feature traits

template<>
struct Traits<Capability::Xml::Feature>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EPolicy, Name::Strict<1010> >, Attribute<Capability::Xml::PFeatureName, Name::Strict<107> > > > marshal_type;

	static int parse(Capability::Xml::Feature& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Feature& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1853 traits

template<>
struct Traits<Capability::Xml::Anonymous1853>
{
	typedef Ordered<mpl::vector<Element<Capability::Xml::Model, Name::Strict<231> >, Optional<Element<Text<QString >, Name::Strict<459> > >, ZeroOrMore<Element<Capability::Xml::Feature, Name::Strict<1009> > > > > marshal_type;

	static int parse(Capability::Xml::Anonymous1853& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Anonymous1853& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mode traits

template<>
struct Traits<Capability::Xml::Mode>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<1000>, Name::Strict<107> >, Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> >, Optional<Fragment<Capability::Xml::Anonymous1853 > > > > marshal_type;

	static int parse(Capability::Xml::Mode& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Mode& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Model1 traits

template<>
struct Traits<Capability::Xml::Model1>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EUsable, Name::Strict<1853> >, Text<QString > > > marshal_type;

	static int parse(Capability::Xml::Model1& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Model1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mode1 traits

template<>
struct Traits<Capability::Xml::Mode1>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<999>, Name::Strict<107> >, Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> >, ZeroOrMore<Element<Capability::Xml::Model1, Name::Strict<231> > > > > marshal_type;

	static int parse(Capability::Xml::Mode1& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Mode1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cpu traits

template<>
struct Traits<Capability::Xml::Cpu>
{
	typedef Ordered<mpl::vector<Element<Ordered<mpl::vector<Attribute<mpl::int_<1001>, Name::Strict<107> >, Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> > > >, Name::Strict<379> >, Element<Capability::Xml::Mode, Name::Strict<379> >, Element<Capability::Xml::Mode1, Name::Strict<379> > > > marshal_type;

	static int parse(Capability::Xml::Cpu& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Cpu& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk traits

template<>
struct Traits<Capability::Xml::Disk>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1849> > > > > marshal_type;

	static int parse(Capability::Xml::Disk& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Disk& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Graphics traits

template<>
struct Traits<Capability::Xml::Graphics>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1849> > > > > marshal_type;

	static int parse(Capability::Xml::Graphics& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Graphics& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Video traits

template<>
struct Traits<Capability::Xml::Video>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1849> > > > > marshal_type;

	static int parse(Capability::Xml::Video& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Video& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hostdev traits

template<>
struct Traits<Capability::Xml::Hostdev>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1849> > > > > marshal_type;

	static int parse(Capability::Xml::Hostdev& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Hostdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Devices traits

template<>
struct Traits<Capability::Xml::Devices>
{
	typedef Unordered<mpl::vector<Element<Capability::Xml::Disk, Name::Strict<471> >, Element<Capability::Xml::Graphics, Name::Strict<711> >, Element<Capability::Xml::Video, Name::Strict<778> >, Element<Capability::Xml::Hostdev, Name::Strict<675> > > > marshal_type;

	static int parse(Capability::Xml::Devices& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Devices& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Gic traits

template<>
struct Traits<Capability::Xml::Gic>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1848> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1849> > > > > marshal_type;

	static int parse(Capability::Xml::Gic& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Gic& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct DomainCapabilities traits

template<>
struct Traits<Capability::Xml::DomainCapabilities>
{
	typedef Unordered<mpl::vector<Element<Text<Capability::Xml::PAbsFilePath >, Name::Strict<355> >, Element<Text<QString >, Name::Strict<1> >, Optional<Element<Text<QString >, Name::Strict<286> > >, Element<Text<QString >, Name::Strict<285> >, Optional<Element<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1847> >, Name::Strict<338> > >, Optional<Element<Capability::Xml::Os, Name::Strict<222> > >, Optional<Element<Capability::Xml::Cpu, Name::Strict<220> > >, Optional<Element<Capability::Xml::Devices, Name::Strict<228> > >, Optional<Element<Element<Capability::Xml::Gic, Name::Strict<991> >, Name::Strict<155> > > > > marshal_type;

	static int parse(Capability::Xml::DomainCapabilities& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::DomainCapabilities& , QDomElement& );
};

} // namespace Libvirt

#endif // __CAPABILITY_TYPE_H__
