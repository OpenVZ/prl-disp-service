///////////////////////////////////////////////////////////////////////////////
///
/// @file Direct_p.h
///
/// Convertor from the libvirt config format into the SDK one. The private part.
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

#ifndef __DIRECT_P_H__
#define __DIRECT_P_H__

#include "domain_type.h"
#include <QFile>
#include <QDomDocument>
#include <boost/function.hpp>
#include <prlcommon/PrlUuid/Uuid.h>
#include <Libraries/PrlNetworking/PrlNetLibrary.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

namespace Transponster
{
namespace Boot
{
///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot
{
	explicit Slot(CVmStartupOptions::CVmBootDevice& device_): m_device(&device_)
	{
	}

	void set(PRL_DEVICE_TYPE t_, uint index_)
	{
		m_device->deviceType = t_;
		m_device->deviceIndex = index_;
	}

private:
	CVmStartupOptions::CVmBootDevice* m_device;
};

} // namespace Boot

///////////////////////////////////////////////////////////////////////////////
// struct Clip

struct Clip
{
	typedef QList<Libvirt::Domain::Xml::Controller> controllerList_type;
	typedef boost::optional<Libvirt::Domain::Xml::PPositiveInteger::value_type>
		order_type;

	Clip(CVmStartupOptions& boot_, const controllerList_type& controllers_)
		: m_bootList(&boot_.m_lstBootDeviceList), m_controllerList(&controllers_)
	{
	}

	Boot::Slot getBootSlot(const order_type& order_);

	boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE> getControllerModel(const Libvirt::Domain::Xml::Disk& disk_) const;

private:
	QList<CVmStartupOptions::CVmBootDevice*>* m_bootList;
	const controllerList_type *m_controllerList;
};

namespace Chipset
{
typedef QPair<quint32, quint32> model_type;

///////////////////////////////////////////////////////////////////////////////
// struct Generic

template<class T>
struct Generic
{
	Prl::Expected<model_type, PRL_RESULT>
		deserialize(const QString& text_) const;
	QString serialize(model_type::second_type version_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct i440fx

struct i440fx: private Generic<i440fx>
{
	enum
	{
		TYPE = 1
	};
	static const QString s_PREFIX;

	Prl::Expected<model_type, PRL_RESULT>
		deserialize(const QString& text_) const;
	QString serialize(model_type::second_type version_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Q35

struct Q35: private Generic<Q35>
{
	enum
	{
		TYPE = 2
	};
	static const QString s_PREFIX;

	Prl::Expected<model_type, PRL_RESULT>
		deserialize(const QString& text_) const;
	QString serialize(model_type::second_type version_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Marshal

struct Marshal
{
	model_type deserialize_(const QString& text_) const;
	::Chipset deserialize(const QString& text_) const;
	QString serialize(const model_type& object_) const;
	QString serialize(const ::Chipset& object_) const;
};

} // namespace Chipset

namespace Visitor
{
namespace Source
{
///////////////////////////////////////////////////////////////////////////////
// struct Clustered

template<class T, int I, int R>
struct Clustered: boost::static_visitor<bool>
{
	Clustered(): m_device()
	{
	}

	template<class U>
	bool operator()(const U& ) const
	{
		return false;
	}
	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 4>::type& source_) const
	{
		const Libvirt::Domain::Xml::Source4* v = source_.getValue().get_ptr();
		if (NULL == v)
			return false;

		if (!v->getFile())
			return false;

		getDevice().setEmulatedType(I);
		getDevice().setSystemName(v->getFile().get());
		getDevice().setUserFriendlyName(v->getFile().get());

		return true;
	}
	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 0>::type& source_) const
	{
		const Libvirt::Domain::Xml::Source* v = source_.getValue().get_ptr();
		if (NULL == v)
			return false;

		getDevice().setEmulatedType(R);
		getDevice().setSystemName(v->getDev());
		getDevice().setUserFriendlyName(v->getDev());

		return true;
	}
	void setDevice(T& value_)
	{
		m_device = &value_;
	}

protected:
	T& getDevice() const
	{
		return *m_device;
	}

private:
	T* m_device;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T>
struct Unit;

template<>
struct Unit<CVmFloppyDisk>: Clustered<CVmFloppyDisk, PVE::FloppyDiskImage, PVE::RealFloppyDisk>
{
};

template<>
struct Unit<CVmOpticalDisk>: Clustered<CVmOpticalDisk, PVE::CdRomImage, PVE::RealCdRom>
{
};

template<>
struct Unit<CVmHardDisk>: Clustered<CVmHardDisk, PVE::HardDiskImage, PVE::RealHardDisk>
{
	using Clustered<CVmHardDisk, PVE::HardDiskImage, PVE::RealHardDisk>::operator();

	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 3>::type& source_) const;
};

} // namespace Source

///////////////////////////////////////////////////////////////////////////////
// struct Clustered

template<class T>
struct Clustered
{
	Clustered(): m_device(new T())
	{
	}

	bool operator()(const Libvirt::Domain::Xml::Disk& disk_)
	{
		if (m_device.isNull())
			return false;

		getDevice().setEnabled(PVE::DeviceEnabled);
		getDevice().setConnected();
		if (disk_.getTarget().getBus())
		{
			switch (disk_.getTarget().getBus().get())
			{
			case Libvirt::Domain::Xml::EBusFdc:
			case Libvirt::Domain::Xml::EBusIde:
				getDevice().setInterfaceType(PMS_IDE_DEVICE);
				break;
			case Libvirt::Domain::Xml::EBusScsi:
				getDevice().setInterfaceType(PMS_SCSI_DEVICE);
				break;
			case Libvirt::Domain::Xml::EBusSata:
				getDevice().setInterfaceType(PMS_SATA_DEVICE);
				break;
			case Libvirt::Domain::Xml::EBusVirtio:
				getDevice().setInterfaceType(PMS_VIRTIO_BLOCK_DEVICE);
				break;
			default:
				return false;
			}
		}
		Source::Unit<T> v;
		v.setDevice(getDevice());
		return boost::apply_visitor(v, disk_.getDiskSource());
	}
	T* getResult()
	{
		return m_device.take();
	}

protected:
	T& getDevice() const
	{
		return *m_device;
	}

private:
	QScopedPointer<T> m_device;
};

///////////////////////////////////////////////////////////////////////////////
// struct Floppy

struct Floppy: private Clustered<CVmFloppyDisk>
{
	Floppy(CVmHardware& hardware_, Clip& clip_):
		m_hardware(&hardware_), m_clip(&clip_)
	{
	}

	PRL_RESULT operator()(const Libvirt::Domain::Xml::Disk& disk_);

private:
	CVmHardware* m_hardware;
	Clip* m_clip;
};

///////////////////////////////////////////////////////////////////////////////
// struct Iotune

struct Iotune: boost::static_visitor<void>
{
	Iotune(CVmHardDisk* disk_): m_disk(disk_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}

	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice1110::types, 0>::type& iopsLimit_) const;
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice1106::types, 0>::type& ioLimit_) const;

private:
	CVmHardDisk* m_disk;
};

///////////////////////////////////////////////////////////////////////////////
// struct BackingChain

struct BackingChain: boost::static_visitor<void>
{
	typedef Libvirt::Domain::Xml::VDiskSource::types list_type;

	explicit BackingChain(CVmHardDisk& disk_): m_disk(&disk_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}

	void operator()(const mpl::at_c<list_type, 4>::type& image_) const;

private:
	CVmHardDisk* m_disk;
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk

struct Disk: private Clustered<CVmHardDisk>
{
	Disk(CVmHardware& hardware_, Clip& clip_):
		m_hardware(&hardware_), m_clip(&clip_)
	{
	}

	PRL_RESULT operator()(const Libvirt::Domain::Xml::Disk& disk_);

private:
	CVmHardware* m_hardware;
	Clip* m_clip;
};

///////////////////////////////////////////////////////////////////////////////
// struct Cdrom

struct Cdrom: private Clustered<CVmOpticalDisk>
{
	Cdrom(CVmHardware& hardware_, Clip& clip_):
		m_hardware(&hardware_), m_clip(&clip_)
	{
	}

	PRL_RESULT operator()(const Libvirt::Domain::Xml::Disk& disk_);

private:
	CVmHardware* m_hardware;
	Clip* m_clip;
};

///////////////////////////////////////////////////////////////////////////////
// struct Graphics

struct Graphics: boost::static_visitor<PRL_RESULT>
{
	explicit Graphics(CVmConfiguration& vm_): m_vm(&vm_)
	{
	}

	template<class T>
	PRL_RESULT operator()(const T& ) const
	{
		return PRL_ERR_UNIMPLEMENTED;
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VGraphics::types, 1>::type& vnc_) const;

private:
	CVmConfiguration* m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Ips

struct Ips
{
	QList<QString> operator()(const QList<Libvirt::Domain::Xml::Ip>& ips_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Network

struct Network: boost::static_visitor<PRL_RESULT>
{
	Network(CVmHardware& hardware_, Clip& clip_):
		m_hardware(&hardware_), m_clip(&clip_)
	{
	}

	template<class T>
	PRL_RESULT operator()(const T& ) const
	{
		return PRL_ERR_UNIMPLEMENTED;
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 0>::type& bridge_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 3>::type& network_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 4>::type& direct_) const;

private:
	static PRL_VM_NET_ADAPTER_TYPE parseAdapterType(const QString& type);
	static CNetPktFilter* buildFilter(
			const boost::optional<Libvirt::Domain::Xml::FilterrefNodeAttributes> &filterref);

	CVmHardware* m_hardware;
	Clip* m_clip;
};

///////////////////////////////////////////////////////////////////////////////
// struct DiskForm

struct DiskForm: boost::static_visitor<PRL_RESULT>
{
	DiskForm(CVmHardware& hardware_, Clip& clip_, const Libvirt::Domain::Xml::Disk& disk_):
		m_clip(&clip_), m_hardware(hardware_), m_disk(disk_)
	{
	}

	template <class T>
	PRL_RESULT operator()(const T& ) const
	{
		return PRL_ERR_SUCCESS;
	}

	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 0>::type& disk_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 1>::type& lun_) const;

private:
	Clip* m_clip;
	CVmHardware& m_hardware;
	const Libvirt::Domain::Xml::Disk& m_disk;
};

///////////////////////////////////////////////////////////////////////////////
// struct Device

struct Device: boost::static_visitor<PRL_RESULT>
{
	Device(CVmConfiguration& vm_, Clip& clip_):
		m_clip(&clip_), m_vm(&vm_)
	{
	}

	template<class T>
	PRL_RESULT operator()(const T& ) const
	{
		return PRL_ERR_SUCCESS;
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 0>::type& disk_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 1>::type& controller_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 4>::type& interface_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 5>::type& input_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 6>::type& sound_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 7>::type& pci_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 8>::type& graphics_) const
	{
		return boost::apply_visitor(Graphics(*m_vm), graphics_.getValue()); 
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 9>::type& video_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 11>::type& parallel_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 12>::type& serial_) const;

private:
	Clip* m_clip;
	CVmConfiguration* m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Clock

struct Clock: boost::static_visitor<void>
{
	explicit Clock(::Clock& clock_): m_clock(&clock_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 0>::type& fixed_) const;
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 2>::type& variable_) const;

private:
	::Clock* m_clock;
};

///////////////////////////////////////////////////////////////////////////////
// struct Adjustment

struct Adjustment: boost::static_visitor<void>
{
	Adjustment(Libvirt::Domain::Xml::Clock& result_, qint64 offset_)
		: m_result(&result_), m_offset(offset_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 0>::type& fixed_) const;
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 2>::type& variable_) const;

private:
	Libvirt::Domain::Xml::Clock* m_result;
	qint64 m_offset;
};

///////////////////////////////////////////////////////////////////////////////
// struct Uuid

template<class T>
struct Uuid: boost::static_visitor<void>
{
	typedef boost::function1<void, const QString& > sink_type;

	explicit Uuid(const sink_type& sink_): m_sink(sink_)
	{
	}

	void operator()(const typename mpl::at_c<typename T::types, 0>::type& /*string_*/) const
	{
	}
	void operator()(const typename mpl::at_c<typename T::types, 1>::type& string_) const
	{
		QString u = "{" + string_.getValue() + "}";
		m_sink(u);
	}

private:
	sink_type m_sink;
};

///////////////////////////////////////////////////////////////////////////////
// struct Addressing

struct Addressing: boost::static_visitor<void>
{
	explicit Addressing(CHwNetAdapter& sink_): m_sink(&sink_)
	{
	}

	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VInterfaceAddressing::types, 0>::type& group_) const
	{
		consume(group_.getValue().getProtocol());
		consume(group_.getValue().getProtocol2());
	}
	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VInterfaceAddressing::types, 1>::type& group_) const
	{
		consume(group_.getValue().getProtocol());
		consume(group_.getValue().getProtocol2());
	}
	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice2470::types, 0>::type& ) const
	{
		m_sink->setConfigureWithDhcp(true);
	}
	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice2470::types, 1>::type& ) const
	{
		m_sink->setConfigureWithDhcp(false);
	}

private:
	void consume(const boost::optional<Libvirt::Iface::Xml::VChoice2470 >& ipv4_) const
	{
		if (ipv4_)
			boost::apply_visitor(*this, ipv4_.get());
	}
	void consume(const boost::optional<Libvirt::Iface::Xml::Protocol >& ipv6_) const
	{
		m_sink->setConfigureWithDhcpIPv6(ipv6_ && ipv6_.get().getDhcp());
	}

	CHwNetAdapter* m_sink;
};

///////////////////////////////////////////////////////////////////////////////
// struct Nodedev

struct Nodedev: boost::static_visitor<void>
{
	Nodedev(const QString& alias_, CVmHardware& sink_):
		m_alias(alias_), m_sink(&sink_)
	{
	}

	template<class T>
	result_type operator()(const T& ) const
	{
		return;
	}

	result_type operator()(const mpl::at_c<Libvirt::Domain::Xml::VHostdevsubsys::types, 0>::type& alternative_) const;
	result_type operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice917::types, 0>::type& alternative_) const
	{
		return boost::apply_visitor(*this, alternative_.getValue().getHostdevsubsys());
	}

private:
	const QString m_alias;
	CVmHardware* m_sink;
};

namespace Controller
{
///////////////////////////////////////////////////////////////////////////////
// struct Collect

struct Collect: boost::static_visitor<void>
{
	typedef QList<Libvirt::Domain::Xml::Controller> output_type;

	explicit Collect(output_type& output_) : m_output(&output_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}

	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 1>::type& controller_) const
	{
		m_output->append(controller_.getValue());
	}

private:
	output_type *m_output;
};

///////////////////////////////////////////////////////////////////////////////
// struct Scsi

struct Scsi: boost::static_visitor<void>
{
	explicit Scsi(boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE>& model_) : m_model(&model_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}

	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice625::types, 1>::type& model_) const
	{
		if (!model_.getValue())
			return;
		switch (model_.getValue().get())
		{
		case Libvirt::Domain::Xml::EModelBuslogic:
			m_model->reset(PCD_BUSLOGIC);
			break;
		case Libvirt::Domain::Xml::EModelLsilogic:
			m_model->reset(PCD_LSI_SPI);
			break;
		case Libvirt::Domain::Xml::EModelLsisas1068:
		case Libvirt::Domain::Xml::EModelLsisas1078:
			m_model->reset(PCD_LSI_SAS);
			break;
		case Libvirt::Domain::Xml::EModelVirtioScsi:
			m_model->reset(PCD_VIRTIO_SCSI);
			break;
		case Libvirt::Domain::Xml::EModelHvScsi:
			m_model->reset(PCD_HYPER_V_SCSI);
			break;
		default:
			break;
		}
	}

private:
	boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE> *m_model;
};

///////////////////////////////////////////////////////////////////////////////
// struct Usb

struct Usb: boost::static_visitor<void>
{
	Usb(CVmUsbController& settings_, CVmHardware& hardware_)
		: m_settings(&settings_), m_hardware(&hardware_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}

	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice625::types, 2>::type& usb_) const;
private:
	CVmUsbController *m_settings;
	CVmHardware *m_hardware;
};

///////////////////////////////////////////////////////////////////////////////
// struct Pci

struct Pci: boost::static_visitor<bool>
{
	typedef CHwGenericPciDevice bin_type;

	explicit Pci(bin_type& bin_): m_bin(&bin_)
	{
	}

	template<class T>
	bool operator()(const T& ) const
	{
		return false;
	}

	bool operator()(const mpl::at_c<Libvirt::Nodedev::Xml::VCapability::types, 1>::type& pci_) const;

private:
	bin_type* m_bin;
};

} // namespace Controller

namespace Address
{
namespace String
{
///////////////////////////////////////////////////////////////////////////////
// struct Conductor

struct Conductor: boost::static_visitor<QString>
{
	template<class T>
	QString operator()(const T& t) const
	{
		return QString("%1").arg(t.getValue());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Ipv4Mask

struct Ipv4Mask: boost::static_visitor<QString>
{
	QString operator()(const mpl::at_c<Libvirt::Domain::Xml::VIpPrefix::types, 0>::type& mask4_) const
	{
		return PrlNet::getIPv4MaskFromPrefix(mask4_.getValue()).toString();
	}
	QString operator()(const mpl::at_c<Libvirt::Domain::Xml::VIpPrefix::types, 1>::type& mask6_) const
	{
		mpl::at_c<Libvirt::Domain::Xml::VIpPrefix::types, 0>::type m;
		if (!m.setValue(mask6_.getValue()))
			m.setValue(32);

		return (*this)(m);
	}
};

} // namespace String

///////////////////////////////////////////////////////////////////////////////
// struct Ip

struct Ip: boost::static_visitor<void>
{
	explicit Ip(CHostOnlyNetwork& result_): m_result(&result_)
	{
	}
	
	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 0>::type& addr4_) const
	{
		m_result->setHostIPAddress(QHostAddress(addr4_.getValue()));
		m_result->setDhcpIPAddress(QHostAddress(addr4_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 1>::type& addr6_) const
	{
		m_result->setHostIP6Address(QHostAddress(addr6_.getValue()));
		m_result->setDhcpIP6Address(QHostAddress(addr6_.getValue()));
	}

private:
	CHostOnlyNetwork* m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct Mask

struct Mask: boost::static_visitor<void>
{
	Mask(CHostOnlyNetwork& result_, const QString &family_):
		m_result(&result_), m_family(family_)
	{
	}
	
	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VChoice2401::types, 0>::type& addr_) const
	{
		m_result->setIPNetMask(QHostAddress(addr_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VChoice2401::types, 1>::type& prefix_) const
	{
		boost::apply_visitor(*this, prefix_.getValue());
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpPrefix::types, 0>::type& prefix4_) const
	{
		// Short netmask is always treated as IPv4.
		if (m_family == "ipv6")
			m_result->setIP6NetMask(PrlNet::getIPv6MaskFromPrefix(prefix4_.getValue()));
		else
			m_result->setIPNetMask(PrlNet::getIPv4MaskFromPrefix(prefix4_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpPrefix::types, 1>::type& prefix6_) const
	{
		m_result->setIP6NetMask(PrlNet::getIPv6MaskFromPrefix(prefix6_.getValue()));
	}

private:
	CHostOnlyNetwork* m_result;
	QString m_family;
};

} // namespace Address

namespace Dhcp
{

///////////////////////////////////////////////////////////////////////////////
// struct Start

struct Start: boost::static_visitor<void>
{
	explicit Start(CHostOnlyNetwork& result_): m_result(&result_)
	{
	}
	
	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 0>::type& addr4_) const
	{
		m_result->getDHCPServer()->setIPScopeStart(QHostAddress(addr4_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 1>::type& addr6_) const
	{
		m_result->getDHCPv6Server()->setIPScopeStart(QHostAddress(addr6_.getValue()));
	}

private:
	CHostOnlyNetwork* m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct End

struct End: boost::static_visitor<void>
{
	explicit End(CHostOnlyNetwork& result_): m_result(&result_)
	{
	}
	
	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 0>::type& addr4_) const
	{
		m_result->getDHCPServer()->setIPScopeEnd(QHostAddress(addr4_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 1>::type& addr6_) const
	{
		m_result->getDHCPv6Server()->setIPScopeEnd(QHostAddress(addr6_.getValue()));
	}

private:
	CHostOnlyNetwork* m_result;
};

} // namespace Dhcp

namespace Bridge
{

struct Master: boost::static_visitor<bool>
{
	explicit Master(CHwNetAdapter& result_): m_result(&result_)
	{
	}

	template<class T>
	bool operator()(const T&) const
	{
		return false;
	}
	bool operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice2462::types, 0>::type& ethernet_) const
	{
		m_result->setDeviceName(ethernet_.getValue().getName());
		if (ethernet_.getValue().getMac())
		{
			m_result->setMacAddress(ethernet_.getValue()
				.getMac().get().toUpper());
		}
		return true;
	}
	bool operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice2462::types, 1>::type& vlan_) const
	{
		Libvirt::Iface::Xml::VlanInterfaceCommon c = vlan_.getValue().getVlanInterfaceCommon();
		if (c.getName())
			m_result->setDeviceName(c.getName().get());

		Libvirt::Iface::Xml::Vlan v = vlan_.getValue().getVlan();
		m_result->setVLANTag(v.getTag());
		return true;
	}
	bool operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice2462::types, 2>::type& bond_) const
	{
		Libvirt::Iface::Xml::BondInterfaceCommon c = bond_.getValue().getBondInterfaceCommon();
		m_result->setDeviceName(c.getName());
		return true;
	}

private:
	CHwNetAdapter* m_result;
};

} // namespace Bridge

namespace Startup
{
///////////////////////////////////////////////////////////////////////////////
// struct Bios

struct Bios: boost::static_visitor<void>
{
	explicit Bios(CVmStartupBios& bios_): m_bios(&bios_)
	{
	}

	template<class T>
	void operator()(const T&) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type& os_) const
	{
		const Libvirt::Domain::Xml::Loader* l = os_.getValue().getLoader().get_ptr();
		if (l && l->getType() && l->getType().get() == Libvirt::Domain::Xml::EType2Pflash)
			m_bios->setEfiEnabled(true);

		const Libvirt::Domain::Xml::Nvram* n = os_.getValue().getNvram().get_ptr();
		if (n && n->getOwnValue())
			m_bios->setNVRAM(n->getOwnValue().get());
	}

private:
	CVmStartupBios* m_bios;
};

///////////////////////////////////////////////////////////////////////////////
// struct Bootmenu

struct Bootmenu: boost::static_visitor<void>
{
	explicit Bootmenu(CVmStartupOptions& opts_): m_opts(&opts_)
	{
	}

	template<class T>
	void operator()(const T&) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type& os_) const
	{
		const Libvirt::Domain::Xml::Bootmenu* m = os_.getValue().getBootmenu().get_ptr();
		if (m)
			m_opts->setAllowSelectBootDevice(m->getEnable() == Libvirt::Domain::Xml::EVirYesNoYes);
	}

private:
	CVmStartupOptions* m_opts;
};

} // namespace Startup

namespace Fixup
{
///////////////////////////////////////////////////////////////////////////////
// struct DiskForm

struct DiskForm: boost::static_visitor<PRL_RESULT>
{
	DiskForm(CVmHardware* hardware_, QList<Libvirt::Domain::Xml::VChoice985>* list_,
			const Libvirt::Domain::Xml::Disk& disk_)
		: m_hardware(hardware_), m_list(list_), m_disk(disk_)
	{
	}

	template <typename T>
	void setDiskSource(const QList<T*> list_, uint index_) const;

	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 0>::type& disk_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 1>::type& lun_) const;

private:
	CVmHardware* m_hardware;
	QList<Libvirt::Domain::Xml::VChoice985>* m_list;
	mutable Libvirt::Domain::Xml::Disk m_disk;
};

///////////////////////////////////////////////////////////////////////////////
// struct Device

struct Device: boost::static_visitor<PRL_RESULT>
{
	Device(const CVmHardware& hardware_, QList<Libvirt::Domain::Xml::VChoice985>& list_)
		: m_hardware(hardware_), m_list(&list_)
	{
	}

	template<class T>
	PRL_RESULT operator()(const T& device_) const
	{
		*m_list << device_;
		return PRL_ERR_SUCCESS;
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 0>::type& disk_);
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 12>::type& serial_);

private:
	CVmHardware m_hardware;
	QList<Libvirt::Domain::Xml::VChoice985>* m_list;
};

///////////////////////////////////////////////////////////////////////////////
// struct Os

struct Os: boost::static_visitor<Libvirt::Domain::Xml::VOs>
{
	explicit Os(const QString& nvram_): m_nvram(nvram_)
	{
	}

	template<class T>
	Libvirt::Domain::Xml::VOs operator()(const T& os_) const
	{
		return Libvirt::Domain::Xml::VOs(os_);
	}
	Libvirt::Domain::Xml::VOs operator()(const mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type& os_) const
	{
		Libvirt::Domain::Xml::Os2 os = os_.getValue();
		Libvirt::Domain::Xml::Nvram n;
		if (os.getNvram())
			n = os.getNvram().get();
		n.setOwnValue(m_nvram);
		os.setNvram(n);
		mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type vos;
		vos.setValue(os);
		return vos;
	}

private:
	QString m_nvram;
};

} // namespace Fixup

namespace Mixer
{
///////////////////////////////////////////////////////////////////////////////
// struct Device

struct Device: boost::static_visitor<void>
{
	template<class T>
	void operator()(const T&) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 7>::type& passthrough_)
	{
		m_result << passthrough_;
	}
	const QList<Libvirt::Domain::Xml::VChoice985>& getResult() const
	{
		return m_result;
	}

private:
	QList<Libvirt::Domain::Xml::VChoice985> m_result;
};      

namespace Os
{
///////////////////////////////////////////////////////////////////////////////
// struct Type

struct Type: boost::static_visitor<Libvirt::Domain::Xml::VChoice305>
{
	typedef mpl::at_c<Libvirt::Domain::Xml::VChoice305::types, 0>::type chosen_type;

	template<class T, class U>
	result_type operator()(const T&, const U& new_) const
	{
		return new_;
	}
	result_type operator()(const chosen_type& old_, chosen_type new_) const
	{
		Libvirt::Domain::Xml::Hvmx86 x = new_.getValue();
		x.setMachine(old_.getValue().getMachine());
		new_.setValue(x);

		return new_;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit: boost::static_visitor<Libvirt::Domain::Xml::VOs>
{
	typedef mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type chosen_type;

	template<class T, class U>
	result_type operator()(const T&, const U& new_) const
	{
		return new_;
	}
	result_type operator()(const chosen_type& old_, chosen_type new_) const
	{
		Libvirt::Domain::Xml::Os2 a = old_.getValue();
		Libvirt::Domain::Xml::Os2 b = new_.getValue();

		typedef boost::optional<Libvirt::Domain::Xml::VChoice305> type_type;
		type_type x = a.getType(), y = b.getType();
		if (x)
		{
			if (y)
				b.setType(boost::apply_visitor(Type(), x.get(), y.get()));
			else
				b.setType(x);
			new_.setValue(b);
		}

		return new_;
	}
};

} // namespace Os
} // namespace Mixer
 
namespace Numatune
{

///////////////////////////////////////////////////////////////////////////////
// struct Memory

struct Memory: boost::static_visitor<QString>
{

	template<class T>
	QString operator()(const T&) const
	{
		return "";
	}

	QString operator()(const mpl::at_c<Libvirt::Domain::Xml::VMemory::types, 0>::type& mask_) const;
};

} // namespace Numatune

///////////////////////////////////////////////////////////////////////////////
// struct Timer

struct Timer: boost::static_visitor<void>
{
	Timer(const CVmCommonOptions& config_, QList<Libvirt::Domain::Xml::Timer>& timers_):
		m_config(config_), m_timers(timers_)
	{
	}

	template<class T>
	void operator()(const T&) const
	{
	}

	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VTimer::types, 0>::type& ) const;
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VTimer::types, 3>::type& ) const;

private:
	const CVmCommonOptions& m_config;
	QList<Libvirt::Domain::Xml::Timer>& m_timers;
};

///////////////////////////////////////////////////////////////////////////////
// struct Chipset

struct Chipset: boost::static_visitor<QString>
{
	template<class T>
	QString operator()(const T&) const
	{
		return QString();
	}
	QString operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice305::types, 0>::type& hvm_) const
	{
		return hvm_.getValue().getMachine() ? hvm_.getValue().getMachine().get() : QString();
	}
	QString operator()(const mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type& os_) const
	{
		Libvirt::Domain::Xml::Os2 a = os_.getValue();
		if (!a.getType())
			return QString();

		return boost::apply_visitor(*this, a.getType().get());
	}
};

namespace Export
{
namespace xml = Libvirt::Domain::Xml;

typedef QHash<QString, QString> map_type;

///////////////////////////////////////////////////////////////////////////////
// struct Image

struct Image: boost::static_visitor<bool>
{
	explicit Image(map_type& map_): m_map(&map_)
	{
	}

	template<class T>
	result_type operator()(const T& ) const
	{
		return false;
	}
	result_type operator()(const boost::mpl::at_c<xml::VChoice985::types, 0>::type& disk_) const;
	result_type operator()(const boost::mpl::at_c<xml::VDisk::types, 0>::type& disk_) const;

private:
	map_type* m_map;
};

///////////////////////////////////////////////////////////////////////////////
// struct Name

struct Name: boost::static_visitor<QString>
{
	explicit Name(const map_type& map_): m_map(&map_)
	{
	}

	result_type operator()(const boost::mpl::at_c<xml::VName1::types, 0>::type& hint_) const
	{
		return m_map->value(hint_.getValue(), QString());
	}
	result_type operator()(const boost::mpl::at_c<xml::VName1::types, 1>::type& path_) const
	{
		return path_.getValue();
	}

private:
	const map_type* m_map;
};

} // namespace Export
} // namespace Visitor

template<class T>
void shape(char* xml_, QScopedPointer<T>& dst_)
{
	Direct::Distiller<T> d;
	PRL_RESULT e = d(Direct::Text(xml_));
	if (PRL_SUCCEEDED(e))
		dst_.reset(new T(d.getValue()));
}

} // namespace Transponster

#endif // __DIRECT_P_H__

