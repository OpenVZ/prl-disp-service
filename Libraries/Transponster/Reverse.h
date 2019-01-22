///////////////////////////////////////////////////////////////////////////////
///
/// @file Reverse.h
///
/// Convertor from the SDK config format into the libvirt one.
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

#ifndef __REVERSE_H__
#define __REVERSE_H__

#include <boost/function.hpp>

#include "capability_type.h"
#include "iface_type.h"
#include "domain_type.h"
#include "network_type.h"
#include "snapshot_type.h"
#include "blockexport_type.h"
#include "blocksnapshot_type.h"
#include <prlxmlmodel/VtInfo/VtInfo.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlxmlmodel/BackupActivity/BackupActivity.h>
#include <prlxmlmodel/NetworkConfig/CVirtualNetwork.h>
#include <prlxmlmodel/HostHardwareInfo/CHwNetAdapter.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>

class CHwUsbDevice;

namespace Transponster
{
namespace Vm
{
namespace Reverse
{
///////////////////////////////////////////////////////////////////////////////
// struct Cpu

struct Cpu
{
	Cpu(const CVmCpu& input_, const VtInfo& vt_);

	PRL_RESULT setMask();
	PRL_RESULT setNode();
	PRL_RESULT setUnits();
	PRL_RESULT setLimit();
	PRL_RESULT setNumber();

	const boost::optional<Libvirt::Domain::Xml::Vcpu>& getVcpu() const
	{
		return m_vcpu;
	}
	const boost::optional<Libvirt::Domain::Xml::Cputune>& getTune() const
	{
		return m_tune;
	}
	const boost::optional<Libvirt::Domain::Xml::Numatune>& getNuma() const
	{
		return m_numa;
	}

private:
	CVmCpu m_input;
	VtInfo m_vt;
	boost::optional<Libvirt::Domain::Xml::Vcpu> m_vcpu;
	boost::optional<Libvirt::Domain::Xml::Cputune> m_tune;
	boost::optional<Libvirt::Domain::Xml::Numatune> m_numa;
};

///////////////////////////////////////////////////////////////////////////////
// struct CpuFeaturesMask

struct CpuFeaturesMask
{
	explicit CpuFeaturesMask(CVmConfiguration& input): m_input(&input)
	{
	}

	void getFeatures(const VtInfo& vt_, Libvirt::Domain::Xml::Cpu &cpu);
	void setDisabledFeatures(const Libvirt::Domain::Xml::Cpu &cpu);

private:
	CVmConfiguration *m_input;
};

///////////////////////////////////////////////////////////////////////////////
// struct Dimm
 
struct Dimm
{
	Dimm(quint32 nodeId_, quint32 size_): m_size(size_), m_nodeId(nodeId_)
 	{
 	}
 
	quint32 getSize() const
	{
		return m_size;
	}
	quint32 getNodeId() const
	{
		return m_nodeId;
	}
 
private:
	const quint32 m_size;
	const quint32 m_nodeId;
};
 
///////////////////////////////////////////////////////////////////////////////
// struct Device

template<class T>
struct Device;

template<>
struct Device<Dimm>
{
	static QString getPlugXml(const Dimm& model_);
};

template<>
struct Device<CVmHardDisk>
{
	static QString getPlugXml(const CVmHardDisk& model_);
	static QString getTargetName(const CVmHardDisk& model_);
};

template<>
struct Device<CVmFloppyDisk>
{
	static QString getUpdateXml(const CVmFloppyDisk& model_);
};

template<>
struct Device<CVmOpticalDisk>
{
	static QString getUpdateXml(const CVmOpticalDisk& model_);
};

template<>
struct Device<CVmSerialPort>
{
	static Prl::Expected<QString, ::Error::Simple>
		getPlugXml(const CVmSerialPort& model_);
	static Prl::Expected<Libvirt::Domain::Xml::Qemucdev, ::Error::Simple>
		getLibvirtXml(const CVmSerialPort& model_);
};

template<>
struct Device<CVmGenericNetworkAdapter>
{
	static Prl::Expected<QString, ::Error::Simple>
		getPlugXml(const CVmGenericNetworkAdapter& model_);
	static Prl::Expected<QString, ::Error::Simple>
		getUpdateXml(const CVmGenericNetworkAdapter& model_);
};

template<>
struct Device<CHwUsbDevice>
{
	static QString getAlias(const CHwUsbDevice& model_);
	static QString getPlugXml(const CHwUsbDevice& model_);
	static Libvirt::Domain::Xml::Hostdev getView(const CHwUsbDevice& model_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder
{
	explicit Builder(const CVmConfiguration& input_);

	PRL_RESULT setBlank();
	PRL_RESULT setSettings();
	PRL_RESULT setDevices();
	PRL_RESULT setResources(const VtInfo& vt_);

	QString getResult();

protected:
	PRL_RESULT setIdentification();

	CVmConfiguration m_input;
	QScopedPointer<Libvirt::Domain::Xml::Domain> m_result;

private:
	bool getStartupOptions(Libvirt::Domain::Xml::Os2& os_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm: Builder
{
	explicit Vm(const CVmConfiguration& input_);

	PRL_RESULT setBlank();
	PRL_RESULT setDevices();
	using Builder::setIdentification;

private:
	void setFeatures();
};

///////////////////////////////////////////////////////////////////////////////
// struct Mixer

struct Mixer: Builder
{
	Mixer(const CVmConfiguration& input_, char* xml_);

	PRL_RESULT setBlank();
	PRL_RESULT setDevices();
	PRL_RESULT setIdentification();
	PRL_RESULT setResources(const VtInfo&);
};

///////////////////////////////////////////////////////////////////////////////
// struct Fixer

struct Fixer: Builder
{
	Fixer(const CVmConfiguration& input_, char* xml_);

	PRL_RESULT setBlank();
	PRL_RESULT setIdentification();
	PRL_RESULT setSettings()
	{
		return PRL_ERR_SUCCESS;
	}
	PRL_RESULT setResources(const VtInfo&);
	PRL_RESULT setDevices();
};

///////////////////////////////////////////////////////////////////////////////
// struct Pipeline

struct Pipeline: Libvirt::Details::Value::Bin<QString>
{
	typedef Libvirt::Domain::Xml::Domain object_type;
	typedef boost::function<PRL_RESULT (object_type& )>
		action_type;

	explicit Pipeline(const action_type& action_): m_action(action_)
	{
	}

	PRL_RESULT operator()(object_type object_);

private:
	action_type m_action;
};

///////////////////////////////////////////////////////////////////////////////
// struct Clock

struct Clock: std::unary_function<Pipeline::object_type&, PRL_RESULT>
{
	explicit Clock(const qint64 offset_): m_offset(offset_)
	{
	}

	result_type operator()(argument_type);

private:
	qint64 m_offset;
};

namespace Usb
{
///////////////////////////////////////////////////////////////////////////////
// struct Operator

struct Operator
{
	explicit Operator(const CHwUsbDevice& model_): m_model(&model_)
	{
	}

	PRL_RESULT plug(Pipeline::object_type& object_);
	PRL_RESULT unplug(Pipeline::object_type& object_);

private:
	const CHwUsbDevice* m_model;
};

} // namespace Usb
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
// struct Internal

struct Internal: std::unary_function<const CVmHardDisk&, boost::optional<Libvirt::Snapshot::Xml::Disk> >
{
	boost::optional<Libvirt::Snapshot::Xml::Disk> operator()(const CVmHardDisk& disk_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct External

struct External: std::unary_function<const CVmHardDisk&, boost::optional<Libvirt::Snapshot::Xml::Disk> >
{

	External(QStringList disks_, QString snapshot_): m_disks(disks_), m_snapshot(snapshot_)
	{
	}

	boost::optional<Libvirt::Snapshot::Xml::Disk> operator()(const CVmHardDisk& disk_) const;

private:
	QStringList m_disks;
	QString m_snapshot;
};

///////////////////////////////////////////////////////////////////////////////
// struct Reverse

struct Reverse
{
	typedef boost::function1<boost::optional<Libvirt::Snapshot::Xml::Disk>, const CVmHardDisk&> policy_type;

	Reverse(const QString& uuid_, const QString& description_,
		const CVmConfiguration& input_);

	PRL_RESULT setIdentity();
	PRL_RESULT setInstructions();
	void setMemory();
	void setPolicy(policy_type policy_)
	{
		m_policy = policy_;
	}

	QString getResult() const;

private:
	QString m_uuid;
	QString m_description;
	CVmHardware m_hardware;
	Libvirt::Snapshot::Xml::Domainsnapshot m_result;
	policy_type m_policy;
};

///////////////////////////////////////////////////////////////////////////////
// struct Block

struct Block: Libvirt::Details::Value::Bin<Libvirt::Blocksnapshot::Xml::Domainblocksnapshot>
{
	typedef BackupSnapshot object_type;

	PRL_RESULT operator()(const object_type& object_);
};

namespace Export
{
///////////////////////////////////////////////////////////////////////////////
// struct Request

struct Request: Libvirt::Details::Value::Bin<Libvirt::Blockexport::Xml::Domainblockexport_>
{
	typedef boost::tuple<QString, QString, QUrl> component_type;
	typedef QList<component_type> object_type;

	PRL_RESULT operator()(const object_type& object_);
};

} // namespace Export
} // namespace Snapshot
} // namespace Transponster

#endif // __REVERSE_H__

