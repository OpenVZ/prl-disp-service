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
// struct Features

namespace Capability
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
	bool getNonpae() const
	{
		return m_nonpae;
	}
	void setNonpae(bool value_)
	{
		m_nonpae = value_;
	}
	bool getVmx() const
	{
		return m_vmx;
	}
	void setVmx(bool value_)
	{
		m_vmx = value_;
	}
	bool getSvm() const
	{
		return m_svm;
	}
	void setSvm(bool value_)
	{
		m_svm = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	bool m_pae;
	bool m_nonpae;
	bool m_vmx;
	bool m_svm;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Topology

namespace Capability
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
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Pages

namespace Capability
{
namespace Xml
{
struct Pages
{
	Pages();

	const boost::optional<PUnit::value_type >& getUnit() const
	{
		return m_unit;
	}
	void setUnit(const boost::optional<PUnit::value_type >& value_)
	{
		m_unit = value_;
	}
	PUnsignedInt::value_type getSize() const
	{
		return m_size;
	}
	void setSize(PUnsignedInt::value_type value_)
	{
		m_size = value_;
	}
	PUnsignedInt::value_type getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(PUnsignedInt::value_type value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PUnit::value_type > m_unit;
	PUnsignedInt::value_type m_size;
	PUnsignedInt::value_type m_ownValue;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Cpuspec

namespace Capability
{
namespace Xml
{
struct Cpuspec
{
	const QString& getModel() const
	{
		return m_model;
	}
	void setModel(const QString& value_)
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
	const Topology& getTopology() const
	{
		return m_topology;
	}
	void setTopology(const Topology& value_)
	{
		m_topology = value_;
	}
	const QList<PFeatureName::value_type >& getFeatureList() const
	{
		return m_featureList;
	}
	void setFeatureList(const QList<PFeatureName::value_type >& value_)
	{
		m_featureList = value_;
	}
	const QList<Pages >& getPagesList() const
	{
		return m_pagesList;
	}
	void setPagesList(const QList<Pages >& value_)
	{
		m_pagesList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	QString m_model;
	boost::optional<QString > m_vendor;
	Topology m_topology;
	QList<PFeatureName::value_type > m_featureList;
	QList<Pages > m_pagesList;
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

	EArchnames getArch() const
	{
		return m_arch;
	}
	void setArch(EArchnames value_)
	{
		m_arch = value_;
	}
	const boost::optional<Features >& getFeatures() const
	{
		return m_features;
	}
	void setFeatures(const boost::optional<Features >& value_)
	{
		m_features = value_;
	}
	const boost::optional<Cpuspec >& getCpuspec() const
	{
		return m_cpuspec;
	}
	void setCpuspec(const boost::optional<Cpuspec >& value_)
	{
		m_cpuspec = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EArchnames m_arch;
	boost::optional<Features > m_features;
	boost::optional<Cpuspec > m_cpuspec;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct PowerManagement

namespace Capability
{
namespace Xml
{
struct PowerManagement
{
	PowerManagement();

	bool getSuspendMem() const
	{
		return m_suspendMem;
	}
	void setSuspendMem(bool value_)
	{
		m_suspendMem = value_;
	}
	bool getSuspendDisk() const
	{
		return m_suspendDisk;
	}
	void setSuspendDisk(bool value_)
	{
		m_suspendDisk = value_;
	}
	bool getSuspendHybrid() const
	{
		return m_suspendHybrid;
	}
	void setSuspendHybrid(bool value_)
	{
		m_suspendHybrid = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	bool m_suspendMem;
	bool m_suspendDisk;
	bool m_suspendHybrid;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct MigrationFeatures

namespace Capability
{
namespace Xml
{
struct MigrationFeatures
{
	MigrationFeatures();

	bool getLive() const
	{
		return m_live;
	}
	void setLive(bool value_)
	{
		m_live = value_;
	}
	const boost::optional<QList<EUriTransport > >& getUriTransports() const
	{
		return m_uriTransports;
	}
	void setUriTransports(const boost::optional<QList<EUriTransport > >& value_)
	{
		m_uriTransports = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	bool m_live;
	boost::optional<QList<EUriTransport > > m_uriTransports;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct ScaledInteger

namespace Capability
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
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Sibling

namespace Capability
{
namespace Xml
{
struct Sibling
{
	Sibling();

	PUnsignedInt::value_type getId() const
	{
		return m_id;
	}
	void setId(PUnsignedInt::value_type value_)
	{
		m_id = value_;
	}
	PUnsignedInt::value_type getValue() const
	{
		return m_value;
	}
	void setValue(PUnsignedInt::value_type value_)
	{
		m_value = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_id;
	PUnsignedInt::value_type m_value;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1844

namespace Capability
{
namespace Xml
{
struct Anonymous1844
{
	Anonymous1844();

	PUnsignedInt::value_type getSocketId() const
	{
		return m_socketId;
	}
	void setSocketId(PUnsignedInt::value_type value_)
	{
		m_socketId = value_;
	}
	PUnsignedInt::value_type getCoreId() const
	{
		return m_coreId;
	}
	void setCoreId(PUnsignedInt::value_type value_)
	{
		m_coreId = value_;
	}
	const PCpuset::value_type& getSiblings() const
	{
		return m_siblings;
	}
	void setSiblings(const PCpuset::value_type& value_)
	{
		m_siblings = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PUnsignedInt::value_type m_socketId;
	PUnsignedInt::value_type m_coreId;
	PCpuset::value_type m_siblings;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Cpu1

namespace Capability
{
namespace Xml
{
struct Cpu1
{
	Cpu1();

	PUnsignedInt::value_type getId() const
	{
		return m_id;
	}
	void setId(PUnsignedInt::value_type value_)
	{
		m_id = value_;
	}
	const boost::optional<Anonymous1844 >& getAnonymous1844() const
	{
		return m_anonymous1844;
	}
	void setAnonymous1844(const boost::optional<Anonymous1844 >& value_)
	{
		m_anonymous1844 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_id;
	boost::optional<Anonymous1844 > m_anonymous1844;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Cpus

namespace Capability
{
namespace Xml
{
struct Cpus
{
	Cpus();

	PUnsignedInt::value_type getNum() const
	{
		return m_num;
	}
	void setNum(PUnsignedInt::value_type value_)
	{
		m_num = value_;
	}
	const QList<Cpu1 >& getCpuList() const
	{
		return m_cpuList;
	}
	void setCpuList(const QList<Cpu1 >& value_)
	{
		m_cpuList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_num;
	QList<Cpu1 > m_cpuList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Cell

namespace Capability
{
namespace Xml
{
struct Cell
{
	Cell();

	PUnsignedInt::value_type getId() const
	{
		return m_id;
	}
	void setId(PUnsignedInt::value_type value_)
	{
		m_id = value_;
	}
	const boost::optional<ScaledInteger >& getMemory() const
	{
		return m_memory;
	}
	void setMemory(const boost::optional<ScaledInteger >& value_)
	{
		m_memory = value_;
	}
	const QList<Pages >& getPagesList() const
	{
		return m_pagesList;
	}
	void setPagesList(const QList<Pages >& value_)
	{
		m_pagesList = value_;
	}
	const boost::optional<QList<Sibling > >& getDistances() const
	{
		return m_distances;
	}
	void setDistances(const boost::optional<QList<Sibling > >& value_)
	{
		m_distances = value_;
	}
	const boost::optional<Cpus >& getCpus() const
	{
		return m_cpus;
	}
	void setCpus(const boost::optional<Cpus >& value_)
	{
		m_cpus = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_id;
	boost::optional<ScaledInteger > m_memory;
	QList<Pages > m_pagesList;
	boost::optional<QList<Sibling > > m_distances;
	boost::optional<Cpus > m_cpus;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Cells

namespace Capability
{
namespace Xml
{
struct Cells
{
	Cells();

	PUnsignedInt::value_type getNum() const
	{
		return m_num;
	}
	void setNum(PUnsignedInt::value_type value_)
	{
		m_num = value_;
	}
	const QList<Cell >& getCellList() const
	{
		return m_cellList;
	}
	void setCellList(const QList<Cell >& value_)
	{
		m_cellList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_num;
	QList<Cell > m_cellList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Control

namespace Capability
{
namespace Xml
{
struct Control
{
	Control();

	PUnsignedInt::value_type getGranularity() const
	{
		return m_granularity;
	}
	void setGranularity(PUnsignedInt::value_type value_)
	{
		m_granularity = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getMin() const
	{
		return m_min;
	}
	void setMin(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_min = value_;
	}
	const PUnit::value_type& getUnit() const
	{
		return m_unit;
	}
	void setUnit(const PUnit::value_type& value_)
	{
		m_unit = value_;
	}
	EType getType() const
	{
		return m_type;
	}
	void setType(EType value_)
	{
		m_type = value_;
	}
	PUnsignedInt::value_type getMaxAllocs() const
	{
		return m_maxAllocs;
	}
	void setMaxAllocs(PUnsignedInt::value_type value_)
	{
		m_maxAllocs = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_granularity;
	boost::optional<PUnsignedInt::value_type > m_min;
	PUnit::value_type m_unit;
	EType m_type;
	PUnsignedInt::value_type m_maxAllocs;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Bank

namespace Capability
{
namespace Xml
{
struct Bank
{
	Bank();

	PUnsignedInt::value_type getId() const
	{
		return m_id;
	}
	void setId(PUnsignedInt::value_type value_)
	{
		m_id = value_;
	}
	PUnsignedInt::value_type getLevel() const
	{
		return m_level;
	}
	void setLevel(PUnsignedInt::value_type value_)
	{
		m_level = value_;
	}
	EType getType() const
	{
		return m_type;
	}
	void setType(EType value_)
	{
		m_type = value_;
	}
	PUnsignedInt::value_type getSize() const
	{
		return m_size;
	}
	void setSize(PUnsignedInt::value_type value_)
	{
		m_size = value_;
	}
	const PUnit::value_type& getUnit() const
	{
		return m_unit;
	}
	void setUnit(const PUnit::value_type& value_)
	{
		m_unit = value_;
	}
	const PCpuset::value_type& getCpus() const
	{
		return m_cpus;
	}
	void setCpus(const PCpuset::value_type& value_)
	{
		m_cpus = value_;
	}
	const QList<Control >& getControlList() const
	{
		return m_controlList;
	}
	void setControlList(const QList<Control >& value_)
	{
		m_controlList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_id;
	PUnsignedInt::value_type m_level;
	EType m_type;
	PUnsignedInt::value_type m_size;
	PUnit::value_type m_unit;
	PCpuset::value_type m_cpus;
	QList<Control > m_controlList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Baselabel

namespace Capability
{
namespace Xml
{
struct Baselabel
{
	const QString& getType() const
	{
		return m_type;
	}
	void setType(const QString& value_)
	{
		m_type = value_;
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
	QString m_type;
	QString m_ownValue;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Secmodel

namespace Capability
{
namespace Xml
{
struct Secmodel
{
	const QString& getModel() const
	{
		return m_model;
	}
	void setModel(const QString& value_)
	{
		m_model = value_;
	}
	const QString& getDoi() const
	{
		return m_doi;
	}
	void setDoi(const QString& value_)
	{
		m_doi = value_;
	}
	const QList<Baselabel >& getBaselabelList() const
	{
		return m_baselabelList;
	}
	void setBaselabelList(const QList<Baselabel >& value_)
	{
		m_baselabelList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_model;
	QString m_doi;
	QList<Baselabel > m_baselabelList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Host

namespace Capability
{
namespace Xml
{
struct Host
{
	const boost::optional<VUUID >& getUuid() const
	{
		return m_uuid;
	}
	void setUuid(const boost::optional<VUUID >& value_)
	{
		m_uuid = value_;
	}
	const Cpu& getCpu() const
	{
		return m_cpu;
	}
	void setCpu(const Cpu& value_)
	{
		m_cpu = value_;
	}
	const boost::optional<PowerManagement >& getPowerManagement() const
	{
		return m_powerManagement;
	}
	void setPowerManagement(const boost::optional<PowerManagement >& value_)
	{
		m_powerManagement = value_;
	}
	const boost::optional<MigrationFeatures >& getMigrationFeatures() const
	{
		return m_migrationFeatures;
	}
	void setMigrationFeatures(const boost::optional<MigrationFeatures >& value_)
	{
		m_migrationFeatures = value_;
	}
	const boost::optional<Cells >& getTopology() const
	{
		return m_topology;
	}
	void setTopology(const boost::optional<Cells >& value_)
	{
		m_topology = value_;
	}
	const boost::optional<QList<Bank > >& getCache() const
	{
		return m_cache;
	}
	void setCache(const boost::optional<QList<Bank > >& value_)
	{
		m_cache = value_;
	}
	const QList<Secmodel >& getSecmodelList() const
	{
		return m_secmodelList;
	}
	void setSecmodelList(const QList<Secmodel >& value_)
	{
		m_secmodelList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<VUUID > m_uuid;
	Cpu m_cpu;
	boost::optional<PowerManagement > m_powerManagement;
	boost::optional<MigrationFeatures > m_migrationFeatures;
	boost::optional<Cells > m_topology;
	boost::optional<QList<Bank > > m_cache;
	QList<Secmodel > m_secmodelList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Machine

namespace Capability
{
namespace Xml
{
struct Machine
{
	const boost::optional<QString >& getCanonical() const
	{
		return m_canonical;
	}
	void setCanonical(const boost::optional<QString >& value_)
	{
		m_canonical = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getMaxCpus() const
	{
		return m_maxCpus;
	}
	void setMaxCpus(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_maxCpus = value_;
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
	boost::optional<QString > m_canonical;
	boost::optional<PUnsignedInt::value_type > m_maxCpus;
	QString m_ownValue;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Domain

namespace Capability
{
namespace Xml
{
struct Domain
{
	Domain();

	EType1 getType() const
	{
		return m_type;
	}
	void setType(EType1 value_)
	{
		m_type = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getEmulator() const
	{
		return m_emulator;
	}
	void setEmulator(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_emulator = value_;
	}
	const QList<Machine >& getMachineList() const
	{
		return m_machineList;
	}
	void setMachineList(const QList<Machine >& value_)
	{
		m_machineList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EType1 m_type;
	boost::optional<PAbsFilePath::value_type > m_emulator;
	QList<Machine > m_machineList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Arch

namespace Capability
{
namespace Xml
{
struct Arch
{
	Arch();

	EArchnames getName() const
	{
		return m_name;
	}
	void setName(EArchnames value_)
	{
		m_name = value_;
	}
	EWordsize getWordsize() const
	{
		return m_wordsize;
	}
	void setWordsize(EWordsize value_)
	{
		m_wordsize = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getEmulator() const
	{
		return m_emulator;
	}
	void setEmulator(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_emulator = value_;
	}
	const boost::optional<PAbsFilePath::value_type >& getLoader() const
	{
		return m_loader;
	}
	void setLoader(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_loader = value_;
	}
	const QList<Machine >& getMachineList() const
	{
		return m_machineList;
	}
	void setMachineList(const QList<Machine >& value_)
	{
		m_machineList = value_;
	}
	const QList<Domain >& getDomainList() const
	{
		return m_domainList;
	}
	void setDomainList(const QList<Domain >& value_)
	{
		m_domainList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EArchnames m_name;
	EWordsize m_wordsize;
	boost::optional<PAbsFilePath::value_type > m_emulator;
	boost::optional<PAbsFilePath::value_type > m_loader;
	QList<Machine > m_machineList;
	QList<Domain > m_domainList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Featuretoggle

namespace Capability
{
namespace Xml
{
struct Featuretoggle
{
	Featuretoggle();

	EVirYesNo getToggle() const
	{
		return m_toggle;
	}
	void setToggle(EVirYesNo value_)
	{
		m_toggle = value_;
	}
	EVirOnOff getDefault() const
	{
		return m_default;
	}
	void setDefault(EVirOnOff value_)
	{
		m_default = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EVirYesNo m_toggle;
	EVirOnOff m_default;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Features1

namespace Capability
{
namespace Xml
{
struct Features1
{
	Features1();

	bool getPae() const
	{
		return m_pae;
	}
	void setPae(bool value_)
	{
		m_pae = value_;
	}
	bool getNonpae() const
	{
		return m_nonpae;
	}
	void setNonpae(bool value_)
	{
		m_nonpae = value_;
	}
	bool getIa64Be() const
	{
		return m_ia64Be;
	}
	void setIa64Be(bool value_)
	{
		m_ia64Be = value_;
	}
	const boost::optional<Featuretoggle >& getAcpi() const
	{
		return m_acpi;
	}
	void setAcpi(const boost::optional<Featuretoggle >& value_)
	{
		m_acpi = value_;
	}
	const boost::optional<Featuretoggle >& getApic() const
	{
		return m_apic;
	}
	void setApic(const boost::optional<Featuretoggle >& value_)
	{
		m_apic = value_;
	}
	bool getCpuselection() const
	{
		return m_cpuselection;
	}
	void setCpuselection(bool value_)
	{
		m_cpuselection = value_;
	}
	bool getDeviceboot() const
	{
		return m_deviceboot;
	}
	void setDeviceboot(bool value_)
	{
		m_deviceboot = value_;
	}
	const boost::optional<Featuretoggle >& getDisksnapshot() const
	{
		return m_disksnapshot;
	}
	void setDisksnapshot(const boost::optional<Featuretoggle >& value_)
	{
		m_disksnapshot = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	bool m_pae;
	bool m_nonpae;
	bool m_ia64Be;
	boost::optional<Featuretoggle > m_acpi;
	boost::optional<Featuretoggle > m_apic;
	bool m_cpuselection;
	bool m_deviceboot;
	boost::optional<Featuretoggle > m_disksnapshot;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Guest

namespace Capability
{
namespace Xml
{
struct Guest
{
	Guest();

	EOsType getOsType() const
	{
		return m_osType;
	}
	void setOsType(EOsType value_)
	{
		m_osType = value_;
	}
	const Arch& getArch() const
	{
		return m_arch;
	}
	void setArch(const Arch& value_)
	{
		m_arch = value_;
	}
	const boost::optional<Features1 >& getFeatures() const
	{
		return m_features;
	}
	void setFeatures(const boost::optional<Features1 >& value_)
	{
		m_features = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EOsType m_osType;
	Arch m_arch;
	boost::optional<Features1 > m_features;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Capabilities

namespace Capability
{
namespace Xml
{
struct Capabilities
{
	const Host& getHost() const
	{
		return m_host;
	}
	void setHost(const Host& value_)
	{
		m_host = value_;
	}
	const QList<Guest >& getGuestList() const
	{
		return m_guestList;
	}
	void setGuestList(const QList<Guest >& value_)
	{
		m_guestList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	Host m_host;
	QList<Guest > m_guestList;
};

} // namespace Xml
} // namespace Capability

///////////////////////////////////////////////////////////////////////////////
// struct Features traits

template<>
struct Traits<Capability::Xml::Features>
{
	typedef Ordered<mpl::vector<Optional<Element<Empty, Name::Strict<955> > >, Optional<Element<Empty, Name::Strict<1801> > >, Optional<Element<Empty, Name::Strict<1802> > >, Optional<Element<Empty, Name::Strict<1803> > > > > marshal_type;

	static int parse(Capability::Xml::Features& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Features& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Topology traits

template<>
struct Traits<Capability::Xml::Topology>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PPositiveInteger, Name::Strict<991> >, Attribute<Capability::Xml::PPositiveInteger, Name::Strict<992> >, Attribute<Capability::Xml::PPositiveInteger, Name::Strict<562> > > > marshal_type;

	static int parse(Capability::Xml::Topology& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Topology& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Pages traits

template<>
struct Traits<Capability::Xml::Pages>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Capability::Xml::PUnit, Name::Strict<61> > >, Attribute<Capability::Xml::PUnsignedInt, Name::Strict<326> >, Text<Capability::Xml::PUnsignedInt > > > marshal_type;

	static int parse(Capability::Xml::Pages& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Pages& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cpuspec traits

template<>
struct Traits<Capability::Xml::Cpuspec>
{
	typedef Ordered<mpl::vector<Element<Text<QString >, Name::Strict<223> >, Optional<Element<Text<QString >, Name::Strict<452> > >, Element<Capability::Xml::Topology, Name::Strict<990> >, ZeroOrMore<Element<Attribute<Capability::Xml::PFeatureName, Name::Strict<102> >, Name::Strict<984> > >, ZeroOrMore<Element<Capability::Xml::Pages, Name::Strict<1844> > > > > marshal_type;

	static int parse(Capability::Xml::Cpuspec& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Cpuspec& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cpu traits

template<>
struct Traits<Capability::Xml::Cpu>
{
	typedef Ordered<mpl::vector<Element<Text<Capability::Xml::EArchnames >, Name::Strict<277> >, Optional<Element<Capability::Xml::Features, Name::Strict<150> > >, Optional<Fragment<Capability::Xml::Cpuspec > > > > marshal_type;

	static int parse(Capability::Xml::Cpu& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Cpu& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct PowerManagement traits

template<>
struct Traits<Capability::Xml::PowerManagement>
{
	typedef Unordered<mpl::vector<Optional<Element<Empty, Name::Strict<1805> > >, Optional<Element<Empty, Name::Strict<1806> > >, Optional<Element<Empty, Name::Strict<1807> > > > > marshal_type;

	static int parse(Capability::Xml::PowerManagement& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::PowerManagement& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct MigrationFeatures traits

template<>
struct Traits<Capability::Xml::MigrationFeatures>
{
	typedef Ordered<mpl::vector<Optional<Element<Empty, Name::Strict<1809> > >, Optional<Element<OneOrMore<Element<Text<Capability::Xml::EUriTransport >, Name::Strict<1811> > >, Name::Strict<1810> > > > > marshal_type;

	static int parse(Capability::Xml::MigrationFeatures& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::MigrationFeatures& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct ScaledInteger traits

template<>
struct Traits<Capability::Xml::ScaledInteger>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Capability::Xml::PUnit, Name::Strict<61> > >, Text<Capability::Xml::PUnsignedLong > > > marshal_type;

	static int parse(Capability::Xml::ScaledInteger& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::ScaledInteger& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Sibling traits

template<>
struct Traits<Capability::Xml::Sibling>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<203> >, Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1049> > > > marshal_type;

	static int parse(Capability::Xml::Sibling& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Sibling& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1844 traits

template<>
struct Traits<Capability::Xml::Anonymous1844>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1818> >, Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1819> >, Attribute<Capability::Xml::PCpuset, Name::Strict<1820> > > > marshal_type;

	static int parse(Capability::Xml::Anonymous1844& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Anonymous1844& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cpu1 traits

template<>
struct Traits<Capability::Xml::Cpu1>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<203> >, Optional<Fragment<Capability::Xml::Anonymous1844 > > > > marshal_type;

	static int parse(Capability::Xml::Cpu1& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Cpu1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cpus traits

template<>
struct Traits<Capability::Xml::Cpus>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1815> >, OneOrMore<Element<Capability::Xml::Cpu1, Name::Strict<212> > > > > marshal_type;

	static int parse(Capability::Xml::Cpus& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Cpus& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cell traits

template<>
struct Traits<Capability::Xml::Cell>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<203> >, Optional<Element<Capability::Xml::ScaledInteger, Name::Strict<318> > >, ZeroOrMore<Element<Capability::Xml::Pages, Name::Strict<1844> > >, Optional<Element<ZeroOrMore<Element<Capability::Xml::Sibling, Name::Strict<1817> > >, Name::Strict<1816> > >, Optional<Element<Capability::Xml::Cpus, Name::Strict<996> > > > > marshal_type;

	static int parse(Capability::Xml::Cell& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Cell& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Cells traits

template<>
struct Traits<Capability::Xml::Cells>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1815> >, OneOrMore<Element<Capability::Xml::Cell, Name::Strict<995> > > > > marshal_type;

	static int parse(Capability::Xml::Cells& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Cells& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Control traits

template<>
struct Traits<Capability::Xml::Control>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1825> >, Optional<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1826> > >, Attribute<Capability::Xml::PUnit, Name::Strict<61> >, Attribute<Capability::Xml::EType, Name::Strict<100> >, Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1827> > > > marshal_type;

	static int parse(Capability::Xml::Control& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Control& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Bank traits

template<>
struct Traits<Capability::Xml::Bank>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<203> >, Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1822> >, Attribute<Capability::Xml::EType, Name::Strict<100> >, Attribute<Capability::Xml::PUnsignedInt, Name::Strict<326> >, Attribute<Capability::Xml::PUnit, Name::Strict<61> >, Attribute<Capability::Xml::PCpuset, Name::Strict<996> >, ZeroOrMore<Element<Capability::Xml::Control, Name::Strict<1824> > > > > marshal_type;

	static int parse(Capability::Xml::Bank& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Bank& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Baselabel traits

template<>
struct Traits<Capability::Xml::Baselabel>
{
	typedef Ordered<mpl::vector<Attribute<QString, Name::Strict<100> >, Text<QString > > > marshal_type;

	static int parse(Capability::Xml::Baselabel& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Baselabel& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Secmodel traits

template<>
struct Traits<Capability::Xml::Secmodel>
{
	typedef Unordered<mpl::vector<Element<Text<QString >, Name::Strict<223> >, Element<Text<QString >, Name::Strict<1800> >, ZeroOrMore<Element<Capability::Xml::Baselabel, Name::Strict<228> > > > > marshal_type;

	static int parse(Capability::Xml::Secmodel& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Secmodel& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Host traits

template<>
struct Traits<Capability::Xml::Host>
{
	typedef Ordered<mpl::vector<Optional<Element<Text<Capability::Xml::VUUID >, Name::Strict<146> > >, Element<Capability::Xml::Cpu, Name::Strict<212> >, Optional<Element<Capability::Xml::PowerManagement, Name::Strict<1797> > >, Optional<Element<Capability::Xml::MigrationFeatures, Name::Strict<1808> > >, Optional<Element<Element<Capability::Xml::Cells, Name::Strict<1814> >, Name::Strict<990> > >, Optional<Element<OneOrMore<Element<Capability::Xml::Bank, Name::Strict<1821> > >, Name::Strict<550> > >, ZeroOrMore<Element<Capability::Xml::Secmodel, Name::Strict<1799> > > > > marshal_type;

	static int parse(Capability::Xml::Host& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Host& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Machine traits

template<>
struct Traits<Capability::Xml::Machine>
{
	typedef Ordered<mpl::vector<Optional<Attribute<QString, Name::Strict<1837> > >, Optional<Attribute<Capability::Xml::PUnsignedInt, Name::Strict<1838> > >, Text<QString > > > marshal_type;

	static int parse(Capability::Xml::Machine& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Machine& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Domain traits

template<>
struct Traits<Capability::Xml::Domain>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EType1, Name::Strict<100> >, Optional<Element<Text<Capability::Xml::PAbsFilePath >, Name::Strict<684> > >, ZeroOrMore<Element<Capability::Xml::Machine, Name::Strict<278> > > > > marshal_type;

	static int parse(Capability::Xml::Domain& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Domain& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Arch traits

template<>
struct Traits<Capability::Xml::Arch>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EArchnames, Name::Strict<102> >, Element<Text<Capability::Xml::EWordsize >, Name::Strict<1833> >, Optional<Element<Text<Capability::Xml::PAbsFilePath >, Name::Strict<684> > >, Optional<Element<Text<Capability::Xml::PAbsFilePath >, Name::Strict<265> > >, ZeroOrMore<Element<Capability::Xml::Machine, Name::Strict<278> > >, OneOrMore<Element<Capability::Xml::Domain, Name::Strict<1> > > > > marshal_type;

	static int parse(Capability::Xml::Arch& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Arch& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Featuretoggle traits

template<>
struct Traits<Capability::Xml::Featuretoggle>
{
	typedef Ordered<mpl::vector<Attribute<Capability::Xml::EVirYesNo, Name::Strict<1843> >, Attribute<Capability::Xml::EVirOnOff, Name::Strict<142> > > > marshal_type;

	static int parse(Capability::Xml::Featuretoggle& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Featuretoggle& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Features1 traits

template<>
struct Traits<Capability::Xml::Features1>
{
	typedef Unordered<mpl::vector<Optional<Element<Empty, Name::Strict<955> > >, Optional<Element<Empty, Name::Strict<1801> > >, Optional<Element<Empty, Name::Strict<1839> > >, Optional<Element<Capability::Xml::Featuretoggle, Name::Strict<958> > >, Optional<Element<Capability::Xml::Featuretoggle, Name::Strict<956> > >, Optional<Element<Empty, Name::Strict<1841> > >, Optional<Element<Empty, Name::Strict<1842> > >, Optional<Element<Capability::Xml::Featuretoggle, Name::Strict<1524> > > > > marshal_type;

	static int parse(Capability::Xml::Features1& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Features1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Guest traits

template<>
struct Traits<Capability::Xml::Guest>
{
	typedef Ordered<mpl::vector<Element<Text<Capability::Xml::EOsType >, Name::Strict<1832> >, Element<Capability::Xml::Arch, Name::Strict<277> >, Optional<Element<Capability::Xml::Features1, Name::Strict<150> > > > > marshal_type;

	static int parse(Capability::Xml::Guest& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Guest& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Capabilities traits

template<>
struct Traits<Capability::Xml::Capabilities>
{
	typedef Ordered<mpl::vector<Element<Capability::Xml::Host, Name::Strict<505> >, ZeroOrMore<Element<Capability::Xml::Guest, Name::Strict<400> > > > > marshal_type;

	static int parse(Capability::Xml::Capabilities& , QStack<QDomElement>& );
	static int generate(const Capability::Xml::Capabilities& , QDomElement& );
};

} // namespace Libvirt

#endif // __CAPABILITY_TYPE_H__
