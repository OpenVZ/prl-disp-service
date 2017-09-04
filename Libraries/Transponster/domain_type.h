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

#ifndef __DOMAIN_TYPE_H__
#define __DOMAIN_TYPE_H__
#include "base.h"
#include "domain_data.h"
#include "domain_enum.h"
#include "patterns.h"
#include <boost/any.hpp>

namespace Libvirt
{
namespace Domain
{
namespace Xml
{
struct VDiskBackingChainBin;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Ids

namespace Domain
{
namespace Xml
{
struct Ids
{
	const boost::optional<PUnsignedInt::value_type >& getId() const
	{
		return m_id;
	}
	void setId(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_id = value_;
	}
	const PDomainName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PDomainName::value_type& value_)
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
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<PUnsignedInt::value_type > m_id;
	PDomainName::value_type m_name;
	boost::optional<VUUID > m_uuid;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Model

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Topology

namespace Domain
{
namespace Xml
{
struct Topology
{
	Topology();

	PPositiveInteger::value_type getSockets() const
	{
		return m_sockets;
	}
	void setSockets(PPositiveInteger::value_type value_)
	{
		m_sockets = value_;
	}
	PPositiveInteger::value_type getCores() const
	{
		return m_cores;
	}
	void setCores(PPositiveInteger::value_type value_)
	{
		m_cores = value_;
	}
	PPositiveInteger::value_type getThreads() const
	{
		return m_threads;
	}
	void setThreads(PPositiveInteger::value_type value_)
	{
		m_threads = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PPositiveInteger::value_type m_sockets;
	PPositiveInteger::value_type m_cores;
	PPositiveInteger::value_type m_threads;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Feature

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Cell

namespace Domain
{
namespace Xml
{
struct Cell
{
	Cell();

	const boost::optional<PUnsignedInt::value_type >& getId() const
	{
		return m_id;
	}
	void setId(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_id = value_;
	}
	const PCpuset::value_type& getCpus() const
	{
		return m_cpus;
	}
	void setCpus(const PCpuset::value_type& value_)
	{
		m_cpus = value_;
	}
	PMemoryKB::value_type getMemory() const
	{
		return m_memory;
	}
	void setMemory(PMemoryKB::value_type value_)
	{
		m_memory = value_;
	}
	const boost::optional<EMemAccess >& getMemAccess() const
	{
		return m_memAccess;
	}
	void setMemAccess(const boost::optional<EMemAccess >& value_)
	{
		m_memAccess = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PUnsignedInt::value_type > m_id;
	PCpuset::value_type m_cpus;
	PMemoryKB::value_type m_memory;
	boost::optional<EMemAccess > m_memAccess;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

namespace Domain
{
namespace Xml
{
struct Cpu
{
	const boost::optional<EMode >& getMode() const
	{
		return m_mode;
	}
	void setMode(const boost::optional<EMode >& value_)
	{
		m_mode = value_;
	}
	const boost::optional<EMatch >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EMatch >& value_)
	{
		m_match = value_;
	}
	const boost::optional<Model >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<Model >& value_)
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
	const boost::optional<Topology >& getTopology() const
	{
		return m_topology;
	}
	void setTopology(const boost::optional<Topology >& value_)
	{
		m_topology = value_;
	}
	const QList<Feature >& getFeatureList() const
	{
		return m_featureList;
	}
	void setFeatureList(const QList<Feature >& value_)
	{
		m_featureList = value_;
	}
	const boost::optional<QList<Cell > >& getNuma() const
	{
		return m_numa;
	}
	void setNuma(const boost::optional<QList<Cell > >& value_)
	{
		m_numa = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EMode > m_mode;
	boost::optional<EMatch > m_match;
	boost::optional<Model > m_model;
	boost::optional<QString > m_vendor;
	boost::optional<Topology > m_topology;
	QList<Feature > m_featureList;
	boost::optional<QList<Cell > > m_numa;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Entry

namespace Domain
{
namespace Xml
{
struct Entry
{
	Entry();

	ESysinfoBiosName getName() const
	{
		return m_name;
	}
	void setName(ESysinfoBiosName value_)
	{
		m_name = value_;
	}
	const PSysinfoValue::value_type& getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(const PSysinfoValue::value_type& value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	ESysinfoBiosName m_name;
	PSysinfoValue::value_type m_ownValue;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Entry1

namespace Domain
{
namespace Xml
{
struct Entry1
{
	Entry1();

	ESysinfoSystemName getName() const
	{
		return m_name;
	}
	void setName(ESysinfoSystemName value_)
	{
		m_name = value_;
	}
	const PSysinfoValue::value_type& getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(const PSysinfoValue::value_type& value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	ESysinfoSystemName m_name;
	PSysinfoValue::value_type m_ownValue;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Sysinfo

namespace Domain
{
namespace Xml
{
struct Sysinfo
{
	const boost::optional<QList<Entry > >& getBios() const
	{
		return m_bios;
	}
	void setBios(const boost::optional<QList<Entry > >& value_)
	{
		m_bios = value_;
	}
	const boost::optional<QList<Entry1 > >& getSystem() const
	{
		return m_system;
	}
	void setSystem(const boost::optional<QList<Entry1 > >& value_)
	{
		m_system = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QList<Entry > > m_bios;
	boost::optional<QList<Entry1 > > m_system;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Bootloader

namespace Domain
{
namespace Xml
{
struct Bootloader
{
	const PAbsFilePath::value_type& getBootloader() const
	{
		return m_bootloader;
	}
	void setBootloader(const PAbsFilePath::value_type& value_)
	{
		m_bootloader = value_;
	}
	const boost::optional<QString >& getBootloaderArgs() const
	{
		return m_bootloaderArgs;
	}
	void setBootloaderArgs(const boost::optional<QString >& value_)
	{
		m_bootloaderArgs = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PAbsFilePath::value_type m_bootloader;
	boost::optional<QString > m_bootloaderArgs;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Type

namespace Domain
{
namespace Xml
{
struct Type
{
	Type();

	const boost::optional<EArch >& getArch() const
	{
		return m_arch;
	}
	void setArch(const boost::optional<EArch >& value_)
	{
		m_arch = value_;
	}
	const boost::optional<EMachine >& getMachine() const
	{
		return m_machine;
	}
	void setMachine(const boost::optional<EMachine >& value_)
	{
		m_machine = value_;
	}
	EType1 getType() const
	{
		return m_type;
	}
	void setType(EType1 value_)
	{
		m_type = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EArch > m_arch;
	boost::optional<EMachine > m_machine;
	EType1 m_type;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Osbootkernel

namespace Domain
{
namespace Xml
{
struct Osbootkernel
{
	const boost::optional<PAbsFilePath::value_type >& getKernel() const
	{
		return m_kernel;
	}
	void setKernel(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_kernel = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getInitrd() const
	{
		return m_initrd;
	}
	void setInitrd(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_initrd = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getRoot() const
	{
		return m_root;
	}
	void setRoot(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_root = value_;
	}
	const boost::optional<QString >& getCmdline() const
	{
		return m_cmdline;
	}
	void setCmdline(const boost::optional<QString >& value_)
	{
		m_cmdline = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getDtb() const
	{
		return m_dtb;
	}
	void setDtb(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_dtb = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<PAbsFilePath::value_type > m_kernel;
	boost::optional<PAbsFilePath::value_type > m_initrd;
	boost::optional<PAbsFilePath::value_type > m_root;
	boost::optional<QString > m_cmdline;
	boost::optional<PAbsFilePath::value_type > m_dtb;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Os

namespace Domain
{
namespace Xml
{
struct Os
{
	const Type& getType() const
	{
		return m_type;
	}
	void setType(const Type& value_)
	{
		m_type = value_;
	}
	const Osbootkernel& getOsbootkernel() const
	{
		return m_osbootkernel;
	}
	void setOsbootkernel(const Osbootkernel& value_)
	{
		m_osbootkernel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	Type m_type;
	Osbootkernel m_osbootkernel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Osxen261

namespace Domain
{
namespace Xml
{
struct Osxen261
{
	const boost::optional<Bootloader >& getBootloader() const
	{
		return m_bootloader;
	}
	void setBootloader(const boost::optional<Bootloader >& value_)
	{
		m_bootloader = value_;
	}
	const Os& getOs() const
	{
		return m_os;
	}
	void setOs(const Os& value_)
	{
		m_os = value_;
	}

private:
	boost::optional<Bootloader > m_bootloader;
	Os m_os;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Os1

namespace Domain
{
namespace Xml
{
struct Os1
{
	const Type& getType() const
	{
		return m_type;
	}
	void setType(const Type& value_)
	{
		m_type = value_;
	}
	const boost::optional<Osbootkernel >& getOsbootkernel() const
	{
		return m_osbootkernel;
	}
	void setOsbootkernel(const boost::optional<Osbootkernel >& value_)
	{
		m_osbootkernel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	Type m_type;
	boost::optional<Osbootkernel > m_osbootkernel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Osxen262

namespace Domain
{
namespace Xml
{
struct Osxen262
{
	const Bootloader& getBootloader() const
	{
		return m_bootloader;
	}
	void setBootloader(const Bootloader& value_)
	{
		m_bootloader = value_;
	}
	const boost::optional<Os1 >& getOs() const
	{
		return m_os;
	}
	void setOs(const boost::optional<Os1 >& value_)
	{
		m_os = value_;
	}

private:
	Bootloader m_bootloader;
	boost::optional<Os1 > m_os;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VOsxen

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Osxen261, Osxen262 > > VOsxenImpl;
typedef VOsxenImpl::value_type VOsxen;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hvmx86

namespace Domain
{
namespace Xml
{
struct Hvmx86
{
	const boost::optional<EArch1 >& getArch() const
	{
		return m_arch;
	}
	void setArch(const boost::optional<EArch1 >& value_)
	{
		m_arch = value_;
	}
	const boost::optional<PMachine::value_type >& getMachine() const
	{
		return m_machine;
	}
	void setMachine(const boost::optional<PMachine::value_type >& value_)
	{
		m_machine = value_;
	}

private:
	boost::optional<EArch1 > m_arch;
	boost::optional<PMachine::value_type > m_machine;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hvmmips

namespace Domain
{
namespace Xml
{
struct Hvmmips
{
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hvmsparc

namespace Domain
{
namespace Xml
{
struct Hvmsparc
{
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hvms390

namespace Domain
{
namespace Xml
{
struct Hvms390
{
	const boost::optional<EArch2 >& getArch() const
	{
		return m_arch;
	}
	void setArch(const boost::optional<EArch2 >& value_)
	{
		m_arch = value_;
	}
	const boost::optional<EMachine3 >& getMachine() const
	{
		return m_machine;
	}
	void setMachine(const boost::optional<EMachine3 >& value_)
	{
		m_machine = value_;
	}

private:
	boost::optional<EArch2 > m_arch;
	boost::optional<EMachine3 > m_machine;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hvmarm

namespace Domain
{
namespace Xml
{
struct Hvmarm
{
	const boost::optional<EArch3 >& getArch() const
	{
		return m_arch;
	}
	void setArch(const boost::optional<EArch3 >& value_)
	{
		m_arch = value_;
	}
	const boost::optional<PMachine::value_type >& getMachine() const
	{
		return m_machine;
	}
	void setMachine(const boost::optional<PMachine::value_type >& value_)
	{
		m_machine = value_;
	}

private:
	boost::optional<EArch3 > m_arch;
	boost::optional<PMachine::value_type > m_machine;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hvmaarch64

namespace Domain
{
namespace Xml
{
struct Hvmaarch64
{
	const boost::optional<EArch4 >& getArch() const
	{
		return m_arch;
	}
	void setArch(const boost::optional<EArch4 >& value_)
	{
		m_arch = value_;
	}
	const boost::optional<PMachine::value_type >& getMachine() const
	{
		return m_machine;
	}
	void setMachine(const boost::optional<PMachine::value_type >& value_)
	{
		m_machine = value_;
	}

private:
	boost::optional<EArch4 > m_arch;
	boost::optional<PMachine::value_type > m_machine;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice297

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Hvmx86, Hvmmips, Hvmsparc, Ordered<mpl::vector<Optional<Attribute<mpl::int_<84>, Name::Strict<277> > >, Optional<Attribute<Domain::Xml::EMachine1, Name::Strict<278> > > > >, Ordered<mpl::vector<Optional<Attribute<mpl::int_<85>, Name::Strict<277> > >, Optional<Attribute<Domain::Xml::EMachine2, Name::Strict<278> > > > >, Hvms390, Hvmarm, Hvmaarch64 > > VChoice297Impl;
typedef VChoice297Impl::value_type VChoice297;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Loader

namespace Domain
{
namespace Xml
{
struct Loader
{
	const boost::optional<EReadonly >& getReadonly() const
	{
		return m_readonly;
	}
	void setReadonly(const boost::optional<EReadonly >& value_)
	{
		m_readonly = value_;
	}
	const boost::optional<EType2 >& getType() const
	{
		return m_type;
	}
	void setType(const boost::optional<EType2 >& value_)
	{
		m_type = value_;
	}
	const PAbsFilePath::value_type& getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(const PAbsFilePath::value_type& value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EReadonly > m_readonly;
	boost::optional<EType2 > m_type;
	PAbsFilePath::value_type m_ownValue;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Nvram

namespace Domain
{
namespace Xml
{
struct Nvram
{
	const boost::optional<PAbsFilePath::value_type >& getTemplate() const
	{
		return m_template;
	}
	void setTemplate(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_template = value_;
	}
	const boost::optional<EFormat >& getFormat() const
	{
		return m_format;
	}
	void setFormat(const boost::optional<EFormat >& value_)
	{
		m_format = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PAbsFilePath::value_type > m_template;
	boost::optional<EFormat > m_format;
	boost::optional<PAbsFilePath::value_type > m_ownValue;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Bootmenu

namespace Domain
{
namespace Xml
{
struct Bootmenu
{
	Bootmenu();

	EVirYesNo getEnable() const
	{
		return m_enable;
	}
	void setEnable(EVirYesNo value_)
	{
		m_enable = value_;
	}
	const boost::optional<PUnsignedShort::value_type >& getTimeout() const
	{
		return m_timeout;
	}
	void setTimeout(const boost::optional<PUnsignedShort::value_type >& value_)
	{
		m_timeout = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_enable;
	boost::optional<PUnsignedShort::value_type > m_timeout;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Bios

namespace Domain
{
namespace Xml
{
struct Bios
{
	const boost::optional<EVirYesNo >& getUseserial() const
	{
		return m_useserial;
	}
	void setUseserial(const boost::optional<EVirYesNo >& value_)
	{
		m_useserial = value_;
	}
	const boost::optional<PRebootTimeoutDelay::value_type >& getRebootTimeout() const
	{
		return m_rebootTimeout;
	}
	void setRebootTimeout(const boost::optional<PRebootTimeoutDelay::value_type >& value_)
	{
		m_rebootTimeout = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_useserial;
	boost::optional<PRebootTimeoutDelay::value_type > m_rebootTimeout;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Os2

namespace Domain
{
namespace Xml
{
struct Os2
{
	const boost::optional<VChoice297 >& getType() const
	{
		return m_type;
	}
	void setType(const boost::optional<VChoice297 >& value_)
	{
		m_type = value_;
	}
	const boost::optional<Loader >& getLoader() const
	{
		return m_loader;
	}
	void setLoader(const boost::optional<Loader >& value_)
	{
		m_loader = value_;
	}
	const boost::optional<Nvram >& getNvram() const
	{
		return m_nvram;
	}
	void setNvram(const boost::optional<Nvram >& value_)
	{
		m_nvram = value_;
	}
	const boost::optional<Osbootkernel >& getOsbootkernel() const
	{
		return m_osbootkernel;
	}
	void setOsbootkernel(const boost::optional<Osbootkernel >& value_)
	{
		m_osbootkernel = value_;
	}
	const QList<EDev >& getBootList() const
	{
		return m_bootList;
	}
	void setBootList(const QList<EDev >& value_)
	{
		m_bootList = value_;
	}
	const boost::optional<Bootmenu >& getBootmenu() const
	{
		return m_bootmenu;
	}
	void setBootmenu(const boost::optional<Bootmenu >& value_)
	{
		m_bootmenu = value_;
	}
	const boost::optional<EMode1 >& getSmbios() const
	{
		return m_smbios;
	}
	void setSmbios(const boost::optional<EMode1 >& value_)
	{
		m_smbios = value_;
	}
	const boost::optional<Bios >& getBios() const
	{
		return m_bios;
	}
	void setBios(const boost::optional<Bios >& value_)
	{
		m_bios = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<VChoice297 > m_type;
	boost::optional<Loader > m_loader;
	boost::optional<Nvram > m_nvram;
	boost::optional<Osbootkernel > m_osbootkernel;
	QList<EDev > m_bootList;
	boost::optional<Bootmenu > m_bootmenu;
	boost::optional<EMode1 > m_smbios;
	boost::optional<Bios > m_bios;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Os3

namespace Domain
{
namespace Xml
{
struct Os3
{
	const boost::optional<EArch5 >& getType() const
	{
		return m_type;
	}
	void setType(const boost::optional<EArch5 >& value_)
	{
		m_type = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getInit() const
	{
		return m_init;
	}
	void setInit(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_init = value_;
	}
	const QList<QString >& getInitargList() const
	{
		return m_initargList;
	}
	void setInitargList(const QList<QString >& value_)
	{
		m_initargList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EArch5 > m_type;
	boost::optional<PAbsFilePath::value_type > m_init;
	QList<QString > m_initargList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VOs

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Domain::Xml::VOsxenImpl, Element<Domain::Xml::Os2, Name::Strict<214> >, Element<Domain::Xml::Os3, Name::Strict<214> > > > VOsImpl;
typedef VOsImpl::value_type VOs;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Clock387

namespace Domain
{
namespace Xml
{
struct Clock387
{
	Clock387();

	EOffset getOffset() const
	{
		return m_offset;
	}
	void setOffset(EOffset value_)
	{
		m_offset = value_;
	}
	const boost::optional<VAdjustment >& getAdjustment() const
	{
		return m_adjustment;
	}
	void setAdjustment(const boost::optional<VAdjustment >& value_)
	{
		m_adjustment = value_;
	}

private:
	EOffset m_offset;
	boost::optional<VAdjustment > m_adjustment;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Clock393

namespace Domain
{
namespace Xml
{
struct Clock393
{
	const boost::optional<PTimeDelta::value_type >& getAdjustment() const
	{
		return m_adjustment;
	}
	void setAdjustment(const boost::optional<PTimeDelta::value_type >& value_)
	{
		m_adjustment = value_;
	}
	const boost::optional<EBasis >& getBasis() const
	{
		return m_basis;
	}
	void setBasis(const boost::optional<EBasis >& value_)
	{
		m_basis = value_;
	}

private:
	boost::optional<PTimeDelta::value_type > m_adjustment;
	boost::optional<EBasis > m_basis;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VClock

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Clock387, Ordered<mpl::vector<Attribute<mpl::int_<389>, Name::Strict<381> >, Optional<Attribute<Domain::Xml::PTimeZone, Name::Strict<389> > > > >, Clock393 > > VClockImpl;
typedef VClockImpl::value_type VClock;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Catchup

namespace Domain
{
namespace Xml
{
struct Catchup
{
	const boost::optional<PUnsignedInt::value_type >& getThreshold() const
	{
		return m_threshold;
	}
	void setThreshold(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_threshold = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getSlew() const
	{
		return m_slew;
	}
	void setSlew(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_slew = value_;
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
	boost::optional<PUnsignedInt::value_type > m_threshold;
	boost::optional<PUnsignedInt::value_type > m_slew;
	boost::optional<PUnsignedInt::value_type > m_limit;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VTickpolicy

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Domain::Xml::ETickpolicy, Name::Strict<402> >, Ordered<mpl::vector<Attribute<mpl::int_<422>, Name::Strict<402> >, Optional<Element<Domain::Xml::Catchup, Name::Strict<422> > > > > > > VTickpolicyImpl;
typedef VTickpolicyImpl::value_type VTickpolicy;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Timer402

namespace Domain
{
namespace Xml
{
struct Timer402
{
	Timer402();

	EName getName() const
	{
		return m_name;
	}
	void setName(EName value_)
	{
		m_name = value_;
	}
	const boost::optional<ETrack >& getTrack() const
	{
		return m_track;
	}
	void setTrack(const boost::optional<ETrack >& value_)
	{
		m_track = value_;
	}
	const boost::optional<VTickpolicy >& getTickpolicy() const
	{
		return m_tickpolicy;
	}
	void setTickpolicy(const boost::optional<VTickpolicy >& value_)
	{
		m_tickpolicy = value_;
	}

private:
	EName m_name;
	boost::optional<ETrack > m_track;
	boost::optional<VTickpolicy > m_tickpolicy;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Timer409

namespace Domain
{
namespace Xml
{
struct Timer409
{
	const boost::optional<VTickpolicy >& getTickpolicy() const
	{
		return m_tickpolicy;
	}
	void setTickpolicy(const boost::optional<VTickpolicy >& value_)
	{
		m_tickpolicy = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getFrequency() const
	{
		return m_frequency;
	}
	void setFrequency(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_frequency = value_;
	}
	const boost::optional<EMode2 >& getMode() const
	{
		return m_mode;
	}
	void setMode(const boost::optional<EMode2 >& value_)
	{
		m_mode = value_;
	}

private:
	boost::optional<VTickpolicy > m_tickpolicy;
	boost::optional<PUnsignedInt::value_type > m_frequency;
	boost::optional<EMode2 > m_mode;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Timer412

namespace Domain
{
namespace Xml
{
struct Timer412
{
	Timer412();

	EName1 getName() const
	{
		return m_name;
	}
	void setName(EName1 value_)
	{
		m_name = value_;
	}
	const boost::optional<VTickpolicy >& getTickpolicy() const
	{
		return m_tickpolicy;
	}
	void setTickpolicy(const boost::optional<VTickpolicy >& value_)
	{
		m_tickpolicy = value_;
	}

private:
	EName1 m_name;
	boost::optional<VTickpolicy > m_tickpolicy;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VTimer

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Timer402, Timer409, Timer412, Attribute<Domain::Xml::EName2, Name::Strict<102> > > > VTimerImpl;
typedef VTimerImpl::value_type VTimer;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Timer

namespace Domain
{
namespace Xml
{
struct Timer
{
	const VTimer& getTimer() const
	{
		return m_timer;
	}
	void setTimer(const VTimer& value_)
	{
		m_timer = value_;
	}
	const boost::optional<EVirYesNo >& getPresent() const
	{
		return m_present;
	}
	void setPresent(const boost::optional<EVirYesNo >& value_)
	{
		m_present = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VTimer m_timer;
	boost::optional<EVirYesNo > m_present;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Clock

namespace Domain
{
namespace Xml
{
struct Clock
{
	const VClock& getClock() const
	{
		return m_clock;
	}
	void setClock(const VClock& value_)
	{
		m_clock = value_;
	}
	const QList<Timer >& getTimerList() const
	{
		return m_timerList;
	}
	void setTimerList(const QList<Timer >& value_)
	{
		m_timerList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VClock m_clock;
	QList<Timer > m_timerList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct ScaledInteger

namespace Domain
{
namespace Xml
{
struct ScaledInteger
{
	ScaledInteger();

	const boost::optional<PUnit::value_type >& getUnit() const
	{
		return m_unit;
	}
	void setUnit(const boost::optional<PUnit::value_type >& value_)
	{
		m_unit = value_;
	}
	PUnsignedLong::value_type getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(PUnsignedLong::value_type value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PUnit::value_type > m_unit;
	PUnsignedLong::value_type m_ownValue;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Memory

namespace Domain
{
namespace Xml
{
struct Memory
{
	const ScaledInteger& getScaledInteger() const
	{
		return m_scaledInteger;
	}
	void setScaledInteger(const ScaledInteger& value_)
	{
		m_scaledInteger = value_;
	}
	const boost::optional<EVirOnOff >& getDumpCore() const
	{
		return m_dumpCore;
	}
	void setDumpCore(const boost::optional<EVirOnOff >& value_)
	{
		m_dumpCore = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	ScaledInteger m_scaledInteger;
	boost::optional<EVirOnOff > m_dumpCore;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct MaxMemory

namespace Domain
{
namespace Xml
{
struct MaxMemory
{
	MaxMemory();

	const ScaledInteger& getScaledInteger() const
	{
		return m_scaledInteger;
	}
	void setScaledInteger(const ScaledInteger& value_)
	{
		m_scaledInteger = value_;
	}
	PUnsignedInt::value_type getSlots() const
	{
		return m_slots;
	}
	void setSlots(PUnsignedInt::value_type value_)
	{
		m_slots = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	ScaledInteger m_scaledInteger;
	PUnsignedInt::value_type m_slots;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Page

namespace Domain
{
namespace Xml
{
struct Page
{
	Page();

	PUnsignedLong::value_type getSize() const
	{
		return m_size;
	}
	void setSize(PUnsignedLong::value_type value_)
	{
		m_size = value_;
	}
	const boost::optional<PUnit::value_type >& getUnit() const
	{
		return m_unit;
	}
	void setUnit(const boost::optional<PUnit::value_type >& value_)
	{
		m_unit = value_;
	}
	const boost::optional<PCpuset::value_type >& getNodeset() const
	{
		return m_nodeset;
	}
	void setNodeset(const boost::optional<PCpuset::value_type >& value_)
	{
		m_nodeset = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedLong::value_type m_size;
	boost::optional<PUnit::value_type > m_unit;
	boost::optional<PCpuset::value_type > m_nodeset;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct MemoryBacking

namespace Domain
{
namespace Xml
{
struct MemoryBacking
{
	MemoryBacking();

	const boost::optional<QList<Page > >& getHugepages() const
	{
		return m_hugepages;
	}
	void setHugepages(const boost::optional<QList<Page > >& value_)
	{
		m_hugepages = value_;
	}
	bool getNosharepages() const
	{
		return m_nosharepages;
	}
	void setNosharepages(bool value_)
	{
		m_nosharepages = value_;
	}
	bool getLocked() const
	{
		return m_locked;
	}
	void setLocked(bool value_)
	{
		m_locked = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QList<Page > > m_hugepages;
	bool m_nosharepages;
	bool m_locked;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Vcpu

namespace Domain
{
namespace Xml
{
struct Vcpu
{
	Vcpu();

	const boost::optional<EPlacement >& getPlacement() const
	{
		return m_placement;
	}
	void setPlacement(const boost::optional<EPlacement >& value_)
	{
		m_placement = value_;
	}
	const boost::optional<PCpuset::value_type >& getCpuset() const
	{
		return m_cpuset;
	}
	void setCpuset(const boost::optional<PCpuset::value_type >& value_)
	{
		m_cpuset = value_;
	}
	const boost::optional<PCountCPU::value_type >& getCurrent() const
	{
		return m_current;
	}
	void setCurrent(const boost::optional<PCountCPU::value_type >& value_)
	{
		m_current = value_;
	}
	PCountCPU::value_type getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(PCountCPU::value_type value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EPlacement > m_placement;
	boost::optional<PCpuset::value_type > m_cpuset;
	boost::optional<PCountCPU::value_type > m_current;
	PCountCPU::value_type m_ownValue;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Vcpu1

namespace Domain
{
namespace Xml
{
struct Vcpu1
{
	Vcpu1();

	PUnsignedInt::value_type getId() const
	{
		return m_id;
	}
	void setId(PUnsignedInt::value_type value_)
	{
		m_id = value_;
	}
	EVirYesNo getEnabled() const
	{
		return m_enabled;
	}
	void setEnabled(EVirYesNo value_)
	{
		m_enabled = value_;
	}
	const boost::optional<EVirYesNo >& getHotpluggable() const
	{
		return m_hotpluggable;
	}
	void setHotpluggable(const boost::optional<EVirYesNo >& value_)
	{
		m_hotpluggable = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getOrder() const
	{
		return m_order;
	}
	void setOrder(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_order = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_id;
	EVirYesNo m_enabled;
	boost::optional<EVirYesNo > m_hotpluggable;
	boost::optional<PUnsignedInt::value_type > m_order;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Device

namespace Domain
{
namespace Xml
{
struct Device
{
	const PAbsFilePath::value_type& getPath() const
	{
		return m_path;
	}
	void setPath(const PAbsFilePath::value_type& value_)
	{
		m_path = value_;
	}
	const boost::optional<PWeight::value_type >& getWeight() const
	{
		return m_weight;
	}
	void setWeight(const boost::optional<PWeight::value_type >& value_)
	{
		m_weight = value_;
	}
	const boost::optional<PReadIopsSec::value_type >& getReadIopsSec() const
	{
		return m_readIopsSec;
	}
	void setReadIopsSec(const boost::optional<PReadIopsSec::value_type >& value_)
	{
		m_readIopsSec = value_;
	}
	const boost::optional<PWriteIopsSec::value_type >& getWriteIopsSec() const
	{
		return m_writeIopsSec;
	}
	void setWriteIopsSec(const boost::optional<PWriteIopsSec::value_type >& value_)
	{
		m_writeIopsSec = value_;
	}
	const boost::optional<PReadBytesSec::value_type >& getReadBytesSec() const
	{
		return m_readBytesSec;
	}
	void setReadBytesSec(const boost::optional<PReadBytesSec::value_type >& value_)
	{
		m_readBytesSec = value_;
	}
	const boost::optional<PWriteBytesSec::value_type >& getWriteBytesSec() const
	{
		return m_writeBytesSec;
	}
	void setWriteBytesSec(const boost::optional<PWriteBytesSec::value_type >& value_)
	{
		m_writeBytesSec = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PAbsFilePath::value_type m_path;
	boost::optional<PWeight::value_type > m_weight;
	boost::optional<PReadIopsSec::value_type > m_readIopsSec;
	boost::optional<PWriteIopsSec::value_type > m_writeIopsSec;
	boost::optional<PReadBytesSec::value_type > m_readBytesSec;
	boost::optional<PWriteBytesSec::value_type > m_writeBytesSec;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Blkiotune

namespace Domain
{
namespace Xml
{
struct Blkiotune
{
	const boost::optional<PWeight::value_type >& getWeight() const
	{
		return m_weight;
	}
	void setWeight(const boost::optional<PWeight::value_type >& value_)
	{
		m_weight = value_;
	}
	const QList<Device >& getDeviceList() const
	{
		return m_deviceList;
	}
	void setDeviceList(const QList<Device >& value_)
	{
		m_deviceList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PWeight::value_type > m_weight;
	QList<Device > m_deviceList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Memtune

namespace Domain
{
namespace Xml
{
struct Memtune
{
	const boost::optional<ScaledInteger >& getHardLimit() const
	{
		return m_hardLimit;
	}
	void setHardLimit(const boost::optional<ScaledInteger >& value_)
	{
		m_hardLimit = value_;
	}
	const boost::optional<ScaledInteger >& getSoftLimit() const
	{
		return m_softLimit;
	}
	void setSoftLimit(const boost::optional<ScaledInteger >& value_)
	{
		m_softLimit = value_;
	}
	const boost::optional<ScaledInteger >& getMinGuarantee() const
	{
		return m_minGuarantee;
	}
	void setMinGuarantee(const boost::optional<ScaledInteger >& value_)
	{
		m_minGuarantee = value_;
	}
	const boost::optional<ScaledInteger >& getSwapHardLimit() const
	{
		return m_swapHardLimit;
	}
	void setSwapHardLimit(const boost::optional<ScaledInteger >& value_)
	{
		m_swapHardLimit = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<ScaledInteger > m_hardLimit;
	boost::optional<ScaledInteger > m_softLimit;
	boost::optional<ScaledInteger > m_minGuarantee;
	boost::optional<ScaledInteger > m_swapHardLimit;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Vcpupin

namespace Domain
{
namespace Xml
{
struct Vcpupin
{
	Vcpupin();

	PVcpuid::value_type getVcpu() const
	{
		return m_vcpu;
	}
	void setVcpu(PVcpuid::value_type value_)
	{
		m_vcpu = value_;
	}
	const PCpuset::value_type& getCpuset() const
	{
		return m_cpuset;
	}
	void setCpuset(const PCpuset::value_type& value_)
	{
		m_cpuset = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PVcpuid::value_type m_vcpu;
	PCpuset::value_type m_cpuset;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Iothreadpin

namespace Domain
{
namespace Xml
{
struct Iothreadpin
{
	Iothreadpin();

	PUnsignedInt::value_type getIothread() const
	{
		return m_iothread;
	}
	void setIothread(PUnsignedInt::value_type value_)
	{
		m_iothread = value_;
	}
	const PCpuset::value_type& getCpuset() const
	{
		return m_cpuset;
	}
	void setCpuset(const PCpuset::value_type& value_)
	{
		m_cpuset = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_iothread;
	PCpuset::value_type m_cpuset;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Cputune

namespace Domain
{
namespace Xml
{
struct Cputune
{
	const boost::optional<PCpushares::value_type >& getShares() const
	{
		return m_shares;
	}
	void setShares(const boost::optional<PCpushares::value_type >& value_)
	{
		m_shares = value_;
	}
	const boost::optional<PCpuperiod::value_type >& getGlobalPeriod() const
	{
		return m_globalPeriod;
	}
	void setGlobalPeriod(const boost::optional<PCpuperiod::value_type >& value_)
	{
		m_globalPeriod = value_;
	}
	const boost::optional<PCpuquota::value_type >& getGlobalQuota() const
	{
		return m_globalQuota;
	}
	void setGlobalQuota(const boost::optional<PCpuquota::value_type >& value_)
	{
		m_globalQuota = value_;
	}
	const boost::optional<PCpuperiod::value_type >& getPeriod() const
	{
		return m_period;
	}
	void setPeriod(const boost::optional<PCpuperiod::value_type >& value_)
	{
		m_period = value_;
	}
	const boost::optional<PCpuquota::value_type >& getQuota() const
	{
		return m_quota;
	}
	void setQuota(const boost::optional<PCpuquota::value_type >& value_)
	{
		m_quota = value_;
	}
	const boost::optional<PCpuperiod::value_type >& getEmulatorPeriod() const
	{
		return m_emulatorPeriod;
	}
	void setEmulatorPeriod(const boost::optional<PCpuperiod::value_type >& value_)
	{
		m_emulatorPeriod = value_;
	}
	const boost::optional<PCpuquota::value_type >& getEmulatorQuota() const
	{
		return m_emulatorQuota;
	}
	void setEmulatorQuota(const boost::optional<PCpuquota::value_type >& value_)
	{
		m_emulatorQuota = value_;
	}
	const QList<Vcpupin >& getVcpupinList() const
	{
		return m_vcpupinList;
	}
	void setVcpupinList(const QList<Vcpupin >& value_)
	{
		m_vcpupinList = value_;
	}
	const boost::optional<PCpuset::value_type >& getEmulatorpin() const
	{
		return m_emulatorpin;
	}
	void setEmulatorpin(const boost::optional<PCpuset::value_type >& value_)
	{
		m_emulatorpin = value_;
	}
	const QList<Iothreadpin >& getIothreadpinList() const
	{
		return m_iothreadpinList;
	}
	void setIothreadpinList(const QList<Iothreadpin >& value_)
	{
		m_iothreadpinList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PCpushares::value_type > m_shares;
	boost::optional<PCpuperiod::value_type > m_globalPeriod;
	boost::optional<PCpuquota::value_type > m_globalQuota;
	boost::optional<PCpuperiod::value_type > m_period;
	boost::optional<PCpuquota::value_type > m_quota;
	boost::optional<PCpuperiod::value_type > m_emulatorPeriod;
	boost::optional<PCpuquota::value_type > m_emulatorQuota;
	QList<Vcpupin > m_vcpupinList;
	boost::optional<PCpuset::value_type > m_emulatorpin;
	QList<Iothreadpin > m_iothreadpinList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Memory1137

namespace Domain
{
namespace Xml
{
struct Memory1137
{
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VMemory

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Ordered<mpl::vector<Optional<Attribute<mpl::int_<230>, Name::Strict<331> > >, Optional<Attribute<Domain::Xml::PCpuset, Name::Strict<327> > > > >, Memory1137 > > VMemoryImpl;
typedef VMemoryImpl::value_type VMemory;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Memory1

namespace Domain
{
namespace Xml
{
struct Memory1
{
	const boost::optional<EMode3 >& getMode() const
	{
		return m_mode;
	}
	void setMode(const boost::optional<EMode3 >& value_)
	{
		m_mode = value_;
	}
	const VMemory& getMemory() const
	{
		return m_memory;
	}
	void setMemory(const VMemory& value_)
	{
		m_memory = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EMode3 > m_mode;
	VMemory m_memory;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Memnode

namespace Domain
{
namespace Xml
{
struct Memnode
{
	Memnode();

	PUnsignedInt::value_type getCellid() const
	{
		return m_cellid;
	}
	void setCellid(PUnsignedInt::value_type value_)
	{
		m_cellid = value_;
	}
	EMode4 getMode() const
	{
		return m_mode;
	}
	void setMode(EMode4 value_)
	{
		m_mode = value_;
	}
	const PCpuset::value_type& getNodeset() const
	{
		return m_nodeset;
	}
	void setNodeset(const PCpuset::value_type& value_)
	{
		m_nodeset = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_cellid;
	EMode4 m_mode;
	PCpuset::value_type m_nodeset;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Numatune

namespace Domain
{
namespace Xml
{
struct Numatune
{
	const boost::optional<Memory1 >& getMemory() const
	{
		return m_memory;
	}
	void setMemory(const boost::optional<Memory1 >& value_)
	{
		m_memory = value_;
	}
	const QList<Memnode >& getMemnodeList() const
	{
		return m_memnodeList;
	}
	void setMemnodeList(const QList<Memnode >& value_)
	{
		m_memnodeList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<Memory1 > m_memory;
	QList<Memnode > m_memnodeList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Apic

namespace Domain
{
namespace Xml
{
struct Apic
{
	const boost::optional<EVirOnOff >& getEoi() const
	{
		return m_eoi;
	}
	void setEoi(const boost::optional<EVirOnOff >& value_)
	{
		m_eoi = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirOnOff > m_eoi;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Spinlocks

namespace Domain
{
namespace Xml
{
struct Spinlocks
{
	Spinlocks();

	EVirOnOff getState() const
	{
		return m_state;
	}
	void setState(EVirOnOff value_)
	{
		m_state = value_;
	}
	const boost::optional<PRetries::value_type >& getRetries() const
	{
		return m_retries;
	}
	void setRetries(const boost::optional<PRetries::value_type >& value_)
	{
		m_retries = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirOnOff m_state;
	boost::optional<PRetries::value_type > m_retries;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VendorId

namespace Domain
{
namespace Xml
{
struct VendorId
{
	VendorId();

	EVirOnOff getState() const
	{
		return m_state;
	}
	void setState(EVirOnOff value_)
	{
		m_state = value_;
	}
	const boost::optional<PValue::value_type >& getValue() const
	{
		return m_value;
	}
	void setValue(const boost::optional<PValue::value_type >& value_)
	{
		m_value = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirOnOff m_state;
	boost::optional<PValue::value_type > m_value;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hyperv

namespace Domain
{
namespace Xml
{
struct Hyperv
{
	const boost::optional<EVirOnOff >& getRelaxed() const
	{
		return m_relaxed;
	}
	void setRelaxed(const boost::optional<EVirOnOff >& value_)
	{
		m_relaxed = value_;
	}
	const boost::optional<EVirOnOff >& getVapic() const
	{
		return m_vapic;
	}
	void setVapic(const boost::optional<EVirOnOff >& value_)
	{
		m_vapic = value_;
	}
	const boost::optional<Spinlocks >& getSpinlocks() const
	{
		return m_spinlocks;
	}
	void setSpinlocks(const boost::optional<Spinlocks >& value_)
	{
		m_spinlocks = value_;
	}
	const boost::optional<EVirOnOff >& getVpindex() const
	{
		return m_vpindex;
	}
	void setVpindex(const boost::optional<EVirOnOff >& value_)
	{
		m_vpindex = value_;
	}
	const boost::optional<EVirOnOff >& getRuntime() const
	{
		return m_runtime;
	}
	void setRuntime(const boost::optional<EVirOnOff >& value_)
	{
		m_runtime = value_;
	}
	const boost::optional<EVirOnOff >& getSynic() const
	{
		return m_synic;
	}
	void setSynic(const boost::optional<EVirOnOff >& value_)
	{
		m_synic = value_;
	}
	const boost::optional<EVirOnOff >& getStimer() const
	{
		return m_stimer;
	}
	void setStimer(const boost::optional<EVirOnOff >& value_)
	{
		m_stimer = value_;
	}
	const boost::optional<EVirOnOff >& getReset() const
	{
		return m_reset;
	}
	void setReset(const boost::optional<EVirOnOff >& value_)
	{
		m_reset = value_;
	}
	const boost::optional<VendorId >& getVendorId() const
	{
		return m_vendorId;
	}
	void setVendorId(const boost::optional<VendorId >& value_)
	{
		m_vendorId = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirOnOff > m_relaxed;
	boost::optional<EVirOnOff > m_vapic;
	boost::optional<Spinlocks > m_spinlocks;
	boost::optional<EVirOnOff > m_vpindex;
	boost::optional<EVirOnOff > m_runtime;
	boost::optional<EVirOnOff > m_synic;
	boost::optional<EVirOnOff > m_stimer;
	boost::optional<EVirOnOff > m_reset;
	boost::optional<VendorId > m_vendorId;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Kvm

namespace Domain
{
namespace Xml
{
struct Kvm
{
	const boost::optional<EVirOnOff >& getHidden() const
	{
		return m_hidden;
	}
	void setHidden(const boost::optional<EVirOnOff >& value_)
	{
		m_hidden = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirOnOff > m_hidden;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Pvspinlock

namespace Domain
{
namespace Xml
{
struct Pvspinlock
{
	const boost::optional<EVirOnOff >& getState() const
	{
		return m_state;
	}
	void setState(const boost::optional<EVirOnOff >& value_)
	{
		m_state = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirOnOff > m_state;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Capabilities

namespace Domain
{
namespace Xml
{
struct Capabilities
{
	Capabilities();

	EPolicy1 getPolicy() const
	{
		return m_policy;
	}
	void setPolicy(EPolicy1 value_)
	{
		m_policy = value_;
	}
	const boost::optional<EVirOnOff >& getAuditControl() const
	{
		return m_auditControl;
	}
	void setAuditControl(const boost::optional<EVirOnOff >& value_)
	{
		m_auditControl = value_;
	}
	const boost::optional<EVirOnOff >& getAuditWrite() const
	{
		return m_auditWrite;
	}
	void setAuditWrite(const boost::optional<EVirOnOff >& value_)
	{
		m_auditWrite = value_;
	}
	const boost::optional<EVirOnOff >& getBlockSuspend() const
	{
		return m_blockSuspend;
	}
	void setBlockSuspend(const boost::optional<EVirOnOff >& value_)
	{
		m_blockSuspend = value_;
	}
	const boost::optional<EVirOnOff >& getChown() const
	{
		return m_chown;
	}
	void setChown(const boost::optional<EVirOnOff >& value_)
	{
		m_chown = value_;
	}
	const boost::optional<EVirOnOff >& getDacOverride() const
	{
		return m_dacOverride;
	}
	void setDacOverride(const boost::optional<EVirOnOff >& value_)
	{
		m_dacOverride = value_;
	}
	const boost::optional<EVirOnOff >& getDacReadSearch() const
	{
		return m_dacReadSearch;
	}
	void setDacReadSearch(const boost::optional<EVirOnOff >& value_)
	{
		m_dacReadSearch = value_;
	}
	const boost::optional<EVirOnOff >& getFowner() const
	{
		return m_fowner;
	}
	void setFowner(const boost::optional<EVirOnOff >& value_)
	{
		m_fowner = value_;
	}
	const boost::optional<EVirOnOff >& getFsetid() const
	{
		return m_fsetid;
	}
	void setFsetid(const boost::optional<EVirOnOff >& value_)
	{
		m_fsetid = value_;
	}
	const boost::optional<EVirOnOff >& getIpcLock() const
	{
		return m_ipcLock;
	}
	void setIpcLock(const boost::optional<EVirOnOff >& value_)
	{
		m_ipcLock = value_;
	}
	const boost::optional<EVirOnOff >& getIpcOwner() const
	{
		return m_ipcOwner;
	}
	void setIpcOwner(const boost::optional<EVirOnOff >& value_)
	{
		m_ipcOwner = value_;
	}
	const boost::optional<EVirOnOff >& getKill() const
	{
		return m_kill;
	}
	void setKill(const boost::optional<EVirOnOff >& value_)
	{
		m_kill = value_;
	}
	const boost::optional<EVirOnOff >& getLease() const
	{
		return m_lease;
	}
	void setLease(const boost::optional<EVirOnOff >& value_)
	{
		m_lease = value_;
	}
	const boost::optional<EVirOnOff >& getLinuxImmutable() const
	{
		return m_linuxImmutable;
	}
	void setLinuxImmutable(const boost::optional<EVirOnOff >& value_)
	{
		m_linuxImmutable = value_;
	}
	const boost::optional<EVirOnOff >& getMacAdmin() const
	{
		return m_macAdmin;
	}
	void setMacAdmin(const boost::optional<EVirOnOff >& value_)
	{
		m_macAdmin = value_;
	}
	const boost::optional<EVirOnOff >& getMacOverride() const
	{
		return m_macOverride;
	}
	void setMacOverride(const boost::optional<EVirOnOff >& value_)
	{
		m_macOverride = value_;
	}
	const boost::optional<EVirOnOff >& getMknod() const
	{
		return m_mknod;
	}
	void setMknod(const boost::optional<EVirOnOff >& value_)
	{
		m_mknod = value_;
	}
	const boost::optional<EVirOnOff >& getNetAdmin() const
	{
		return m_netAdmin;
	}
	void setNetAdmin(const boost::optional<EVirOnOff >& value_)
	{
		m_netAdmin = value_;
	}
	const boost::optional<EVirOnOff >& getNetBindService() const
	{
		return m_netBindService;
	}
	void setNetBindService(const boost::optional<EVirOnOff >& value_)
	{
		m_netBindService = value_;
	}
	const boost::optional<EVirOnOff >& getNetBroadcast() const
	{
		return m_netBroadcast;
	}
	void setNetBroadcast(const boost::optional<EVirOnOff >& value_)
	{
		m_netBroadcast = value_;
	}
	const boost::optional<EVirOnOff >& getNetRaw() const
	{
		return m_netRaw;
	}
	void setNetRaw(const boost::optional<EVirOnOff >& value_)
	{
		m_netRaw = value_;
	}
	const boost::optional<EVirOnOff >& getSetgid() const
	{
		return m_setgid;
	}
	void setSetgid(const boost::optional<EVirOnOff >& value_)
	{
		m_setgid = value_;
	}
	const boost::optional<EVirOnOff >& getSetfcap() const
	{
		return m_setfcap;
	}
	void setSetfcap(const boost::optional<EVirOnOff >& value_)
	{
		m_setfcap = value_;
	}
	const boost::optional<EVirOnOff >& getSetpcap() const
	{
		return m_setpcap;
	}
	void setSetpcap(const boost::optional<EVirOnOff >& value_)
	{
		m_setpcap = value_;
	}
	const boost::optional<EVirOnOff >& getSetuid() const
	{
		return m_setuid;
	}
	void setSetuid(const boost::optional<EVirOnOff >& value_)
	{
		m_setuid = value_;
	}
	const boost::optional<EVirOnOff >& getSysAdmin() const
	{
		return m_sysAdmin;
	}
	void setSysAdmin(const boost::optional<EVirOnOff >& value_)
	{
		m_sysAdmin = value_;
	}
	const boost::optional<EVirOnOff >& getSysBoot() const
	{
		return m_sysBoot;
	}
	void setSysBoot(const boost::optional<EVirOnOff >& value_)
	{
		m_sysBoot = value_;
	}
	const boost::optional<EVirOnOff >& getSysChroot() const
	{
		return m_sysChroot;
	}
	void setSysChroot(const boost::optional<EVirOnOff >& value_)
	{
		m_sysChroot = value_;
	}
	const boost::optional<EVirOnOff >& getSysModule() const
	{
		return m_sysModule;
	}
	void setSysModule(const boost::optional<EVirOnOff >& value_)
	{
		m_sysModule = value_;
	}
	const boost::optional<EVirOnOff >& getSysNice() const
	{
		return m_sysNice;
	}
	void setSysNice(const boost::optional<EVirOnOff >& value_)
	{
		m_sysNice = value_;
	}
	const boost::optional<EVirOnOff >& getSysPacct() const
	{
		return m_sysPacct;
	}
	void setSysPacct(const boost::optional<EVirOnOff >& value_)
	{
		m_sysPacct = value_;
	}
	const boost::optional<EVirOnOff >& getSysPtrace() const
	{
		return m_sysPtrace;
	}
	void setSysPtrace(const boost::optional<EVirOnOff >& value_)
	{
		m_sysPtrace = value_;
	}
	const boost::optional<EVirOnOff >& getSysRawio() const
	{
		return m_sysRawio;
	}
	void setSysRawio(const boost::optional<EVirOnOff >& value_)
	{
		m_sysRawio = value_;
	}
	const boost::optional<EVirOnOff >& getSysResource() const
	{
		return m_sysResource;
	}
	void setSysResource(const boost::optional<EVirOnOff >& value_)
	{
		m_sysResource = value_;
	}
	const boost::optional<EVirOnOff >& getSysTime() const
	{
		return m_sysTime;
	}
	void setSysTime(const boost::optional<EVirOnOff >& value_)
	{
		m_sysTime = value_;
	}
	const boost::optional<EVirOnOff >& getSysTtyConfig() const
	{
		return m_sysTtyConfig;
	}
	void setSysTtyConfig(const boost::optional<EVirOnOff >& value_)
	{
		m_sysTtyConfig = value_;
	}
	const boost::optional<EVirOnOff >& getSyslog() const
	{
		return m_syslog;
	}
	void setSyslog(const boost::optional<EVirOnOff >& value_)
	{
		m_syslog = value_;
	}
	const boost::optional<EVirOnOff >& getWakeAlarm() const
	{
		return m_wakeAlarm;
	}
	void setWakeAlarm(const boost::optional<EVirOnOff >& value_)
	{
		m_wakeAlarm = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EPolicy1 m_policy;
	boost::optional<EVirOnOff > m_auditControl;
	boost::optional<EVirOnOff > m_auditWrite;
	boost::optional<EVirOnOff > m_blockSuspend;
	boost::optional<EVirOnOff > m_chown;
	boost::optional<EVirOnOff > m_dacOverride;
	boost::optional<EVirOnOff > m_dacReadSearch;
	boost::optional<EVirOnOff > m_fowner;
	boost::optional<EVirOnOff > m_fsetid;
	boost::optional<EVirOnOff > m_ipcLock;
	boost::optional<EVirOnOff > m_ipcOwner;
	boost::optional<EVirOnOff > m_kill;
	boost::optional<EVirOnOff > m_lease;
	boost::optional<EVirOnOff > m_linuxImmutable;
	boost::optional<EVirOnOff > m_macAdmin;
	boost::optional<EVirOnOff > m_macOverride;
	boost::optional<EVirOnOff > m_mknod;
	boost::optional<EVirOnOff > m_netAdmin;
	boost::optional<EVirOnOff > m_netBindService;
	boost::optional<EVirOnOff > m_netBroadcast;
	boost::optional<EVirOnOff > m_netRaw;
	boost::optional<EVirOnOff > m_setgid;
	boost::optional<EVirOnOff > m_setfcap;
	boost::optional<EVirOnOff > m_setpcap;
	boost::optional<EVirOnOff > m_setuid;
	boost::optional<EVirOnOff > m_sysAdmin;
	boost::optional<EVirOnOff > m_sysBoot;
	boost::optional<EVirOnOff > m_sysChroot;
	boost::optional<EVirOnOff > m_sysModule;
	boost::optional<EVirOnOff > m_sysNice;
	boost::optional<EVirOnOff > m_sysPacct;
	boost::optional<EVirOnOff > m_sysPtrace;
	boost::optional<EVirOnOff > m_sysRawio;
	boost::optional<EVirOnOff > m_sysResource;
	boost::optional<EVirOnOff > m_sysTime;
	boost::optional<EVirOnOff > m_sysTtyConfig;
	boost::optional<EVirOnOff > m_syslog;
	boost::optional<EVirOnOff > m_wakeAlarm;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Pmu

namespace Domain
{
namespace Xml
{
struct Pmu
{
	const boost::optional<EVirOnOff >& getState() const
	{
		return m_state;
	}
	void setState(const boost::optional<EVirOnOff >& value_)
	{
		m_state = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirOnOff > m_state;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Vmport

namespace Domain
{
namespace Xml
{
struct Vmport
{
	const boost::optional<EVirOnOff >& getState() const
	{
		return m_state;
	}
	void setState(const boost::optional<EVirOnOff >& value_)
	{
		m_state = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirOnOff > m_state;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Gic

namespace Domain
{
namespace Xml
{
struct Gic
{
	const boost::optional<PPositiveInteger::value_type >& getVersion() const
	{
		return m_version;
	}
	void setVersion(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_version = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PPositiveInteger::value_type > m_version;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Features

namespace Domain
{
namespace Xml
{
struct Features
{
	Features();

	bool getPae() const
	{
		return m_pae;
	}
	void setPae(bool value_)
	{
		m_pae = value_;
	}
	const boost::optional<Apic >& getApic() const
	{
		return m_apic;
	}
	void setApic(const boost::optional<Apic >& value_)
	{
		m_apic = value_;
	}
	bool getAcpi() const
	{
		return m_acpi;
	}
	void setAcpi(bool value_)
	{
		m_acpi = value_;
	}
	bool getHap() const
	{
		return m_hap;
	}
	void setHap(bool value_)
	{
		m_hap = value_;
	}
	const boost::optional<Hyperv >& getHyperv() const
	{
		return m_hyperv;
	}
	void setHyperv(const boost::optional<Hyperv >& value_)
	{
		m_hyperv = value_;
	}
	bool getViridian() const
	{
		return m_viridian;
	}
	void setViridian(bool value_)
	{
		m_viridian = value_;
	}
	const boost::optional<Kvm >& getKvm() const
	{
		return m_kvm;
	}
	void setKvm(const boost::optional<Kvm >& value_)
	{
		m_kvm = value_;
	}
	bool getPrivnet() const
	{
		return m_privnet;
	}
	void setPrivnet(bool value_)
	{
		m_privnet = value_;
	}
	const boost::optional<Pvspinlock >& getPvspinlock() const
	{
		return m_pvspinlock;
	}
	void setPvspinlock(const boost::optional<Pvspinlock >& value_)
	{
		m_pvspinlock = value_;
	}
	const boost::optional<Capabilities >& getCapabilities() const
	{
		return m_capabilities;
	}
	void setCapabilities(const boost::optional<Capabilities >& value_)
	{
		m_capabilities = value_;
	}
	const boost::optional<Pmu >& getPmu() const
	{
		return m_pmu;
	}
	void setPmu(const boost::optional<Pmu >& value_)
	{
		m_pmu = value_;
	}
	const boost::optional<Vmport >& getVmport() const
	{
		return m_vmport;
	}
	void setVmport(const boost::optional<Vmport >& value_)
	{
		m_vmport = value_;
	}
	const boost::optional<Gic >& getGic() const
	{
		return m_gic;
	}
	void setGic(const boost::optional<Gic >& value_)
	{
		m_gic = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	bool m_pae;
	boost::optional<Apic > m_apic;
	bool m_acpi;
	bool m_hap;
	boost::optional<Hyperv > m_hyperv;
	bool m_viridian;
	boost::optional<Kvm > m_kvm;
	bool m_privnet;
	boost::optional<Pvspinlock > m_pvspinlock;
	boost::optional<Capabilities > m_capabilities;
	boost::optional<Pmu > m_pmu;
	boost::optional<Vmport > m_vmport;
	boost::optional<Gic > m_gic;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct SuspendToMem

namespace Domain
{
namespace Xml
{
struct SuspendToMem
{
	const boost::optional<EVirYesNo >& getEnabled() const
	{
		return m_enabled;
	}
	void setEnabled(const boost::optional<EVirYesNo >& value_)
	{
		m_enabled = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_enabled;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct SuspendToDisk

namespace Domain
{
namespace Xml
{
struct SuspendToDisk
{
	const boost::optional<EVirYesNo >& getEnabled() const
	{
		return m_enabled;
	}
	void setEnabled(const boost::optional<EVirYesNo >& value_)
	{
		m_enabled = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_enabled;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Pm

namespace Domain
{
namespace Xml
{
struct Pm
{
	const boost::optional<SuspendToMem >& getSuspendToMem() const
	{
		return m_suspendToMem;
	}
	void setSuspendToMem(const boost::optional<SuspendToMem >& value_)
	{
		m_suspendToMem = value_;
	}
	const boost::optional<SuspendToDisk >& getSuspendToDisk() const
	{
		return m_suspendToDisk;
	}
	void setSuspendToDisk(const boost::optional<SuspendToDisk >& value_)
	{
		m_suspendToDisk = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<SuspendToMem > m_suspendToMem;
	boost::optional<SuspendToDisk > m_suspendToDisk;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Uid

namespace Domain
{
namespace Xml
{
struct Uid
{
	Uid();

	PUnsignedInt::value_type getStart() const
	{
		return m_start;
	}
	void setStart(PUnsignedInt::value_type value_)
	{
		m_start = value_;
	}
	PUnsignedInt::value_type getTarget() const
	{
		return m_target;
	}
	void setTarget(PUnsignedInt::value_type value_)
	{
		m_target = value_;
	}
	PUnsignedInt::value_type getCount() const
	{
		return m_count;
	}
	void setCount(PUnsignedInt::value_type value_)
	{
		m_count = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_start;
	PUnsignedInt::value_type m_target;
	PUnsignedInt::value_type m_count;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Gid

namespace Domain
{
namespace Xml
{
struct Gid
{
	Gid();

	PUnsignedInt::value_type getStart() const
	{
		return m_start;
	}
	void setStart(PUnsignedInt::value_type value_)
	{
		m_start = value_;
	}
	PUnsignedInt::value_type getTarget() const
	{
		return m_target;
	}
	void setTarget(PUnsignedInt::value_type value_)
	{
		m_target = value_;
	}
	PUnsignedInt::value_type getCount() const
	{
		return m_count;
	}
	void setCount(PUnsignedInt::value_type value_)
	{
		m_count = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_start;
	PUnsignedInt::value_type m_target;
	PUnsignedInt::value_type m_count;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Idmap

namespace Domain
{
namespace Xml
{
struct Idmap
{
	const QList<Uid >& getUidList() const
	{
		return m_uidList;
	}
	void setUidList(const QList<Uid >& value_)
	{
		m_uidList = value_;
	}
	const QList<Gid >& getGidList() const
	{
		return m_gidList;
	}
	void setGidList(const QList<Gid >& value_)
	{
		m_gidList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QList<Uid > m_uidList;
	QList<Gid > m_gidList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Disk471

namespace Domain
{
namespace Xml
{
struct Disk471
{
	Disk471();

	EDevice1 getDevice() const
	{
		return m_device;
	}
	void setDevice(EDevice1 value_)
	{
		m_device = value_;
	}
	const boost::optional<EVirYesNo >& getRawio() const
	{
		return m_rawio;
	}
	void setRawio(const boost::optional<EVirYesNo >& value_)
	{
		m_rawio = value_;
	}
	const boost::optional<ESgio >& getSgio() const
	{
		return m_sgio;
	}
	void setSgio(const boost::optional<ESgio >& value_)
	{
		m_sgio = value_;
	}

private:
	EDevice1 m_device;
	boost::optional<EVirYesNo > m_rawio;
	boost::optional<ESgio > m_sgio;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VDisk

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Domain::Xml::EDevice, Name::Strict<346> >, Disk471 > > VDiskImpl;
typedef VDiskImpl::value_type VDisk;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel234

namespace Domain
{
namespace Xml
{
struct Seclabel234
{
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel236

namespace Domain
{
namespace Xml
{
struct Seclabel236
{
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VSeclabel

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Seclabel234, Seclabel236, Ordered<mpl::vector<Optional<Attribute<mpl::int_<130>, Name::Strict<225> > >, OneOrMore<Element<Text<QString >, Name::Strict<226> > > > > > > VSeclabelImpl;
typedef VSeclabelImpl::value_type VSeclabel;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel

namespace Domain
{
namespace Xml
{
struct Seclabel
{
	const boost::optional<QString >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<QString >& value_)
	{
		m_model = value_;
	}
	const VSeclabel& getSeclabel() const
	{
		return m_seclabel;
	}
	void setSeclabel(const VSeclabel& value_)
	{
		m_seclabel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QString > m_model;
	VSeclabel m_seclabel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source

namespace Domain
{
namespace Xml
{
struct Source
{
	const PAbsFilePath::value_type& getDev() const
	{
		return m_dev;
	}
	void setDev(const PAbsFilePath::value_type& value_)
	{
		m_dev = value_;
	}
	const boost::optional<EStartupPolicy >& getStartupPolicy() const
	{
		return m_startupPolicy;
	}
	void setStartupPolicy(const boost::optional<EStartupPolicy >& value_)
	{
		m_startupPolicy = value_;
	}
	const boost::optional<Seclabel >& getSeclabel() const
	{
		return m_seclabel;
	}
	void setSeclabel(const boost::optional<Seclabel >& value_)
	{
		m_seclabel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PAbsFilePath::value_type m_dev;
	boost::optional<EStartupPolicy > m_startupPolicy;
	boost::optional<Seclabel > m_seclabel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source1

namespace Domain
{
namespace Xml
{
struct Source1
{
	const PAbsFilePath::value_type& getDir() const
	{
		return m_dir;
	}
	void setDir(const PAbsFilePath::value_type& value_)
	{
		m_dir = value_;
	}
	const boost::optional<EStartupPolicy >& getStartupPolicy() const
	{
		return m_startupPolicy;
	}
	void setStartupPolicy(const boost::optional<EStartupPolicy >& value_)
	{
		m_startupPolicy = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PAbsFilePath::value_type m_dir;
	boost::optional<EStartupPolicy > m_startupPolicy;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Host510

namespace Domain
{
namespace Xml
{
struct Host510
{
	const boost::optional<ETransport >& getTransport() const
	{
		return m_transport;
	}
	void setTransport(const boost::optional<ETransport >& value_)
	{
		m_transport = value_;
	}
	const VName& getName() const
	{
		return m_name;
	}
	void setName(const VName& value_)
	{
		m_name = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_port = value_;
	}

private:
	boost::optional<ETransport > m_transport;
	VName m_name;
	boost::optional<PUnsignedInt::value_type > m_port;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VHost

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Host510, Ordered<mpl::vector<Attribute<mpl::int_<512>, Name::Strict<506> >, Attribute<Domain::Xml::PAbsFilePath, Name::Strict<513> > > > > > VHostImpl;
typedef VHostImpl::value_type VHost;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source2

namespace Domain
{
namespace Xml
{
struct Source2
{
	Source2();

	EProtocol getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(EProtocol value_)
	{
		m_protocol = value_;
	}
	const boost::optional<QString >& getName() const
	{
		return m_name;
	}
	void setName(const boost::optional<QString >& value_)
	{
		m_name = value_;
	}
	const QList<VHost >& getHostList() const
	{
		return m_hostList;
	}
	void setHostList(const QList<VHost >& value_)
	{
		m_hostList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EProtocol m_protocol;
	boost::optional<QString > m_name;
	QList<VHost > m_hostList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source3

namespace Domain
{
namespace Xml
{
struct Source3
{
	const PGenericName::value_type& getPool() const
	{
		return m_pool;
	}
	void setPool(const PGenericName::value_type& value_)
	{
		m_pool = value_;
	}
	const PVolName::value_type& getVolume() const
	{
		return m_volume;
	}
	void setVolume(const PVolName::value_type& value_)
	{
		m_volume = value_;
	}
	const boost::optional<EMode5 >& getMode() const
	{
		return m_mode;
	}
	void setMode(const boost::optional<EMode5 >& value_)
	{
		m_mode = value_;
	}
	const boost::optional<EStartupPolicy >& getStartupPolicy() const
	{
		return m_startupPolicy;
	}
	void setStartupPolicy(const boost::optional<EStartupPolicy >& value_)
	{
		m_startupPolicy = value_;
	}
	const boost::optional<Seclabel >& getSeclabel() const
	{
		return m_seclabel;
	}
	void setSeclabel(const boost::optional<Seclabel >& value_)
	{
		m_seclabel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PGenericName::value_type m_pool;
	PVolName::value_type m_volume;
	boost::optional<EMode5 > m_mode;
	boost::optional<EStartupPolicy > m_startupPolicy;
	boost::optional<Seclabel > m_seclabel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source4

namespace Domain
{
namespace Xml
{
struct Source4
{
	const boost::optional<PAbsFilePath::value_type >& getFile() const
	{
		return m_file;
	}
	void setFile(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_file = value_;
	}
	const boost::optional<EStartupPolicy >& getStartupPolicy() const
	{
		return m_startupPolicy;
	}
	void setStartupPolicy(const boost::optional<EStartupPolicy >& value_)
	{
		m_startupPolicy = value_;
	}
	const boost::optional<Seclabel >& getSeclabel() const
	{
		return m_seclabel;
	}
	void setSeclabel(const boost::optional<Seclabel >& value_)
	{
		m_seclabel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PAbsFilePath::value_type > m_file;
	boost::optional<EStartupPolicy > m_startupPolicy;
	boost::optional<Seclabel > m_seclabel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VDiskSource

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Ordered<mpl::vector<Attribute<mpl::int_<494>, Name::Strict<100> >, Optional<Element<Domain::Xml::Source, Name::Strict<493> > > > >, Ordered<mpl::vector<Attribute<mpl::int_<159>, Name::Strict<100> >, Optional<Element<Domain::Xml::Source1, Name::Strict<493> > > > >, Ordered<mpl::vector<Attribute<mpl::int_<438>, Name::Strict<100> >, Element<Domain::Xml::Source2, Name::Strict<493> > > >, Ordered<mpl::vector<Attribute<mpl::int_<515>, Name::Strict<100> >, Optional<Element<Domain::Xml::Source3, Name::Strict<493> > > > >, Ordered<mpl::vector<Optional<Attribute<mpl::int_<492>, Name::Strict<100> > >, Optional<Element<Domain::Xml::Source4, Name::Strict<493> > > > > > > VDiskSourceImpl;
typedef VDiskSourceImpl::value_type VDiskSource;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct DriverFormat

namespace Domain
{
namespace Xml
{
struct DriverFormat
{
	const PGenericName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PGenericName::value_type& value_)
	{
		m_name = value_;
	}
	const boost::optional<VType >& getType() const
	{
		return m_type;
	}
	void setType(const boost::optional<VType >& value_)
	{
		m_type = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PGenericName::value_type m_name;
	boost::optional<VType > m_type;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Driver

namespace Domain
{
namespace Xml
{
struct Driver
{
	const boost::optional<DriverFormat >& getDriverFormat() const
	{
		return m_driverFormat;
	}
	void setDriverFormat(const boost::optional<DriverFormat >& value_)
	{
		m_driverFormat = value_;
	}
	const boost::optional<ECache >& getCache() const
	{
		return m_cache;
	}
	void setCache(const boost::optional<ECache >& value_)
	{
		m_cache = value_;
	}
	const boost::optional<EErrorPolicy >& getErrorPolicy() const
	{
		return m_errorPolicy;
	}
	void setErrorPolicy(const boost::optional<EErrorPolicy >& value_)
	{
		m_errorPolicy = value_;
	}
	const boost::optional<ERerrorPolicy >& getRerrorPolicy() const
	{
		return m_rerrorPolicy;
	}
	void setRerrorPolicy(const boost::optional<ERerrorPolicy >& value_)
	{
		m_rerrorPolicy = value_;
	}
	const boost::optional<EIo >& getIo() const
	{
		return m_io;
	}
	void setIo(const boost::optional<EIo >& value_)
	{
		m_io = value_;
	}
	const boost::optional<EVirOnOff >& getIoeventfd() const
	{
		return m_ioeventfd;
	}
	void setIoeventfd(const boost::optional<EVirOnOff >& value_)
	{
		m_ioeventfd = value_;
	}
	const boost::optional<EVirOnOff >& getEventIdx() const
	{
		return m_eventIdx;
	}
	void setEventIdx(const boost::optional<EVirOnOff >& value_)
	{
		m_eventIdx = value_;
	}
	const boost::optional<EVirOnOff >& getCopyOnRead() const
	{
		return m_copyOnRead;
	}
	void setCopyOnRead(const boost::optional<EVirOnOff >& value_)
	{
		m_copyOnRead = value_;
	}
	const boost::optional<EDiscard >& getDiscard() const
	{
		return m_discard;
	}
	void setDiscard(const boost::optional<EDiscard >& value_)
	{
		m_discard = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getIothread() const
	{
		return m_iothread;
	}
	void setIothread(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_iothread = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<DriverFormat > m_driverFormat;
	boost::optional<ECache > m_cache;
	boost::optional<EErrorPolicy > m_errorPolicy;
	boost::optional<ERerrorPolicy > m_rerrorPolicy;
	boost::optional<EIo > m_io;
	boost::optional<EVirOnOff > m_ioeventfd;
	boost::optional<EVirOnOff > m_eventIdx;
	boost::optional<EVirOnOff > m_copyOnRead;
	boost::optional<EDiscard > m_discard;
	boost::optional<PUnsignedInt::value_type > m_iothread;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1138

namespace Domain
{
namespace Xml
{
struct Anonymous1138
{
	const boost::optional<Source4 >& getSource() const
	{
		return m_source;
	}
	void setSource(const boost::optional<Source4 >& value_)
	{
		m_source = value_;
	}
	const boost::optional<VStorageFormat >& getFormat() const
	{
		return m_format;
	}
	void setFormat(const boost::optional<VStorageFormat >& value_)
	{
		m_format = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<Source4 > m_source;
	boost::optional<VStorageFormat > m_format;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Mirror1053

namespace Domain
{
namespace Xml
{
struct Mirror1053
{
	const PAbsFilePath::value_type& getFile() const
	{
		return m_file;
	}
	void setFile(const PAbsFilePath::value_type& value_)
	{
		m_file = value_;
	}
	const boost::optional<VStorageFormat >& getFormat() const
	{
		return m_format;
	}
	void setFormat(const boost::optional<VStorageFormat >& value_)
	{
		m_format = value_;
	}
	const boost::optional<EJob >& getJob() const
	{
		return m_job;
	}
	void setJob(const boost::optional<EJob >& value_)
	{
		m_job = value_;
	}
	const boost::optional<Anonymous1138 >& getAnonymous1138() const
	{
		return m_anonymous1138;
	}
	void setAnonymous1138(const boost::optional<Anonymous1138 >& value_)
	{
		m_anonymous1138 = value_;
	}

private:
	PAbsFilePath::value_type m_file;
	boost::optional<VStorageFormat > m_format;
	boost::optional<EJob > m_job;
	boost::optional<Anonymous1138 > m_anonymous1138;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Mirror1055

namespace Domain
{
namespace Xml
{
struct Mirror1055
{
	Mirror1055();

	EJob1 getJob() const
	{
		return m_job;
	}
	void setJob(EJob1 value_)
	{
		m_job = value_;
	}
	const VDiskSource& getDiskSource() const
	{
		return m_diskSource;
	}
	void setDiskSource(const VDiskSource& value_)
	{
		m_diskSource = value_;
	}
	const boost::optional<VStorageFormat >& getFormat() const
	{
		return m_format;
	}
	void setFormat(const boost::optional<VStorageFormat >& value_)
	{
		m_format = value_;
	}

private:
	EJob1 m_job;
	VDiskSource m_diskSource;
	boost::optional<VStorageFormat > m_format;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VMirror

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Mirror1053, Mirror1055 > > VMirrorImpl;
typedef VMirrorImpl::value_type VMirror;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Mirror

namespace Domain
{
namespace Xml
{
struct Mirror
{
	const VMirror& getMirror() const
	{
		return m_mirror;
	}
	void setMirror(const VMirror& value_)
	{
		m_mirror = value_;
	}
	const boost::optional<EReady >& getReady() const
	{
		return m_ready;
	}
	void setReady(const boost::optional<EReady >& value_)
	{
		m_ready = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VMirror m_mirror;
	boost::optional<EReady > m_ready;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VSecret

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Domain::Xml::VUUID, Name::Strict<146> >, Attribute<Domain::Xml::PGenericName, Name::Strict<616> > > > VSecretImpl;
typedef VSecretImpl::value_type VSecret;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Secret

namespace Domain
{
namespace Xml
{
struct Secret
{
	Secret();

	EType4 getType() const
	{
		return m_type;
	}
	void setType(EType4 value_)
	{
		m_type = value_;
	}
	const VSecret& getSecret() const
	{
		return m_secret;
	}
	void setSecret(const VSecret& value_)
	{
		m_secret = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EType4 m_type;
	VSecret m_secret;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Auth

namespace Domain
{
namespace Xml
{
struct Auth
{
	const PGenericName::value_type& getUsername() const
	{
		return m_username;
	}
	void setUsername(const PGenericName::value_type& value_)
	{
		m_username = value_;
	}
	const Secret& getSecret() const
	{
		return m_secret;
	}
	void setSecret(const Secret& value_)
	{
		m_secret = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PGenericName::value_type m_username;
	Secret m_secret;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Target

namespace Domain
{
namespace Xml
{
struct Target
{
	const PDiskTarget::value_type& getDev() const
	{
		return m_dev;
	}
	void setDev(const PDiskTarget::value_type& value_)
	{
		m_dev = value_;
	}
	const boost::optional<EBus >& getBus() const
	{
		return m_bus;
	}
	void setBus(const boost::optional<EBus >& value_)
	{
		m_bus = value_;
	}
	const boost::optional<ETray >& getTray() const
	{
		return m_tray;
	}
	void setTray(const boost::optional<ETray >& value_)
	{
		m_tray = value_;
	}
	const boost::optional<EVirOnOff >& getRemovable() const
	{
		return m_removable;
	}
	void setRemovable(const boost::optional<EVirOnOff >& value_)
	{
		m_removable = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PDiskTarget::value_type m_dev;
	boost::optional<EBus > m_bus;
	boost::optional<ETray > m_tray;
	boost::optional<EVirOnOff > m_removable;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Secret1

namespace Domain
{
namespace Xml
{
struct Secret1
{
	Secret1();

	EType5 getType() const
	{
		return m_type;
	}
	void setType(EType5 value_)
	{
		m_type = value_;
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
	EType5 m_type;
	VUUID m_uuid;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Encryption

namespace Domain
{
namespace Xml
{
struct Encryption
{
	Encryption();

	EFormat1 getFormat() const
	{
		return m_format;
	}
	void setFormat(EFormat1 value_)
	{
		m_format = value_;
	}
	const QList<Secret1 >& getSecretList() const
	{
		return m_secretList;
	}
	void setSecretList(const QList<Secret1 >& value_)
	{
		m_secretList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EFormat1 m_format;
	QList<Secret1 > m_secretList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Variant1068

namespace Domain
{
namespace Xml
{
struct Variant1068
{
	const boost::optional<PReadBytesSec::value_type >& getReadBytesSec() const
	{
		return m_readBytesSec;
	}
	void setReadBytesSec(const boost::optional<PReadBytesSec::value_type >& value_)
	{
		m_readBytesSec = value_;
	}
	const boost::optional<PWriteBytesSec::value_type >& getWriteBytesSec() const
	{
		return m_writeBytesSec;
	}
	void setWriteBytesSec(const boost::optional<PWriteBytesSec::value_type >& value_)
	{
		m_writeBytesSec = value_;
	}

private:
	boost::optional<PReadBytesSec::value_type > m_readBytesSec;
	boost::optional<PWriteBytesSec::value_type > m_writeBytesSec;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1069

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Text<Domain::Xml::PTotalBytesSec >, Name::Strict<1067> >, Variant1068 > > VChoice1069Impl;
typedef VChoice1069Impl::value_type VChoice1069;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Variant1072

namespace Domain
{
namespace Xml
{
struct Variant1072
{
	const boost::optional<PReadIopsSec1::value_type >& getReadIopsSec() const
	{
		return m_readIopsSec;
	}
	void setReadIopsSec(const boost::optional<PReadIopsSec1::value_type >& value_)
	{
		m_readIopsSec = value_;
	}
	const boost::optional<PWriteIopsSec1::value_type >& getWriteIopsSec() const
	{
		return m_writeIopsSec;
	}
	void setWriteIopsSec(const boost::optional<PWriteIopsSec1::value_type >& value_)
	{
		m_writeIopsSec = value_;
	}

private:
	boost::optional<PReadIopsSec1::value_type > m_readIopsSec;
	boost::optional<PWriteIopsSec1::value_type > m_writeIopsSec;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1073

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Text<Domain::Xml::PTotalIopsSec >, Name::Strict<1071> >, Variant1072 > > VChoice1073Impl;
typedef VChoice1073Impl::value_type VChoice1073;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Iotune

namespace Domain
{
namespace Xml
{
struct Iotune
{
	const VChoice1069& getChoice1069() const
	{
		return m_choice1069;
	}
	void setChoice1069(const VChoice1069& value_)
	{
		m_choice1069 = value_;
	}
	const VChoice1073& getChoice1073() const
	{
		return m_choice1073;
	}
	void setChoice1073(const VChoice1073& value_)
	{
		m_choice1073 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VChoice1069 m_choice1069;
	VChoice1073 m_choice1073;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Pciaddress

namespace Domain
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
	bool save(QDomDocument& ) const;

private:
	boost::optional<PPciDomain::value_type > m_domain;
	PPciBus::value_type m_bus;
	PPciSlot::value_type m_slot;
	PPciFunc::value_type m_function;
	boost::optional<EVirOnOff > m_multifunction;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Driveaddress

namespace Domain
{
namespace Xml
{
struct Driveaddress
{
	const boost::optional<PDriveController::value_type >& getController() const
	{
		return m_controller;
	}
	void setController(const boost::optional<PDriveController::value_type >& value_)
	{
		m_controller = value_;
	}
	const boost::optional<PDriveBus::value_type >& getBus() const
	{
		return m_bus;
	}
	void setBus(const boost::optional<PDriveBus::value_type >& value_)
	{
		m_bus = value_;
	}
	const boost::optional<PDriveTarget::value_type >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<PDriveTarget::value_type >& value_)
	{
		m_target = value_;
	}
	const boost::optional<PDriveUnit::value_type >& getUnit() const
	{
		return m_unit;
	}
	void setUnit(const boost::optional<PDriveUnit::value_type >& value_)
	{
		m_unit = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<PDriveController::value_type > m_controller;
	boost::optional<PDriveBus::value_type > m_bus;
	boost::optional<PDriveTarget::value_type > m_target;
	boost::optional<PDriveUnit::value_type > m_unit;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Virtioserialaddress

namespace Domain
{
namespace Xml
{
struct Virtioserialaddress
{
	const PDriveController::value_type& getController() const
	{
		return m_controller;
	}
	void setController(const PDriveController::value_type& value_)
	{
		m_controller = value_;
	}
	const boost::optional<PDriveBus::value_type >& getBus() const
	{
		return m_bus;
	}
	void setBus(const boost::optional<PDriveBus::value_type >& value_)
	{
		m_bus = value_;
	}
	const boost::optional<PDriveUnit::value_type >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<PDriveUnit::value_type >& value_)
	{
		m_port = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PDriveController::value_type m_controller;
	boost::optional<PDriveBus::value_type > m_bus;
	boost::optional<PDriveUnit::value_type > m_port;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Ccidaddress

namespace Domain
{
namespace Xml
{
struct Ccidaddress
{
	const PDriveController::value_type& getController() const
	{
		return m_controller;
	}
	void setController(const PDriveController::value_type& value_)
	{
		m_controller = value_;
	}
	const boost::optional<PDriveUnit::value_type >& getSlot() const
	{
		return m_slot;
	}
	void setSlot(const boost::optional<PDriveUnit::value_type >& value_)
	{
		m_slot = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PDriveController::value_type m_controller;
	boost::optional<PDriveUnit::value_type > m_slot;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Usbportaddress

namespace Domain
{
namespace Xml
{
struct Usbportaddress
{
	const PUsbAddr::value_type& getBus() const
	{
		return m_bus;
	}
	void setBus(const PUsbAddr::value_type& value_)
	{
		m_bus = value_;
	}
	const PUsbPort::value_type& getPort() const
	{
		return m_port;
	}
	void setPort(const PUsbPort::value_type& value_)
	{
		m_port = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PUsbAddr::value_type m_bus;
	PUsbPort::value_type m_port;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1139

namespace Domain
{
namespace Xml
{
struct Anonymous1139
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
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	VCcwCssidRange m_cssid;
	PCcwSsidRange::value_type m_ssid;
	VCcwDevnoRange m_devno;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Isaaddress

namespace Domain
{
namespace Xml
{
struct Isaaddress
{
	const boost::optional<PIobase::value_type >& getIobase() const
	{
		return m_iobase;
	}
	void setIobase(const boost::optional<PIobase::value_type >& value_)
	{
		m_iobase = value_;
	}
	const boost::optional<PIrq::value_type >& getIrq() const
	{
		return m_irq;
	}
	void setIrq(const boost::optional<PIrq::value_type >& value_)
	{
		m_irq = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<PIobase::value_type > m_iobase;
	boost::optional<PIrq::value_type > m_irq;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Dimmaddress

namespace Domain
{
namespace Xml
{
struct Dimmaddress
{
	const boost::optional<PUnsignedInt::value_type >& getSlot() const
	{
		return m_slot;
	}
	void setSlot(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_slot = value_;
	}
	const boost::optional<PHexuint::value_type >& getBase() const
	{
		return m_base;
	}
	void setBase(const boost::optional<PHexuint::value_type >& value_)
	{
		m_base = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<PUnsignedInt::value_type > m_slot;
	boost::optional<PHexuint::value_type > m_base;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VAddress

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Ordered<mpl::vector<Attribute<mpl::int_<588>, Name::Strict<100> >, Fragment<Pciaddress > > >, Ordered<mpl::vector<Attribute<mpl::int_<1014>, Name::Strict<100> >, Fragment<Driveaddress > > >, Ordered<mpl::vector<Attribute<mpl::int_<598>, Name::Strict<100> >, Fragment<Virtioserialaddress > > >, Ordered<mpl::vector<Attribute<mpl::int_<565>, Name::Strict<100> >, Fragment<Ccidaddress > > >, Ordered<mpl::vector<Attribute<mpl::int_<523>, Name::Strict<100> >, Fragment<Usbportaddress > > >, Ordered<mpl::vector<Attribute<mpl::int_<1019>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::PSpaprvioReg, Name::Strict<914> > > > >, Ordered<mpl::vector<Attribute<mpl::int_<1021>, Name::Strict<100> >, Optional<Fragment<Anonymous1139 > > > >, Ordered<mpl::vector<Attribute<mpl::int_<1023>, Name::Strict<100> >, Fragment<Isaaddress > > >, Ordered<mpl::vector<Attribute<mpl::int_<1025>, Name::Strict<100> >, Fragment<Dimmaddress > > >, Ordered<mpl::vector<Attribute<mpl::int_<1027>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::VUUID, Name::Strict<179> > > > > > > VAddressImpl;
typedef VAddressImpl::value_type VAddress;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Geometry

namespace Domain
{
namespace Xml
{
struct Geometry
{
	Geometry();

	PCyls::value_type getCyls() const
	{
		return m_cyls;
	}
	void setCyls(PCyls::value_type value_)
	{
		m_cyls = value_;
	}
	PHeads::value_type getHeads() const
	{
		return m_heads;
	}
	void setHeads(PHeads::value_type value_)
	{
		m_heads = value_;
	}
	PSecs::value_type getSecs() const
	{
		return m_secs;
	}
	void setSecs(PSecs::value_type value_)
	{
		m_secs = value_;
	}
	const boost::optional<ETrans >& getTrans() const
	{
		return m_trans;
	}
	void setTrans(const boost::optional<ETrans >& value_)
	{
		m_trans = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PCyls::value_type m_cyls;
	PHeads::value_type m_heads;
	PSecs::value_type m_secs;
	boost::optional<ETrans > m_trans;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Blockio

namespace Domain
{
namespace Xml
{
struct Blockio
{
	const boost::optional<PLogicalBlockSize::value_type >& getLogicalBlockSize() const
	{
		return m_logicalBlockSize;
	}
	void setLogicalBlockSize(const boost::optional<PLogicalBlockSize::value_type >& value_)
	{
		m_logicalBlockSize = value_;
	}
	const boost::optional<PPhysicalBlockSize::value_type >& getPhysicalBlockSize() const
	{
		return m_physicalBlockSize;
	}
	void setPhysicalBlockSize(const boost::optional<PPhysicalBlockSize::value_type >& value_)
	{
		m_physicalBlockSize = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PLogicalBlockSize::value_type > m_logicalBlockSize;
	boost::optional<PPhysicalBlockSize::value_type > m_physicalBlockSize;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct BackingStore

namespace Domain
{
namespace Xml
{
struct BackingStore
{
	BackingStore();

	PPositiveInteger::value_type getIndex() const
	{
		return m_index;
	}
	void setIndex(PPositiveInteger::value_type value_)
	{
		m_index = value_;
	}
	const VDiskSource& getDiskSource() const
	{
		return m_diskSource;
	}
	void setDiskSource(const VDiskSource& value_)
	{
		m_diskSource = value_;
	}
	const VDiskBackingChainBin* getDiskBackingChain() const;
	void setDiskBackingChain(const VDiskBackingChainBin& value_);
	const VStorageFormat& getFormat() const
	{
		return m_format;
	}
	void setFormat(const VStorageFormat& value_)
	{
		m_format = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PPositiveInteger::value_type m_index;
	VDiskSource m_diskSource;
	boost::any m_diskBackingChain;
	VStorageFormat m_format;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VDiskBackingChain

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Domain::Xml::BackingStore, Name::Strict<479> >, Optional<Element<Empty, Name::Strict<479> > > > > VDiskBackingChainImpl;
typedef VDiskBackingChainImpl::value_type VDiskBackingChain;
struct VDiskBackingChainBin
{
	VDiskBackingChain value;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Domain
{
namespace Xml
{
struct Disk
{
	Disk();

	const VDisk& getDisk() const
	{
		return m_disk;
	}
	void setDisk(const VDisk& value_)
	{
		m_disk = value_;
	}
	const boost::optional<ESnapshot >& getSnapshot() const
	{
		return m_snapshot;
	}
	void setSnapshot(const boost::optional<ESnapshot >& value_)
	{
		m_snapshot = value_;
	}
	const VDiskSource& getDiskSource() const
	{
		return m_diskSource;
	}
	void setDiskSource(const VDiskSource& value_)
	{
		m_diskSource = value_;
	}
	const boost::optional<Driver >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<Mirror >& getMirror() const
	{
		return m_mirror;
	}
	void setMirror(const boost::optional<Mirror >& value_)
	{
		m_mirror = value_;
	}
	const boost::optional<Auth >& getAuth() const
	{
		return m_auth;
	}
	void setAuth(const boost::optional<Auth >& value_)
	{
		m_auth = value_;
	}
	const Target& getTarget() const
	{
		return m_target;
	}
	void setTarget(const Target& value_)
	{
		m_target = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	bool getReadonly() const
	{
		return m_readonly;
	}
	void setReadonly(bool value_)
	{
		m_readonly = value_;
	}
	bool getShareable() const
	{
		return m_shareable;
	}
	void setShareable(bool value_)
	{
		m_shareable = value_;
	}
	bool getTransient() const
	{
		return m_transient;
	}
	void setTransient(bool value_)
	{
		m_transient = value_;
	}
	const boost::optional<PDiskSerial::value_type >& getSerial() const
	{
		return m_serial;
	}
	void setSerial(const boost::optional<PDiskSerial::value_type >& value_)
	{
		m_serial = value_;
	}
	const boost::optional<Encryption >& getEncryption() const
	{
		return m_encryption;
	}
	void setEncryption(const boost::optional<Encryption >& value_)
	{
		m_encryption = value_;
	}
	const boost::optional<Iotune >& getIotune() const
	{
		return m_iotune;
	}
	void setIotune(const boost::optional<Iotune >& value_)
	{
		m_iotune = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<Geometry >& getGeometry() const
	{
		return m_geometry;
	}
	void setGeometry(const boost::optional<Geometry >& value_)
	{
		m_geometry = value_;
	}
	const boost::optional<Blockio >& getBlockio() const
	{
		return m_blockio;
	}
	void setBlockio(const boost::optional<Blockio >& value_)
	{
		m_blockio = value_;
	}
	const boost::optional<PWwn::value_type >& getWwn() const
	{
		return m_wwn;
	}
	void setWwn(const boost::optional<PWwn::value_type >& value_)
	{
		m_wwn = value_;
	}
	const boost::optional<PVendor::value_type >& getVendor() const
	{
		return m_vendor;
	}
	void setVendor(const boost::optional<PVendor::value_type >& value_)
	{
		m_vendor = value_;
	}
	const boost::optional<PProduct::value_type >& getProduct() const
	{
		return m_product;
	}
	void setProduct(const boost::optional<PProduct::value_type >& value_)
	{
		m_product = value_;
	}
	const VDiskBackingChain& getDiskBackingChain() const
	{
		return m_diskBackingChain;
	}
	void setDiskBackingChain(const VDiskBackingChain& value_)
	{
		m_diskBackingChain = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VDisk m_disk;
	boost::optional<ESnapshot > m_snapshot;
	VDiskSource m_diskSource;
	boost::optional<Driver > m_driver;
	boost::optional<Mirror > m_mirror;
	boost::optional<Auth > m_auth;
	Target m_target;
	boost::optional<PPositiveInteger::value_type > m_boot;
	bool m_readonly;
	bool m_shareable;
	bool m_transient;
	boost::optional<PDiskSerial::value_type > m_serial;
	boost::optional<Encryption > m_encryption;
	boost::optional<Iotune > m_iotune;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<Geometry > m_geometry;
	boost::optional<Blockio > m_blockio;
	boost::optional<PWwn::value_type > m_wwn;
	boost::optional<PVendor::value_type > m_vendor;
	boost::optional<PProduct::value_type > m_product;
	VDiskBackingChain m_diskBackingChain;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Variant586

namespace Domain
{
namespace Xml
{
struct Variant586
{
	const boost::optional<EModel1 >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<EModel1 >& value_)
	{
		m_model = value_;
	}
	const boost::optional<PUsbPort::value_type >& getMaster() const
	{
		return m_master;
	}
	void setMaster(const boost::optional<PUsbPort::value_type >& value_)
	{
		m_master = value_;
	}

private:
	boost::optional<EModel1 > m_model;
	boost::optional<PUsbPort::value_type > m_master;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Variant591

namespace Domain
{
namespace Xml
{
struct Variant591
{
	Variant591();

	EModel2 getModel() const
	{
		return m_model;
	}
	void setModel(EModel2 value_)
	{
		m_model = value_;
	}
	const boost::optional<ScaledInteger >& getPcihole64() const
	{
		return m_pcihole64;
	}
	void setPcihole64(const boost::optional<ScaledInteger >& value_)
	{
		m_pcihole64 = value_;
	}

private:
	EModel2 m_model;
	boost::optional<ScaledInteger > m_pcihole64;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice595

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Variant591, Attribute<Domain::Xml::EModel3, Name::Strict<223> > > > VChoice595Impl;
typedef VChoice595Impl::value_type VChoice595;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Variant600

namespace Domain
{
namespace Xml
{
struct Variant600
{
	const boost::optional<PUnsignedInt::value_type >& getPorts() const
	{
		return m_ports;
	}
	void setPorts(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_ports = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getVectors() const
	{
		return m_vectors;
	}
	void setVectors(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_vectors = value_;
	}

private:
	boost::optional<PUnsignedInt::value_type > m_ports;
	boost::optional<PUnsignedInt::value_type > m_vectors;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice601

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Domain::Xml::EType6, Name::Strict<100> >, Ordered<mpl::vector<Attribute<mpl::int_<521>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::EModel, Name::Strict<223> > > > >, Variant586, Ordered<mpl::vector<Attribute<mpl::int_<588>, Name::Strict<100> >, Domain::Xml::VChoice595Impl > >, Variant600 > > VChoice601Impl;
typedef VChoice601Impl::value_type VChoice601;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Driver1

namespace Domain
{
namespace Xml
{
struct Driver1
{
	const boost::optional<PUnsignedInt::value_type >& getQueues() const
	{
		return m_queues;
	}
	void setQueues(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_queues = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getCmdPerLun() const
	{
		return m_cmdPerLun;
	}
	void setCmdPerLun(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_cmdPerLun = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getMaxSectors() const
	{
		return m_maxSectors;
	}
	void setMaxSectors(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_maxSectors = value_;
	}
	const boost::optional<EVirOnOff >& getIoeventfd() const
	{
		return m_ioeventfd;
	}
	void setIoeventfd(const boost::optional<EVirOnOff >& value_)
	{
		m_ioeventfd = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getIothread() const
	{
		return m_iothread;
	}
	void setIothread(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_iothread = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PUnsignedInt::value_type > m_queues;
	boost::optional<PUnsignedInt::value_type > m_cmdPerLun;
	boost::optional<PUnsignedInt::value_type > m_maxSectors;
	boost::optional<EVirOnOff > m_ioeventfd;
	boost::optional<PUnsignedInt::value_type > m_iothread;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Controller

namespace Domain
{
namespace Xml
{
struct Controller
{
	Controller();

	PUnsignedInt::value_type getIndex() const
	{
		return m_index;
	}
	void setIndex(PUnsignedInt::value_type value_)
	{
		m_index = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const VChoice601& getChoice601() const
	{
		return m_choice601;
	}
	void setChoice601(const VChoice601& value_)
	{
		m_choice601 = value_;
	}
	const boost::optional<Driver1 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver1 >& value_)
	{
		m_driver = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_index;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	VChoice601 m_choice601;
	boost::optional<Driver1 > m_driver;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Target1

namespace Domain
{
namespace Xml
{
struct Target1
{
	const QString& getPath() const
	{
		return m_path;
	}
	void setPath(const QString& value_)
	{
		m_path = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getOffset() const
	{
		return m_offset;
	}
	void setOffset(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_offset = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_path;
	boost::optional<PUnsignedInt::value_type > m_offset;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Lease

namespace Domain
{
namespace Xml
{
struct Lease
{
	const QString& getLockspace() const
	{
		return m_lockspace;
	}
	void setLockspace(const QString& value_)
	{
		m_lockspace = value_;
	}
	const QString& getKey() const
	{
		return m_key;
	}
	void setKey(const QString& value_)
	{
		m_key = value_;
	}
	const Target1& getTarget() const
	{
		return m_target;
	}
	void setTarget(const Target1& value_)
	{
		m_target = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_lockspace;
	QString m_key;
	Target1 m_target;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Driver2

namespace Domain
{
namespace Xml
{
struct Driver2
{
	const boost::optional<EType7 >& getType() const
	{
		return m_type;
	}
	void setType(const boost::optional<EType7 >& value_)
	{
		m_type = value_;
	}
	const boost::optional<VStorageFormat >& getFormat() const
	{
		return m_format;
	}
	void setFormat(const boost::optional<VStorageFormat >& value_)
	{
		m_format = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EType7 > m_type;
	boost::optional<VStorageFormat > m_format;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem607

namespace Domain
{
namespace Xml
{
struct Filesystem607
{
	const boost::optional<Driver2 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver2 >& value_)
	{
		m_driver = value_;
	}
	const PAbsFilePath::value_type& getSource() const
	{
		return m_source;
	}
	void setSource(const PAbsFilePath::value_type& value_)
	{
		m_source = value_;
	}

private:
	boost::optional<Driver2 > m_driver;
	PAbsFilePath::value_type m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem608

namespace Domain
{
namespace Xml
{
struct Filesystem608
{
	const boost::optional<Driver2 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver2 >& value_)
	{
		m_driver = value_;
	}
	const PAbsFilePath::value_type& getSource() const
	{
		return m_source;
	}
	void setSource(const PAbsFilePath::value_type& value_)
	{
		m_source = value_;
	}

private:
	boost::optional<Driver2 > m_driver;
	PAbsFilePath::value_type m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem610

namespace Domain
{
namespace Xml
{
struct Filesystem610
{
	const boost::optional<Driver2 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver2 >& value_)
	{
		m_driver = value_;
	}
	const PAbsDirPath::value_type& getSource() const
	{
		return m_source;
	}
	void setSource(const PAbsDirPath::value_type& value_)
	{
		m_source = value_;
	}

private:
	boost::optional<Driver2 > m_driver;
	PAbsDirPath::value_type m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem612

namespace Domain
{
namespace Xml
{
struct Filesystem612
{
	const boost::optional<Driver2 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver2 >& value_)
	{
		m_driver = value_;
	}
	const PAbsDirPath::value_type& getSource() const
	{
		return m_source;
	}
	void setSource(const PAbsDirPath::value_type& value_)
	{
		m_source = value_;
	}

private:
	boost::optional<Driver2 > m_driver;
	PAbsDirPath::value_type m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem613

namespace Domain
{
namespace Xml
{
struct Filesystem613
{
	const boost::optional<Driver2 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver2 >& value_)
	{
		m_driver = value_;
	}
	const PGenericName::value_type& getSource() const
	{
		return m_source;
	}
	void setSource(const PGenericName::value_type& value_)
	{
		m_source = value_;
	}

private:
	boost::optional<Driver2 > m_driver;
	PGenericName::value_type m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source5

namespace Domain
{
namespace Xml
{
struct Source5
{
	Source5();

	PUnsignedLong::value_type getUsage() const
	{
		return m_usage;
	}
	void setUsage(PUnsignedLong::value_type value_)
	{
		m_usage = value_;
	}
	const boost::optional<PUnit::value_type >& getUnits() const
	{
		return m_units;
	}
	void setUnits(const boost::optional<PUnit::value_type >& value_)
	{
		m_units = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedLong::value_type m_usage;
	boost::optional<PUnit::value_type > m_units;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem617

namespace Domain
{
namespace Xml
{
struct Filesystem617
{
	const boost::optional<Driver2 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver2 >& value_)
	{
		m_driver = value_;
	}
	const Source5& getSource() const
	{
		return m_source;
	}
	void setSource(const Source5& value_)
	{
		m_source = value_;
	}

private:
	boost::optional<Driver2 > m_driver;
	Source5 m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VFilesystem

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Filesystem607, Filesystem608, Filesystem610, Filesystem612, Filesystem613, Filesystem617 > > VFilesystemImpl;
typedef VFilesystemImpl::value_type VFilesystem;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem

namespace Domain
{
namespace Xml
{
struct Filesystem
{
	Filesystem();

	const VFilesystem& getFilesystem() const
	{
		return m_filesystem;
	}
	void setFilesystem(const VFilesystem& value_)
	{
		m_filesystem = value_;
	}
	const PAbsDirPath::value_type& getTarget() const
	{
		return m_target;
	}
	void setTarget(const PAbsDirPath::value_type& value_)
	{
		m_target = value_;
	}
	const boost::optional<EAccessmode >& getAccessmode() const
	{
		return m_accessmode;
	}
	void setAccessmode(const boost::optional<EAccessmode >& value_)
	{
		m_accessmode = value_;
	}
	bool getReadonly() const
	{
		return m_readonly;
	}
	void setReadonly(bool value_)
	{
		m_readonly = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<ScaledInteger >& getSpaceHardLimit() const
	{
		return m_spaceHardLimit;
	}
	void setSpaceHardLimit(const boost::optional<ScaledInteger >& value_)
	{
		m_spaceHardLimit = value_;
	}
	const boost::optional<ScaledInteger >& getSpaceSoftLimit() const
	{
		return m_spaceSoftLimit;
	}
	void setSpaceSoftLimit(const boost::optional<ScaledInteger >& value_)
	{
		m_spaceSoftLimit = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VFilesystem m_filesystem;
	PAbsDirPath::value_type m_target;
	boost::optional<EAccessmode > m_accessmode;
	bool m_readonly;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<ScaledInteger > m_spaceHardLimit;
	boost::optional<ScaledInteger > m_spaceSoftLimit;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source6

namespace Domain
{
namespace Xml
{
struct Source6
{
	const boost::optional<PDeviceName::value_type >& getBridge() const
	{
		return m_bridge;
	}
	void setBridge(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_bridge = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PDeviceName::value_type > m_bridge;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Parameters

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Parameters1

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Parameters2

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Parameters3

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Virtualport

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VVirtualPortProfile

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Ordered<mpl::vector<Attribute<mpl::int_<174>, Name::Strict<100> >, Optional<Element<Domain::Xml::Parameters, Name::Strict<175> > > > >, Name::Strict<173> >, Element<Ordered<mpl::vector<Attribute<mpl::int_<181>, Name::Strict<100> >, Optional<Element<Domain::Xml::Parameters1, Name::Strict<175> > > > >, Name::Strict<173> >, Element<Ordered<mpl::vector<Attribute<mpl::int_<184>, Name::Strict<100> >, Optional<Element<Domain::Xml::Parameters2, Name::Strict<175> > > > >, Name::Strict<173> >, Element<Domain::Xml::Virtualport, Name::Strict<173> > > > VVirtualPortProfileImpl;
typedef VVirtualPortProfileImpl::value_type VVirtualPortProfile;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Guest

namespace Domain
{
namespace Xml
{
struct Guest
{
	const boost::optional<PDeviceName::value_type >& getDev() const
	{
		return m_dev;
	}
	void setDev(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_dev = value_;
	}
	const boost::optional<PDeviceName::value_type >& getActual() const
	{
		return m_actual;
	}
	void setActual(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_actual = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PDeviceName::value_type > m_dev;
	boost::optional<PDeviceName::value_type > m_actual;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Ip

namespace Domain
{
namespace Xml
{
struct Ip
{
	const VIpAddr& getAddress() const
	{
		return m_address;
	}
	void setAddress(const VIpAddr& value_)
	{
		m_address = value_;
	}
	const boost::optional<PAddrFamily::value_type >& getFamily() const
	{
		return m_family;
	}
	void setFamily(const boost::optional<PAddrFamily::value_type >& value_)
	{
		m_family = value_;
	}
	const boost::optional<VIpPrefix >& getPrefix() const
	{
		return m_prefix;
	}
	void setPrefix(const boost::optional<VIpPrefix >& value_)
	{
		m_prefix = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VIpAddr m_address;
	boost::optional<PAddrFamily::value_type > m_family;
	boost::optional<VIpPrefix > m_prefix;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Backend

namespace Domain
{
namespace Xml
{
struct Backend
{
	const boost::optional<PAbsFilePath::value_type >& getTap() const
	{
		return m_tap;
	}
	void setTap(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_tap = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getVhost() const
	{
		return m_vhost;
	}
	void setVhost(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_vhost = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PAbsFilePath::value_type > m_tap;
	boost::optional<PAbsFilePath::value_type > m_vhost;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Driver672

namespace Domain
{
namespace Xml
{
struct Driver672
{
	const boost::optional<EName4 >& getName() const
	{
		return m_name;
	}
	void setName(const boost::optional<EName4 >& value_)
	{
		m_name = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getQueues() const
	{
		return m_queues;
	}
	void setQueues(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_queues = value_;
	}
	const boost::optional<ETxmode >& getTxmode() const
	{
		return m_txmode;
	}
	void setTxmode(const boost::optional<ETxmode >& value_)
	{
		m_txmode = value_;
	}
	const boost::optional<EVirOnOff >& getIoeventfd() const
	{
		return m_ioeventfd;
	}
	void setIoeventfd(const boost::optional<EVirOnOff >& value_)
	{
		m_ioeventfd = value_;
	}
	const boost::optional<EVirOnOff >& getEventIdx() const
	{
		return m_eventIdx;
	}
	void setEventIdx(const boost::optional<EVirOnOff >& value_)
	{
		m_eventIdx = value_;
	}

private:
	boost::optional<EName4 > m_name;
	boost::optional<PPositiveInteger::value_type > m_queues;
	boost::optional<ETxmode > m_txmode;
	boost::optional<EVirOnOff > m_ioeventfd;
	boost::optional<EVirOnOff > m_eventIdx;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VDriver

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Domain::Xml::EName3, Name::Strict<102> >, Driver672 > > VDriverImpl;
typedef VDriverImpl::value_type VDriver;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Host

namespace Domain
{
namespace Xml
{
struct Host
{
	const boost::optional<EVirOnOff >& getCsum() const
	{
		return m_csum;
	}
	void setCsum(const boost::optional<EVirOnOff >& value_)
	{
		m_csum = value_;
	}
	const boost::optional<EVirOnOff >& getGso() const
	{
		return m_gso;
	}
	void setGso(const boost::optional<EVirOnOff >& value_)
	{
		m_gso = value_;
	}
	const boost::optional<EVirOnOff >& getTso4() const
	{
		return m_tso4;
	}
	void setTso4(const boost::optional<EVirOnOff >& value_)
	{
		m_tso4 = value_;
	}
	const boost::optional<EVirOnOff >& getTso6() const
	{
		return m_tso6;
	}
	void setTso6(const boost::optional<EVirOnOff >& value_)
	{
		m_tso6 = value_;
	}
	const boost::optional<EVirOnOff >& getEcn() const
	{
		return m_ecn;
	}
	void setEcn(const boost::optional<EVirOnOff >& value_)
	{
		m_ecn = value_;
	}
	const boost::optional<EVirOnOff >& getUfo() const
	{
		return m_ufo;
	}
	void setUfo(const boost::optional<EVirOnOff >& value_)
	{
		m_ufo = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirOnOff > m_csum;
	boost::optional<EVirOnOff > m_gso;
	boost::optional<EVirOnOff > m_tso4;
	boost::optional<EVirOnOff > m_tso6;
	boost::optional<EVirOnOff > m_ecn;
	boost::optional<EVirOnOff > m_ufo;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Guest1

namespace Domain
{
namespace Xml
{
struct Guest1
{
	const boost::optional<EVirOnOff >& getCsum() const
	{
		return m_csum;
	}
	void setCsum(const boost::optional<EVirOnOff >& value_)
	{
		m_csum = value_;
	}
	const boost::optional<EVirOnOff >& getTso4() const
	{
		return m_tso4;
	}
	void setTso4(const boost::optional<EVirOnOff >& value_)
	{
		m_tso4 = value_;
	}
	const boost::optional<EVirOnOff >& getTso6() const
	{
		return m_tso6;
	}
	void setTso6(const boost::optional<EVirOnOff >& value_)
	{
		m_tso6 = value_;
	}
	const boost::optional<EVirOnOff >& getEcn() const
	{
		return m_ecn;
	}
	void setEcn(const boost::optional<EVirOnOff >& value_)
	{
		m_ecn = value_;
	}
	const boost::optional<EVirOnOff >& getUfo() const
	{
		return m_ufo;
	}
	void setUfo(const boost::optional<EVirOnOff >& value_)
	{
		m_ufo = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirOnOff > m_csum;
	boost::optional<EVirOnOff > m_tso4;
	boost::optional<EVirOnOff > m_tso6;
	boost::optional<EVirOnOff > m_ecn;
	boost::optional<EVirOnOff > m_ufo;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Driver3

namespace Domain
{
namespace Xml
{
struct Driver3
{
	const VDriver& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const VDriver& value_)
	{
		m_driver = value_;
	}
	const boost::optional<Host >& getHost() const
	{
		return m_host;
	}
	void setHost(const boost::optional<Host >& value_)
	{
		m_host = value_;
	}
	const boost::optional<Guest1 >& getGuest() const
	{
		return m_guest;
	}
	void setGuest(const boost::optional<Guest1 >& value_)
	{
		m_guest = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VDriver m_driver;
	boost::optional<Host > m_host;
	boost::optional<Guest1 > m_guest;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Parameter

namespace Domain
{
namespace Xml
{
struct Parameter
{
	const PFilterParamName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PFilterParamName::value_type& value_)
	{
		m_name = value_;
	}
	const PFilterParamValue::value_type& getValue() const
	{
		return m_value;
	}
	void setValue(const PFilterParamValue::value_type& value_)
	{
		m_value = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PFilterParamName::value_type m_name;
	PFilterParamValue::value_type m_value;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct FilterrefNodeAttributes

namespace Domain
{
namespace Xml
{
struct FilterrefNodeAttributes
{
	const PFilter::value_type& getFilter() const
	{
		return m_filter;
	}
	void setFilter(const PFilter::value_type& value_)
	{
		m_filter = value_;
	}
	const QList<Parameter >& getParameterList() const
	{
		return m_parameterList;
	}
	void setParameterList(const QList<Parameter >& value_)
	{
		m_parameterList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PFilter::value_type m_filter;
	QList<Parameter > m_parameterList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Tune

namespace Domain
{
namespace Xml
{
struct Tune
{
	const boost::optional<PUnsignedInt::value_type >& getSndbuf() const
	{
		return m_sndbuf;
	}
	void setSndbuf(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_sndbuf = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PUnsignedInt::value_type > m_sndbuf;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Rom

namespace Domain
{
namespace Xml
{
struct Rom
{
	const boost::optional<EVirOnOff >& getBar() const
	{
		return m_bar;
	}
	void setBar(const boost::optional<EVirOnOff >& value_)
	{
		m_bar = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getFile() const
	{
		return m_file;
	}
	void setFile(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_file = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirOnOff > m_bar;
	boost::optional<PAbsFilePath::value_type > m_file;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct BandwidthAttributes

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Bandwidth

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Tag

namespace Domain
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
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Interface631

namespace Domain
{
namespace Xml
{
struct Interface631
{
	const boost::optional<Source6 >& getSource() const
	{
		return m_source;
	}
	void setSource(const boost::optional<Source6 >& value_)
	{
		m_source = value_;
	}
	const boost::optional<VVirtualPortProfile >& getVirtualPortProfile() const
	{
		return m_virtualPortProfile;
	}
	void setVirtualPortProfile(const boost::optional<VVirtualPortProfile >& value_)
	{
		m_virtualPortProfile = value_;
	}
	const boost::optional<EState >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<EState >& value_)
	{
		m_link = value_;
	}
	const boost::optional<PDeviceName::value_type >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_target = value_;
	}
	const boost::optional<Guest >& getGuest() const
	{
		return m_guest;
	}
	void setGuest(const boost::optional<Guest >& value_)
	{
		m_guest = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const QList<Ip >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip >& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<PFilePath::value_type >& getScript() const
	{
		return m_script;
	}
	void setScript(const boost::optional<PFilePath::value_type >& value_)
	{
		m_script = value_;
	}
	const boost::optional<PType::value_type >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<PType::value_type >& value_)
	{
		m_model = value_;
	}
	const boost::optional<Backend >& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const boost::optional<Backend >& value_)
	{
		m_backend = value_;
	}
	const boost::optional<Driver3 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver3 >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<FilterrefNodeAttributes >& getFilterref() const
	{
		return m_filterref;
	}
	void setFilterref(const boost::optional<FilterrefNodeAttributes >& value_)
	{
		m_filterref = value_;
	}
	const boost::optional<Tune >& getTune() const
	{
		return m_tune;
	}
	void setTune(const boost::optional<Tune >& value_)
	{
		m_tune = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	const boost::optional<Rom >& getRom() const
	{
		return m_rom;
	}
	void setRom(const boost::optional<Rom >& value_)
	{
		m_rom = value_;
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

private:
	boost::optional<Source6 > m_source;
	boost::optional<VVirtualPortProfile > m_virtualPortProfile;
	boost::optional<EState > m_link;
	boost::optional<PDeviceName::value_type > m_target;
	boost::optional<Guest > m_guest;
	boost::optional<PUniMacAddr::value_type > m_mac;
	QList<Ip > m_ipList;
	boost::optional<PFilePath::value_type > m_script;
	boost::optional<PType::value_type > m_model;
	boost::optional<Backend > m_backend;
	boost::optional<Driver3 > m_driver;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<FilterrefNodeAttributes > m_filterref;
	boost::optional<Tune > m_tune;
	boost::optional<PPositiveInteger::value_type > m_boot;
	boost::optional<Rom > m_rom;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Interface633

namespace Domain
{
namespace Xml
{
struct Interface633
{
	const boost::optional<PDeviceName::value_type >& getSource() const
	{
		return m_source;
	}
	void setSource(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_source = value_;
	}
	const boost::optional<EState >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<EState >& value_)
	{
		m_link = value_;
	}
	const boost::optional<PDeviceName::value_type >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_target = value_;
	}
	const boost::optional<Guest >& getGuest() const
	{
		return m_guest;
	}
	void setGuest(const boost::optional<Guest >& value_)
	{
		m_guest = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const QList<Ip >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip >& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<PFilePath::value_type >& getScript() const
	{
		return m_script;
	}
	void setScript(const boost::optional<PFilePath::value_type >& value_)
	{
		m_script = value_;
	}
	const boost::optional<PType::value_type >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<PType::value_type >& value_)
	{
		m_model = value_;
	}
	const boost::optional<Backend >& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const boost::optional<Backend >& value_)
	{
		m_backend = value_;
	}
	const boost::optional<Driver3 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver3 >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<FilterrefNodeAttributes >& getFilterref() const
	{
		return m_filterref;
	}
	void setFilterref(const boost::optional<FilterrefNodeAttributes >& value_)
	{
		m_filterref = value_;
	}
	const boost::optional<Tune >& getTune() const
	{
		return m_tune;
	}
	void setTune(const boost::optional<Tune >& value_)
	{
		m_tune = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	const boost::optional<Rom >& getRom() const
	{
		return m_rom;
	}
	void setRom(const boost::optional<Rom >& value_)
	{
		m_rom = value_;
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

private:
	boost::optional<PDeviceName::value_type > m_source;
	boost::optional<EState > m_link;
	boost::optional<PDeviceName::value_type > m_target;
	boost::optional<Guest > m_guest;
	boost::optional<PUniMacAddr::value_type > m_mac;
	QList<Ip > m_ipList;
	boost::optional<PFilePath::value_type > m_script;
	boost::optional<PType::value_type > m_model;
	boost::optional<Backend > m_backend;
	boost::optional<Driver3 > m_driver;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<FilterrefNodeAttributes > m_filterref;
	boost::optional<Tune > m_tune;
	boost::optional<PPositiveInteger::value_type > m_boot;
	boost::optional<Rom > m_rom;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source7

namespace Domain
{
namespace Xml
{
struct Source7
{
	Source7();

	EType8 getType() const
	{
		return m_type;
	}
	void setType(EType8 value_)
	{
		m_type = value_;
	}
	const PAbsFilePath::value_type& getPath() const
	{
		return m_path;
	}
	void setPath(const PAbsFilePath::value_type& value_)
	{
		m_path = value_;
	}
	EMode6 getMode() const
	{
		return m_mode;
	}
	void setMode(EMode6 value_)
	{
		m_mode = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EType8 m_type;
	PAbsFilePath::value_type m_path;
	EMode6 m_mode;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Interface637

namespace Domain
{
namespace Xml
{
struct Interface637
{
	const Source7& getSource() const
	{
		return m_source;
	}
	void setSource(const Source7& value_)
	{
		m_source = value_;
	}
	const boost::optional<EState >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<EState >& value_)
	{
		m_link = value_;
	}
	const boost::optional<PDeviceName::value_type >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_target = value_;
	}
	const boost::optional<Guest >& getGuest() const
	{
		return m_guest;
	}
	void setGuest(const boost::optional<Guest >& value_)
	{
		m_guest = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const QList<Ip >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip >& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<PFilePath::value_type >& getScript() const
	{
		return m_script;
	}
	void setScript(const boost::optional<PFilePath::value_type >& value_)
	{
		m_script = value_;
	}
	const boost::optional<PType::value_type >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<PType::value_type >& value_)
	{
		m_model = value_;
	}
	const boost::optional<Backend >& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const boost::optional<Backend >& value_)
	{
		m_backend = value_;
	}
	const boost::optional<Driver3 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver3 >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<FilterrefNodeAttributes >& getFilterref() const
	{
		return m_filterref;
	}
	void setFilterref(const boost::optional<FilterrefNodeAttributes >& value_)
	{
		m_filterref = value_;
	}
	const boost::optional<Tune >& getTune() const
	{
		return m_tune;
	}
	void setTune(const boost::optional<Tune >& value_)
	{
		m_tune = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	const boost::optional<Rom >& getRom() const
	{
		return m_rom;
	}
	void setRom(const boost::optional<Rom >& value_)
	{
		m_rom = value_;
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

private:
	Source7 m_source;
	boost::optional<EState > m_link;
	boost::optional<PDeviceName::value_type > m_target;
	boost::optional<Guest > m_guest;
	boost::optional<PUniMacAddr::value_type > m_mac;
	QList<Ip > m_ipList;
	boost::optional<PFilePath::value_type > m_script;
	boost::optional<PType::value_type > m_model;
	boost::optional<Backend > m_backend;
	boost::optional<Driver3 > m_driver;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<FilterrefNodeAttributes > m_filterref;
	boost::optional<Tune > m_tune;
	boost::optional<PPositiveInteger::value_type > m_boot;
	boost::optional<Rom > m_rom;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source8

namespace Domain
{
namespace Xml
{
struct Source8
{
	const PDeviceName::value_type& getNetwork() const
	{
		return m_network;
	}
	void setNetwork(const PDeviceName::value_type& value_)
	{
		m_network = value_;
	}
	const boost::optional<PDeviceName::value_type >& getPortgroup() const
	{
		return m_portgroup;
	}
	void setPortgroup(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_portgroup = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PDeviceName::value_type m_network;
	boost::optional<PDeviceName::value_type > m_portgroup;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Interface639

namespace Domain
{
namespace Xml
{
struct Interface639
{
	const Source8& getSource() const
	{
		return m_source;
	}
	void setSource(const Source8& value_)
	{
		m_source = value_;
	}
	const boost::optional<VVirtualPortProfile >& getVirtualPortProfile() const
	{
		return m_virtualPortProfile;
	}
	void setVirtualPortProfile(const boost::optional<VVirtualPortProfile >& value_)
	{
		m_virtualPortProfile = value_;
	}
	const boost::optional<EState >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<EState >& value_)
	{
		m_link = value_;
	}
	const boost::optional<PDeviceName::value_type >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_target = value_;
	}
	const boost::optional<Guest >& getGuest() const
	{
		return m_guest;
	}
	void setGuest(const boost::optional<Guest >& value_)
	{
		m_guest = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const QList<Ip >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip >& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<PFilePath::value_type >& getScript() const
	{
		return m_script;
	}
	void setScript(const boost::optional<PFilePath::value_type >& value_)
	{
		m_script = value_;
	}
	const boost::optional<PType::value_type >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<PType::value_type >& value_)
	{
		m_model = value_;
	}
	const boost::optional<Backend >& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const boost::optional<Backend >& value_)
	{
		m_backend = value_;
	}
	const boost::optional<Driver3 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver3 >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<FilterrefNodeAttributes >& getFilterref() const
	{
		return m_filterref;
	}
	void setFilterref(const boost::optional<FilterrefNodeAttributes >& value_)
	{
		m_filterref = value_;
	}
	const boost::optional<Tune >& getTune() const
	{
		return m_tune;
	}
	void setTune(const boost::optional<Tune >& value_)
	{
		m_tune = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	const boost::optional<Rom >& getRom() const
	{
		return m_rom;
	}
	void setRom(const boost::optional<Rom >& value_)
	{
		m_rom = value_;
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

private:
	Source8 m_source;
	boost::optional<VVirtualPortProfile > m_virtualPortProfile;
	boost::optional<EState > m_link;
	boost::optional<PDeviceName::value_type > m_target;
	boost::optional<Guest > m_guest;
	boost::optional<PUniMacAddr::value_type > m_mac;
	QList<Ip > m_ipList;
	boost::optional<PFilePath::value_type > m_script;
	boost::optional<PType::value_type > m_model;
	boost::optional<Backend > m_backend;
	boost::optional<Driver3 > m_driver;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<FilterrefNodeAttributes > m_filterref;
	boost::optional<Tune > m_tune;
	boost::optional<PPositiveInteger::value_type > m_boot;
	boost::optional<Rom > m_rom;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source9

namespace Domain
{
namespace Xml
{
struct Source9
{
	const PDeviceName::value_type& getDev() const
	{
		return m_dev;
	}
	void setDev(const PDeviceName::value_type& value_)
	{
		m_dev = value_;
	}
	const boost::optional<PBridgeMode::value_type >& getMode() const
	{
		return m_mode;
	}
	void setMode(const boost::optional<PBridgeMode::value_type >& value_)
	{
		m_mode = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PDeviceName::value_type m_dev;
	boost::optional<PBridgeMode::value_type > m_mode;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Interface641

namespace Domain
{
namespace Xml
{
struct Interface641
{
	const Source9& getSource() const
	{
		return m_source;
	}
	void setSource(const Source9& value_)
	{
		m_source = value_;
	}
	const boost::optional<VVirtualPortProfile >& getVirtualPortProfile() const
	{
		return m_virtualPortProfile;
	}
	void setVirtualPortProfile(const boost::optional<VVirtualPortProfile >& value_)
	{
		m_virtualPortProfile = value_;
	}
	const boost::optional<EState >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<EState >& value_)
	{
		m_link = value_;
	}
	const boost::optional<PDeviceName::value_type >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_target = value_;
	}
	const boost::optional<Guest >& getGuest() const
	{
		return m_guest;
	}
	void setGuest(const boost::optional<Guest >& value_)
	{
		m_guest = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const QList<Ip >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip >& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<PFilePath::value_type >& getScript() const
	{
		return m_script;
	}
	void setScript(const boost::optional<PFilePath::value_type >& value_)
	{
		m_script = value_;
	}
	const boost::optional<PType::value_type >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<PType::value_type >& value_)
	{
		m_model = value_;
	}
	const boost::optional<Backend >& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const boost::optional<Backend >& value_)
	{
		m_backend = value_;
	}
	const boost::optional<Driver3 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver3 >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<FilterrefNodeAttributes >& getFilterref() const
	{
		return m_filterref;
	}
	void setFilterref(const boost::optional<FilterrefNodeAttributes >& value_)
	{
		m_filterref = value_;
	}
	const boost::optional<Tune >& getTune() const
	{
		return m_tune;
	}
	void setTune(const boost::optional<Tune >& value_)
	{
		m_tune = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	const boost::optional<Rom >& getRom() const
	{
		return m_rom;
	}
	void setRom(const boost::optional<Rom >& value_)
	{
		m_rom = value_;
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

private:
	Source9 m_source;
	boost::optional<VVirtualPortProfile > m_virtualPortProfile;
	boost::optional<EState > m_link;
	boost::optional<PDeviceName::value_type > m_target;
	boost::optional<Guest > m_guest;
	boost::optional<PUniMacAddr::value_type > m_mac;
	QList<Ip > m_ipList;
	boost::optional<PFilePath::value_type > m_script;
	boost::optional<PType::value_type > m_model;
	boost::optional<Backend > m_backend;
	boost::optional<Driver3 > m_driver;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<FilterrefNodeAttributes > m_filterref;
	boost::optional<Tune > m_tune;
	boost::optional<PPositiveInteger::value_type > m_boot;
	boost::optional<Rom > m_rom;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct InterfaceOptions

namespace Domain
{
namespace Xml
{
struct InterfaceOptions
{
	const boost::optional<EState >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<EState >& value_)
	{
		m_link = value_;
	}
	const boost::optional<PDeviceName::value_type >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_target = value_;
	}
	const boost::optional<Guest >& getGuest() const
	{
		return m_guest;
	}
	void setGuest(const boost::optional<Guest >& value_)
	{
		m_guest = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const QList<Ip >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip >& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<PFilePath::value_type >& getScript() const
	{
		return m_script;
	}
	void setScript(const boost::optional<PFilePath::value_type >& value_)
	{
		m_script = value_;
	}
	const boost::optional<PType::value_type >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<PType::value_type >& value_)
	{
		m_model = value_;
	}
	const boost::optional<Backend >& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const boost::optional<Backend >& value_)
	{
		m_backend = value_;
	}
	const boost::optional<Driver3 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver3 >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<FilterrefNodeAttributes >& getFilterref() const
	{
		return m_filterref;
	}
	void setFilterref(const boost::optional<FilterrefNodeAttributes >& value_)
	{
		m_filterref = value_;
	}
	const boost::optional<Tune >& getTune() const
	{
		return m_tune;
	}
	void setTune(const boost::optional<Tune >& value_)
	{
		m_tune = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	const boost::optional<Rom >& getRom() const
	{
		return m_rom;
	}
	void setRom(const boost::optional<Rom >& value_)
	{
		m_rom = value_;
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

private:
	boost::optional<EState > m_link;
	boost::optional<PDeviceName::value_type > m_target;
	boost::optional<Guest > m_guest;
	boost::optional<PUniMacAddr::value_type > m_mac;
	QList<Ip > m_ipList;
	boost::optional<PFilePath::value_type > m_script;
	boost::optional<PType::value_type > m_model;
	boost::optional<Backend > m_backend;
	boost::optional<Driver3 > m_driver;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<FilterrefNodeAttributes > m_filterref;
	boost::optional<Tune > m_tune;
	boost::optional<PPositiveInteger::value_type > m_boot;
	boost::optional<Rom > m_rom;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Interface644

namespace Domain
{
namespace Xml
{
struct Interface644
{
	const PDeviceName::value_type& getSource() const
	{
		return m_source;
	}
	void setSource(const PDeviceName::value_type& value_)
	{
		m_source = value_;
	}
	const boost::optional<EState >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<EState >& value_)
	{
		m_link = value_;
	}
	const boost::optional<PDeviceName::value_type >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_target = value_;
	}
	const boost::optional<Guest >& getGuest() const
	{
		return m_guest;
	}
	void setGuest(const boost::optional<Guest >& value_)
	{
		m_guest = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const QList<Ip >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip >& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<PFilePath::value_type >& getScript() const
	{
		return m_script;
	}
	void setScript(const boost::optional<PFilePath::value_type >& value_)
	{
		m_script = value_;
	}
	const boost::optional<PType::value_type >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<PType::value_type >& value_)
	{
		m_model = value_;
	}
	const boost::optional<Backend >& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const boost::optional<Backend >& value_)
	{
		m_backend = value_;
	}
	const boost::optional<Driver3 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver3 >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<FilterrefNodeAttributes >& getFilterref() const
	{
		return m_filterref;
	}
	void setFilterref(const boost::optional<FilterrefNodeAttributes >& value_)
	{
		m_filterref = value_;
	}
	const boost::optional<Tune >& getTune() const
	{
		return m_tune;
	}
	void setTune(const boost::optional<Tune >& value_)
	{
		m_tune = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	const boost::optional<Rom >& getRom() const
	{
		return m_rom;
	}
	void setRom(const boost::optional<Rom >& value_)
	{
		m_rom = value_;
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

private:
	PDeviceName::value_type m_source;
	boost::optional<EState > m_link;
	boost::optional<PDeviceName::value_type > m_target;
	boost::optional<Guest > m_guest;
	boost::optional<PUniMacAddr::value_type > m_mac;
	QList<Ip > m_ipList;
	boost::optional<PFilePath::value_type > m_script;
	boost::optional<PType::value_type > m_model;
	boost::optional<Backend > m_backend;
	boost::optional<Driver3 > m_driver;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<FilterrefNodeAttributes > m_filterref;
	boost::optional<Tune > m_tune;
	boost::optional<PPositiveInteger::value_type > m_boot;
	boost::optional<Rom > m_rom;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source10

namespace Domain
{
namespace Xml
{
struct Source10
{
	Source10();

	const PIpv4Addr::value_type& getAddress() const
	{
		return m_address;
	}
	void setAddress(const PIpv4Addr::value_type& value_)
	{
		m_address = value_;
	}
	PPortNumber::value_type getPort() const
	{
		return m_port;
	}
	void setPort(PPortNumber::value_type value_)
	{
		m_port = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PIpv4Addr::value_type m_address;
	PPortNumber::value_type m_port;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Interface647

namespace Domain
{
namespace Xml
{
struct Interface647
{
	Interface647();

	EType9 getType() const
	{
		return m_type;
	}
	void setType(EType9 value_)
	{
		m_type = value_;
	}
	const Source10& getSource() const
	{
		return m_source;
	}
	void setSource(const Source10& value_)
	{
		m_source = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}

private:
	EType9 m_type;
	Source10 m_source;
	boost::optional<PUniMacAddr::value_type > m_mac;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source11

namespace Domain
{
namespace Xml
{
struct Source11
{
	Source11();

	const boost::optional<PIpv4Addr::value_type >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<PIpv4Addr::value_type >& value_)
	{
		m_address = value_;
	}
	PPortNumber::value_type getPort() const
	{
		return m_port;
	}
	void setPort(PPortNumber::value_type value_)
	{
		m_port = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PIpv4Addr::value_type > m_address;
	PPortNumber::value_type m_port;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Interface648

namespace Domain
{
namespace Xml
{
struct Interface648
{
	const Source11& getSource() const
	{
		return m_source;
	}
	void setSource(const Source11& value_)
	{
		m_source = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}

private:
	Source11 m_source;
	boost::optional<PUniMacAddr::value_type > m_mac;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Usbproduct

namespace Domain
{
namespace Xml
{
struct Usbproduct
{
	const PUsbId::value_type& getVendor() const
	{
		return m_vendor;
	}
	void setVendor(const PUsbId::value_type& value_)
	{
		m_vendor = value_;
	}
	const PUsbId::value_type& getProduct() const
	{
		return m_product;
	}
	void setProduct(const PUsbId::value_type& value_)
	{
		m_product = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PUsbId::value_type m_vendor;
	PUsbId::value_type m_product;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Address

namespace Domain
{
namespace Xml
{
struct Address
{
	const PUsbAddr::value_type& getBus() const
	{
		return m_bus;
	}
	void setBus(const PUsbAddr::value_type& value_)
	{
		m_bus = value_;
	}
	const PUsbPort::value_type& getDevice() const
	{
		return m_device;
	}
	void setDevice(const PUsbPort::value_type& value_)
	{
		m_device = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUsbAddr::value_type m_bus;
	PUsbPort::value_type m_device;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source654

namespace Domain
{
namespace Xml
{
struct Source654
{
	const Usbproduct& getUsbproduct() const
	{
		return m_usbproduct;
	}
	void setUsbproduct(const Usbproduct& value_)
	{
		m_usbproduct = value_;
	}
	const boost::optional<Address >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<Address >& value_)
	{
		m_address = value_;
	}

private:
	Usbproduct m_usbproduct;
	boost::optional<Address > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Address658

namespace Domain
{
namespace Xml
{
struct Address658
{
	const PUsbAddr::value_type& getBus() const
	{
		return m_bus;
	}
	void setBus(const PUsbAddr::value_type& value_)
	{
		m_bus = value_;
	}
	const PUsbPort::value_type& getDevice() const
	{
		return m_device;
	}
	void setDevice(const PUsbPort::value_type& value_)
	{
		m_device = value_;
	}

private:
	PUsbAddr::value_type m_bus;
	PUsbPort::value_type m_device;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VAddress1

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Ordered<mpl::vector<Attribute<mpl::int_<588>, Name::Strict<100> >, Fragment<Pciaddress > > >, Address658 > > VAddress1Impl;
typedef VAddress1Impl::value_type VAddress1;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VSource

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Source654, Element<Domain::Xml::VAddress1Impl, Name::Strict<106> > > > VSourceImpl;
typedef VSourceImpl::value_type VSource;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source12

namespace Domain
{
namespace Xml
{
struct Source12
{
	const boost::optional<EVirYesNo >& getMissing() const
	{
		return m_missing;
	}
	void setMissing(const boost::optional<EVirYesNo >& value_)
	{
		m_missing = value_;
	}
	const VSource& getSource() const
	{
		return m_source;
	}
	void setSource(const VSource& value_)
	{
		m_source = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_missing;
	VSource m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Interface660

namespace Domain
{
namespace Xml
{
struct Interface660
{
	const boost::optional<EVirYesNo >& getManaged() const
	{
		return m_managed;
	}
	void setManaged(const boost::optional<EVirYesNo >& value_)
	{
		m_managed = value_;
	}
	const Source12& getSource() const
	{
		return m_source;
	}
	void setSource(const Source12& value_)
	{
		m_source = value_;
	}
	const boost::optional<VVirtualPortProfile >& getVirtualPortProfile() const
	{
		return m_virtualPortProfile;
	}
	void setVirtualPortProfile(const boost::optional<VVirtualPortProfile >& value_)
	{
		m_virtualPortProfile = value_;
	}
	const boost::optional<EState >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<EState >& value_)
	{
		m_link = value_;
	}
	const boost::optional<PDeviceName::value_type >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_target = value_;
	}
	const boost::optional<Guest >& getGuest() const
	{
		return m_guest;
	}
	void setGuest(const boost::optional<Guest >& value_)
	{
		m_guest = value_;
	}
	const boost::optional<PUniMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PUniMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const QList<Ip >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip >& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<PFilePath::value_type >& getScript() const
	{
		return m_script;
	}
	void setScript(const boost::optional<PFilePath::value_type >& value_)
	{
		m_script = value_;
	}
	const boost::optional<PType::value_type >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<PType::value_type >& value_)
	{
		m_model = value_;
	}
	const boost::optional<Backend >& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const boost::optional<Backend >& value_)
	{
		m_backend = value_;
	}
	const boost::optional<Driver3 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver3 >& value_)
	{
		m_driver = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<FilterrefNodeAttributes >& getFilterref() const
	{
		return m_filterref;
	}
	void setFilterref(const boost::optional<FilterrefNodeAttributes >& value_)
	{
		m_filterref = value_;
	}
	const boost::optional<Tune >& getTune() const
	{
		return m_tune;
	}
	void setTune(const boost::optional<Tune >& value_)
	{
		m_tune = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	const boost::optional<Rom >& getRom() const
	{
		return m_rom;
	}
	void setRom(const boost::optional<Rom >& value_)
	{
		m_rom = value_;
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

private:
	boost::optional<EVirYesNo > m_managed;
	Source12 m_source;
	boost::optional<VVirtualPortProfile > m_virtualPortProfile;
	boost::optional<EState > m_link;
	boost::optional<PDeviceName::value_type > m_target;
	boost::optional<Guest > m_guest;
	boost::optional<PUniMacAddr::value_type > m_mac;
	QList<Ip > m_ipList;
	boost::optional<PFilePath::value_type > m_script;
	boost::optional<PType::value_type > m_model;
	boost::optional<Backend > m_backend;
	boost::optional<Driver3 > m_driver;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<FilterrefNodeAttributes > m_filterref;
	boost::optional<Tune > m_tune;
	boost::optional<PPositiveInteger::value_type > m_boot;
	boost::optional<Rom > m_rom;
	boost::optional<Bandwidth > m_bandwidth;
	boost::optional<QList<Tag > > m_vlan;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VInterface

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Interface631, Interface633, Interface637, Interface639, Interface641, Ordered<mpl::vector<Attribute<mpl::int_<643>, Name::Strict<100> >, Fragment<InterfaceOptions > > >, Interface644, Interface647, Interface648, Interface660 > > VInterfaceImpl;
typedef VInterfaceImpl::value_type VInterface;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Input

namespace Domain
{
namespace Xml
{
struct Input
{
	Input();

	EType10 getType() const
	{
		return m_type;
	}
	void setType(EType10 value_)
	{
		m_type = value_;
	}
	const boost::optional<EBus1 >& getBus() const
	{
		return m_bus;
	}
	void setBus(const boost::optional<EBus1 >& value_)
	{
		m_bus = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EType10 m_type;
	boost::optional<EBus1 > m_bus;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Sound

namespace Domain
{
namespace Xml
{
struct Sound
{
	Sound();

	EModel4 getModel() const
	{
		return m_model;
	}
	void setModel(EModel4 value_)
	{
		m_model = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const QList<EType11 >& getCodecList() const
	{
		return m_codecList;
	}
	void setCodecList(const QList<EType11 >& value_)
	{
		m_codecList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EModel4 m_model;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	QList<EType11 > m_codecList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source13

namespace Domain
{
namespace Xml
{
struct Source13
{
	const boost::optional<EStartupPolicy >& getStartupPolicy() const
	{
		return m_startupPolicy;
	}
	void setStartupPolicy(const boost::optional<EStartupPolicy >& value_)
	{
		m_startupPolicy = value_;
	}
	const Pciaddress& getAddress() const
	{
		return m_address;
	}
	void setAddress(const Pciaddress& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EStartupPolicy > m_startupPolicy;
	Pciaddress m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hostdevsubsyspci

namespace Domain
{
namespace Xml
{
struct Hostdevsubsyspci
{
	const boost::optional<EName5 >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<EName5 >& value_)
	{
		m_driver = value_;
	}
	const Source13& getSource() const
	{
		return m_source;
	}
	void setSource(const Source13& value_)
	{
		m_source = value_;
	}

private:
	boost::optional<EName5 > m_driver;
	Source13 m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source899

namespace Domain
{
namespace Xml
{
struct Source899
{
	const Usbproduct& getUsbproduct() const
	{
		return m_usbproduct;
	}
	void setUsbproduct(const Usbproduct& value_)
	{
		m_usbproduct = value_;
	}
	const boost::optional<Address >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<Address >& value_)
	{
		m_address = value_;
	}

private:
	Usbproduct m_usbproduct;
	boost::optional<Address > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VSource1

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Source899, Element<Domain::Xml::Address, Name::Strict<106> > > > VSource1Impl;
typedef VSource1Impl::value_type VSource1;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source14

namespace Domain
{
namespace Xml
{
struct Source14
{
	const boost::optional<EStartupPolicy >& getStartupPolicy() const
	{
		return m_startupPolicy;
	}
	void setStartupPolicy(const boost::optional<EStartupPolicy >& value_)
	{
		m_startupPolicy = value_;
	}
	const VSource1& getSource() const
	{
		return m_source;
	}
	void setSource(const VSource1& value_)
	{
		m_source = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EStartupPolicy > m_startupPolicy;
	VSource1 m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Parentaddr

namespace Domain
{
namespace Xml
{
struct Parentaddr
{
	const boost::optional<PPositiveInteger::value_type >& getUniqueId() const
	{
		return m_uniqueId;
	}
	void setUniqueId(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_uniqueId = value_;
	}
	const Pciaddress& getAddress() const
	{
		return m_address;
	}
	void setAddress(const Pciaddress& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PPositiveInteger::value_type > m_uniqueId;
	Pciaddress m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice107

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<QString, Name::Strict<102> >, Element<Domain::Xml::Parentaddr, Name::Strict<104> > > > VChoice107Impl;
typedef VChoice107Impl::value_type VChoice107;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Adapter113

namespace Domain
{
namespace Xml
{
struct Adapter113
{
	const boost::optional<QString >& getParent() const
	{
		return m_parent;
	}
	void setParent(const boost::optional<QString >& value_)
	{
		m_parent = value_;
	}
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
	boost::optional<QString > m_parent;
	PWwn::value_type m_wwnn;
	PWwn::value_type m_wwpn;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VAdapter

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Ordered<mpl::vector<Optional<Attribute<mpl::int_<101>, Name::Strict<100> > >, Domain::Xml::VChoice107Impl > >, Adapter113 > > VAdapterImpl;
typedef VAdapterImpl::value_type VAdapter;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Scsiaddress

namespace Domain
{
namespace Xml
{
struct Scsiaddress
{
	const PDriveBus::value_type& getBus() const
	{
		return m_bus;
	}
	void setBus(const PDriveBus::value_type& value_)
	{
		m_bus = value_;
	}
	const PDriveTarget::value_type& getTarget() const
	{
		return m_target;
	}
	void setTarget(const PDriveTarget::value_type& value_)
	{
		m_target = value_;
	}
	const PDriveUnit::value_type& getUnit() const
	{
		return m_unit;
	}
	void setUnit(const PDriveUnit::value_type& value_)
	{
		m_unit = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PDriveBus::value_type m_bus;
	PDriveTarget::value_type m_target;
	PDriveUnit::value_type m_unit;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source902

namespace Domain
{
namespace Xml
{
struct Source902
{
	const boost::optional<EProtocol1 >& getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(const boost::optional<EProtocol1 >& value_)
	{
		m_protocol = value_;
	}
	const VAdapter& getAdapter() const
	{
		return m_adapter;
	}
	void setAdapter(const VAdapter& value_)
	{
		m_adapter = value_;
	}
	const Scsiaddress& getAddress() const
	{
		return m_address;
	}
	void setAddress(const Scsiaddress& value_)
	{
		m_address = value_;
	}

private:
	boost::optional<EProtocol1 > m_protocol;
	VAdapter m_adapter;
	Scsiaddress m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Host1

namespace Domain
{
namespace Xml
{
struct Host1
{
	const QString& getName() const
	{
		return m_name;
	}
	void setName(const QString& value_)
	{
		m_name = value_;
	}
	const boost::optional<PPortNumber::value_type >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<PPortNumber::value_type >& value_)
	{
		m_port = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_name;
	boost::optional<PPortNumber::value_type > m_port;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source903

namespace Domain
{
namespace Xml
{
struct Source903
{
	Source903();

	EProtocol2 getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(EProtocol2 value_)
	{
		m_protocol = value_;
	}
	const QString& getName() const
	{
		return m_name;
	}
	void setName(const QString& value_)
	{
		m_name = value_;
	}
	const QList<Host1 >& getHostList() const
	{
		return m_hostList;
	}
	void setHostList(const QList<Host1 >& value_)
	{
		m_hostList = value_;
	}
	const boost::optional<Auth >& getAuth() const
	{
		return m_auth;
	}
	void setAuth(const boost::optional<Auth >& value_)
	{
		m_auth = value_;
	}

private:
	EProtocol2 m_protocol;
	QString m_name;
	QList<Host1 > m_hostList;
	boost::optional<Auth > m_auth;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VSource2

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Source902, Source903 > > VSource2Impl;
typedef VSource2Impl::value_type VSource2;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hostdevsubsysscsi

namespace Domain
{
namespace Xml
{
struct Hostdevsubsysscsi
{
	const boost::optional<ESgio1 >& getSgio() const
	{
		return m_sgio;
	}
	void setSgio(const boost::optional<ESgio1 >& value_)
	{
		m_sgio = value_;
	}
	const boost::optional<EVirYesNo >& getRawio() const
	{
		return m_rawio;
	}
	void setRawio(const boost::optional<EVirYesNo >& value_)
	{
		m_rawio = value_;
	}
	const VSource2& getSource() const
	{
		return m_source;
	}
	void setSource(const VSource2& value_)
	{
		m_source = value_;
	}

private:
	boost::optional<ESgio1 > m_sgio;
	boost::optional<EVirYesNo > m_rawio;
	VSource2 m_source;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VHostdevsubsys

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Hostdevsubsyspci, Ordered<mpl::vector<Attribute<mpl::int_<523>, Name::Strict<100> >, Element<Domain::Xml::Source14, Name::Strict<493> > > >, Hostdevsubsysscsi > > VHostdevsubsysImpl;
typedef VHostdevsubsysImpl::value_type VHostdevsubsys;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hostdevsubsys

namespace Domain
{
namespace Xml
{
struct Hostdevsubsys
{
	const boost::optional<EVirYesNo >& getManaged() const
	{
		return m_managed;
	}
	void setManaged(const boost::optional<EVirYesNo >& value_)
	{
		m_managed = value_;
	}
	const VHostdevsubsys& getHostdevsubsys() const
	{
		return m_hostdevsubsys;
	}
	void setHostdevsubsys(const VHostdevsubsys& value_)
	{
		m_hostdevsubsys = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<EVirYesNo > m_managed;
	VHostdevsubsys m_hostdevsubsys;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VHostdevcaps

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Ordered<mpl::vector<Ordered<mpl::vector<Attribute<mpl::int_<905>, Name::Strict<100> >, Element<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<494> >, Name::Strict<493> > > > > >, Ordered<mpl::vector<Ordered<mpl::vector<Attribute<mpl::int_<906>, Name::Strict<100> >, Element<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<907> >, Name::Strict<493> > > > > >, Ordered<mpl::vector<Ordered<mpl::vector<Attribute<mpl::int_<908>, Name::Strict<100> >, Element<Element<Text<Domain::Xml::PDeviceName >, Name::Strict<629> >, Name::Strict<493> > > > > > > > VHostdevcapsImpl;
typedef VHostdevcapsImpl::value_type VHostdevcaps;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice884

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Fragment<Hostdevsubsys >, Ordered<mpl::vector<Ordered<mpl::vector<Attribute<mpl::int_<893>, Name::Strict<371> >, Domain::Xml::VHostdevcapsImpl > > > > > > VChoice884Impl;
typedef VChoice884Impl::value_type VChoice884;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hostdev

namespace Domain
{
namespace Xml
{
struct Hostdev
{
	Hostdev();

	const VChoice884& getChoice884() const
	{
		return m_choice884;
	}
	void setChoice884(const VChoice884& value_)
	{
		m_choice884 = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	const boost::optional<Rom >& getRom() const
	{
		return m_rom;
	}
	void setRom(const boost::optional<Rom >& value_)
	{
		m_rom = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool getReadonly() const
	{
		return m_readonly;
	}
	void setReadonly(bool value_)
	{
		m_readonly = value_;
	}
	bool getShareable() const
	{
		return m_shareable;
	}
	void setShareable(bool value_)
	{
		m_shareable = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VChoice884 m_choice884;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<PPositiveInteger::value_type > m_boot;
	boost::optional<Rom > m_rom;
	boost::optional<VAddress > m_address;
	bool m_readonly;
	bool m_shareable;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Graphics690

namespace Domain
{
namespace Xml
{
struct Graphics690
{
	const boost::optional<QString >& getDisplay() const
	{
		return m_display;
	}
	void setDisplay(const boost::optional<QString >& value_)
	{
		m_display = value_;
	}
	const boost::optional<QString >& getXauth() const
	{
		return m_xauth;
	}
	void setXauth(const boost::optional<QString >& value_)
	{
		m_xauth = value_;
	}
	const boost::optional<EVirYesNo >& getFullscreen() const
	{
		return m_fullscreen;
	}
	void setFullscreen(const boost::optional<EVirYesNo >& value_)
	{
		m_fullscreen = value_;
	}

private:
	boost::optional<QString > m_display;
	boost::optional<QString > m_xauth;
	boost::optional<EVirYesNo > m_fullscreen;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Variant699

namespace Domain
{
namespace Xml
{
struct Variant699
{
	const boost::optional<PPortNumber::value_type >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<PPortNumber::value_type >& value_)
	{
		m_port = value_;
	}
	const boost::optional<EVirYesNo >& getAutoport() const
	{
		return m_autoport;
	}
	void setAutoport(const boost::optional<EVirYesNo >& value_)
	{
		m_autoport = value_;
	}
	const boost::optional<PPortNumber::value_type >& getWebsocket() const
	{
		return m_websocket;
	}
	void setWebsocket(const boost::optional<PPortNumber::value_type >& value_)
	{
		m_websocket = value_;
	}
	const boost::optional<PAddrIPorName::value_type >& getListen() const
	{
		return m_listen;
	}
	void setListen(const boost::optional<PAddrIPorName::value_type >& value_)
	{
		m_listen = value_;
	}
	const boost::optional<ESharePolicy >& getSharePolicy() const
	{
		return m_sharePolicy;
	}
	void setSharePolicy(const boost::optional<ESharePolicy >& value_)
	{
		m_sharePolicy = value_;
	}

private:
	boost::optional<PPortNumber::value_type > m_port;
	boost::optional<EVirYesNo > m_autoport;
	boost::optional<PPortNumber::value_type > m_websocket;
	boost::optional<PAddrIPorName::value_type > m_listen;
	boost::optional<ESharePolicy > m_sharePolicy;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice701

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Variant699, Optional<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<513> > > > > VChoice701Impl;
typedef VChoice701Impl::value_type VChoice701;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Listen751

namespace Domain
{
namespace Xml
{
struct Listen751
{
	const QString& getNetwork() const
	{
		return m_network;
	}
	void setNetwork(const QString& value_)
	{
		m_network = value_;
	}
	const boost::optional<PAddrIPorName::value_type >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<PAddrIPorName::value_type >& value_)
	{
		m_address = value_;
	}

private:
	QString m_network;
	boost::optional<PAddrIPorName::value_type > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VListen

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Ordered<mpl::vector<Attribute<mpl::int_<106>, Name::Strict<100> >, Attribute<Domain::Xml::PAddrIPorName, Name::Strict<106> > > >, Listen751 > > VListenImpl;
typedef VListenImpl::value_type VListen;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Graphics708

namespace Domain
{
namespace Xml
{
struct Graphics708
{
	const VChoice701& getChoice701() const
	{
		return m_choice701;
	}
	void setChoice701(const VChoice701& value_)
	{
		m_choice701 = value_;
	}
	const boost::optional<QString >& getPasswd() const
	{
		return m_passwd;
	}
	void setPasswd(const boost::optional<QString >& value_)
	{
		m_passwd = value_;
	}
	const boost::optional<QString >& getKeymap() const
	{
		return m_keymap;
	}
	void setKeymap(const boost::optional<QString >& value_)
	{
		m_keymap = value_;
	}
	const boost::optional<PPasswdValidTo::value_type >& getPasswdValidTo() const
	{
		return m_passwdValidTo;
	}
	void setPasswdValidTo(const boost::optional<PPasswdValidTo::value_type >& value_)
	{
		m_passwdValidTo = value_;
	}
	const boost::optional<EConnected >& getConnected() const
	{
		return m_connected;
	}
	void setConnected(const boost::optional<EConnected >& value_)
	{
		m_connected = value_;
	}
	const QList<VListen >& getListenList() const
	{
		return m_listenList;
	}
	void setListenList(const QList<VListen >& value_)
	{
		m_listenList = value_;
	}

private:
	VChoice701 m_choice701;
	boost::optional<QString > m_passwd;
	boost::optional<QString > m_keymap;
	boost::optional<PPasswdValidTo::value_type > m_passwdValidTo;
	boost::optional<EConnected > m_connected;
	QList<VListen > m_listenList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Channel

namespace Domain
{
namespace Xml
{
struct Channel
{
	Channel();

	EName6 getName() const
	{
		return m_name;
	}
	void setName(EName6 value_)
	{
		m_name = value_;
	}
	EMode7 getMode() const
	{
		return m_mode;
	}
	void setMode(EMode7 value_)
	{
		m_mode = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EName6 m_name;
	EMode7 m_mode;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Graphics743

namespace Domain
{
namespace Xml
{
struct Graphics743
{
	const boost::optional<PPortNumber::value_type >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<PPortNumber::value_type >& value_)
	{
		m_port = value_;
	}
	const boost::optional<PPortNumber::value_type >& getTlsPort() const
	{
		return m_tlsPort;
	}
	void setTlsPort(const boost::optional<PPortNumber::value_type >& value_)
	{
		m_tlsPort = value_;
	}
	const boost::optional<EVirYesNo >& getAutoport() const
	{
		return m_autoport;
	}
	void setAutoport(const boost::optional<EVirYesNo >& value_)
	{
		m_autoport = value_;
	}
	const boost::optional<PAddrIPorName::value_type >& getListen() const
	{
		return m_listen;
	}
	void setListen(const boost::optional<PAddrIPorName::value_type >& value_)
	{
		m_listen = value_;
	}
	const boost::optional<QString >& getPasswd() const
	{
		return m_passwd;
	}
	void setPasswd(const boost::optional<QString >& value_)
	{
		m_passwd = value_;
	}
	const boost::optional<QString >& getKeymap() const
	{
		return m_keymap;
	}
	void setKeymap(const boost::optional<QString >& value_)
	{
		m_keymap = value_;
	}
	const boost::optional<PPasswdValidTo::value_type >& getPasswdValidTo() const
	{
		return m_passwdValidTo;
	}
	void setPasswdValidTo(const boost::optional<PPasswdValidTo::value_type >& value_)
	{
		m_passwdValidTo = value_;
	}
	const boost::optional<EConnected1 >& getConnected() const
	{
		return m_connected;
	}
	void setConnected(const boost::optional<EConnected1 >& value_)
	{
		m_connected = value_;
	}
	const boost::optional<EDefaultMode >& getDefaultMode() const
	{
		return m_defaultMode;
	}
	void setDefaultMode(const boost::optional<EDefaultMode >& value_)
	{
		m_defaultMode = value_;
	}
	const QList<VListen >& getListenList() const
	{
		return m_listenList;
	}
	void setListenList(const QList<VListen >& value_)
	{
		m_listenList = value_;
	}
	const QList<Channel >& getChannelList() const
	{
		return m_channelList;
	}
	void setChannelList(const QList<Channel >& value_)
	{
		m_channelList = value_;
	}
	const boost::optional<ECompression >& getImage() const
	{
		return m_image;
	}
	void setImage(const boost::optional<ECompression >& value_)
	{
		m_image = value_;
	}
	const boost::optional<ECompression1 >& getJpeg() const
	{
		return m_jpeg;
	}
	void setJpeg(const boost::optional<ECompression1 >& value_)
	{
		m_jpeg = value_;
	}
	const boost::optional<ECompression2 >& getZlib() const
	{
		return m_zlib;
	}
	void setZlib(const boost::optional<ECompression2 >& value_)
	{
		m_zlib = value_;
	}
	const boost::optional<EVirOnOff >& getPlayback() const
	{
		return m_playback;
	}
	void setPlayback(const boost::optional<EVirOnOff >& value_)
	{
		m_playback = value_;
	}
	const boost::optional<EMode8 >& getStreaming() const
	{
		return m_streaming;
	}
	void setStreaming(const boost::optional<EMode8 >& value_)
	{
		m_streaming = value_;
	}
	const boost::optional<EVirYesNo >& getClipboard() const
	{
		return m_clipboard;
	}
	void setClipboard(const boost::optional<EVirYesNo >& value_)
	{
		m_clipboard = value_;
	}
	const boost::optional<EMode9 >& getMouse() const
	{
		return m_mouse;
	}
	void setMouse(const boost::optional<EMode9 >& value_)
	{
		m_mouse = value_;
	}
	const boost::optional<EVirYesNo >& getFiletransfer() const
	{
		return m_filetransfer;
	}
	void setFiletransfer(const boost::optional<EVirYesNo >& value_)
	{
		m_filetransfer = value_;
	}

private:
	boost::optional<PPortNumber::value_type > m_port;
	boost::optional<PPortNumber::value_type > m_tlsPort;
	boost::optional<EVirYesNo > m_autoport;
	boost::optional<PAddrIPorName::value_type > m_listen;
	boost::optional<QString > m_passwd;
	boost::optional<QString > m_keymap;
	boost::optional<PPasswdValidTo::value_type > m_passwdValidTo;
	boost::optional<EConnected1 > m_connected;
	boost::optional<EDefaultMode > m_defaultMode;
	QList<VListen > m_listenList;
	QList<Channel > m_channelList;
	boost::optional<ECompression > m_image;
	boost::optional<ECompression1 > m_jpeg;
	boost::optional<ECompression2 > m_zlib;
	boost::optional<EVirOnOff > m_playback;
	boost::optional<EMode8 > m_streaming;
	boost::optional<EVirYesNo > m_clipboard;
	boost::optional<EMode9 > m_mouse;
	boost::optional<EVirYesNo > m_filetransfer;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Graphics747

namespace Domain
{
namespace Xml
{
struct Graphics747
{
	const boost::optional<PPortNumber::value_type >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<PPortNumber::value_type >& value_)
	{
		m_port = value_;
	}
	const boost::optional<EVirYesNo >& getAutoport() const
	{
		return m_autoport;
	}
	void setAutoport(const boost::optional<EVirYesNo >& value_)
	{
		m_autoport = value_;
	}
	const boost::optional<EVirYesNo >& getReplaceUser() const
	{
		return m_replaceUser;
	}
	void setReplaceUser(const boost::optional<EVirYesNo >& value_)
	{
		m_replaceUser = value_;
	}
	const boost::optional<EVirYesNo >& getMultiUser() const
	{
		return m_multiUser;
	}
	void setMultiUser(const boost::optional<EVirYesNo >& value_)
	{
		m_multiUser = value_;
	}
	const boost::optional<PAddrIPorName::value_type >& getListen() const
	{
		return m_listen;
	}
	void setListen(const boost::optional<PAddrIPorName::value_type >& value_)
	{
		m_listen = value_;
	}
	const QList<VListen >& getListenList() const
	{
		return m_listenList;
	}
	void setListenList(const QList<VListen >& value_)
	{
		m_listenList = value_;
	}

private:
	boost::optional<PPortNumber::value_type > m_port;
	boost::optional<EVirYesNo > m_autoport;
	boost::optional<EVirYesNo > m_replaceUser;
	boost::optional<EVirYesNo > m_multiUser;
	boost::optional<PAddrIPorName::value_type > m_listen;
	QList<VListen > m_listenList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Graphics749

namespace Domain
{
namespace Xml
{
struct Graphics749
{
	const boost::optional<QString >& getDisplay() const
	{
		return m_display;
	}
	void setDisplay(const boost::optional<QString >& value_)
	{
		m_display = value_;
	}
	const boost::optional<EVirYesNo >& getFullscreen() const
	{
		return m_fullscreen;
	}
	void setFullscreen(const boost::optional<EVirYesNo >& value_)
	{
		m_fullscreen = value_;
	}

private:
	boost::optional<QString > m_display;
	boost::optional<EVirYesNo > m_fullscreen;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VGraphics

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Graphics690, Graphics708, Graphics743, Graphics747, Graphics749 > > VGraphicsImpl;
typedef VGraphicsImpl::value_type VGraphics;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VModel

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Domain::Xml::EType12, Name::Strict<100> >, Ordered<mpl::vector<Attribute<mpl::int_<758>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<615> > > > > > > VModelImpl;
typedef VModelImpl::value_type VModel;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Acceleration

namespace Domain
{
namespace Xml
{
struct Acceleration
{
	const boost::optional<EVirYesNo >& getAccel3d() const
	{
		return m_accel3d;
	}
	void setAccel3d(const boost::optional<EVirYesNo >& value_)
	{
		m_accel3d = value_;
	}
	const boost::optional<EVirYesNo >& getAccel2d() const
	{
		return m_accel2d;
	}
	void setAccel2d(const boost::optional<EVirYesNo >& value_)
	{
		m_accel2d = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_accel3d;
	boost::optional<EVirYesNo > m_accel2d;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Model1

namespace Domain
{
namespace Xml
{
struct Model1
{
	const VModel& getModel() const
	{
		return m_model;
	}
	void setModel(const VModel& value_)
	{
		m_model = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getVram() const
	{
		return m_vram;
	}
	void setVram(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_vram = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getHeads() const
	{
		return m_heads;
	}
	void setHeads(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_heads = value_;
	}
	const boost::optional<EVirYesNo >& getPrimary() const
	{
		return m_primary;
	}
	void setPrimary(const boost::optional<EVirYesNo >& value_)
	{
		m_primary = value_;
	}
	const boost::optional<Acceleration >& getAcceleration() const
	{
		return m_acceleration;
	}
	void setAcceleration(const boost::optional<Acceleration >& value_)
	{
		m_acceleration = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VModel m_model;
	boost::optional<PUnsignedInt::value_type > m_vram;
	boost::optional<PUnsignedInt::value_type > m_heads;
	boost::optional<EVirYesNo > m_primary;
	boost::optional<Acceleration > m_acceleration;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Video

namespace Domain
{
namespace Xml
{
struct Video
{
	const boost::optional<Model1 >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<Model1 >& value_)
	{
		m_model = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<Model1 > m_model;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source15

namespace Domain
{
namespace Xml
{
struct Source15
{
	const boost::optional<QString >& getMode() const
	{
		return m_mode;
	}
	void setMode(const boost::optional<QString >& value_)
	{
		m_mode = value_;
	}
	const boost::optional<QString >& getPath() const
	{
		return m_path;
	}
	void setPath(const boost::optional<QString >& value_)
	{
		m_path = value_;
	}
	const boost::optional<QString >& getHost() const
	{
		return m_host;
	}
	void setHost(const boost::optional<QString >& value_)
	{
		m_host = value_;
	}
	const boost::optional<QString >& getService() const
	{
		return m_service;
	}
	void setService(const boost::optional<QString >& value_)
	{
		m_service = value_;
	}
	const boost::optional<QString >& getWiremode() const
	{
		return m_wiremode;
	}
	void setWiremode(const boost::optional<QString >& value_)
	{
		m_wiremode = value_;
	}
	const boost::optional<QString >& getChannel() const
	{
		return m_channel;
	}
	void setChannel(const boost::optional<QString >& value_)
	{
		m_channel = value_;
	}
	const boost::optional<QString >& getMaster() const
	{
		return m_master;
	}
	void setMaster(const boost::optional<QString >& value_)
	{
		m_master = value_;
	}
	const boost::optional<QString >& getSlave() const
	{
		return m_slave;
	}
	void setSlave(const boost::optional<QString >& value_)
	{
		m_slave = value_;
	}
	const boost::optional<EVirOnOff >& getAppend() const
	{
		return m_append;
	}
	void setAppend(const boost::optional<EVirOnOff >& value_)
	{
		m_append = value_;
	}
	const boost::optional<Seclabel >& getSeclabel() const
	{
		return m_seclabel;
	}
	void setSeclabel(const boost::optional<Seclabel >& value_)
	{
		m_seclabel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QString > m_mode;
	boost::optional<QString > m_path;
	boost::optional<QString > m_host;
	boost::optional<QString > m_service;
	boost::optional<QString > m_wiremode;
	boost::optional<QString > m_channel;
	boost::optional<QString > m_master;
	boost::optional<QString > m_slave;
	boost::optional<EVirOnOff > m_append;
	boost::optional<Seclabel > m_seclabel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Protocol

namespace Domain
{
namespace Xml
{
struct Protocol
{
	const boost::optional<EType13 >& getType() const
	{
		return m_type;
	}
	void setType(const boost::optional<EType13 >& value_)
	{
		m_type = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EType13 > m_type;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct QemucdevSrcDef

namespace Domain
{
namespace Xml
{
struct QemucdevSrcDef
{
	const QList<Source15 >& getSourceList() const
	{
		return m_sourceList;
	}
	void setSourceList(const QList<Source15 >& value_)
	{
		m_sourceList = value_;
	}
	const boost::optional<Protocol >& getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(const boost::optional<Protocol >& value_)
	{
		m_protocol = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	QList<Source15 > m_sourceList;
	boost::optional<Protocol > m_protocol;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice795

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Optional<Attribute<Domain::Xml::EType14, Name::Strict<100> > >, Optional<Attribute<Domain::Xml::EType15, Name::Strict<100> > > > > VChoice795Impl;
typedef VChoice795Impl::value_type VChoice795;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Target2

namespace Domain
{
namespace Xml
{
struct Target2
{
	const VChoice795& getChoice795() const
	{
		return m_choice795;
	}
	void setChoice795(const VChoice795& value_)
	{
		m_choice795 = value_;
	}
	const boost::optional<QString >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<QString >& value_)
	{
		m_port = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VChoice795 m_choice795;
	boost::optional<QString > m_port;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Qemucdev

namespace Domain
{
namespace Xml
{
struct Qemucdev
{
	Qemucdev();

	EQemucdevSrcTypeChoice getType() const
	{
		return m_type;
	}
	void setType(EQemucdevSrcTypeChoice value_)
	{
		m_type = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getTty() const
	{
		return m_tty;
	}
	void setTty(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_tty = value_;
	}
	const QemucdevSrcDef& getQemucdevSrcDef() const
	{
		return m_qemucdevSrcDef;
	}
	void setQemucdevSrcDef(const QemucdevSrcDef& value_)
	{
		m_qemucdevSrcDef = value_;
	}
	const boost::optional<Target2 >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<Target2 >& value_)
	{
		m_target = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EQemucdevSrcTypeChoice m_type;
	boost::optional<PAbsFilePath::value_type > m_tty;
	QemucdevSrcDef m_qemucdevSrcDef;
	boost::optional<Target2 > m_target;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VConsole

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Optional<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<785> > >, Qemucdev > > VConsoleImpl;
typedef VConsoleImpl::value_type VConsole;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Target3

namespace Domain
{
namespace Xml
{
struct Target3
{
	const QString& getAddress() const
	{
		return m_address;
	}
	void setAddress(const QString& value_)
	{
		m_address = value_;
	}
	const QString& getPort() const
	{
		return m_port;
	}
	void setPort(const QString& value_)
	{
		m_port = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_address;
	QString m_port;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice861

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Domain::Xml::Target3, Name::Strict<315> >, Element<Ordered<mpl::vector<Attribute<mpl::int_<522>, Name::Strict<100> >, Optional<Attribute<QString, Name::Strict<102> > > > >, Name::Strict<315> > > > VChoice861Impl;
typedef VChoice861Impl::value_type VChoice861;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Channel1

namespace Domain
{
namespace Xml
{
struct Channel1
{
	Channel1();

	EQemucdevSrcTypeChoice getType() const
	{
		return m_type;
	}
	void setType(EQemucdevSrcTypeChoice value_)
	{
		m_type = value_;
	}
	const QemucdevSrcDef& getQemucdevSrcDef() const
	{
		return m_qemucdevSrcDef;
	}
	void setQemucdevSrcDef(const QemucdevSrcDef& value_)
	{
		m_qemucdevSrcDef = value_;
	}
	const VChoice861& getChoice861() const
	{
		return m_choice861;
	}
	void setChoice861(const VChoice861& value_)
	{
		m_choice861 = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EQemucdevSrcTypeChoice m_type;
	QemucdevSrcDef m_qemucdevSrcDef;
	VChoice861 m_choice861;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard862

namespace Domain
{
namespace Xml
{
struct Smartcard862
{
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard866

namespace Domain
{
namespace Xml
{
struct Smartcard866
{
	const QString& getCertificate() const
	{
		return m_certificate;
	}
	void setCertificate(const QString& value_)
	{
		m_certificate = value_;
	}
	const QString& getCertificate2() const
	{
		return m_certificate2;
	}
	void setCertificate2(const QString& value_)
	{
		m_certificate2 = value_;
	}
	const QString& getCertificate3() const
	{
		return m_certificate3;
	}
	void setCertificate3(const QString& value_)
	{
		m_certificate3 = value_;
	}
	const boost::optional<PAbsDirPath::value_type >& getDatabase() const
	{
		return m_database;
	}
	void setDatabase(const boost::optional<PAbsDirPath::value_type >& value_)
	{
		m_database = value_;
	}

private:
	QString m_certificate;
	QString m_certificate2;
	QString m_certificate3;
	boost::optional<PAbsDirPath::value_type > m_database;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard867

namespace Domain
{
namespace Xml
{
struct Smartcard867
{
	Smartcard867();

	EQemucdevSrcTypeChoice getType() const
	{
		return m_type;
	}
	void setType(EQemucdevSrcTypeChoice value_)
	{
		m_type = value_;
	}
	const QemucdevSrcDef& getQemucdevSrcDef() const
	{
		return m_qemucdevSrcDef;
	}
	void setQemucdevSrcDef(const QemucdevSrcDef& value_)
	{
		m_qemucdevSrcDef = value_;
	}
	const boost::optional<Target2 >& getTarget() const
	{
		return m_target;
	}
	void setTarget(const boost::optional<Target2 >& value_)
	{
		m_target = value_;
	}

private:
	EQemucdevSrcTypeChoice m_type;
	QemucdevSrcDef m_qemucdevSrcDef;
	boost::optional<Target2 > m_target;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VSmartcard

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Smartcard862, Smartcard866, Smartcard867 > > VSmartcardImpl;
typedef VSmartcardImpl::value_type VSmartcard;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard

namespace Domain
{
namespace Xml
{
struct Smartcard
{
	const VSmartcard& getSmartcard() const
	{
		return m_smartcard;
	}
	void setSmartcard(const VSmartcard& value_)
	{
		m_smartcard = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VSmartcard m_smartcard;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Hub

namespace Domain
{
namespace Xml
{
struct Hub
{
	Hub();

	EType16 getType() const
	{
		return m_type;
	}
	void setType(EType16 value_)
	{
		m_type = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EType16 m_type;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Redirdev

namespace Domain
{
namespace Xml
{
struct Redirdev
{
	Redirdev();

	EBus2 getBus() const
	{
		return m_bus;
	}
	void setBus(EBus2 value_)
	{
		m_bus = value_;
	}
	EQemucdevSrcTypeChoice getType() const
	{
		return m_type;
	}
	void setType(EQemucdevSrcTypeChoice value_)
	{
		m_type = value_;
	}
	const QemucdevSrcDef& getQemucdevSrcDef() const
	{
		return m_qemucdevSrcDef;
	}
	void setQemucdevSrcDef(const QemucdevSrcDef& value_)
	{
		m_qemucdevSrcDef = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getBoot() const
	{
		return m_boot;
	}
	void setBoot(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_boot = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EBus2 m_bus;
	EQemucdevSrcTypeChoice m_type;
	QemucdevSrcDef m_qemucdevSrcDef;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<PPositiveInteger::value_type > m_boot;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Usbdev

namespace Domain
{
namespace Xml
{
struct Usbdev
{
	Usbdev();

	EVirYesNo getAllow() const
	{
		return m_allow;
	}
	void setAllow(EVirYesNo value_)
	{
		m_allow = value_;
	}
	const boost::optional<VClass >& getClass() const
	{
		return m_class;
	}
	void setClass(const boost::optional<VClass >& value_)
	{
		m_class = value_;
	}
	const boost::optional<VVendor >& getVendor() const
	{
		return m_vendor;
	}
	void setVendor(const boost::optional<VVendor >& value_)
	{
		m_vendor = value_;
	}
	const boost::optional<VProduct >& getProduct() const
	{
		return m_product;
	}
	void setProduct(const boost::optional<VProduct >& value_)
	{
		m_product = value_;
	}
	const boost::optional<VVersion >& getVersion() const
	{
		return m_version;
	}
	void setVersion(const boost::optional<VVersion >& value_)
	{
		m_version = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_allow;
	boost::optional<VClass > m_class;
	boost::optional<VVendor > m_vendor;
	boost::optional<VProduct > m_product;
	boost::optional<VVersion > m_version;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Backend1042

namespace Domain
{
namespace Xml
{
struct Backend1042
{
	Backend1042();

	EQemucdevSrcTypeChoice getType() const
	{
		return m_type;
	}
	void setType(EQemucdevSrcTypeChoice value_)
	{
		m_type = value_;
	}
	const QemucdevSrcDef& getQemucdevSrcDef() const
	{
		return m_qemucdevSrcDef;
	}
	void setQemucdevSrcDef(const QemucdevSrcDef& value_)
	{
		m_qemucdevSrcDef = value_;
	}

private:
	EQemucdevSrcTypeChoice m_type;
	QemucdevSrcDef m_qemucdevSrcDef;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VBackend

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Ordered<mpl::vector<Attribute<mpl::int_<1037>, Name::Strict<223> >, Text<Domain::Xml::EChoice1039 > > >, Backend1042 > > VBackendImpl;
typedef VBackendImpl::value_type VBackend;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Rate

namespace Domain
{
namespace Xml
{
struct Rate
{
	Rate();

	PPositiveInteger::value_type getBytes() const
	{
		return m_bytes;
	}
	void setBytes(PPositiveInteger::value_type value_)
	{
		m_bytes = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getPeriod() const
	{
		return m_period;
	}
	void setPeriod(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_period = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PPositiveInteger::value_type m_bytes;
	boost::optional<PPositiveInteger::value_type > m_period;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Rng

namespace Domain
{
namespace Xml
{
struct Rng
{
	Rng();

	EModel5 getModel() const
	{
		return m_model;
	}
	void setModel(EModel5 value_)
	{
		m_model = value_;
	}
	const VBackend& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const VBackend& value_)
	{
		m_backend = value_;
	}
	const boost::optional<Rate >& getRate() const
	{
		return m_rate;
	}
	void setRate(const boost::optional<Rate >& value_)
	{
		m_rate = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EModel5 m_model;
	VBackend m_backend;
	boost::optional<Rate > m_rate;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Device1

namespace Domain
{
namespace Xml
{
struct Device1
{
	const boost::optional<PFilePath::value_type >& getPath() const
	{
		return m_path;
	}
	void setPath(const boost::optional<PFilePath::value_type >& value_)
	{
		m_path = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PFilePath::value_type > m_path;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Tpm

namespace Domain
{
namespace Xml
{
struct Tpm
{
	const boost::optional<EModel6 >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<EModel6 >& value_)
	{
		m_model = value_;
	}
	const boost::optional<Device1 >& getBackend() const
	{
		return m_backend;
	}
	void setBackend(const boost::optional<Device1 >& value_)
	{
		m_backend = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EModel6 > m_model;
	boost::optional<Device1 > m_backend;
	boost::optional<PAliasName::value_type > m_alias;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Source16

namespace Domain
{
namespace Xml
{
struct Source16
{
	const boost::optional<ScaledInteger >& getPagesize() const
	{
		return m_pagesize;
	}
	void setPagesize(const boost::optional<ScaledInteger >& value_)
	{
		m_pagesize = value_;
	}
	const boost::optional<PCpuset::value_type >& getNodemask() const
	{
		return m_nodemask;
	}
	void setNodemask(const boost::optional<PCpuset::value_type >& value_)
	{
		m_nodemask = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<ScaledInteger > m_pagesize;
	boost::optional<PCpuset::value_type > m_nodemask;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Target4

namespace Domain
{
namespace Xml
{
struct Target4
{
	Target4();

	const ScaledInteger& getSize() const
	{
		return m_size;
	}
	void setSize(const ScaledInteger& value_)
	{
		m_size = value_;
	}
	PUnsignedInt::value_type getNode() const
	{
		return m_node;
	}
	void setNode(PUnsignedInt::value_type value_)
	{
		m_node = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	ScaledInteger m_size;
	PUnsignedInt::value_type m_node;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Memory2

namespace Domain
{
namespace Xml
{
struct Memory2
{
	Memory2();

	EModel7 getModel() const
	{
		return m_model;
	}
	void setModel(EModel7 value_)
	{
		m_model = value_;
	}
	const boost::optional<Source16 >& getSource() const
	{
		return m_source;
	}
	void setSource(const boost::optional<Source16 >& value_)
	{
		m_source = value_;
	}
	const Target4& getTarget() const
	{
		return m_target;
	}
	void setTarget(const Target4& value_)
	{
		m_target = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EModel7 m_model;
	boost::optional<Source16 > m_source;
	Target4 m_target;
	boost::optional<VAddress > m_address;
	boost::optional<PAliasName::value_type > m_alias;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VChoice952

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Domain::Xml::Disk, Name::Strict<464> >, Element<Domain::Xml::Controller, Name::Strict<564> >, Element<Domain::Xml::Lease, Name::Strict<457> >, Element<Domain::Xml::Filesystem, Name::Strict<606> >, Element<Domain::Xml::VInterfaceImpl, Name::Strict<629> >, Element<Domain::Xml::Input, Name::Strict<874> >, Element<Domain::Xml::Sound, Name::Strict<839> >, Element<Domain::Xml::Hostdev, Name::Strict<650> >, Element<Domain::Xml::VGraphicsImpl, Name::Strict<686> >, Element<Domain::Xml::Video, Name::Strict<753> >, Element<Domain::Xml::VConsoleImpl, Name::Strict<832> >, Element<Domain::Xml::Qemucdev, Name::Strict<856> >, Element<Domain::Xml::Qemucdev, Name::Strict<446> >, Element<Domain::Xml::Channel1, Name::Strict<718> >, Element<Domain::Xml::Smartcard, Name::Strict<724> >, Element<Domain::Xml::Hub, Name::Strict<878> >, Element<Domain::Xml::Redirdev, Name::Strict<879> >, Element<ZeroOrMore<Element<Domain::Xml::Usbdev, Name::Strict<808> > >, Name::Strict<880> >, Element<Domain::Xml::Rng, Name::Strict<948> >, Element<Domain::Xml::Tpm, Name::Strict<869> >, Element<Domain::Xml::Memory2, Name::Strict<318> > > > VChoice952Impl;
typedef VChoice952Impl::value_type VChoice952;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Watchdog

namespace Domain
{
namespace Xml
{
struct Watchdog
{
	Watchdog();

	EModel8 getModel() const
	{
		return m_model;
	}
	void setModel(EModel8 value_)
	{
		m_model = value_;
	}
	const boost::optional<EAction >& getAction() const
	{
		return m_action;
	}
	void setAction(const boost::optional<EAction >& value_)
	{
		m_action = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EModel8 m_model;
	boost::optional<EAction > m_action;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Memballoon

namespace Domain
{
namespace Xml
{
struct Memballoon
{
	Memballoon();

	EModel9 getModel() const
	{
		return m_model;
	}
	void setModel(EModel9 value_)
	{
		m_model = value_;
	}
	const boost::optional<EVirOnOff >& getAutodeflate() const
	{
		return m_autodeflate;
	}
	void setAutodeflate(const boost::optional<EVirOnOff >& value_)
	{
		m_autodeflate = value_;
	}
	const boost::optional<PAliasName::value_type >& getAlias() const
	{
		return m_alias;
	}
	void setAlias(const boost::optional<PAliasName::value_type >& value_)
	{
		m_alias = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	const boost::optional<PPositiveInteger::value_type >& getStats() const
	{
		return m_stats;
	}
	void setStats(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_stats = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EModel9 m_model;
	boost::optional<EVirOnOff > m_autodeflate;
	boost::optional<PAliasName::value_type > m_alias;
	boost::optional<VAddress > m_address;
	boost::optional<PPositiveInteger::value_type > m_stats;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Nvram1

namespace Domain
{
namespace Xml
{
struct Nvram1
{
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Panic

namespace Domain
{
namespace Xml
{
struct Panic
{
	const boost::optional<EModel10 >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<EModel10 >& value_)
	{
		m_model = value_;
	}
	const boost::optional<VAddress >& getAddress() const
	{
		return m_address;
	}
	void setAddress(const boost::optional<VAddress >& value_)
	{
		m_address = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EModel10 > m_model;
	boost::optional<VAddress > m_address;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Devices

namespace Domain
{
namespace Xml
{
struct Devices
{
	const boost::optional<PAbsFilePath::value_type >& getEmulator() const
	{
		return m_emulator;
	}
	void setEmulator(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_emulator = value_;
	}
	const QList<VChoice952 >& getChoice952List() const
	{
		return m_choice952List;
	}
	void setChoice952List(const QList<VChoice952 >& value_)
	{
		m_choice952List = value_;
	}
	const boost::optional<Watchdog >& getWatchdog() const
	{
		return m_watchdog;
	}
	void setWatchdog(const boost::optional<Watchdog >& value_)
	{
		m_watchdog = value_;
	}
	const boost::optional<Memballoon >& getMemballoon() const
	{
		return m_memballoon;
	}
	void setMemballoon(const boost::optional<Memballoon >& value_)
	{
		m_memballoon = value_;
	}
	const boost::optional<Nvram1 >& getNvram() const
	{
		return m_nvram;
	}
	void setNvram(const boost::optional<Nvram1 >& value_)
	{
		m_nvram = value_;
	}
	const QList<Panic >& getPanicList() const
	{
		return m_panicList;
	}
	void setPanicList(const QList<Panic >& value_)
	{
		m_panicList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PAbsFilePath::value_type > m_emulator;
	QList<VChoice952 > m_choice952List;
	boost::optional<Watchdog > m_watchdog;
	boost::optional<Memballoon > m_memballoon;
	boost::optional<Nvram1 > m_nvram;
	QList<Panic > m_panicList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel228

namespace Domain
{
namespace Xml
{
struct Seclabel228
{
	const boost::optional<QString >& getLabel() const
	{
		return m_label;
	}
	void setLabel(const boost::optional<QString >& value_)
	{
		m_label = value_;
	}
	const boost::optional<QString >& getImagelabel() const
	{
		return m_imagelabel;
	}
	void setImagelabel(const boost::optional<QString >& value_)
	{
		m_imagelabel = value_;
	}
	const boost::optional<QString >& getBaselabel() const
	{
		return m_baselabel;
	}
	void setBaselabel(const boost::optional<QString >& value_)
	{
		m_baselabel = value_;
	}

private:
	boost::optional<QString > m_label;
	boost::optional<QString > m_imagelabel;
	boost::optional<QString > m_baselabel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel230

namespace Domain
{
namespace Xml
{
struct Seclabel230
{
	const boost::optional<EVirYesNo >& getRelabel() const
	{
		return m_relabel;
	}
	void setRelabel(const boost::optional<EVirYesNo >& value_)
	{
		m_relabel = value_;
	}
	const QString& getLabel() const
	{
		return m_label;
	}
	void setLabel(const QString& value_)
	{
		m_label = value_;
	}
	const boost::optional<QString >& getImagelabel() const
	{
		return m_imagelabel;
	}
	void setImagelabel(const boost::optional<QString >& value_)
	{
		m_imagelabel = value_;
	}

private:
	boost::optional<EVirYesNo > m_relabel;
	QString m_label;
	boost::optional<QString > m_imagelabel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel232

namespace Domain
{
namespace Xml
{
struct Seclabel232
{
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VSeclabel1

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<Seclabel228, Seclabel230, Seclabel232 > > VSeclabel1Impl;
typedef VSeclabel1Impl::value_type VSeclabel1;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel1

namespace Domain
{
namespace Xml
{
struct Seclabel1
{
	const boost::optional<QString >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<QString >& value_)
	{
		m_model = value_;
	}
	const VSeclabel1& getSeclabel() const
	{
		return m_seclabel;
	}
	void setSeclabel(const VSeclabel1& value_)
	{
		m_seclabel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QString > m_model;
	VSeclabel1 m_seclabel;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Env

namespace Domain
{
namespace Xml
{
struct Env
{
	const PFilterParamName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PFilterParamName::value_type& value_)
	{
		m_name = value_;
	}
	const boost::optional<QString >& getValue() const
	{
		return m_value;
	}
	void setValue(const boost::optional<QString >& value_)
	{
		m_value = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PFilterParamName::value_type m_name;
	boost::optional<QString > m_value;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Commandline

namespace Domain
{
namespace Xml
{
struct Commandline
{
	const QList<QString >& getArgList() const
	{
		return m_argList;
	}
	void setArgList(const QList<QString >& value_)
	{
		m_argList = value_;
	}
	const QList<Env >& getEnvList() const
	{
		return m_envList;
	}
	void setEnvList(const QList<Env >& value_)
	{
		m_envList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QList<QString > m_argList;
	QList<Env > m_envList;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Domain

namespace Domain
{
namespace Xml
{
struct Domain
{
	Domain();

	EType getType() const
	{
		return m_type;
	}
	void setType(EType value_)
	{
		m_type = value_;
	}
	const Ids& getIds() const
	{
		return m_ids;
	}
	void setIds(const Ids& value_)
	{
		m_ids = value_;
	}
	const boost::optional<PTitle::value_type >& getTitle() const
	{
		return m_title;
	}
	void setTitle(const boost::optional<PTitle::value_type >& value_)
	{
		m_title = value_;
	}
	const boost::optional<QString >& getDescription() const
	{
		return m_description;
	}
	void setDescription(const boost::optional<QString >& value_)
	{
		m_description = value_;
	}
	const boost::optional<Cpu >& getCpu() const
	{
		return m_cpu;
	}
	void setCpu(const boost::optional<Cpu >& value_)
	{
		m_cpu = value_;
	}
	const boost::optional<Sysinfo >& getSysinfo() const
	{
		return m_sysinfo;
	}
	void setSysinfo(const boost::optional<Sysinfo >& value_)
	{
		m_sysinfo = value_;
	}
	const VOs& getOs() const
	{
		return m_os;
	}
	void setOs(const VOs& value_)
	{
		m_os = value_;
	}
	const boost::optional<Clock >& getClock() const
	{
		return m_clock;
	}
	void setClock(const boost::optional<Clock >& value_)
	{
		m_clock = value_;
	}
	const Memory& getMemory() const
	{
		return m_memory;
	}
	void setMemory(const Memory& value_)
	{
		m_memory = value_;
	}
	const boost::optional<MaxMemory >& getMaxMemory() const
	{
		return m_maxMemory;
	}
	void setMaxMemory(const boost::optional<MaxMemory >& value_)
	{
		m_maxMemory = value_;
	}
	const boost::optional<ScaledInteger >& getCurrentMemory() const
	{
		return m_currentMemory;
	}
	void setCurrentMemory(const boost::optional<ScaledInteger >& value_)
	{
		m_currentMemory = value_;
	}
	const boost::optional<MemoryBacking >& getMemoryBacking() const
	{
		return m_memoryBacking;
	}
	void setMemoryBacking(const boost::optional<MemoryBacking >& value_)
	{
		m_memoryBacking = value_;
	}
	const boost::optional<Vcpu >& getVcpu() const
	{
		return m_vcpu;
	}
	void setVcpu(const boost::optional<Vcpu >& value_)
	{
		m_vcpu = value_;
	}
	const boost::optional<QList<Vcpu1 > >& getVcpus() const
	{
		return m_vcpus;
	}
	void setVcpus(const boost::optional<QList<Vcpu1 > >& value_)
	{
		m_vcpus = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getIothreads() const
	{
		return m_iothreads;
	}
	void setIothreads(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_iothreads = value_;
	}
	const boost::optional<Blkiotune >& getBlkiotune() const
	{
		return m_blkiotune;
	}
	void setBlkiotune(const boost::optional<Blkiotune >& value_)
	{
		m_blkiotune = value_;
	}
	const boost::optional<Memtune >& getMemtune() const
	{
		return m_memtune;
	}
	void setMemtune(const boost::optional<Memtune >& value_)
	{
		m_memtune = value_;
	}
	const boost::optional<Cputune >& getCputune() const
	{
		return m_cputune;
	}
	void setCputune(const boost::optional<Cputune >& value_)
	{
		m_cputune = value_;
	}
	const boost::optional<Numatune >& getNumatune() const
	{
		return m_numatune;
	}
	void setNumatune(const boost::optional<Numatune >& value_)
	{
		m_numatune = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getResource() const
	{
		return m_resource;
	}
	void setResource(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_resource = value_;
	}
	const boost::optional<Features >& getFeatures() const
	{
		return m_features;
	}
	void setFeatures(const boost::optional<Features >& value_)
	{
		m_features = value_;
	}
	const boost::optional<EOffOptions >& getOnReboot() const
	{
		return m_onReboot;
	}
	void setOnReboot(const boost::optional<EOffOptions >& value_)
	{
		m_onReboot = value_;
	}
	const boost::optional<EOffOptions >& getOnPoweroff() const
	{
		return m_onPoweroff;
	}
	void setOnPoweroff(const boost::optional<EOffOptions >& value_)
	{
		m_onPoweroff = value_;
	}
	const boost::optional<ECrashOptions >& getOnCrash() const
	{
		return m_onCrash;
	}
	void setOnCrash(const boost::optional<ECrashOptions >& value_)
	{
		m_onCrash = value_;
	}
	const boost::optional<ELockfailureOptions >& getOnLockfailure() const
	{
		return m_onLockfailure;
	}
	void setOnLockfailure(const boost::optional<ELockfailureOptions >& value_)
	{
		m_onLockfailure = value_;
	}
	const boost::optional<Pm >& getPm() const
	{
		return m_pm;
	}
	void setPm(const boost::optional<Pm >& value_)
	{
		m_pm = value_;
	}
	const boost::optional<Idmap >& getIdmap() const
	{
		return m_idmap;
	}
	void setIdmap(const boost::optional<Idmap >& value_)
	{
		m_idmap = value_;
	}
	const boost::optional<Devices >& getDevices() const
	{
		return m_devices;
	}
	void setDevices(const boost::optional<Devices >& value_)
	{
		m_devices = value_;
	}
	const QList<Seclabel1 >& getSeclabelList() const
	{
		return m_seclabelList;
	}
	void setSeclabelList(const QList<Seclabel1 >& value_)
	{
		m_seclabelList = value_;
	}
	const boost::optional<Commandline >& getCommandline() const
	{
		return m_commandline;
	}
	void setCommandline(const boost::optional<Commandline >& value_)
	{
		m_commandline = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EType m_type;
	Ids m_ids;
	boost::optional<PTitle::value_type > m_title;
	boost::optional<QString > m_description;
	boost::optional<Cpu > m_cpu;
	boost::optional<Sysinfo > m_sysinfo;
	VOs m_os;
	boost::optional<Clock > m_clock;
	Memory m_memory;
	boost::optional<MaxMemory > m_maxMemory;
	boost::optional<ScaledInteger > m_currentMemory;
	boost::optional<MemoryBacking > m_memoryBacking;
	boost::optional<Vcpu > m_vcpu;
	boost::optional<QList<Vcpu1 > > m_vcpus;
	boost::optional<PUnsignedInt::value_type > m_iothreads;
	boost::optional<Blkiotune > m_blkiotune;
	boost::optional<Memtune > m_memtune;
	boost::optional<Cputune > m_cputune;
	boost::optional<Numatune > m_numatune;
	boost::optional<PAbsFilePath::value_type > m_resource;
	boost::optional<Features > m_features;
	boost::optional<EOffOptions > m_onReboot;
	boost::optional<EOffOptions > m_onPoweroff;
	boost::optional<ECrashOptions > m_onCrash;
	boost::optional<ELockfailureOptions > m_onLockfailure;
	boost::optional<Pm > m_pm;
	boost::optional<Idmap > m_idmap;
	boost::optional<Devices > m_devices;
	QList<Seclabel1 > m_seclabelList;
	boost::optional<Commandline > m_commandline;
};

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct Ids traits

template<>
struct Traits<Domain::Xml::Ids>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<203> > >, Unordered<mpl::vector<Element<Text<Domain::Xml::PDomainName >, Name::Strict<102> >, Optional<Element<Text<Domain::Xml::VUUID >, Name::Strict<146> > > > > > > marshal_type;

	static int parse(Domain::Xml::Ids& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Ids& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Model traits

template<>
struct Traits<Domain::Xml::Model>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EFallback, Name::Strict<980> > >, Optional<Attribute<Domain::Xml::PVendorId, Name::Strict<982> > >, Text<QString > > > marshal_type;

	static int parse(Domain::Xml::Model& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Model& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Topology traits

template<>
struct Traits<Domain::Xml::Topology>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<991> >, Attribute<Domain::Xml::PPositiveInteger, Name::Strict<992> >, Attribute<Domain::Xml::PPositiveInteger, Name::Strict<562> > > > marshal_type;

	static int parse(Domain::Xml::Topology& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Topology& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Feature traits

template<>
struct Traits<Domain::Xml::Feature>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EPolicy, Name::Strict<985> >, Attribute<Domain::Xml::PFeatureName, Name::Strict<102> > > > marshal_type;

	static int parse(Domain::Xml::Feature& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Feature& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cell traits

template<>
struct Traits<Domain::Xml::Cell>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<203> > >, Attribute<Domain::Xml::PCpuset, Name::Strict<996> >, Attribute<Domain::Xml::PMemoryKB, Name::Strict<318> >, Optional<Attribute<Domain::Xml::EMemAccess, Name::Strict<998> > > > > marshal_type;

	static int parse(Domain::Xml::Cell& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Cell& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cpu traits

template<>
struct Traits<Domain::Xml::Cpu>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EMode, Name::Strict<371> > >, Optional<Attribute<Domain::Xml::EMatch, Name::Strict<977> > >, Unordered<mpl::vector<Optional<Element<Domain::Xml::Model, Name::Strict<223> > >, Optional<Element<Text<QString >, Name::Strict<452> > >, Optional<Element<Domain::Xml::Topology, Name::Strict<990> > >, ZeroOrMore<Element<Domain::Xml::Feature, Name::Strict<984> > >, Optional<Element<OneOrMore<Element<Domain::Xml::Cell, Name::Strict<995> > >, Name::Strict<993> > > > > > > marshal_type;

	static int parse(Domain::Xml::Cpu& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Cpu& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Entry traits

template<>
struct Traits<Domain::Xml::Entry>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::ESysinfoBiosName, Name::Strict<102> >, Text<Domain::Xml::PSysinfoValue > > > marshal_type;

	static int parse(Domain::Xml::Entry& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Entry& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Entry1 traits

template<>
struct Traits<Domain::Xml::Entry1>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::ESysinfoSystemName, Name::Strict<102> >, Text<Domain::Xml::PSysinfoValue > > > marshal_type;

	static int parse(Domain::Xml::Entry1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Entry1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Sysinfo traits

template<>
struct Traits<Domain::Xml::Sysinfo>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<275>, Name::Strict<100> >, Unordered<mpl::vector<Optional<Element<OneOrMore<Element<Domain::Xml::Entry, Name::Strict<1001> > >, Name::Strict<276> > >, Optional<Element<OneOrMore<Element<Domain::Xml::Entry1, Name::Strict<1001> > >, Name::Strict<1004> > > > > > > marshal_type;

	static int parse(Domain::Xml::Sysinfo& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Sysinfo& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Bootloader traits

template<>
struct Traits<Domain::Xml::Bootloader>
{
	typedef Unordered<mpl::vector<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<259> >, Optional<Element<Text<QString >, Name::Strict<428> > > > > marshal_type;

	static int parse(Domain::Xml::Bootloader& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Bootloader& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Type traits

template<>
struct Traits<Domain::Xml::Type>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EArch, Name::Strict<277> > >, Optional<Attribute<Domain::Xml::EMachine, Name::Strict<278> > >, Text<Domain::Xml::EType1 > > > marshal_type;

	static int parse(Domain::Xml::Type& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Type& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Osbootkernel traits

template<>
struct Traits<Domain::Xml::Osbootkernel>
{
	typedef Unordered<mpl::vector<Optional<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<429> > >, Optional<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<430> > >, Optional<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<431> > >, Optional<Element<Text<QString >, Name::Strict<432> > >, Optional<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<433> > > > > marshal_type;

	static int parse(Domain::Xml::Osbootkernel& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Osbootkernel& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Os traits

template<>
struct Traits<Domain::Xml::Os>
{
	typedef Ordered<mpl::vector<Element<Domain::Xml::Type, Name::Strict<100> >, Fragment<Domain::Xml::Osbootkernel > > > marshal_type;

	static int parse(Domain::Xml::Os& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Os& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Osxen261 traits

template<>
struct Traits<Domain::Xml::Osxen261>
{
	typedef Ordered<mpl::vector<Optional<Fragment<Domain::Xml::Bootloader > >, Element<Domain::Xml::Os, Name::Strict<214> > > > marshal_type;

	static int parse(Domain::Xml::Osxen261& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Osxen261& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Os1 traits

template<>
struct Traits<Domain::Xml::Os1>
{
	typedef Ordered<mpl::vector<Element<Domain::Xml::Type, Name::Strict<100> >, Optional<Fragment<Domain::Xml::Osbootkernel > > > > marshal_type;

	static int parse(Domain::Xml::Os1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Os1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Osxen262 traits

template<>
struct Traits<Domain::Xml::Osxen262>
{
	typedef Ordered<mpl::vector<Fragment<Domain::Xml::Bootloader >, Optional<Element<Domain::Xml::Os1, Name::Strict<214> > > > > marshal_type;

	static int parse(Domain::Xml::Osxen262& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Osxen262& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hvmx86 traits

template<>
struct Traits<Domain::Xml::Hvmx86>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EArch1, Name::Strict<277> > >, Optional<Attribute<Domain::Xml::PMachine, Name::Strict<278> > > > > marshal_type;

	static int parse(Domain::Xml::Hvmx86& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hvmx86& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hvmmips traits

template<>
struct Traits<Domain::Xml::Hvmmips>
{
	typedef Ordered<mpl::vector<Optional<Attribute<mpl::int_<77>, Name::Strict<277> > >, Optional<Attribute<mpl::int_<77>, Name::Strict<278> > > > > marshal_type;

	static int parse(Domain::Xml::Hvmmips& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hvmmips& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hvmsparc traits

template<>
struct Traits<Domain::Xml::Hvmsparc>
{
	typedef Ordered<mpl::vector<Optional<Attribute<mpl::int_<91>, Name::Strict<277> > >, Optional<Attribute<mpl::int_<300>, Name::Strict<278> > > > > marshal_type;

	static int parse(Domain::Xml::Hvmsparc& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hvmsparc& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hvms390 traits

template<>
struct Traits<Domain::Xml::Hvms390>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EArch2, Name::Strict<277> > >, Optional<Attribute<Domain::Xml::EMachine3, Name::Strict<278> > > > > marshal_type;

	static int parse(Domain::Xml::Hvms390& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hvms390& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hvmarm traits

template<>
struct Traits<Domain::Xml::Hvmarm>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EArch3, Name::Strict<277> > >, Optional<Attribute<Domain::Xml::PMachine, Name::Strict<278> > > > > marshal_type;

	static int parse(Domain::Xml::Hvmarm& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hvmarm& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hvmaarch64 traits

template<>
struct Traits<Domain::Xml::Hvmaarch64>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EArch4, Name::Strict<277> > >, Optional<Attribute<Domain::Xml::PMachine, Name::Strict<278> > > > > marshal_type;

	static int parse(Domain::Xml::Hvmaarch64& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hvmaarch64& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Loader traits

template<>
struct Traits<Domain::Xml::Loader>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EReadonly, Name::Strict<266> > >, Optional<Attribute<Domain::Xml::EType2, Name::Strict<100> > >, Text<Domain::Xml::PAbsFilePath > > > marshal_type;

	static int parse(Domain::Xml::Loader& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Loader& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Nvram traits

template<>
struct Traits<Domain::Xml::Nvram>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<270> > >, Optional<Attribute<Domain::Xml::EFormat, Name::Strict<141> > >, Optional<Text<Domain::Xml::PAbsFilePath > > > > marshal_type;

	static int parse(Domain::Xml::Nvram& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Nvram& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Bootmenu traits

template<>
struct Traits<Domain::Xml::Bootmenu>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EVirYesNo, Name::Strict<273> >, Optional<Attribute<Domain::Xml::PUnsignedShort, Name::Strict<274> > > > > marshal_type;

	static int parse(Domain::Xml::Bootmenu& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Bootmenu& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Bios traits

template<>
struct Traits<Domain::Xml::Bios>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<1010> > >, Optional<Attribute<Domain::Xml::PRebootTimeoutDelay, Name::Strict<1011> > > > > marshal_type;

	static int parse(Domain::Xml::Bios& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Bios& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Os2 traits

template<>
struct Traits<Domain::Xml::Os2>
{
	typedef Ordered<mpl::vector<Element<Ordered<mpl::vector<Optional<Domain::Xml::VChoice297Impl >, Text<mpl::int_<299> > > >, Name::Strict<100> >, Unordered<mpl::vector<Optional<Element<Domain::Xml::Loader, Name::Strict<265> > >, Optional<Element<Domain::Xml::Nvram, Name::Strict<269> > >, Optional<Fragment<Domain::Xml::Osbootkernel > >, ZeroOrMore<Element<Attribute<Domain::Xml::EDev, Name::Strict<434> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Bootmenu, Name::Strict<272> > >, Optional<Element<Attribute<Domain::Xml::EMode1, Name::Strict<371> >, Name::Strict<275> > >, Optional<Element<Domain::Xml::Bios, Name::Strict<276> > > > > > > marshal_type;

	static int parse(Domain::Xml::Os2& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Os2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Os3 traits

template<>
struct Traits<Domain::Xml::Os3>
{
	typedef Ordered<mpl::vector<Element<Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EArch5, Name::Strict<277> > >, Text<mpl::int_<309> > > >, Name::Strict<100> >, Unordered<mpl::vector<Optional<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<310> > >, ZeroOrMore<Element<Text<QString >, Name::Strict<311> > > > > > > marshal_type;

	static int parse(Domain::Xml::Os3& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Os3& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Clock387 traits

template<>
struct Traits<Domain::Xml::Clock387>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EOffset, Name::Strict<381> >, Optional<Attribute<Domain::Xml::VAdjustment, Name::Strict<384> > > > > marshal_type;

	static int parse(Domain::Xml::Clock387& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Clock387& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Clock393 traits

template<>
struct Traits<Domain::Xml::Clock393>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<392>, Name::Strict<381> >, Optional<Attribute<Domain::Xml::PTimeDelta, Name::Strict<384> > >, Optional<Attribute<Domain::Xml::EBasis, Name::Strict<393> > > > > marshal_type;

	static int parse(Domain::Xml::Clock393& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Clock393& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Catchup traits

template<>
struct Traits<Domain::Xml::Catchup>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<423> > >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<424> > >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<425> > > > > marshal_type;

	static int parse(Domain::Xml::Catchup& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Catchup& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Timer402 traits

template<>
struct Traits<Domain::Xml::Timer402>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EName, Name::Strict<102> >, Optional<Attribute<Domain::Xml::ETrack, Name::Strict<398> > >, Optional<Domain::Xml::VTickpolicyImpl > > > marshal_type;

	static int parse(Domain::Xml::Timer402& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Timer402& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Timer409 traits

template<>
struct Traits<Domain::Xml::Timer409>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<404>, Name::Strict<102> >, Optional<Domain::Xml::VTickpolicyImpl >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<405> > >, Optional<Attribute<Domain::Xml::EMode2, Name::Strict<371> > > > > marshal_type;

	static int parse(Domain::Xml::Timer409& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Timer409& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Timer412 traits

template<>
struct Traits<Domain::Xml::Timer412>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EName1, Name::Strict<102> >, Optional<Domain::Xml::VTickpolicyImpl > > > marshal_type;

	static int parse(Domain::Xml::Timer412& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Timer412& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Timer traits

template<>
struct Traits<Domain::Xml::Timer>
{
	typedef Ordered<mpl::vector<Domain::Xml::VTimerImpl, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<417> > > > > marshal_type;

	static int parse(Domain::Xml::Timer& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Timer& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Clock traits

template<>
struct Traits<Domain::Xml::Clock>
{
	typedef Ordered<mpl::vector<Domain::Xml::VClockImpl, ZeroOrMore<Element<Domain::Xml::Timer, Name::Strict<395> > > > > marshal_type;

	static int parse(Domain::Xml::Clock& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Clock& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct ScaledInteger traits

template<>
struct Traits<Domain::Xml::ScaledInteger>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PUnit, Name::Strict<61> > >, Text<Domain::Xml::PUnsignedLong > > > marshal_type;

	static int parse(Domain::Xml::ScaledInteger& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::ScaledInteger& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Memory traits

template<>
struct Traits<Domain::Xml::Memory>
{
	typedef Ordered<mpl::vector<Fragment<Domain::Xml::ScaledInteger >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<319> > > > > marshal_type;

	static int parse(Domain::Xml::Memory& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Memory& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct MaxMemory traits

template<>
struct Traits<Domain::Xml::MaxMemory>
{
	typedef Ordered<mpl::vector<Fragment<Domain::Xml::ScaledInteger >, Attribute<Domain::Xml::PUnsignedInt, Name::Strict<321> > > > marshal_type;

	static int parse(Domain::Xml::MaxMemory& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::MaxMemory& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Page traits

template<>
struct Traits<Domain::Xml::Page>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUnsignedLong, Name::Strict<326> >, Optional<Attribute<Domain::Xml::PUnit, Name::Strict<61> > >, Optional<Attribute<Domain::Xml::PCpuset, Name::Strict<327> > > > > marshal_type;

	static int parse(Domain::Xml::Page& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Page& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct MemoryBacking traits

template<>
struct Traits<Domain::Xml::MemoryBacking>
{
	typedef Unordered<mpl::vector<Optional<Element<ZeroOrMore<Element<Domain::Xml::Page, Name::Strict<325> > >, Name::Strict<324> > >, Optional<Element<Empty, Name::Strict<328> > >, Optional<Element<Empty, Name::Strict<329> > > > > marshal_type;

	static int parse(Domain::Xml::MemoryBacking& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::MemoryBacking& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Vcpu traits

template<>
struct Traits<Domain::Xml::Vcpu>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EPlacement, Name::Strict<331> > >, Optional<Attribute<Domain::Xml::PCpuset, Name::Strict<64> > >, Optional<Attribute<Domain::Xml::PCountCPU, Name::Strict<333> > >, Text<Domain::Xml::PCountCPU > > > marshal_type;

	static int parse(Domain::Xml::Vcpu& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Vcpu& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Vcpu1 traits

template<>
struct Traits<Domain::Xml::Vcpu1>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<203> >, Attribute<Domain::Xml::EVirYesNo, Name::Strict<336> >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<337> > >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<338> > > > > marshal_type;

	static int parse(Domain::Xml::Vcpu1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Vcpu1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Device traits

template<>
struct Traits<Domain::Xml::Device>
{
	typedef Unordered<mpl::vector<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<347> >, Optional<Element<Text<Domain::Xml::PWeight >, Name::Strict<345> > >, Optional<Element<Text<Domain::Xml::PReadIopsSec >, Name::Strict<348> > >, Optional<Element<Text<Domain::Xml::PWriteIopsSec >, Name::Strict<349> > >, Optional<Element<Text<Domain::Xml::PReadBytesSec >, Name::Strict<350> > >, Optional<Element<Text<Domain::Xml::PWriteBytesSec >, Name::Strict<351> > > > > marshal_type;

	static int parse(Domain::Xml::Device& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Device& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Blkiotune traits

template<>
struct Traits<Domain::Xml::Blkiotune>
{
	typedef Unordered<mpl::vector<Optional<Element<Text<Domain::Xml::PWeight >, Name::Strict<345> > >, ZeroOrMore<Element<Domain::Xml::Device, Name::Strict<346> > > > > marshal_type;

	static int parse(Domain::Xml::Blkiotune& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Blkiotune& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Memtune traits

template<>
struct Traits<Domain::Xml::Memtune>
{
	typedef Ordered<mpl::vector<Optional<Element<Domain::Xml::ScaledInteger, Name::Strict<352> > >, Optional<Element<Domain::Xml::ScaledInteger, Name::Strict<353> > >, Optional<Element<Domain::Xml::ScaledInteger, Name::Strict<354> > >, Optional<Element<Domain::Xml::ScaledInteger, Name::Strict<355> > > > > marshal_type;

	static int parse(Domain::Xml::Memtune& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Memtune& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Vcpupin traits

template<>
struct Traits<Domain::Xml::Vcpupin>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PVcpuid, Name::Strict<330> >, Attribute<Domain::Xml::PCpuset, Name::Strict<64> > > > marshal_type;

	static int parse(Domain::Xml::Vcpupin& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Vcpupin& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Iothreadpin traits

template<>
struct Traits<Domain::Xml::Iothreadpin>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<370> >, Attribute<Domain::Xml::PCpuset, Name::Strict<64> > > > marshal_type;

	static int parse(Domain::Xml::Iothreadpin& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Iothreadpin& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cputune traits

template<>
struct Traits<Domain::Xml::Cputune>
{
	typedef Unordered<mpl::vector<Optional<Element<Text<Domain::Xml::PCpushares >, Name::Strict<356> > >, Optional<Element<Text<Domain::Xml::PCpuperiod >, Name::Strict<358> > >, Optional<Element<Text<Domain::Xml::PCpuquota >, Name::Strict<360> > >, Optional<Element<Text<Domain::Xml::PCpuperiod >, Name::Strict<362> > >, Optional<Element<Text<Domain::Xml::PCpuquota >, Name::Strict<363> > >, Optional<Element<Text<Domain::Xml::PCpuperiod >, Name::Strict<364> > >, Optional<Element<Text<Domain::Xml::PCpuquota >, Name::Strict<365> > >, ZeroOrMore<Element<Domain::Xml::Vcpupin, Name::Strict<366> > >, Optional<Element<Attribute<Domain::Xml::PCpuset, Name::Strict<64> >, Name::Strict<368> > >, ZeroOrMore<Element<Domain::Xml::Iothreadpin, Name::Strict<369> > > > > marshal_type;

	static int parse(Domain::Xml::Cputune& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Cputune& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Memory1137 traits

template<>
struct Traits<Domain::Xml::Memory1137>
{
	typedef Attribute<mpl::int_<332>, Name::Strict<331> > marshal_type;

	static int parse(Domain::Xml::Memory1137& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Memory1137& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Memory1 traits

template<>
struct Traits<Domain::Xml::Memory1>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EMode3, Name::Strict<371> > >, Domain::Xml::VMemoryImpl > > marshal_type;

	static int parse(Domain::Xml::Memory1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Memory1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Memnode traits

template<>
struct Traits<Domain::Xml::Memnode>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<378> >, Attribute<Domain::Xml::EMode4, Name::Strict<371> >, Attribute<Domain::Xml::PCpuset, Name::Strict<327> > > > marshal_type;

	static int parse(Domain::Xml::Memnode& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Memnode& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Numatune traits

template<>
struct Traits<Domain::Xml::Numatune>
{
	typedef Unordered<mpl::vector<Optional<Element<Domain::Xml::Memory1, Name::Strict<318> > >, ZeroOrMore<Element<Domain::Xml::Memnode, Name::Strict<377> > > > > marshal_type;

	static int parse(Domain::Xml::Numatune& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Numatune& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Apic traits

template<>
struct Traits<Domain::Xml::Apic>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<957> > > > > marshal_type;

	static int parse(Domain::Xml::Apic& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Apic& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Spinlocks traits

template<>
struct Traits<Domain::Xml::Spinlocks>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Optional<Attribute<Domain::Xml::PRetries, Name::Strict<1078> > > > > marshal_type;

	static int parse(Domain::Xml::Spinlocks& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Spinlocks& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct VendorId traits

template<>
struct Traits<Domain::Xml::VendorId>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Optional<Attribute<Domain::Xml::PValue, Name::Strict<1049> > > > > marshal_type;

	static int parse(Domain::Xml::VendorId& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::VendorId& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hyperv traits

template<>
struct Traits<Domain::Xml::Hyperv>
{
	typedef Unordered<mpl::vector<Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1075> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1076> > >, Optional<Element<Domain::Xml::Spinlocks, Name::Strict<1077> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1079> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1080> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1081> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1082> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<387> > >, Optional<Element<Domain::Xml::VendorId, Name::Strict<982> > > > > marshal_type;

	static int parse(Domain::Xml::Hyperv& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hyperv& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Kvm traits

template<>
struct Traits<Domain::Xml::Kvm>
{
	typedef Ordered<mpl::vector<Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1083> > > > > marshal_type;

	static int parse(Domain::Xml::Kvm& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Kvm& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Pvspinlock traits

template<>
struct Traits<Domain::Xml::Pvspinlock>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> > > > > marshal_type;

	static int parse(Domain::Xml::Pvspinlock& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Pvspinlock& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capabilities traits

template<>
struct Traits<Domain::Xml::Capabilities>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EPolicy1, Name::Strict<985> >, Unordered<mpl::vector<Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1085> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1086> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1087> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1088> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1089> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1090> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1091> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1092> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1093> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1094> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1095> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<457> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1096> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1097> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1098> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1099> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1100> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1101> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1102> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1103> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1104> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1105> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1106> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1107> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1108> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1109> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1110> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1111> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1112> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1113> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1114> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1115> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1116> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1117> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1118> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1119> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> >, Name::Strict<1120> > > > > > > marshal_type;

	static int parse(Domain::Xml::Capabilities& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Capabilities& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Pmu traits

template<>
struct Traits<Domain::Xml::Pmu>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> > > > > marshal_type;

	static int parse(Domain::Xml::Pmu& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Pmu& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Vmport traits

template<>
struct Traits<Domain::Xml::Vmport>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<121> > > > > marshal_type;

	static int parse(Domain::Xml::Vmport& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Vmport& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Gic traits

template<>
struct Traits<Domain::Xml::Gic>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<820> > > > > marshal_type;

	static int parse(Domain::Xml::Gic& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Gic& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Features traits

template<>
struct Traits<Domain::Xml::Features>
{
	typedef Unordered<mpl::vector<Optional<Element<Empty, Name::Strict<955> > >, Optional<Element<Domain::Xml::Apic, Name::Strict<956> > >, Optional<Element<Empty, Name::Strict<958> > >, Optional<Element<Empty, Name::Strict<959> > >, Optional<Element<Domain::Xml::Hyperv, Name::Strict<248> > >, Optional<Element<Empty, Name::Strict<960> > >, Optional<Element<Domain::Xml::Kvm, Name::Strict<241> > >, Optional<Element<Empty, Name::Strict<961> > >, Optional<Element<Domain::Xml::Pvspinlock, Name::Strict<962> > >, Optional<Element<Domain::Xml::Capabilities, Name::Strict<893> > >, Optional<Element<Domain::Xml::Pmu, Name::Strict<964> > >, Optional<Element<Domain::Xml::Vmport, Name::Strict<965> > >, Optional<Element<Domain::Xml::Gic, Name::Strict<966> > > > > marshal_type;

	static int parse(Domain::Xml::Features& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Features& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct SuspendToMem traits

template<>
struct Traits<Domain::Xml::SuspendToMem>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<336> > > > > marshal_type;

	static int parse(Domain::Xml::SuspendToMem& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::SuspendToMem& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct SuspendToDisk traits

template<>
struct Traits<Domain::Xml::SuspendToDisk>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<336> > > > > marshal_type;

	static int parse(Domain::Xml::SuspendToDisk& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::SuspendToDisk& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Pm traits

template<>
struct Traits<Domain::Xml::Pm>
{
	typedef Unordered<mpl::vector<Optional<Element<Domain::Xml::SuspendToMem, Name::Strict<780> > >, Optional<Element<Domain::Xml::SuspendToDisk, Name::Strict<782> > > > > marshal_type;

	static int parse(Domain::Xml::Pm& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Pm& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Uid traits

template<>
struct Traits<Domain::Xml::Uid>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<314> >, Attribute<Domain::Xml::PUnsignedInt, Name::Strict<315> >, Attribute<Domain::Xml::PUnsignedInt, Name::Strict<316> > > > marshal_type;

	static int parse(Domain::Xml::Uid& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Uid& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Gid traits

template<>
struct Traits<Domain::Xml::Gid>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<314> >, Attribute<Domain::Xml::PUnsignedInt, Name::Strict<315> >, Attribute<Domain::Xml::PUnsignedInt, Name::Strict<316> > > > marshal_type;

	static int parse(Domain::Xml::Gid& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Gid& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Idmap traits

template<>
struct Traits<Domain::Xml::Idmap>
{
	typedef Unordered<mpl::vector<ZeroOrMore<Element<Domain::Xml::Uid, Name::Strict<313> > >, ZeroOrMore<Element<Domain::Xml::Gid, Name::Strict<317> > > > > marshal_type;

	static int parse(Domain::Xml::Idmap& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Idmap& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk471 traits

template<>
struct Traits<Domain::Xml::Disk471>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EDevice1, Name::Strict<346> >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<1137> > >, Optional<Attribute<Domain::Xml::ESgio, Name::Strict<469> > > > > marshal_type;

	static int parse(Domain::Xml::Disk471& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Disk471& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel234 traits

template<>
struct Traits<Domain::Xml::Seclabel234>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<131>, Name::Strict<225> > > > marshal_type;

	static int parse(Domain::Xml::Seclabel234& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Seclabel234& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel236 traits

template<>
struct Traits<Domain::Xml::Seclabel236>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<130>, Name::Strict<236> > > > marshal_type;

	static int parse(Domain::Xml::Seclabel236& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Seclabel236& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel traits

template<>
struct Traits<Domain::Xml::Seclabel>
{
	typedef Ordered<mpl::vector<Optional<Attribute<QString, Name::Strict<223> > >, Domain::Xml::VSeclabelImpl > > marshal_type;

	static int parse(Domain::Xml::Seclabel& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Seclabel& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source traits

template<>
struct Traits<Domain::Xml::Source>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<434> >, Optional<Attribute<Domain::Xml::EStartupPolicy, Name::Strict<460> > >, Optional<Element<Domain::Xml::Seclabel, Name::Strict<221> > > > > marshal_type;

	static int parse(Domain::Xml::Source& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source1 traits

template<>
struct Traits<Domain::Xml::Source1>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<159> >, Optional<Attribute<Domain::Xml::EStartupPolicy, Name::Strict<460> > > > > marshal_type;

	static int parse(Domain::Xml::Source1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Host510 traits

template<>
struct Traits<Domain::Xml::Host510>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::ETransport, Name::Strict<506> > >, Attribute<Domain::Xml::VName, Name::Strict<102> >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<207> > > > > marshal_type;

	static int parse(Domain::Xml::Host510& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Host510& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source2 traits

template<>
struct Traits<Domain::Xml::Source2>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EProtocol, Name::Strict<198> >, Optional<Attribute<QString, Name::Strict<102> > >, ZeroOrMore<Element<Domain::Xml::VHostImpl, Name::Strict<505> > > > > marshal_type;

	static int parse(Domain::Xml::Source2& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source3 traits

template<>
struct Traits<Domain::Xml::Source3>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PGenericName, Name::Strict<516> >, Attribute<Domain::Xml::PVolName, Name::Strict<515> >, Optional<Attribute<Domain::Xml::EMode5, Name::Strict<371> > >, Optional<Attribute<Domain::Xml::EStartupPolicy, Name::Strict<460> > >, Optional<Element<Domain::Xml::Seclabel, Name::Strict<221> > > > > marshal_type;

	static int parse(Domain::Xml::Source3& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source3& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source4 traits

template<>
struct Traits<Domain::Xml::Source4>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<492> > >, Optional<Attribute<Domain::Xml::EStartupPolicy, Name::Strict<460> > >, Optional<Element<Domain::Xml::Seclabel, Name::Strict<221> > > > > marshal_type;

	static int parse(Domain::Xml::Source4& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source4& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct DriverFormat traits

template<>
struct Traits<Domain::Xml::DriverFormat>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PGenericName, Name::Strict<102> >, Optional<Attribute<Domain::Xml::VType, Name::Strict<100> > > > > marshal_type;

	static int parse(Domain::Xml::DriverFormat& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::DriverFormat& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver traits

template<>
struct Traits<Domain::Xml::Driver>
{
	typedef Ordered<mpl::vector<Optional<Fragment<Domain::Xml::DriverFormat > >, Optional<Attribute<Domain::Xml::ECache, Name::Strict<550> > >, Optional<Attribute<Domain::Xml::EErrorPolicy, Name::Strict<555> > >, Optional<Attribute<Domain::Xml::ERerrorPolicy, Name::Strict<560> > >, Optional<Attribute<Domain::Xml::EIo, Name::Strict<561> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<544> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<545> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<546> > >, Optional<Attribute<Domain::Xml::EDiscard, Name::Strict<420> > >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<370> > > > > marshal_type;

	static int parse(Domain::Xml::Driver& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Driver& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1138 traits

template<>
struct Traits<Domain::Xml::Anonymous1138>
{
	typedef Unordered<mpl::vector<Ordered<mpl::vector<Optional<Attribute<mpl::int_<492>, Name::Strict<100> > >, Optional<Element<Domain::Xml::Source4, Name::Strict<493> > > > >, Optional<Element<Attribute<Domain::Xml::VStorageFormat, Name::Strict<100> >, Name::Strict<141> > > > > marshal_type;

	static int parse(Domain::Xml::Anonymous1138& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Anonymous1138& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mirror1053 traits

template<>
struct Traits<Domain::Xml::Mirror1053>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<492> >, Optional<Attribute<Domain::Xml::VStorageFormat, Name::Strict<141> > >, Optional<Attribute<Domain::Xml::EJob, Name::Strict<1052> > >, Optional<Fragment<Domain::Xml::Anonymous1138 > > > > marshal_type;

	static int parse(Domain::Xml::Mirror1053& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Mirror1053& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mirror1055 traits

template<>
struct Traits<Domain::Xml::Mirror1055>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EJob1, Name::Strict<1052> >, Unordered<mpl::vector<Domain::Xml::VDiskSourceImpl, Optional<Element<Attribute<Domain::Xml::VStorageFormat, Name::Strict<100> >, Name::Strict<141> > > > > > > marshal_type;

	static int parse(Domain::Xml::Mirror1055& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Mirror1055& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mirror traits

template<>
struct Traits<Domain::Xml::Mirror>
{
	typedef Ordered<mpl::vector<Domain::Xml::VMirrorImpl, Optional<Attribute<Domain::Xml::EReady, Name::Strict<1057> > > > > marshal_type;

	static int parse(Domain::Xml::Mirror& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Mirror& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Secret traits

template<>
struct Traits<Domain::Xml::Secret>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EType4, Name::Strict<100> >, Domain::Xml::VSecretImpl > > marshal_type;

	static int parse(Domain::Xml::Secret& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Secret& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Auth traits

template<>
struct Traits<Domain::Xml::Auth>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PGenericName, Name::Strict<1061> >, Element<Domain::Xml::Secret, Name::Strict<144> > > > marshal_type;

	static int parse(Domain::Xml::Auth& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Auth& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Target traits

template<>
struct Traits<Domain::Xml::Target>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PDiskTarget, Name::Strict<434> >, Optional<Attribute<Domain::Xml::EBus, Name::Strict<24> > >, Optional<Attribute<Domain::Xml::ETray, Name::Strict<526> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<529> > > > > marshal_type;

	static int parse(Domain::Xml::Target& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Target& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Secret1 traits

template<>
struct Traits<Domain::Xml::Secret1>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EType5, Name::Strict<100> >, Attribute<Domain::Xml::VUUID, Name::Strict<146> > > > marshal_type;

	static int parse(Domain::Xml::Secret1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Secret1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Encryption traits

template<>
struct Traits<Domain::Xml::Encryption>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EFormat1, Name::Strict<141> >, ZeroOrMore<Element<Domain::Xml::Secret1, Name::Strict<144> > > > > marshal_type;

	static int parse(Domain::Xml::Encryption& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Encryption& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant1068 traits

template<>
struct Traits<Domain::Xml::Variant1068>
{
	typedef Ordered<mpl::vector<Unordered<mpl::vector<Optional<Element<Text<Domain::Xml::PReadBytesSec >, Name::Strict<350> > >, Optional<Element<Text<Domain::Xml::PWriteBytesSec >, Name::Strict<351> > > > > > > marshal_type;

	static int parse(Domain::Xml::Variant1068& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Variant1068& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant1072 traits

template<>
struct Traits<Domain::Xml::Variant1072>
{
	typedef Ordered<mpl::vector<Unordered<mpl::vector<Optional<Element<Text<Domain::Xml::PReadIopsSec1 >, Name::Strict<348> > >, Optional<Element<Text<Domain::Xml::PWriteIopsSec1 >, Name::Strict<349> > > > > > > marshal_type;

	static int parse(Domain::Xml::Variant1072& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Variant1072& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Iotune traits

template<>
struct Traits<Domain::Xml::Iotune>
{
	typedef Unordered<mpl::vector<Domain::Xml::VChoice1069Impl, Domain::Xml::VChoice1073Impl > > marshal_type;

	static int parse(Domain::Xml::Iotune& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Iotune& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Pciaddress traits

template<>
struct Traits<Domain::Xml::Pciaddress>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PPciDomain, Name::Strict<1> > >, Attribute<Domain::Xml::PPciBus, Name::Strict<24> >, Attribute<Domain::Xml::PPciSlot, Name::Strict<26> >, Attribute<Domain::Xml::PPciFunc, Name::Strict<28> >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<30> > > > > marshal_type;

	static int parse(Domain::Xml::Pciaddress& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Pciaddress& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Driveaddress traits

template<>
struct Traits<Domain::Xml::Driveaddress>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PDriveController, Name::Strict<564> > >, Optional<Attribute<Domain::Xml::PDriveBus, Name::Strict<24> > >, Optional<Attribute<Domain::Xml::PDriveTarget, Name::Strict<315> > >, Optional<Attribute<Domain::Xml::PDriveUnit, Name::Strict<61> > > > > marshal_type;

	static int parse(Domain::Xml::Driveaddress& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Driveaddress& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Virtioserialaddress traits

template<>
struct Traits<Domain::Xml::Virtioserialaddress>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PDriveController, Name::Strict<564> >, Optional<Attribute<Domain::Xml::PDriveBus, Name::Strict<24> > >, Optional<Attribute<Domain::Xml::PDriveUnit, Name::Strict<207> > > > > marshal_type;

	static int parse(Domain::Xml::Virtioserialaddress& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Virtioserialaddress& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Ccidaddress traits

template<>
struct Traits<Domain::Xml::Ccidaddress>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PDriveController, Name::Strict<564> >, Optional<Attribute<Domain::Xml::PDriveUnit, Name::Strict<26> > > > > marshal_type;

	static int parse(Domain::Xml::Ccidaddress& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Ccidaddress& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Usbportaddress traits

template<>
struct Traits<Domain::Xml::Usbportaddress>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUsbAddr, Name::Strict<24> >, Attribute<Domain::Xml::PUsbPort, Name::Strict<207> > > > marshal_type;

	static int parse(Domain::Xml::Usbportaddress& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Usbportaddress& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1139 traits

template<>
struct Traits<Domain::Xml::Anonymous1139>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::VCcwCssidRange, Name::Strict<917> >, Attribute<Domain::Xml::PCcwSsidRange, Name::Strict<919> >, Attribute<Domain::Xml::VCcwDevnoRange, Name::Strict<921> > > > marshal_type;

	static int parse(Domain::Xml::Anonymous1139& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Anonymous1139& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Isaaddress traits

template<>
struct Traits<Domain::Xml::Isaaddress>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PIobase, Name::Strict<116> > >, Optional<Attribute<Domain::Xml::PIrq, Name::Strict<117> > > > > marshal_type;

	static int parse(Domain::Xml::Isaaddress& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Isaaddress& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Dimmaddress traits

template<>
struct Traits<Domain::Xml::Dimmaddress>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<26> > >, Optional<Attribute<Domain::Xml::PHexuint, Name::Strict<928> > > > > marshal_type;

	static int parse(Domain::Xml::Dimmaddress& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Dimmaddress& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Geometry traits

template<>
struct Traits<Domain::Xml::Geometry>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PCyls, Name::Strict<530> >, Attribute<Domain::Xml::PHeads, Name::Strict<531> >, Attribute<Domain::Xml::PSecs, Name::Strict<532> >, Optional<Attribute<Domain::Xml::ETrans, Name::Strict<533> > > > > marshal_type;

	static int parse(Domain::Xml::Geometry& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Geometry& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Blockio traits

template<>
struct Traits<Domain::Xml::Blockio>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PLogicalBlockSize, Name::Strict<536> > >, Optional<Attribute<Domain::Xml::PPhysicalBlockSize, Name::Strict<537> > > > > marshal_type;

	static int parse(Domain::Xml::Blockio& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Blockio& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct BackingStore traits

template<>
struct Traits<Domain::Xml::BackingStore>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<480> >, Unordered<mpl::vector<Domain::Xml::VDiskSourceImpl, Domain::Xml::VDiskBackingChainImpl, Element<Attribute<Domain::Xml::VStorageFormat, Name::Strict<100> >, Name::Strict<141> > > > > > marshal_type;

	static int parse(Domain::Xml::BackingStore& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::BackingStore& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk traits

template<>
struct Traits<Domain::Xml::Disk>
{
	typedef Ordered<mpl::vector<Domain::Xml::VDiskImpl, Optional<Attribute<Domain::Xml::ESnapshot, Name::Strict<454> > >, Unordered<mpl::vector<Domain::Xml::VDiskSourceImpl, Optional<Element<Domain::Xml::Driver, Name::Strict<538> > >, Optional<Element<Domain::Xml::Mirror, Name::Strict<1051> > >, Optional<Element<Domain::Xml::Auth, Name::Strict<1060> > >, Element<Domain::Xml::Target, Name::Strict<315> >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Empty, Name::Strict<266> > >, Optional<Element<Empty, Name::Strict<444> > >, Optional<Element<Empty, Name::Strict<445> > >, Optional<Element<Text<Domain::Xml::PDiskSerial >, Name::Strict<446> > >, Optional<Element<Domain::Xml::Encryption, Name::Strict<140> > >, Optional<Element<Domain::Xml::Iotune, Name::Strict<1066> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Domain::Xml::Geometry, Name::Strict<450> > >, Optional<Element<Domain::Xml::Blockio, Name::Strict<535> > >, Optional<Element<Text<Domain::Xml::PWwn >, Name::Strict<63> > >, Optional<Element<Text<Domain::Xml::PVendor >, Name::Strict<452> > >, Optional<Element<Text<Domain::Xml::PProduct >, Name::Strict<453> > >, Domain::Xml::VDiskBackingChainImpl > > > > marshal_type;

	static int parse(Domain::Xml::Disk& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Disk& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant586 traits

template<>
struct Traits<Domain::Xml::Variant586>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<523>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::EModel1, Name::Strict<223> > >, Optional<Element<Attribute<Domain::Xml::PUsbPort, Name::Strict<1046> >, Name::Strict<826> > > > > marshal_type;

	static int parse(Domain::Xml::Variant586& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Variant586& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant591 traits

template<>
struct Traits<Domain::Xml::Variant591>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EModel2, Name::Strict<223> >, Optional<Element<Domain::Xml::ScaledInteger, Name::Strict<591> > > > > marshal_type;

	static int parse(Domain::Xml::Variant591& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Variant591& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant600 traits

template<>
struct Traits<Domain::Xml::Variant600>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<598>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<599> > >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<600> > > > > marshal_type;

	static int parse(Domain::Xml::Variant600& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Variant600& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver1 traits

template<>
struct Traits<Domain::Xml::Driver1>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<603> > >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<604> > >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<605> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<544> > >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<370> > > > > marshal_type;

	static int parse(Domain::Xml::Driver1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Driver1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Controller traits

template<>
struct Traits<Domain::Xml::Controller>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<480> >, Unordered<mpl::vector<Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Domain::Xml::VChoice601Impl, Optional<Element<Domain::Xml::Driver1, Name::Strict<538> > > > > > > marshal_type;

	static int parse(Domain::Xml::Controller& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Controller& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Target1 traits

template<>
struct Traits<Domain::Xml::Target1>
{
	typedef Ordered<mpl::vector<Attribute<QString, Name::Strict<347> >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<381> > > > > marshal_type;

	static int parse(Domain::Xml::Target1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Target1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Lease traits

template<>
struct Traits<Domain::Xml::Lease>
{
	typedef Unordered<mpl::vector<Element<Text<QString >, Name::Strict<458> >, Element<Text<QString >, Name::Strict<459> >, Element<Domain::Xml::Target1, Name::Strict<315> > > > marshal_type;

	static int parse(Domain::Xml::Lease& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Lease& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver2 traits

template<>
struct Traits<Domain::Xml::Driver2>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EType7, Name::Strict<100> > >, Optional<Attribute<Domain::Xml::VStorageFormat, Name::Strict<141> > >, Optional<Attribute<mpl::int_<628>, Name::Strict<627> > > > > marshal_type;

	static int parse(Domain::Xml::Driver2& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Driver2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem607 traits

template<>
struct Traits<Domain::Xml::Filesystem607>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<492>, Name::Strict<100> >, Optional<Element<Domain::Xml::Driver2, Name::Strict<538> > >, Element<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<492> >, Name::Strict<493> > > > marshal_type;

	static int parse(Domain::Xml::Filesystem607& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Filesystem607& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem608 traits

template<>
struct Traits<Domain::Xml::Filesystem608>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<494>, Name::Strict<100> >, Optional<Element<Domain::Xml::Driver2, Name::Strict<538> > >, Element<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<434> >, Name::Strict<493> > > > marshal_type;

	static int parse(Domain::Xml::Filesystem608& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Filesystem608& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem610 traits

template<>
struct Traits<Domain::Xml::Filesystem610>
{
	typedef Ordered<mpl::vector<Optional<Attribute<mpl::int_<610>, Name::Strict<100> > >, Optional<Element<Domain::Xml::Driver2, Name::Strict<538> > >, Element<Attribute<Domain::Xml::PAbsDirPath, Name::Strict<159> >, Name::Strict<493> > > > marshal_type;

	static int parse(Domain::Xml::Filesystem610& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Filesystem610& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem612 traits

template<>
struct Traits<Domain::Xml::Filesystem612>
{
	typedef Ordered<mpl::vector<Optional<Attribute<mpl::int_<612>, Name::Strict<100> > >, Optional<Element<Domain::Xml::Driver2, Name::Strict<538> > >, Element<Attribute<Domain::Xml::PAbsDirPath, Name::Strict<159> >, Name::Strict<493> > > > marshal_type;

	static int parse(Domain::Xml::Filesystem612& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Filesystem612& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem613 traits

template<>
struct Traits<Domain::Xml::Filesystem613>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<270>, Name::Strict<100> >, Optional<Element<Domain::Xml::Driver2, Name::Strict<538> > >, Element<Attribute<Domain::Xml::PGenericName, Name::Strict<102> >, Name::Strict<493> > > > marshal_type;

	static int parse(Domain::Xml::Filesystem613& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Filesystem613& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source5 traits

template<>
struct Traits<Domain::Xml::Source5>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUnsignedLong, Name::Strict<616> >, Optional<Attribute<Domain::Xml::PUnit, Name::Strict<617> > > > > marshal_type;

	static int parse(Domain::Xml::Source5& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source5& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem617 traits

template<>
struct Traits<Domain::Xml::Filesystem617>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<615>, Name::Strict<100> >, Optional<Element<Domain::Xml::Driver2, Name::Strict<538> > >, Element<Domain::Xml::Source5, Name::Strict<493> > > > marshal_type;

	static int parse(Domain::Xml::Filesystem617& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Filesystem617& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem traits

template<>
struct Traits<Domain::Xml::Filesystem>
{
	typedef Ordered<mpl::vector<Domain::Xml::VFilesystemImpl, Unordered<mpl::vector<Element<Attribute<Domain::Xml::PAbsDirPath, Name::Strict<159> >, Name::Strict<315> >, Optional<Attribute<Domain::Xml::EAccessmode, Name::Strict<619> > >, Optional<Element<Empty, Name::Strict<266> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > >, Unordered<mpl::vector<Optional<Element<Domain::Xml::ScaledInteger, Name::Strict<623> > >, Optional<Element<Domain::Xml::ScaledInteger, Name::Strict<624> > > > > > > marshal_type;

	static int parse(Domain::Xml::Filesystem& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Filesystem& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source6 traits

template<>
struct Traits<Domain::Xml::Source6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PDeviceName, Name::Strict<630> > > > > marshal_type;

	static int parse(Domain::Xml::Source6& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameters traits

template<>
struct Traits<Domain::Xml::Parameters>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::VUint8range, Name::Strict<176> > >, Optional<Attribute<Domain::Xml::VUint24range, Name::Strict<177> > >, Optional<Attribute<Domain::Xml::VUint8range, Name::Strict<178> > >, Optional<Attribute<Domain::Xml::VUUID, Name::Strict<179> > > > > marshal_type;

	static int parse(Domain::Xml::Parameters& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Parameters& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameters1 traits

template<>
struct Traits<Domain::Xml::Parameters1>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PVirtualPortProfileID, Name::Strict<182> > > > > marshal_type;

	static int parse(Domain::Xml::Parameters1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Parameters1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameters2 traits

template<>
struct Traits<Domain::Xml::Parameters2>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PVirtualPortProfileID, Name::Strict<182> > >, Optional<Attribute<Domain::Xml::VUUID, Name::Strict<185> > > > > marshal_type;

	static int parse(Domain::Xml::Parameters2& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Parameters2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameters3 traits

template<>
struct Traits<Domain::Xml::Parameters3>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::VUint8range, Name::Strict<176> > >, Optional<Attribute<Domain::Xml::VUint24range, Name::Strict<177> > >, Optional<Attribute<Domain::Xml::VUint8range, Name::Strict<178> > >, Optional<Attribute<Domain::Xml::VUUID, Name::Strict<179> > >, Optional<Attribute<Domain::Xml::PVirtualPortProfileID, Name::Strict<182> > >, Optional<Attribute<Domain::Xml::VUUID, Name::Strict<185> > > > > marshal_type;

	static int parse(Domain::Xml::Parameters3& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Parameters3& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Virtualport traits

template<>
struct Traits<Domain::Xml::Virtualport>
{
	typedef Ordered<mpl::vector<Optional<Element<Domain::Xml::Parameters3, Name::Strict<175> > > > > marshal_type;

	static int parse(Domain::Xml::Virtualport& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Virtualport& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Guest traits

template<>
struct Traits<Domain::Xml::Guest>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> > >, Optional<Attribute<Domain::Xml::PDeviceName, Name::Strict<662> > > > > marshal_type;

	static int parse(Domain::Xml::Guest& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Guest& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Ip traits

template<>
struct Traits<Domain::Xml::Ip>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::VIpAddr, Name::Strict<106> >, Optional<Attribute<Domain::Xml::PAddrFamily, Name::Strict<664> > >, Optional<Attribute<Domain::Xml::VIpPrefix, Name::Strict<665> > > > > marshal_type;

	static int parse(Domain::Xml::Ip& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Ip& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Backend traits

template<>
struct Traits<Domain::Xml::Backend>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<668> > >, Optional<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<669> > > > > marshal_type;

	static int parse(Domain::Xml::Backend& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Backend& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver672 traits

template<>
struct Traits<Domain::Xml::Driver672>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EName4, Name::Strict<102> > >, Optional<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<603> > >, Optional<Attribute<Domain::Xml::ETxmode, Name::Strict<672> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<544> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<545> > > > > marshal_type;

	static int parse(Domain::Xml::Driver672& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Driver672& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Host traits

template<>
struct Traits<Domain::Xml::Host>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<674> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<675> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<676> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<677> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<678> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<679> > > > > marshal_type;

	static int parse(Domain::Xml::Host& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Host& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Guest1 traits

template<>
struct Traits<Domain::Xml::Guest1>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<674> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<676> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<677> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<678> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<679> > > > > marshal_type;

	static int parse(Domain::Xml::Guest1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Guest1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver3 traits

template<>
struct Traits<Domain::Xml::Driver3>
{
	typedef Ordered<mpl::vector<Domain::Xml::VDriverImpl, Unordered<mpl::vector<Optional<Element<Domain::Xml::Host, Name::Strict<505> > >, Optional<Element<Domain::Xml::Guest1, Name::Strict<400> > > > > > > marshal_type;

	static int parse(Domain::Xml::Driver3& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Driver3& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameter traits

template<>
struct Traits<Domain::Xml::Parameter>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PFilterParamName, Name::Strict<102> >, Attribute<Domain::Xml::PFilterParamValue, Name::Strict<1049> > > > marshal_type;

	static int parse(Domain::Xml::Parameter& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Parameter& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct FilterrefNodeAttributes traits

template<>
struct Traits<Domain::Xml::FilterrefNodeAttributes>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PFilter, Name::Strict<738> >, ZeroOrMore<Element<Domain::Xml::Parameter, Name::Strict<1047> > > > > marshal_type;

	static int parse(Domain::Xml::FilterrefNodeAttributes& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::FilterrefNodeAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Tune traits

template<>
struct Traits<Domain::Xml::Tune>
{
	typedef Ordered<mpl::vector<Optional<Element<Text<Domain::Xml::PUnsignedInt >, Name::Strict<683> > > > > marshal_type;

	static int parse(Domain::Xml::Tune& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Tune& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Rom traits

template<>
struct Traits<Domain::Xml::Rom>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<1029> > >, Optional<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<492> > > > > marshal_type;

	static int parse(Domain::Xml::Rom& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Rom& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct BandwidthAttributes traits

template<>
struct Traits<Domain::Xml::BandwidthAttributes>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PSpeed, Name::Strict<192> >, Optional<Attribute<Domain::Xml::PSpeed, Name::Strict<193> > >, Optional<Attribute<Domain::Xml::PSpeed, Name::Strict<194> > >, Optional<Attribute<Domain::Xml::PBurstSize, Name::Strict<195> > > > > marshal_type;

	static int parse(Domain::Xml::BandwidthAttributes& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::BandwidthAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Bandwidth traits

template<>
struct Traits<Domain::Xml::Bandwidth>
{
	typedef Unordered<mpl::vector<Optional<Element<Domain::Xml::BandwidthAttributes, Name::Strict<189> > >, Optional<Element<Domain::Xml::BandwidthAttributes, Name::Strict<191> > > > > marshal_type;

	static int parse(Domain::Xml::Bandwidth& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Bandwidth& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Tag traits

template<>
struct Traits<Domain::Xml::Tag>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PId, Name::Strict<203> >, Optional<Attribute<Domain::Xml::ENativeMode, Name::Strict<204> > > > > marshal_type;

	static int parse(Domain::Xml::Tag& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Tag& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface631 traits

template<>
struct Traits<Domain::Xml::Interface631>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<630>, Name::Strict<100> >, Unordered<mpl::vector<Optional<Element<Domain::Xml::Source6, Name::Strict<493> > >, Optional<Domain::Xml::VVirtualPortProfileImpl >, Optional<Element<Attribute<Domain::Xml::EState, Name::Strict<121> >, Name::Strict<119> > >, Optional<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Name::Strict<315> > >, Optional<Element<Domain::Xml::Guest, Name::Strict<400> > >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > >, ZeroOrMore<Element<Domain::Xml::Ip, Name::Strict<663> > >, Optional<Element<Attribute<Domain::Xml::PFilePath, Name::Strict<347> >, Name::Strict<666> > >, Optional<Element<Attribute<Domain::Xml::PType, Name::Strict<100> >, Name::Strict<223> > >, Optional<Element<Domain::Xml::Backend, Name::Strict<667> > >, Optional<Element<Domain::Xml::Driver3, Name::Strict<538> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Domain::Xml::FilterrefNodeAttributes, Name::Strict<680> > >, Optional<Element<Domain::Xml::Tune, Name::Strict<682> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Rom, Name::Strict<267> > >, Optional<Element<Domain::Xml::Bandwidth, Name::Strict<188> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<130>, Name::Strict<201> > >, OneOrMore<Element<Domain::Xml::Tag, Name::Strict<202> > > > >, Name::Strict<200> > > > > > > marshal_type;

	static int parse(Domain::Xml::Interface631& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Interface631& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface633 traits

template<>
struct Traits<Domain::Xml::Interface633>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<633>, Name::Strict<100> >, Unordered<mpl::vector<Optional<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Name::Strict<493> > >, Optional<Element<Attribute<Domain::Xml::EState, Name::Strict<121> >, Name::Strict<119> > >, Optional<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Name::Strict<315> > >, Optional<Element<Domain::Xml::Guest, Name::Strict<400> > >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > >, ZeroOrMore<Element<Domain::Xml::Ip, Name::Strict<663> > >, Optional<Element<Attribute<Domain::Xml::PFilePath, Name::Strict<347> >, Name::Strict<666> > >, Optional<Element<Attribute<Domain::Xml::PType, Name::Strict<100> >, Name::Strict<223> > >, Optional<Element<Domain::Xml::Backend, Name::Strict<667> > >, Optional<Element<Domain::Xml::Driver3, Name::Strict<538> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Domain::Xml::FilterrefNodeAttributes, Name::Strict<680> > >, Optional<Element<Domain::Xml::Tune, Name::Strict<682> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Rom, Name::Strict<267> > >, Optional<Element<Domain::Xml::Bandwidth, Name::Strict<188> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<130>, Name::Strict<201> > >, OneOrMore<Element<Domain::Xml::Tag, Name::Strict<202> > > > >, Name::Strict<200> > > > > > > marshal_type;

	static int parse(Domain::Xml::Interface633& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Interface633& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source7 traits

template<>
struct Traits<Domain::Xml::Source7>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EType8, Name::Strict<100> >, Attribute<Domain::Xml::PAbsFilePath, Name::Strict<347> >, Attribute<Domain::Xml::EMode6, Name::Strict<371> > > > marshal_type;

	static int parse(Domain::Xml::Source7& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source7& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface637 traits

template<>
struct Traits<Domain::Xml::Interface637>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<635>, Name::Strict<100> >, Unordered<mpl::vector<Element<Domain::Xml::Source7, Name::Strict<493> >, Optional<Element<Attribute<Domain::Xml::EState, Name::Strict<121> >, Name::Strict<119> > >, Optional<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Name::Strict<315> > >, Optional<Element<Domain::Xml::Guest, Name::Strict<400> > >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > >, ZeroOrMore<Element<Domain::Xml::Ip, Name::Strict<663> > >, Optional<Element<Attribute<Domain::Xml::PFilePath, Name::Strict<347> >, Name::Strict<666> > >, Optional<Element<Attribute<Domain::Xml::PType, Name::Strict<100> >, Name::Strict<223> > >, Optional<Element<Domain::Xml::Backend, Name::Strict<667> > >, Optional<Element<Domain::Xml::Driver3, Name::Strict<538> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Domain::Xml::FilterrefNodeAttributes, Name::Strict<680> > >, Optional<Element<Domain::Xml::Tune, Name::Strict<682> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Rom, Name::Strict<267> > >, Optional<Element<Domain::Xml::Bandwidth, Name::Strict<188> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<130>, Name::Strict<201> > >, OneOrMore<Element<Domain::Xml::Tag, Name::Strict<202> > > > >, Name::Strict<200> > > > > > > marshal_type;

	static int parse(Domain::Xml::Interface637& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Interface637& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source8 traits

template<>
struct Traits<Domain::Xml::Source8>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PDeviceName, Name::Strict<438> >, Optional<Attribute<Domain::Xml::PDeviceName, Name::Strict<639> > > > > marshal_type;

	static int parse(Domain::Xml::Source8& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source8& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface639 traits

template<>
struct Traits<Domain::Xml::Interface639>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<438>, Name::Strict<100> >, Unordered<mpl::vector<Element<Domain::Xml::Source8, Name::Strict<493> >, Optional<Domain::Xml::VVirtualPortProfileImpl >, Optional<Element<Attribute<Domain::Xml::EState, Name::Strict<121> >, Name::Strict<119> > >, Optional<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Name::Strict<315> > >, Optional<Element<Domain::Xml::Guest, Name::Strict<400> > >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > >, ZeroOrMore<Element<Domain::Xml::Ip, Name::Strict<663> > >, Optional<Element<Attribute<Domain::Xml::PFilePath, Name::Strict<347> >, Name::Strict<666> > >, Optional<Element<Attribute<Domain::Xml::PType, Name::Strict<100> >, Name::Strict<223> > >, Optional<Element<Domain::Xml::Backend, Name::Strict<667> > >, Optional<Element<Domain::Xml::Driver3, Name::Strict<538> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Domain::Xml::FilterrefNodeAttributes, Name::Strict<680> > >, Optional<Element<Domain::Xml::Tune, Name::Strict<682> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Rom, Name::Strict<267> > >, Optional<Element<Domain::Xml::Bandwidth, Name::Strict<188> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<130>, Name::Strict<201> > >, OneOrMore<Element<Domain::Xml::Tag, Name::Strict<202> > > > >, Name::Strict<200> > > > > > > marshal_type;

	static int parse(Domain::Xml::Interface639& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Interface639& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source9 traits

template<>
struct Traits<Domain::Xml::Source9>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Optional<Attribute<Domain::Xml::PBridgeMode, Name::Strict<371> > > > > marshal_type;

	static int parse(Domain::Xml::Source9& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source9& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface641 traits

template<>
struct Traits<Domain::Xml::Interface641>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<517>, Name::Strict<100> >, Unordered<mpl::vector<Element<Domain::Xml::Source9, Name::Strict<493> >, Optional<Domain::Xml::VVirtualPortProfileImpl >, Optional<Element<Attribute<Domain::Xml::EState, Name::Strict<121> >, Name::Strict<119> > >, Optional<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Name::Strict<315> > >, Optional<Element<Domain::Xml::Guest, Name::Strict<400> > >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > >, ZeroOrMore<Element<Domain::Xml::Ip, Name::Strict<663> > >, Optional<Element<Attribute<Domain::Xml::PFilePath, Name::Strict<347> >, Name::Strict<666> > >, Optional<Element<Attribute<Domain::Xml::PType, Name::Strict<100> >, Name::Strict<223> > >, Optional<Element<Domain::Xml::Backend, Name::Strict<667> > >, Optional<Element<Domain::Xml::Driver3, Name::Strict<538> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Domain::Xml::FilterrefNodeAttributes, Name::Strict<680> > >, Optional<Element<Domain::Xml::Tune, Name::Strict<682> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Rom, Name::Strict<267> > >, Optional<Element<Domain::Xml::Bandwidth, Name::Strict<188> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<130>, Name::Strict<201> > >, OneOrMore<Element<Domain::Xml::Tag, Name::Strict<202> > > > >, Name::Strict<200> > > > > > > marshal_type;

	static int parse(Domain::Xml::Interface641& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Interface641& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct InterfaceOptions traits

template<>
struct Traits<Domain::Xml::InterfaceOptions>
{
	typedef Unordered<mpl::vector<Optional<Element<Attribute<Domain::Xml::EState, Name::Strict<121> >, Name::Strict<119> > >, Optional<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Name::Strict<315> > >, Optional<Element<Domain::Xml::Guest, Name::Strict<400> > >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > >, ZeroOrMore<Element<Domain::Xml::Ip, Name::Strict<663> > >, Optional<Element<Attribute<Domain::Xml::PFilePath, Name::Strict<347> >, Name::Strict<666> > >, Optional<Element<Attribute<Domain::Xml::PType, Name::Strict<100> >, Name::Strict<223> > >, Optional<Element<Domain::Xml::Backend, Name::Strict<667> > >, Optional<Element<Domain::Xml::Driver3, Name::Strict<538> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Domain::Xml::FilterrefNodeAttributes, Name::Strict<680> > >, Optional<Element<Domain::Xml::Tune, Name::Strict<682> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Rom, Name::Strict<267> > >, Optional<Element<Domain::Xml::Bandwidth, Name::Strict<188> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<130>, Name::Strict<201> > >, OneOrMore<Element<Domain::Xml::Tag, Name::Strict<202> > > > >, Name::Strict<200> > > > > marshal_type;

	static int parse(Domain::Xml::InterfaceOptions& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::InterfaceOptions& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface644 traits

template<>
struct Traits<Domain::Xml::Interface644>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<455>, Name::Strict<100> >, Unordered<mpl::vector<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<102> >, Name::Strict<493> >, Optional<Element<Attribute<Domain::Xml::EState, Name::Strict<121> >, Name::Strict<119> > >, Optional<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Name::Strict<315> > >, Optional<Element<Domain::Xml::Guest, Name::Strict<400> > >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > >, ZeroOrMore<Element<Domain::Xml::Ip, Name::Strict<663> > >, Optional<Element<Attribute<Domain::Xml::PFilePath, Name::Strict<347> >, Name::Strict<666> > >, Optional<Element<Attribute<Domain::Xml::PType, Name::Strict<100> >, Name::Strict<223> > >, Optional<Element<Domain::Xml::Backend, Name::Strict<667> > >, Optional<Element<Domain::Xml::Driver3, Name::Strict<538> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Domain::Xml::FilterrefNodeAttributes, Name::Strict<680> > >, Optional<Element<Domain::Xml::Tune, Name::Strict<682> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Rom, Name::Strict<267> > >, Optional<Element<Domain::Xml::Bandwidth, Name::Strict<188> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<130>, Name::Strict<201> > >, OneOrMore<Element<Domain::Xml::Tag, Name::Strict<202> > > > >, Name::Strict<200> > > > > > > marshal_type;

	static int parse(Domain::Xml::Interface644& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Interface644& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source10 traits

template<>
struct Traits<Domain::Xml::Source10>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PIpv4Addr, Name::Strict<106> >, Attribute<Domain::Xml::PPortNumber, Name::Strict<207> > > > marshal_type;

	static int parse(Domain::Xml::Source10& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source10& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface647 traits

template<>
struct Traits<Domain::Xml::Interface647>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EType9, Name::Strict<100> >, Unordered<mpl::vector<Element<Domain::Xml::Source10, Name::Strict<493> >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > > > > > > marshal_type;

	static int parse(Domain::Xml::Interface647& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Interface647& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source11 traits

template<>
struct Traits<Domain::Xml::Source11>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PIpv4Addr, Name::Strict<106> > >, Attribute<Domain::Xml::PPortNumber, Name::Strict<207> > > > marshal_type;

	static int parse(Domain::Xml::Source11& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source11& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface648 traits

template<>
struct Traits<Domain::Xml::Interface648>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<636>, Name::Strict<100> >, Unordered<mpl::vector<Element<Domain::Xml::Source11, Name::Strict<493> >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > > > > > > marshal_type;

	static int parse(Domain::Xml::Interface648& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Interface648& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Usbproduct traits

template<>
struct Traits<Domain::Xml::Usbproduct>
{
	typedef Ordered<mpl::vector<Element<Attribute<Domain::Xml::PUsbId, Name::Strict<203> >, Name::Strict<452> >, Element<Attribute<Domain::Xml::PUsbId, Name::Strict<203> >, Name::Strict<453> > > > marshal_type;

	static int parse(Domain::Xml::Usbproduct& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Usbproduct& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Address traits

template<>
struct Traits<Domain::Xml::Address>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PUsbAddr, Name::Strict<24> >, Attribute<Domain::Xml::PUsbPort, Name::Strict<346> > > > marshal_type;

	static int parse(Domain::Xml::Address& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Address& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source654 traits

template<>
struct Traits<Domain::Xml::Source654>
{
	typedef Ordered<mpl::vector<Fragment<Domain::Xml::Usbproduct >, Optional<Element<Domain::Xml::Address, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Source654& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source654& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Address658 traits

template<>
struct Traits<Domain::Xml::Address658>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<523>, Name::Strict<100> >, Attribute<Domain::Xml::PUsbAddr, Name::Strict<24> >, Attribute<Domain::Xml::PUsbPort, Name::Strict<346> > > > marshal_type;

	static int parse(Domain::Xml::Address658& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Address658& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source12 traits

template<>
struct Traits<Domain::Xml::Source12>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<652> > >, Domain::Xml::VSourceImpl > > marshal_type;

	static int parse(Domain::Xml::Source12& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source12& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface660 traits

template<>
struct Traits<Domain::Xml::Interface660>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<650>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<651> > >, Unordered<mpl::vector<Element<Domain::Xml::Source12, Name::Strict<493> >, Optional<Domain::Xml::VVirtualPortProfileImpl >, Optional<Element<Attribute<Domain::Xml::EState, Name::Strict<121> >, Name::Strict<119> > >, Optional<Element<Attribute<Domain::Xml::PDeviceName, Name::Strict<434> >, Name::Strict<315> > >, Optional<Element<Domain::Xml::Guest, Name::Strict<400> > >, Optional<Element<Attribute<Domain::Xml::PUniMacAddr, Name::Strict<106> >, Name::Strict<647> > >, ZeroOrMore<Element<Domain::Xml::Ip, Name::Strict<663> > >, Optional<Element<Attribute<Domain::Xml::PFilePath, Name::Strict<347> >, Name::Strict<666> > >, Optional<Element<Attribute<Domain::Xml::PType, Name::Strict<100> >, Name::Strict<223> > >, Optional<Element<Domain::Xml::Backend, Name::Strict<667> > >, Optional<Element<Domain::Xml::Driver3, Name::Strict<538> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Domain::Xml::FilterrefNodeAttributes, Name::Strict<680> > >, Optional<Element<Domain::Xml::Tune, Name::Strict<682> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Rom, Name::Strict<267> > >, Optional<Element<Domain::Xml::Bandwidth, Name::Strict<188> > >, Optional<Element<Ordered<mpl::vector<Optional<Attribute<mpl::int_<130>, Name::Strict<201> > >, OneOrMore<Element<Domain::Xml::Tag, Name::Strict<202> > > > >, Name::Strict<200> > > > > > > marshal_type;

	static int parse(Domain::Xml::Interface660& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Interface660& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Input traits

template<>
struct Traits<Domain::Xml::Input>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EType10, Name::Strict<100> >, Optional<Attribute<Domain::Xml::EBus1, Name::Strict<24> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Input& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Input& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Sound traits

template<>
struct Traits<Domain::Xml::Sound>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EModel4, Name::Strict<223> >, Unordered<mpl::vector<Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, ZeroOrMore<Element<Attribute<Domain::Xml::EType11, Name::Strict<100> >, Name::Strict<836> > > > > > > marshal_type;

	static int parse(Domain::Xml::Sound& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Sound& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source13 traits

template<>
struct Traits<Domain::Xml::Source13>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EStartupPolicy, Name::Strict<460> > >, Element<Domain::Xml::Pciaddress, Name::Strict<106> > > > marshal_type;

	static int parse(Domain::Xml::Source13& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source13& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hostdevsubsyspci traits

template<>
struct Traits<Domain::Xml::Hostdevsubsyspci>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<588>, Name::Strict<100> >, Unordered<mpl::vector<Optional<Element<Attribute<Domain::Xml::EName5, Name::Strict<102> >, Name::Strict<538> > >, Element<Domain::Xml::Source13, Name::Strict<493> > > > > > marshal_type;

	static int parse(Domain::Xml::Hostdevsubsyspci& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hostdevsubsyspci& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source899 traits

template<>
struct Traits<Domain::Xml::Source899>
{
	typedef Ordered<mpl::vector<Fragment<Domain::Xml::Usbproduct >, Optional<Element<Domain::Xml::Address, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Source899& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source899& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source14 traits

template<>
struct Traits<Domain::Xml::Source14>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EStartupPolicy, Name::Strict<460> > >, Domain::Xml::VSource1Impl > > marshal_type;

	static int parse(Domain::Xml::Source14& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source14& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parentaddr traits

template<>
struct Traits<Domain::Xml::Parentaddr>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<105> > >, Element<Domain::Xml::Pciaddress, Name::Strict<106> > > > marshal_type;

	static int parse(Domain::Xml::Parentaddr& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Parentaddr& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Adapter113 traits

template<>
struct Traits<Domain::Xml::Adapter113>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<110>, Name::Strict<100> >, Optional<Attribute<QString, Name::Strict<111> > >, Attribute<Domain::Xml::PWwn, Name::Strict<112> >, Attribute<Domain::Xml::PWwn, Name::Strict<113> > > > marshal_type;

	static int parse(Domain::Xml::Adapter113& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Adapter113& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Scsiaddress traits

template<>
struct Traits<Domain::Xml::Scsiaddress>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PDriveBus, Name::Strict<24> >, Attribute<Domain::Xml::PDriveTarget, Name::Strict<315> >, Attribute<Domain::Xml::PDriveUnit, Name::Strict<61> > > > marshal_type;

	static int parse(Domain::Xml::Scsiaddress& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Scsiaddress& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source902 traits

template<>
struct Traits<Domain::Xml::Source902>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EProtocol1, Name::Strict<198> > >, Unordered<mpl::vector<Element<Domain::Xml::VAdapterImpl, Name::Strict<99> >, Element<Domain::Xml::Scsiaddress, Name::Strict<106> > > > > > marshal_type;

	static int parse(Domain::Xml::Source902& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source902& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Host1 traits

template<>
struct Traits<Domain::Xml::Host1>
{
	typedef Ordered<mpl::vector<Attribute<QString, Name::Strict<102> >, Optional<Attribute<Domain::Xml::PPortNumber, Name::Strict<207> > > > > marshal_type;

	static int parse(Domain::Xml::Host1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Host1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source903 traits

template<>
struct Traits<Domain::Xml::Source903>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EProtocol2, Name::Strict<198> >, Attribute<QString, Name::Strict<102> >, Unordered<mpl::vector<OneOrMore<Element<Domain::Xml::Host1, Name::Strict<505> > >, Optional<Element<Domain::Xml::Auth, Name::Strict<1060> > > > > > > marshal_type;

	static int parse(Domain::Xml::Source903& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source903& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hostdevsubsysscsi traits

template<>
struct Traits<Domain::Xml::Hostdevsubsysscsi>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<521>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::ESgio1, Name::Strict<469> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<1137> > >, Element<Domain::Xml::VSource2Impl, Name::Strict<493> > > > marshal_type;

	static int parse(Domain::Xml::Hostdevsubsysscsi& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hostdevsubsysscsi& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hostdevsubsys traits

template<>
struct Traits<Domain::Xml::Hostdevsubsys>
{
	typedef Ordered<mpl::vector<Optional<Attribute<mpl::int_<886>, Name::Strict<371> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<651> > >, Domain::Xml::VHostdevsubsysImpl > > marshal_type;

	static int parse(Domain::Xml::Hostdevsubsys& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hostdevsubsys& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hostdev traits

template<>
struct Traits<Domain::Xml::Hostdev>
{
	typedef Unordered<mpl::vector<Domain::Xml::VChoice884Impl, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > >, Optional<Element<Domain::Xml::Rom, Name::Strict<267> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Empty, Name::Strict<266> > >, Optional<Element<Empty, Name::Strict<444> > > > > marshal_type;

	static int parse(Domain::Xml::Hostdev& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hostdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Graphics690 traits

template<>
struct Traits<Domain::Xml::Graphics690>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<687>, Name::Strict<100> >, Optional<Attribute<QString, Name::Strict<688> > >, Optional<Attribute<QString, Name::Strict<689> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<690> > > > > marshal_type;

	static int parse(Domain::Xml::Graphics690& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Graphics690& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant699 traits

template<>
struct Traits<Domain::Xml::Variant699>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PPortNumber, Name::Strict<207> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<693> > >, Optional<Attribute<Domain::Xml::PPortNumber, Name::Strict<694> > >, Optional<Attribute<Domain::Xml::PAddrIPorName, Name::Strict<695> > >, Optional<Attribute<Domain::Xml::ESharePolicy, Name::Strict<697> > > > > marshal_type;

	static int parse(Domain::Xml::Variant699& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Variant699& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Listen751 traits

template<>
struct Traits<Domain::Xml::Listen751>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<438>, Name::Strict<100> >, Attribute<QString, Name::Strict<438> >, Optional<Attribute<Domain::Xml::PAddrIPorName, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Listen751& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Listen751& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Graphics708 traits

template<>
struct Traits<Domain::Xml::Graphics708>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<692>, Name::Strict<100> >, Domain::Xml::VChoice701Impl, Optional<Attribute<QString, Name::Strict<703> > >, Optional<Attribute<QString, Name::Strict<704> > >, Optional<Attribute<Domain::Xml::PPasswdValidTo, Name::Strict<705> > >, Optional<Attribute<Domain::Xml::EConnected, Name::Strict<706> > >, ZeroOrMore<Element<Domain::Xml::VListenImpl, Name::Strict<695> > > > > marshal_type;

	static int parse(Domain::Xml::Graphics708& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Graphics708& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Channel traits

template<>
struct Traits<Domain::Xml::Channel>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EName6, Name::Strict<102> >, Attribute<Domain::Xml::EMode7, Name::Strict<371> > > > marshal_type;

	static int parse(Domain::Xml::Channel& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Channel& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Graphics743 traits

template<>
struct Traits<Domain::Xml::Graphics743>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<710>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::PPortNumber, Name::Strict<207> > >, Optional<Attribute<Domain::Xml::PPortNumber, Name::Strict<711> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<693> > >, Optional<Attribute<Domain::Xml::PAddrIPorName, Name::Strict<695> > >, Optional<Attribute<QString, Name::Strict<703> > >, Optional<Attribute<QString, Name::Strict<704> > >, Optional<Attribute<Domain::Xml::PPasswdValidTo, Name::Strict<705> > >, Optional<Attribute<Domain::Xml::EConnected1, Name::Strict<706> > >, Optional<Attribute<Domain::Xml::EDefaultMode, Name::Strict<714> > >, Unordered<mpl::vector<ZeroOrMore<Element<Domain::Xml::VListenImpl, Name::Strict<695> > >, ZeroOrMore<Element<Domain::Xml::Channel, Name::Strict<718> > >, Optional<Element<Attribute<Domain::Xml::ECompression, Name::Strict<727> >, Name::Strict<726> > >, Optional<Element<Attribute<Domain::Xml::ECompression1, Name::Strict<727> >, Name::Strict<733> > >, Optional<Element<Attribute<Domain::Xml::ECompression2, Name::Strict<727> >, Name::Strict<736> > >, Optional<Element<Attribute<Domain::Xml::EVirOnOff, Name::Strict<727> >, Name::Strict<722> > >, Optional<Element<Attribute<Domain::Xml::EMode8, Name::Strict<371> >, Name::Strict<737> > >, Optional<Element<Attribute<Domain::Xml::EVirYesNo, Name::Strict<741> >, Name::Strict<740> > >, Optional<Element<Attribute<Domain::Xml::EMode9, Name::Strict<371> >, Name::Strict<742> > >, Optional<Element<Attribute<Domain::Xml::EVirYesNo, Name::Strict<273> >, Name::Strict<743> > > > > > > marshal_type;

	static int parse(Domain::Xml::Graphics743& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Graphics743& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Graphics747 traits

template<>
struct Traits<Domain::Xml::Graphics747>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<745>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::PPortNumber, Name::Strict<207> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<693> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<746> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<747> > >, Optional<Attribute<Domain::Xml::PAddrIPorName, Name::Strict<695> > >, ZeroOrMore<Element<Domain::Xml::VListenImpl, Name::Strict<695> > > > > marshal_type;

	static int parse(Domain::Xml::Graphics747& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Graphics747& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Graphics749 traits

template<>
struct Traits<Domain::Xml::Graphics749>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<749>, Name::Strict<100> >, Optional<Attribute<QString, Name::Strict<688> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<690> > > > > marshal_type;

	static int parse(Domain::Xml::Graphics749& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Graphics749& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Acceleration traits

template<>
struct Traits<Domain::Xml::Acceleration>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<763> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<764> > > > > marshal_type;

	static int parse(Domain::Xml::Acceleration& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Acceleration& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Model1 traits

template<>
struct Traits<Domain::Xml::Model1>
{
	typedef Ordered<mpl::vector<Domain::Xml::VModelImpl, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<760> > >, Optional<Attribute<Domain::Xml::PUnsignedInt, Name::Strict<531> > >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<761> > >, Optional<Element<Domain::Xml::Acceleration, Name::Strict<762> > > > > marshal_type;

	static int parse(Domain::Xml::Model1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Model1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Video traits

template<>
struct Traits<Domain::Xml::Video>
{
	typedef Ordered<mpl::vector<Optional<Element<Domain::Xml::Model1, Name::Strict<223> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Video& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Video& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source15 traits

template<>
struct Traits<Domain::Xml::Source15>
{
	typedef Ordered<mpl::vector<Optional<Attribute<QString, Name::Strict<371> > >, Optional<Attribute<QString, Name::Strict<347> > >, Optional<Attribute<QString, Name::Strict<505> > >, Optional<Attribute<QString, Name::Strict<824> > >, Optional<Attribute<QString, Name::Strict<825> > >, Optional<Attribute<QString, Name::Strict<718> > >, Optional<Attribute<QString, Name::Strict<826> > >, Optional<Attribute<QString, Name::Strict<827> > >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<828> > >, Optional<Element<Domain::Xml::Seclabel, Name::Strict<221> > > > > marshal_type;

	static int parse(Domain::Xml::Source15& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source15& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Protocol traits

template<>
struct Traits<Domain::Xml::Protocol>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EType13, Name::Strict<100> > > > > marshal_type;

	static int parse(Domain::Xml::Protocol& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Protocol& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct QemucdevSrcDef traits

template<>
struct Traits<Domain::Xml::QemucdevSrcDef>
{
	typedef Ordered<mpl::vector<ZeroOrMore<Element<Domain::Xml::Source15, Name::Strict<493> > >, Optional<Element<Domain::Xml::Protocol, Name::Strict<198> > > > > marshal_type;

	static int parse(Domain::Xml::QemucdevSrcDef& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::QemucdevSrcDef& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Target2 traits

template<>
struct Traits<Domain::Xml::Target2>
{
	typedef Unordered<mpl::vector<Domain::Xml::VChoice795Impl, Optional<Attribute<QString, Name::Strict<207> > > > > marshal_type;

	static int parse(Domain::Xml::Target2& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Target2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Qemucdev traits

template<>
struct Traits<Domain::Xml::Qemucdev>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EQemucdevSrcTypeChoice, Name::Strict<100> >, Optional<Attribute<Domain::Xml::PAbsFilePath, Name::Strict<785> > >, Unordered<mpl::vector<Fragment<Domain::Xml::QemucdevSrcDef >, Optional<Element<Domain::Xml::Target2, Name::Strict<315> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > > > marshal_type;

	static int parse(Domain::Xml::Qemucdev& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Qemucdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Target3 traits

template<>
struct Traits<Domain::Xml::Target3>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<858>, Name::Strict<100> >, Attribute<QString, Name::Strict<106> >, Attribute<QString, Name::Strict<207> > > > marshal_type;

	static int parse(Domain::Xml::Target3& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Target3& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Channel1 traits

template<>
struct Traits<Domain::Xml::Channel1>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EQemucdevSrcTypeChoice, Name::Strict<100> >, Unordered<mpl::vector<Fragment<Domain::Xml::QemucdevSrcDef >, Domain::Xml::VChoice861Impl, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > > > marshal_type;

	static int parse(Domain::Xml::Channel1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Channel1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard862 traits

template<>
struct Traits<Domain::Xml::Smartcard862>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<505>, Name::Strict<371> > > > marshal_type;

	static int parse(Domain::Xml::Smartcard862& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Smartcard862& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard866 traits

template<>
struct Traits<Domain::Xml::Smartcard866>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<864>, Name::Strict<371> >, Element<Text<QString >, Name::Strict<865> >, Element<Text<QString >, Name::Strict<865> >, Element<Text<QString >, Name::Strict<865> >, Optional<Element<Text<Domain::Xml::PAbsDirPath >, Name::Strict<866> > > > > marshal_type;

	static int parse(Domain::Xml::Smartcard866& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Smartcard866& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard867 traits

template<>
struct Traits<Domain::Xml::Smartcard867>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<620>, Name::Strict<371> >, Attribute<Domain::Xml::EQemucdevSrcTypeChoice, Name::Strict<100> >, Unordered<mpl::vector<Fragment<Domain::Xml::QemucdevSrcDef >, Optional<Element<Domain::Xml::Target2, Name::Strict<315> > > > > > > marshal_type;

	static int parse(Domain::Xml::Smartcard867& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Smartcard867& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard traits

template<>
struct Traits<Domain::Xml::Smartcard>
{
	typedef Ordered<mpl::vector<Domain::Xml::VSmartcardImpl, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Smartcard& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Smartcard& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub traits

template<>
struct Traits<Domain::Xml::Hub>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EType16, Name::Strict<100> >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Hub& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Hub& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Redirdev traits

template<>
struct Traits<Domain::Xml::Redirdev>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EBus2, Name::Strict<24> >, Attribute<Domain::Xml::EQemucdevSrcTypeChoice, Name::Strict<100> >, Fragment<Domain::Xml::QemucdevSrcDef >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<338> >, Name::Strict<399> > > > > marshal_type;

	static int parse(Domain::Xml::Redirdev& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Redirdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Usbdev traits

template<>
struct Traits<Domain::Xml::Usbdev>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EVirYesNo, Name::Strict<809> >, Optional<Attribute<Domain::Xml::VClass, Name::Strict<810> > >, Optional<Attribute<Domain::Xml::VVendor, Name::Strict<452> > >, Optional<Attribute<Domain::Xml::VProduct, Name::Strict<453> > >, Optional<Attribute<Domain::Xml::VVersion, Name::Strict<820> > > > > marshal_type;

	static int parse(Domain::Xml::Usbdev& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Usbdev& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Backend1042 traits

template<>
struct Traits<Domain::Xml::Backend1042>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<1042>, Name::Strict<223> >, Attribute<Domain::Xml::EQemucdevSrcTypeChoice, Name::Strict<100> >, Fragment<Domain::Xml::QemucdevSrcDef > > > marshal_type;

	static int parse(Domain::Xml::Backend1042& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Backend1042& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Rate traits

template<>
struct Traits<Domain::Xml::Rate>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<1045> >, Optional<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<362> > > > > marshal_type;

	static int parse(Domain::Xml::Rate& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Rate& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Rng traits

template<>
struct Traits<Domain::Xml::Rng>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EModel5, Name::Strict<223> >, Unordered<mpl::vector<Element<Domain::Xml::VBackendImpl, Name::Strict<667> >, Optional<Element<Domain::Xml::Rate, Name::Strict<1044> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > > > marshal_type;

	static int parse(Domain::Xml::Rng& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Rng& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Device1 traits

template<>
struct Traits<Domain::Xml::Device1>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::PFilePath, Name::Strict<347> > > > > marshal_type;

	static int parse(Domain::Xml::Device1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Device1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Tpm traits

template<>
struct Traits<Domain::Xml::Tpm>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EModel6, Name::Strict<223> > >, Element<Ordered<mpl::vector<Attribute<mpl::int_<620>, Name::Strict<100> >, Optional<Element<Domain::Xml::Device1, Name::Strict<346> > > > >, Name::Strict<667> >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > > > > marshal_type;

	static int parse(Domain::Xml::Tpm& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Tpm& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source16 traits

template<>
struct Traits<Domain::Xml::Source16>
{
	typedef Unordered<mpl::vector<Optional<Element<Domain::Xml::ScaledInteger, Name::Strict<1032> > >, Optional<Element<Text<Domain::Xml::PCpuset >, Name::Strict<1033> > > > > marshal_type;

	static int parse(Domain::Xml::Source16& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Source16& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Target4 traits

template<>
struct Traits<Domain::Xml::Target4>
{
	typedef Unordered<mpl::vector<Element<Domain::Xml::ScaledInteger, Name::Strict<326> >, Element<Text<Domain::Xml::PUnsignedInt >, Name::Strict<1034> > > > marshal_type;

	static int parse(Domain::Xml::Target4& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Target4& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Memory2 traits

template<>
struct Traits<Domain::Xml::Memory2>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EModel7, Name::Strict<223> >, Unordered<mpl::vector<Optional<Element<Domain::Xml::Source16, Name::Strict<493> > >, Element<Domain::Xml::Target4, Name::Strict<315> >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > > > > > > marshal_type;

	static int parse(Domain::Xml::Memory2& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Memory2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Watchdog traits

template<>
struct Traits<Domain::Xml::Watchdog>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EModel8, Name::Strict<223> >, Optional<Attribute<Domain::Xml::EAction, Name::Strict<850> > >, Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Watchdog& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Watchdog& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Memballoon traits

template<>
struct Traits<Domain::Xml::Memballoon>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EModel9, Name::Strict<223> >, Optional<Attribute<Domain::Xml::EVirOnOff, Name::Strict<854> > >, Unordered<mpl::vector<Optional<Element<Attribute<Domain::Xml::PAliasName, Name::Strict<102> >, Name::Strict<449> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > >, Optional<Element<Attribute<Domain::Xml::PPositiveInteger, Name::Strict<362> >, Name::Strict<855> > > > > > > marshal_type;

	static int parse(Domain::Xml::Memballoon& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Memballoon& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Nvram1 traits

template<>
struct Traits<Domain::Xml::Nvram1>
{
	typedef Ordered<mpl::vector<Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Nvram1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Nvram1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Panic traits

template<>
struct Traits<Domain::Xml::Panic>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Domain::Xml::EModel10, Name::Strict<223> > >, Optional<Element<Domain::Xml::VAddressImpl, Name::Strict<106> > > > > marshal_type;

	static int parse(Domain::Xml::Panic& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Panic& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Devices traits

template<>
struct Traits<Domain::Xml::Devices>
{
	typedef Unordered<mpl::vector<Optional<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<684> > >, ZeroOrMore<Domain::Xml::VChoice952Impl >, Optional<Element<Domain::Xml::Watchdog, Name::Strict<847> > >, Optional<Element<Domain::Xml::Memballoon, Name::Strict<853> > >, Optional<Element<Domain::Xml::Nvram1, Name::Strict<269> > >, ZeroOrMore<Element<Domain::Xml::Panic, Name::Strict<954> > > > > marshal_type;

	static int parse(Domain::Xml::Devices& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Devices& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel228 traits

template<>
struct Traits<Domain::Xml::Seclabel228>
{
	typedef Ordered<mpl::vector<Optional<Attribute<mpl::int_<224>, Name::Strict<100> > >, Optional<Attribute<mpl::int_<130>, Name::Strict<225> > >, Unordered<mpl::vector<Optional<Element<Text<QString >, Name::Strict<226> > >, Optional<Element<Text<QString >, Name::Strict<227> > >, Optional<Element<Text<QString >, Name::Strict<228> > > > > > > marshal_type;

	static int parse(Domain::Xml::Seclabel228& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Seclabel228& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel230 traits

template<>
struct Traits<Domain::Xml::Seclabel230>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<230>, Name::Strict<100> >, Optional<Attribute<Domain::Xml::EVirYesNo, Name::Strict<225> > >, Unordered<mpl::vector<Element<Text<QString >, Name::Strict<226> >, Optional<Element<Text<QString >, Name::Strict<227> > > > > > > marshal_type;

	static int parse(Domain::Xml::Seclabel230& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Seclabel230& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel232 traits

template<>
struct Traits<Domain::Xml::Seclabel232>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<232>, Name::Strict<100> >, Optional<Attribute<mpl::int_<131>, Name::Strict<225> > > > > marshal_type;

	static int parse(Domain::Xml::Seclabel232& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Seclabel232& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel1 traits

template<>
struct Traits<Domain::Xml::Seclabel1>
{
	typedef Ordered<mpl::vector<Optional<Attribute<QString, Name::Strict<223> > >, Domain::Xml::VSeclabel1Impl > > marshal_type;

	static int parse(Domain::Xml::Seclabel1& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Seclabel1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Env traits

template<>
struct Traits<Domain::Xml::Env>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::PFilterParamName, Name::Strict<102> >, Optional<Attribute<QString, Name::Strict<1049> > > > > marshal_type;

	static int parse(Domain::Xml::Env& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Env& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Commandline traits

template<>
struct Traits<Domain::Xml::Commandline>
{
	typedef Ordered<mpl::vector<ZeroOrMore<Element<Attribute<QString, Name::Strict<1049> >, Name::Strict<1123> > >, ZeroOrMore<Element<Domain::Xml::Env, Name::Strict<1124> > > > > marshal_type;

	static int parse(Domain::Xml::Commandline& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Commandline& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Domain traits

template<>
struct Traits<Domain::Xml::Domain>
{
	typedef Ordered<mpl::vector<Attribute<Domain::Xml::EType, Name::Strict<100> >, Fragment<Domain::Xml::Ids >, Unordered<mpl::vector<Optional<Element<Text<Domain::Xml::PTitle >, Name::Strict<209> > >, Optional<Element<Text<QString >, Name::Strict<208> > >, Optional<Element<Domain::Xml::Cpu, Name::Strict<212> > >, Optional<Element<Domain::Xml::Sysinfo, Name::Strict<213> > >, Domain::Xml::VOsImpl, Optional<Element<Domain::Xml::Clock, Name::Strict<215> > >, Element<Domain::Xml::Memory, Name::Strict<318> >, Optional<Element<Domain::Xml::MaxMemory, Name::Strict<320> > >, Optional<Element<Domain::Xml::ScaledInteger, Name::Strict<322> > >, Optional<Element<Domain::Xml::MemoryBacking, Name::Strict<323> > >, Optional<Element<Domain::Xml::Vcpu, Name::Strict<330> > >, Optional<Element<ZeroOrMore<Element<Domain::Xml::Vcpu1, Name::Strict<330> > >, Name::Strict<335> > >, Optional<Element<Text<Domain::Xml::PUnsignedInt >, Name::Strict<339> > >, Optional<Element<Domain::Xml::Blkiotune, Name::Strict<340> > >, Optional<Element<Domain::Xml::Memtune, Name::Strict<341> > >, Optional<Element<Domain::Xml::Cputune, Name::Strict<342> > >, Optional<Element<Domain::Xml::Numatune, Name::Strict<343> > >, Optional<Element<Element<Text<Domain::Xml::PAbsFilePath >, Name::Strict<380> >, Name::Strict<379> > >, Optional<Element<Domain::Xml::Features, Name::Strict<150> > >, Optional<Element<Text<Domain::Xml::EOffOptions >, Name::Strict<765> > >, Optional<Element<Text<Domain::Xml::EOffOptions >, Name::Strict<767> > >, Optional<Element<Text<Domain::Xml::ECrashOptions >, Name::Strict<768> > >, Optional<Element<Text<Domain::Xml::ELockfailureOptions >, Name::Strict<770> > >, Optional<Element<Domain::Xml::Pm, Name::Strict<218> > >, Optional<Element<Domain::Xml::Idmap, Name::Strict<219> > >, Optional<Element<Domain::Xml::Devices, Name::Strict<220> > >, ZeroOrMore<Element<Domain::Xml::Seclabel1, Name::Strict<221> > >, Optional<Element<Domain::Xml::Commandline, Name::Scoped<1122, 1125> > > > > > > marshal_type;

	static int parse(Domain::Xml::Domain& , QStack<QDomElement>& );
	static int generate(const Domain::Xml::Domain& , QDomElement& );
};

} // namespace Libvirt

#endif // __DOMAIN_TYPE_H__
