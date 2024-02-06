/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2021 Virtuozzo International GmbH. All rights reserved.
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
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
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
	const QList<Enum >& getEnumList() const
	{
		return m_enumList;
	}
	void setEnumList(const QList<Enum >& value_)
	{
		m_enumList = value_;
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
	QList<Enum > m_enumList;
	boost::optional<Loader > m_loader;
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
	const boost::optional<QList<Enum > >& getEnumList() const
	{
		return m_enumList;
	}
	void setEnumList(const boost::optional<QList<Enum > >& value_)
	{
		m_enumList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	boost::optional<QList<Enum > > m_enumList;
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
	const boost::optional<QList<Enum > >& getEnumList() const
	{
		return m_enumList;
	}
	void setEnumList(const boost::optional<QList<Enum > >& value_)
	{
		m_enumList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	boost::optional<QList<Enum > > m_enumList;
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
// struct Maxphysaddr

namespace Capability
{
namespace Xml
{
struct Maxphysaddr
{
	Maxphysaddr();

	EMode getMode() const
	{
		return m_mode;
	}
	void setMode(EMode value_)
	{
		m_mode = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getBits() const
	{
		return m_bits;
	}
	void setBits(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_bits = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getLimit() const
	{
		return m_limit;
	}
	void setLimit(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_limit = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EMode m_mode;
	boost::optional<PUnsignedInt::value_type > m_bits;
	boost::optional<PUnsignedInt::value_type > m_limit;
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
// struct Anonymous4936

namespace Capability
{
namespace Xml
{
struct Anonymous4936
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
	const boost::optional<Maxphysaddr >& getMaxphysaddr() const
	{
		return m_maxphysaddr;
	}
	void setMaxphysaddr(const boost::optional<Maxphysaddr >& value_)
	{
		m_maxphysaddr = value_;
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
	boost::optional<Maxphysaddr > m_maxphysaddr;
	QList<Feature > m_featureList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Mode2

namespace Capability
{
namespace Xml
{
struct Mode2
{
	Mode2();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const boost::optional<Anonymous4936 >& getAnonymous4936() const
	{
		return m_anonymous4936;
	}
	void setAnonymous4936(const boost::optional<Anonymous4936 >& value_)
	{
		m_anonymous4936 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	boost::optional<Anonymous4936 > m_anonymous4936;
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
	const boost::optional<EVirYesNo >& getDeprecated() const
	{
		return m_deprecated;
	}
	void setDeprecated(const boost::optional<EVirYesNo >& value_)
	{
		m_deprecated = value_;
	}
	const QString& getVendor() const
	{
		return m_vendor;
	}
	void setVendor(const QString& value_)
	{
		m_vendor = value_;
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
	boost::optional<EVirYesNo > m_deprecated;
	QString m_vendor;
	QString m_ownValue;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Mode3

namespace Capability
{
namespace Xml
{
struct Mode3
{
	Mode3();

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
	const Mode& getMode() const
	{
		return m_mode;
	}
	void setMode(const Mode& value_)
	{
		m_mode = value_;
	}
	const Mode1& getMode2() const
	{
		return m_mode2;
	}
	void setMode2(const Mode1& value_)
	{
		m_mode2 = value_;
	}
	const Mode2& getMode3() const
	{
		return m_mode3;
	}
	void setMode3(const Mode2& value_)
	{
		m_mode3 = value_;
	}
	const Mode3& getMode4() const
	{
		return m_mode4;
	}
	void setMode4(const Mode3& value_)
	{
		m_mode4 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	Mode m_mode;
	Mode1 m_mode2;
	Mode2 m_mode3;
	Mode3 m_mode4;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct MemoryBacking

namespace Capability
{
namespace Xml
{
struct MemoryBacking
{
	MemoryBacking();

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
// struct Rng

namespace Capability
{
namespace Xml
{
struct Rng
{
	Rng();

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
// struct Filesystem

namespace Capability
{
namespace Xml
{
struct Filesystem
{
	Filesystem();

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
// struct Tpm

namespace Capability
{
namespace Xml
{
struct Tpm
{
	Tpm();

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
// struct Redirdev

namespace Capability
{
namespace Xml
{
struct Redirdev
{
	Redirdev();

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
// struct Channel

namespace Capability
{
namespace Xml
{
struct Channel
{
	Channel();

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
// struct Crypto

namespace Capability
{
namespace Xml
{
struct Crypto
{
	Crypto();

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
	const boost::optional<Disk >& getDisk() const
	{
		return m_disk;
	}
	void setDisk(const boost::optional<Disk >& value_)
	{
		m_disk = value_;
	}
	const boost::optional<Graphics >& getGraphics() const
	{
		return m_graphics;
	}
	void setGraphics(const boost::optional<Graphics >& value_)
	{
		m_graphics = value_;
	}
	const boost::optional<Video >& getVideo() const
	{
		return m_video;
	}
	void setVideo(const boost::optional<Video >& value_)
	{
		m_video = value_;
	}
	const boost::optional<Hostdev >& getHostdev() const
	{
		return m_hostdev;
	}
	void setHostdev(const boost::optional<Hostdev >& value_)
	{
		m_hostdev = value_;
	}
	const boost::optional<Rng >& getRng() const
	{
		return m_rng;
	}
	void setRng(const boost::optional<Rng >& value_)
	{
		m_rng = value_;
	}
	const boost::optional<Filesystem >& getFilesystem() const
	{
		return m_filesystem;
	}
	void setFilesystem(const boost::optional<Filesystem >& value_)
	{
		m_filesystem = value_;
	}
	const boost::optional<Tpm >& getTpm() const
	{
		return m_tpm;
	}
	void setTpm(const boost::optional<Tpm >& value_)
	{
		m_tpm = value_;
	}
	const boost::optional<Redirdev >& getRedirdev() const
	{
		return m_redirdev;
	}
	void setRedirdev(const boost::optional<Redirdev >& value_)
	{
		m_redirdev = value_;
	}
	const boost::optional<Channel >& getChannel() const
	{
		return m_channel;
	}
	void setChannel(const boost::optional<Channel >& value_)
	{
		m_channel = value_;
	}
	const boost::optional<Crypto >& getCrypto() const
	{
		return m_crypto;
	}
	void setCrypto(const boost::optional<Crypto >& value_)
	{
		m_crypto = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<Disk > m_disk;
	boost::optional<Graphics > m_graphics;
	boost::optional<Video > m_video;
	boost::optional<Hostdev > m_hostdev;
	boost::optional<Rng > m_rng;
	boost::optional<Filesystem > m_filesystem;
	boost::optional<Tpm > m_tpm;
	boost::optional<Redirdev > m_redirdev;
	boost::optional<Channel > m_channel;
	boost::optional<Crypto > m_crypto;
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
// struct Anonymous5138

namespace Capability
{
namespace Xml
{
struct Anonymous5138
{
	Anonymous5138();

	PCbitpos::value_type getCbitpos() const
	{
		return m_cbitpos;
	}
	void setCbitpos(PCbitpos::value_type value_)
	{
		m_cbitpos = value_;
	}
	PReducedPhysBits::value_type getReducedPhysBits() const
	{
		return m_reducedPhysBits;
	}
	void setReducedPhysBits(PReducedPhysBits::value_type value_)
	{
		m_reducedPhysBits = value_;
	}
	PMaxGuests::value_type getMaxGuests() const
	{
		return m_maxGuests;
	}
	void setMaxGuests(PMaxGuests::value_type value_)
	{
		m_maxGuests = value_;
	}
	PMaxESGuests::value_type getMaxESGuests() const
	{
		return m_maxESGuests;
	}
	void setMaxESGuests(PMaxESGuests::value_type value_)
	{
		m_maxESGuests = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PCbitpos::value_type m_cbitpos;
	PReducedPhysBits::value_type m_reducedPhysBits;
	PMaxGuests::value_type m_maxGuests;
	PMaxESGuests::value_type m_maxESGuests;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Sev

namespace Capability
{
namespace Xml
{
struct Sev
{
	Sev();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const boost::optional<Anonymous5138 >& getAnonymous5138() const
	{
		return m_anonymous5138;
	}
	void setAnonymous5138(const boost::optional<Anonymous5138 >& value_)
	{
		m_anonymous5138 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	boost::optional<Anonymous5138 > m_anonymous5138;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Section

namespace Capability
{
namespace Xml
{
struct Section
{
	Section();

	PNode::value_type getNode() const
	{
		return m_node;
	}
	void setNode(PNode::value_type value_)
	{
		m_node = value_;
	}
	PSize::value_type getSize() const
	{
		return m_size;
	}
	void setSize(PSize::value_type value_)
	{
		m_size = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PNode::value_type m_node;
	PSize::value_type m_size;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous5139

namespace Capability
{
namespace Xml
{
struct Anonymous5139
{
	Anonymous5139();

	EVirYesNo getFlc() const
	{
		return m_flc;
	}
	void setFlc(EVirYesNo value_)
	{
		m_flc = value_;
	}
	EVirYesNo getSgx1() const
	{
		return m_sgx1;
	}
	void setSgx1(EVirYesNo value_)
	{
		m_sgx1 = value_;
	}
	EVirYesNo getSgx2() const
	{
		return m_sgx2;
	}
	void setSgx2(EVirYesNo value_)
	{
		m_sgx2 = value_;
	}
	PSectionSize::value_type getSectionSize() const
	{
		return m_sectionSize;
	}
	void setSectionSize(PSectionSize::value_type value_)
	{
		m_sectionSize = value_;
	}
	const boost::optional<QList<Section > >& getSections() const
	{
		return m_sections;
	}
	void setSections(const boost::optional<QList<Section > >& value_)
	{
		m_sections = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	EVirYesNo m_flc;
	EVirYesNo m_sgx1;
	EVirYesNo m_sgx2;
	PSectionSize::value_type m_sectionSize;
	boost::optional<QList<Section > > m_sections;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Sgx

namespace Capability
{
namespace Xml
{
struct Sgx
{
	Sgx();

	EVirYesNo getSupported() const
	{
		return m_supported;
	}
	void setSupported(EVirYesNo value_)
	{
		m_supported = value_;
	}
	const boost::optional<Anonymous5139 >& getAnonymous5139() const
	{
		return m_anonymous5139;
	}
	void setAnonymous5139(const boost::optional<Anonymous5139 >& value_)
	{
		m_anonymous5139 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_supported;
	boost::optional<Anonymous5139 > m_anonymous5139;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Hyperv

namespace Capability
{
namespace Xml
{
struct Hyperv
{
	Hyperv();

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
// struct Features

namespace Capability
{
namespace Xml
{
struct Features
{
	const boost::optional<Gic >& getGic() const
	{
		return m_gic;
	}
	void setGic(const boost::optional<Gic >& value_)
	{
		m_gic = value_;
	}
	const boost::optional<EVirYesNo >& getVmcoreinfo() const
	{
		return m_vmcoreinfo;
	}
	void setVmcoreinfo(const boost::optional<EVirYesNo >& value_)
	{
		m_vmcoreinfo = value_;
	}
	const boost::optional<EVirYesNo >& getGenid() const
	{
		return m_genid;
	}
	void setGenid(const boost::optional<EVirYesNo >& value_)
	{
		m_genid = value_;
	}
	const boost::optional<EVirYesNo >& getBackingStoreInput() const
	{
		return m_backingStoreInput;
	}
	void setBackingStoreInput(const boost::optional<EVirYesNo >& value_)
	{
		m_backingStoreInput = value_;
	}
	const boost::optional<EVirYesNo >& getBackup() const
	{
		return m_backup;
	}
	void setBackup(const boost::optional<EVirYesNo >& value_)
	{
		m_backup = value_;
	}
	const boost::optional<EVirYesNo >& getAsyncTeardown() const
	{
		return m_asyncTeardown;
	}
	void setAsyncTeardown(const boost::optional<EVirYesNo >& value_)
	{
		m_asyncTeardown = value_;
	}
	const boost::optional<EVirYesNo >& getS390Pv() const
	{
		return m_s390Pv;
	}
	void setS390Pv(const boost::optional<EVirYesNo >& value_)
	{
		m_s390Pv = value_;
	}
	const boost::optional<Sev >& getSev() const
	{
		return m_sev;
	}
	void setSev(const boost::optional<Sev >& value_)
	{
		m_sev = value_;
	}
	const boost::optional<Sgx >& getSgx() const
	{
		return m_sgx;
	}
	void setSgx(const boost::optional<Sgx >& value_)
	{
		m_sgx = value_;
	}
	const boost::optional<Hyperv >& getHyperv() const
	{
		return m_hyperv;
	}
	void setHyperv(const boost::optional<Hyperv >& value_)
	{
		m_hyperv = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<Gic > m_gic;
	boost::optional<EVirYesNo > m_vmcoreinfo;
	boost::optional<EVirYesNo > m_genid;
	boost::optional<EVirYesNo > m_backingStoreInput;
	boost::optional<EVirYesNo > m_backup;
	boost::optional<EVirYesNo > m_asyncTeardown;
	boost::optional<EVirYesNo > m_s390Pv;
	boost::optional<Sev > m_sev;
	boost::optional<Sgx > m_sgx;
	boost::optional<Hyperv > m_hyperv;
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
	const boost::optional<EVirYesNo >& getIothreads() const
	{
		return m_iothreads;
	}
	void setIothreads(const boost::optional<EVirYesNo >& value_)
	{
		m_iothreads = value_;
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
	const boost::optional<MemoryBacking >& getMemoryBacking() const
	{
		return m_memoryBacking;
	}
	void setMemoryBacking(const boost::optional<MemoryBacking >& value_)
	{
		m_memoryBacking = value_;
	}
	const boost::optional<Devices >& getDevices() const
	{
		return m_devices;
	}
	void setDevices(const boost::optional<Devices >& value_)
	{
		m_devices = value_;
	}
	const boost::optional<Features >& getFeatures() const
	{
		return m_features;
	}
	void setFeatures(const boost::optional<Features >& value_)
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
	boost::optional<EVirYesNo > m_iothreads;
	boost::optional<Os > m_os;
	boost::optional<Cpu > m_cpu;
	boost::optional<MemoryBacking > m_memoryBacking;
	boost::optional<Devices > m_devices;
	boost::optional<Features > m_features;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Enum traits

template<>
struct Traits<Capability::Xml::Enum>
{
	typedef Ordered<mpl::vector<Attribute<QString, Name::Strict<107> >, ZeroOrMore<Element<Text<QString >, Name::Strict<1086> > > > > marshal_type;

	static int parse(Capability::Xml::Enum& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Enum& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Loader traits

template<>
struct Traits<Capability::Xml::Loader>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Optional<ZeroOrMore<Element<Text<QString >, Name::Strict<1086> > > >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Loader& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Loader& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Os traits

template<>
struct Traits<Capability::Xml::Os>
{
	typedef Unordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > >, Optional<Element<Capability::Xml::Loader, Name::Strict<273> > > > > marshal_type;

	static int parse(Capability::Xml::Os& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Os& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mode traits

template<>
struct Traits<Capability::Xml::Mode>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<1014>, Name::Strict<107> >, Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Optional<ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > > marshal_type;

	static int parse(Capability::Xml::Mode& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Mode& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mode1 traits

template<>
struct Traits<Capability::Xml::Mode1>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<5778>, Name::Strict<107> >, Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Optional<ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > > marshal_type;

	static int parse(Capability::Xml::Mode1& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Mode1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Model traits

template<>
struct Traits<Capability::Xml::Model>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Capability::Xml::EFallback, Name::Strict<1018> > >, Optional<Attribute<Capability::Xml::PVendorId, Name::Strict<1020> > >, Text<QString > > > marshal_type;

	static int parse(Capability::Xml::Model& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Model& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Maxphysaddr traits

template<>
struct Traits<Capability::Xml::Maxphysaddr>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EMode, Name::Strict<379> >, Optional<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<9861> > >, Optional<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<433> > > > > marshal_type;

	static int parse(Capability::Xml::Maxphysaddr& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Maxphysaddr& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Feature traits

template<>
struct Traits<Capability::Xml::Feature>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EPolicy, Name::Strict<1023> >, Attribute<Capability::Xml::PFeatureName, Name::Strict<107> > > > marshal_type;

	static int parse(Capability::Xml::Feature& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Feature& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous4936 traits

template<>
struct Traits<Capability::Xml::Anonymous4936>
{
	typedef Ordered<mpl::vector<Element<Capability::Xml::Model, Name::Strict<231> >, Optional<Element<Text<QString >, Name::Strict<459> > >, Optional<Element<Capability::Xml::Maxphysaddr, Name::Strict<9860> > >, ZeroOrMore<Element<Capability::Xml::Feature, Name::Strict<1022> > > > > marshal_type;

	static int parse(Capability::Xml::Anonymous4936& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Anonymous4936& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mode2 traits

template<>
struct Traits<Capability::Xml::Mode2>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<1013>, Name::Strict<107> >, Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Optional<Fragment<Capability::Xml::Anonymous4936 > > > > marshal_type;

	static int parse(Capability::Xml::Mode2& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Mode2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Model1 traits

template<>
struct Traits<Capability::Xml::Model1>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EUsable, Name::Strict<1886> >, Optional<Attribute<Capability::Xml::EVirYesNo, Name::Strict<6576> > >, Attribute<QString, Name::Strict<459> >, Text<QString > > > marshal_type;

	static int parse(Capability::Xml::Model1& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Model1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mode3 traits

template<>
struct Traits<Capability::Xml::Mode3>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<1012>, Name::Strict<107> >, Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Model1, Name::Strict<231> > > > > marshal_type;

	static int parse(Capability::Xml::Mode3& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Mode3& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cpu traits

template<>
struct Traits<Capability::Xml::Cpu>
{
	typedef Ordered<mpl::vector<Element<Capability::Xml::Mode, Name::Strict<379> >, Element<Capability::Xml::Mode1, Name::Strict<379> >, Element<Capability::Xml::Mode2, Name::Strict<379> >, Element<Capability::Xml::Mode3, Name::Strict<379> > > > marshal_type;

	static int parse(Capability::Xml::Cpu& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Cpu& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct MemoryBacking traits

template<>
struct Traits<Capability::Xml::MemoryBacking>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::MemoryBacking& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::MemoryBacking& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk traits

template<>
struct Traits<Capability::Xml::Disk>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Disk& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Disk& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Graphics traits

template<>
struct Traits<Capability::Xml::Graphics>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Graphics& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Graphics& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Video traits

template<>
struct Traits<Capability::Xml::Video>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Video& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Video& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hostdev traits

template<>
struct Traits<Capability::Xml::Hostdev>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Hostdev& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Hostdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Rng traits

template<>
struct Traits<Capability::Xml::Rng>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Rng& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Rng& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem traits

template<>
struct Traits<Capability::Xml::Filesystem>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Filesystem& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Filesystem& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Tpm traits

template<>
struct Traits<Capability::Xml::Tpm>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Tpm& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Tpm& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Redirdev traits

template<>
struct Traits<Capability::Xml::Redirdev>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Redirdev& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Redirdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Channel traits

template<>
struct Traits<Capability::Xml::Channel>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Channel& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Channel& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Crypto traits

template<>
struct Traits<Capability::Xml::Crypto>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Crypto& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Crypto& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Devices traits

template<>
struct Traits<Capability::Xml::Devices>
{
	typedef Ordered<mpl::vector<Optional<Element<Capability::Xml::Disk, Name::Strict<472> > >, Optional<Element<Capability::Xml::Graphics, Name::Strict<712> > >, Optional<Element<Capability::Xml::Video, Name::Strict<779> > >, Optional<Element<Capability::Xml::Hostdev, Name::Strict<676> > >, Optional<Element<Capability::Xml::Rng, Name::Strict<981> > >, Optional<Element<Capability::Xml::Filesystem, Name::Strict<630> > >, Optional<Element<Capability::Xml::Tpm, Name::Strict<902> > >, Optional<Element<Capability::Xml::Redirdev, Name::Strict<912> > >, Optional<Element<Capability::Xml::Channel, Name::Strict<744> > >, Optional<Element<Capability::Xml::Crypto, Name::Strict<9864> > > > > marshal_type;

	static int parse(Capability::Xml::Devices& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Devices& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Gic traits

template<>
struct Traits<Capability::Xml::Gic>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Gic& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Gic& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous5138 traits

template<>
struct Traits<Capability::Xml::Anonymous5138>
{
	typedef Ordered<mpl::vector<Element<Text<Capability::Xml::PCbitpos >, Name::Strict<3564> >, Element<Text<Capability::Xml::PReducedPhysBits >, Name::Strict<3565> >, Element<Text<Capability::Xml::PMaxGuests >, Name::Strict<9502> >, Element<Text<Capability::Xml::PMaxESGuests >, Name::Strict<9503> > > > marshal_type;

	static int parse(Capability::Xml::Anonymous5138& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Anonymous5138& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Sev traits

template<>
struct Traits<Capability::Xml::Sev>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Optional<Fragment<Capability::Xml::Anonymous5138 > > > > marshal_type;

	static int parse(Capability::Xml::Sev& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Sev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Section traits

template<>
struct Traits<Capability::Xml::Section>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PNode, Name::Strict<609> >, Attribute<Capability::Xml::PSize, Name::Strict<334> >, Attribute<mpl::int_<9871>, Name::Strict<66> > > > marshal_type;

	static int parse(Capability::Xml::Section& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Section& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous5139 traits

template<>
struct Traits<Capability::Xml::Anonymous5139>
{
	typedef Ordered<mpl::vector<Element<Text<Capability::Xml::EVirYesNo >, Name::Strict<9867> >, Element<Text<Capability::Xml::EVirYesNo >, Name::Strict<9868> >, Element<Text<Capability::Xml::EVirYesNo >, Name::Strict<9869> >, Element<Ordered<mpl::vector<Attribute<mpl::int_<9871>, Name::Strict<66> >, Text<Capability::Xml::PSectionSize > > >, Name::Strict<9870> >, Optional<Element<ZeroOrMore<Element<Capability::Xml::Section, Name::Strict<9873> > >, Name::Strict<9872> > > > > marshal_type;

	static int parse(Capability::Xml::Anonymous5139& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Anonymous5139& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Sgx traits

template<>
struct Traits<Capability::Xml::Sgx>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Optional<Fragment<Capability::Xml::Anonymous5139 > > > > marshal_type;

	static int parse(Capability::Xml::Sgx& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Sgx& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hyperv traits

template<>
struct Traits<Capability::Xml::Hyperv>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, ZeroOrMore<Element<Capability::Xml::Enum, Name::Strict<1882> > > > > marshal_type;

	static int parse(Capability::Xml::Hyperv& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Hyperv& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Features traits

template<>
struct Traits<Capability::Xml::Features>
{
	typedef Ordered<mpl::vector<Optional<Element<Capability::Xml::Gic, Name::Strict<1001> > >, Optional<Element<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Name::Strict<1004> > >, Optional<Element<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Name::Strict<3563> > >, Optional<Element<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Name::Strict<5683> > >, Optional<Element<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Name::Strict<1343> > >, Optional<Element<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Name::Strict<9865> > >, Optional<Element<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Name::Strict<6577> > >, Optional<Element<Capability::Xml::Sev, Name::Strict<3562> > >, Optional<Element<Capability::Xml::Sgx, Name::Strict<9866> > >, Optional<Element<Capability::Xml::Hyperv, Name::Strict<256> > > > > marshal_type;

	static int parse(Capability::Xml::Features& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Features& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct DomainCapabilities traits

template<>
struct Traits<Capability::Xml::DomainCapabilities>
{
	typedef Unordered<mpl::vector<Element<Text<Capability::Xml::PAbsFilePath >, Name::Strict<355> >, Element<Text<QString >, Name::Strict<1> >, Optional<Element<Text<QString >, Name::Strict<286> > >, Element<Text<QString >, Name::Strict<285> >, Optional<Element<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1880> >, Name::Strict<338> > >, Optional<Element<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1881> >, Name::Strict<347> > >, Optional<Element<Capability::Xml::Os, Name::Strict<222> > >, Optional<Element<Capability::Xml::Cpu, Name::Strict<220> > >, Optional<Element<Capability::Xml::MemoryBacking, Name::Strict<331> > >, Optional<Element<Capability::Xml::Devices, Name::Strict<228> > >, Optional<Element<Capability::Xml::Features, Name::Strict<155> > > > > marshal_type;

	static int parse(Capability::Xml::DomainCapabilities& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::DomainCapabilities& , QDomElement& );
};

} // namespace Libvirt

#endif // __CAPABILITY_TYPE_H__
