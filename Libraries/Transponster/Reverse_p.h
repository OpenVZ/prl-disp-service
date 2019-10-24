///////////////////////////////////////////////////////////////////////////////
///
/// @file Reverse_p.h
///
/// Convertor from the SDK config format into the libvirt one. The private part.
///
/// @author shrike
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __REVERSE_P_H__
#define __REVERSE_P_H__

#include "domain_type.h"
#include <QDomDocument>
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>
#include <prlxmlmodel/VtInfo/VtInfo.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

class CHwUsbDevice;

namespace Transponster
{
///////////////////////////////////////////////////////////////////////////////
// struct Extract

template<class T>
struct Extract;

template<typename T>
struct Extract<Libvirt::Choice<T> >
{
	typedef typename T::type type;
};

namespace Device
{
namespace Boot
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

struct Reverse
{
	typedef CVmStartupOptions::CVmBootDevice device_type;
	typedef boost::optional<Libvirt::Domain::Xml::PPositiveInteger::value_type >
		order_type;

	explicit Reverse(const QList<device_type* >& list_);

	order_type operator()(const CVmDevice& device_) const;

private:
	typedef QPair<PRL_DEVICE_TYPE, unsigned> key_type;
	typedef QHash<key_type, order_type::value_type> map_type;

	map_type m_map;
};

} // namespace Boot
} // namespace Device

namespace Network
{
namespace Address
{
///////////////////////////////////////////////////////////////////////////////
// struct IPv4

struct IPv4
{
	static const int index = 0;
	static QHostAddress patchEnd(const QHostAddress& start_, const QHostAddress& end_);
	static int getMask(const QHostAddress& mask_);
	static const char* getFamily();
	static bool isEligible(const QHostAddress& address_);
};

///////////////////////////////////////////////////////////////////////////////
// struct IPv6

struct IPv6
{
	static const int index = 1;
	static QHostAddress patchEnd(const QHostAddress& start_, const QHostAddress& end_);
	static int getMask(const QHostAddress& mask_);
	static const char* getFamily();
	static bool isEligible(const QHostAddress& address_);
};

} // namespace Address
} // namespace Network

namespace Device
{

struct List;
typedef QList<Libvirt::Domain::Xml::VChoice985 > deviceList_type;
typedef Boot::Reverse::order_type boot_type;

///////////////////////////////////////////////////////////////////////////////
// struct Alias

struct Alias
{
	bool feature(const QString& value_) const;

	QString operator()(const CHwUsbDevice& model_) const;
	QString operator()(const CVmGenericPciDevice model_) const;

private:
	static const QString s_PREFIX;
};

namespace Clustered
{
///////////////////////////////////////////////////////////////////////////////
// struct Flavor

template<class T>
struct Flavor;

template<>
struct Flavor<CVmHardDisk>
{
	typedef PVE::HardDiskEmulatedType emulated_type;

	static const Libvirt::Domain::Xml::EDevice kind;
	static const emulated_type real = PVE::RealHardDisk;
	static const emulated_type image = PVE::HardDiskImage;
	static const bool readonly = false;
	static const boost::none_t snapshot;

	static const char* getTarget()
	{
		return "hd";
	}
	static const char* getProductTemplate()
	{
		return "Vz HARDDISK%1";
	}
	static mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 1>::type
		getDriverFormat()
	{
		mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 1>::type output;
		output.setValue(Libvirt::Domain::Xml::EStorageFormatBackingQcow2);
		return output;
	}
	static boost::optional<Libvirt::Domain::Xml::ETray> getTray(int )
	{
		return boost::optional<Libvirt::Domain::Xml::ETray>();
	}
};

template<>
struct Flavor<CVmOpticalDisk>
{
	typedef PVE::CdromEmulatedType emulated_type;

	static const Libvirt::Domain::Xml::EDevice kind;
	static const emulated_type real = PVE::RealCdRom;
	static const emulated_type image = PVE::CdRomImage;
	static const bool readonly = true;
	static const Libvirt::Domain::Xml::ESnapshot snapshot;

	static const char* getTarget()
	{
		return "sd";
	}
	static const char* getProductTemplate()
	{
		return "Vz CD-ROM%1";
	}
	static mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 0>::type
		getDriverFormat()
	{
		mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 0>::type output;
		output.setValue(Libvirt::Domain::Xml::EStorageFormatRaw);
		return output;
	}
	static boost::optional<Libvirt::Domain::Xml::ETray> getTray(emulated_type type_)
	{
		if (real == type_)
			return Libvirt::Domain::Xml::ETrayOpen;
		if (image == type_)
			return Libvirt::Domain::Xml::ETrayClosed;

		return boost::optional<Libvirt::Domain::Xml::ETray>();
	}
};

template<>
struct Flavor<CVmFloppyDisk>
{
	typedef PVE::FloppyEmulatedType emulated_type;

	static const Libvirt::Domain::Xml::EDevice kind;
	static const emulated_type real = PVE::RealFloppyDisk;
	static const emulated_type image = PVE::FloppyDiskImage;
	static const bool readonly = true;
	static const Libvirt::Domain::Xml::ESnapshot snapshot;

	static const char* getTarget()
	{
		return "fd";
	}
	static const char* getProductTemplate()
	{
		return "Vz FLOPPY%1";
	}
	static mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 0>::type
		getDriverFormat()
	{
		mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 0>::type output;
		output.setValue(Libvirt::Domain::Xml::EStorageFormatRaw);
		return output;
	}
	static boost::optional<Libvirt::Domain::Xml::ETray> getTray(emulated_type type_)
	{
		if (real == type_)
			return Libvirt::Domain::Xml::ETrayOpen;

		return boost::optional<Libvirt::Domain::Xml::ETray>();
	}
};


///////////////////////////////////////////////////////////////////////////////
// struct Model

template<class T>
struct Model
{
	explicit Model(const T& dataSource_): m_dataSource(&dataSource_)
	{
	}

	const char* getTarget() const
	{
		switch (m_dataSource->getInterfaceType())
		{
		case PMS_IDE_DEVICE:
			return "hd";
		case PMS_SCSI_DEVICE:
			return "sd";
		case PMS_SATA_DEVICE:
			return "sd";
		case PMS_VIRTIO_BLOCK_DEVICE:
			return "vd";
		default:
			return "xx";
		}
	}

	boost::optional<QString> getProductName() const
	{
		boost::optional<Libvirt::Domain::Xml::EBus> b = getBus();
		if (!(b && b == Libvirt::Domain::Xml::EBusScsi))
			return boost::none;

		return QString(Flavor<T>::getProductTemplate())
			.arg(m_dataSource->getStackIndex());
	}
	QString getTargetName() const
	{
		return getTarget()
			+ Parallels::toBase26(m_dataSource->getStackIndex());
	}
	quint32 getIndex() const
	{
		return m_dataSource->getStackIndex();
	}

	QString getImageFile() const
	{
		QString f = m_dataSource->getUserFriendlyName();
		if (!f.isEmpty())
			return f;

		return m_dataSource->getSystemName();
	}
	QString getRealDeviceName() const
	{
		QRegExp r("[(]([^)]+)[)]");
		if (-1 < r.indexIn(m_dataSource->getUserFriendlyName()))
			return r.cap(1);

		return m_dataSource->getSystemName();
	}
	typename Flavor<T>::emulated_type getEmulatedType() const
	{
		return m_dataSource->getEmulatedType();
	}
	boost::optional<Libvirt::Domain::Xml::EBus> getBus() const
	{
		switch (m_dataSource->getInterfaceType())
		{
		case PMS_IDE_DEVICE:
			return Libvirt::Domain::Xml::EBusIde;
		case PMS_SCSI_DEVICE:
			return Libvirt::Domain::Xml::EBusScsi;
		case PMS_SATA_DEVICE:
			return Libvirt::Domain::Xml::EBusSata;
		case PMS_VIRTIO_BLOCK_DEVICE:
			return Libvirt::Domain::Xml::EBusVirtio;
		default:
			return boost::optional<Libvirt::Domain::Xml::EBus>();
		}
	}
	bool isConnected() const
	{
		return PVE::DeviceConnected == m_dataSource->getConnected();
	}

	boost::optional<Libvirt::Domain::Xml::EModel> getScsiModel() const
	{
		switch (m_dataSource->getSubType())
		{
		case PCD_BUSLOGIC:
			return Libvirt::Domain::Xml::EModelBuslogic;
		case PCD_LSI_SAS:
			return Libvirt::Domain::Xml::EModelLsisas1078;
		case PCD_LSI_SPI:
			return Libvirt::Domain::Xml::EModelLsilogic;
		case PCD_VIRTIO_SCSI:
			return Libvirt::Domain::Xml::EModelVirtioScsi;
		case PCD_HYPER_V_SCSI:
			return Libvirt::Domain::Xml::EModelHvScsi;
		default:
			return boost::optional<Libvirt::Domain::Xml::EModel>();
		}
	}

protected:
	const T* getDataSource() const
	{
		return m_dataSource;
	}
private:
	const T* m_dataSource;
};

namespace Builder
{
///////////////////////////////////////////////////////////////////////////////
// struct Ordinary

template<class T>
struct Ordinary
{
	typedef Libvirt::Domain::Xml::Disk result_type;

	Ordinary(const T& dataSource_, const boot_type& boot_ = boot_type()):
		m_model(dataSource_), m_boot(boot_)
	{
	}

	void setDisk();
	void setFlags();
	void setDriver();
	void setSource()
	{
		m_result.setDiskSource(getSource());
	}
	void setTarget();
	void setBackingChain();

	const result_type& getResult() const
	{
		return m_result;
	}

	const Model<T>& getModel() const
	{
		return m_model;
	}

protected:
	result_type& getResult()
	{
		return m_result;
	}

	Libvirt::Domain::Xml::VDiskSource getSource() const;

private:
	Model<T> m_model;
	boot_type m_boot;
	result_type m_result;
};

template<class T>
void Ordinary<T>::setDisk()
{
	mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 0>::type x;
	x.setValue(Flavor<T>::kind);
	m_result.setDisk(Libvirt::Domain::Xml::VDisk(x));
	if (Flavor<T>::image == getModel().getEmulatedType())
	{
		boost::optional<QString> p = getModel().getProductName();
		if (p)
			m_result.setProduct(p.get());
	}

}

template<class T>
void Ordinary<T>::setFlags()
{
	// boot
	m_result.setBoot(m_boot);
	// readonly
	m_result.setReadonly(Flavor<T>::readonly);
	// snapshot
	if (Flavor<T>::image == getModel().getEmulatedType())
		m_result.setSnapshot(Flavor<T>::snapshot);
	else
		m_result.setSnapshot(Libvirt::Domain::Xml::ESnapshotNo);
}

template<class T>
void Ordinary<T>::setDriver()
{
	Libvirt::Domain::Xml::Driver d;
	d.setCache(Libvirt::Domain::Xml::ECacheNone);
	d.setIo(Libvirt::Domain::Xml::EIoNative);

	if (Flavor<T>::image == getModel().getEmulatedType())
	{
		mpl::at_c<Libvirt::Domain::Xml::VType::types, 1>::type a;
		a.setValue(Libvirt::Domain::Xml::VStorageFormat(Flavor<T>::getDriverFormat()));
		Libvirt::Domain::Xml::DriverFormat b;
		b.setName("qemu");
		b.setType(Libvirt::Domain::Xml::VType(a));
		d.setDiscard(Libvirt::Domain::Xml::EDiscardUnmap);
		d.setDriverFormat(b);
	}
//	boost::optional<Libvirt::Domain::Xml::EBus> u = getModel().getBus();
//	if (u && u.get() == Libvirt::Domain::Xml::EBusVirtio)
//		d.setIothread(1);

	m_result.setDriver(d);
}

template<class T>
Libvirt::Domain::Xml::VDiskSource Ordinary<T>::getSource() const
{
	switch (getModel().getEmulatedType())
	{
	case Flavor<T>::image:
	{
		Libvirt::Domain::Xml::Source4 s;
		QString f = getModel().getImageFile();
		if (!f.isEmpty() && getModel().isConnected())
		{
			s.setFile(f);
			s.setStartupPolicy(Libvirt::Domain::Xml::EStartupPolicyOptional);
		}
		mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 4>::type x;
		x.setValue(s);
		return x;
	}
	case Flavor<T>::real:
	{
		Libvirt::Domain::Xml::Source s;
		s.setDev(getModel().getRealDeviceName());
		mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 0>::type x;
		x.setValue(s);
		return x;
	}
	default:
		return Libvirt::Domain::Xml::VDiskSource();
	}
}

template<class T>
void Ordinary<T>::setTarget()
{
	Libvirt::Domain::Xml::Target t;
	t.setBus(getModel().getBus());
	t.setDev(getModel().getTargetName());
	t.setTray(Flavor<T>::getTray(getModel().getEmulatedType()));
	m_result.setTarget(t);
}

template<class T>
void Ordinary<T>::setBackingChain()
{
	mpl::at_c<Libvirt::Domain::Xml::VDiskBackingChain::types, 1>::type x;
	x.setValue(false);
	m_result.setDiskBackingChain(Libvirt::Domain::Xml::VDiskBackingChain(x));
}

///////////////////////////////////////////////////////////////////////////////
// struct Hdd

struct Hdd: Ordinary<CVmHardDisk>
{
	Hdd(const CVmHardDisk& dataSource_, const boot_type& boot_ = boot_type()):
		Ordinary<CVmHardDisk>(dataSource_, boot_), m_hdd(dataSource_)
	{
	}

	void setIoLimit(const CVmIoLimit* global_);
	void setIopsLimit(const CVmRunTimeOptions& runtime_);
	void setSerial(const QString& serial_);
	void setDisk();
	void setBackingChain();
	void setDriver();

private:
	const CVmHardDisk& m_hdd;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

template<class T>
struct ChangeableMedia
{
	static QString getUpdateXml(const T& model_)
	{
		typedef Ordinary<T> builder_type;
		builder_type b(model_);
		b.setDisk();
		b.setFlags();
		b.setSource();
		b.setTarget();
		b.setBackingChain();
		QDomDocument x;
		static_cast<const builder_type&>(b).getResult().save(x);

		return x.toString();
	}
};

} // namespace Builder
} // namespace Clustered

///////////////////////////////////////////////////////////////////////////////
// struct Ips

struct Ips
{
	QList<Libvirt::Domain::Xml::Ip> operator()(const QList<QString>& ips_);

	template<class T>
	Libvirt::Domain::Xml::Ip craft(const QString& address_, int prefix_)
	{
		typename mpl::at_c<Libvirt::Domain::Xml::VIpAddr::types, T::index>::type a;
		a.setValue(address_);
		typename mpl::at_c<Libvirt::Domain::Xml::VIpPrefix::types, T::index>::type p;
		p.setValue(prefix_);
		Libvirt::Domain::Xml::Ip ip;
		ip.setAddress(Libvirt::Domain::Xml::VIpAddr(a));
		ip.setPrefix(Libvirt::Domain::Xml::VIpPrefix(p));
		return ip;
	}
};

namespace Network
{

///////////////////////////////////////////////////////////////////////////////
// struct View

struct View
{
	explicit View(const CVmGenericNetworkAdapter &network_):
		m_network(network_)
	{
	}

	QString getAdapterType() const;
	QString getMac() const;
	boost::optional<Libvirt::Domain::Xml::FilterrefNodeAttributes> getFilterref() const;

private:
	QString getFilterName() const;
	static QString normalizeMac(const QString &mac_);
	QStringList getIpv4() const;

	CVmGenericNetworkAdapter m_network;
};

///////////////////////////////////////////////////////////////////////////////
// struct Adapter

template<int N>
struct Adapter
{
	Libvirt::Domain::Xml::VInterface operator()(
		const CVmGenericNetworkAdapter& network_, const boot_type& boot_);

private:
	typedef typename mpl::at_c<Libvirt::Domain::Xml::VInterface::types, N>::type
		access_type;

	static typename Libvirt::Details::Value::Grab<access_type>::type
		prepare(const CVmGenericNetworkAdapter& network_);
};

Prl::Expected<Libvirt::Domain::Xml::VInterface, ::Error::Simple>
	build(const CVmGenericNetworkAdapter& network_,
	const boot_type& boot_ = boot_type());

} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Address

struct Address
{
	Address& setUnit(quint16 unit_)
	{
		m_address.setUnit(QString::number(unit_));
		return *this;
	}
	Address& setTarget(quint16 target_)
	{
		m_address.setTarget(QString::number(target_));
		return *this;
	}
	Address& setBus(quint16 bus_)
	{
		m_address.setBus(QString::number(bus_));
		return *this;
	}	
	Libvirt::Domain::Xml::VAddress operator()(quint16 controller_)
	{
		address_type a;
		std::swap(a, m_address);
		a.setController(QString::number(controller_));
		mpl::at_c<Libvirt::Domain::Xml::VAddress::types, 1>::type v;
		v.setValue(a);
		return Libvirt::Domain::Xml::VAddress(v);
	}

private:
	typedef Libvirt::Domain::Xml::Driveaddress address_type;

	address_type m_address;
};

namespace Controller
{
///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	typedef Libvirt::Domain::Xml::VChoice985 result_type;
	Libvirt::Domain::Xml::Controller craft
		(const Libvirt::Domain::Xml::VChoice625& bus_, quint16 index_);
	result_type wrap(const Libvirt::Domain::Xml::Controller& object_);
};

///////////////////////////////////////////////////////////////////////////////
// class Moldy

class Moldy: Factory
{
public: 
	result_type operator()(Libvirt::Domain::Xml::EType6 bus_, quint16 index_);
};

///////////////////////////////////////////////////////////////////////////////
// class Virtio

class Virtio: Factory
{
public: 
	result_type operator()(quint16 index_);
};

///////////////////////////////////////////////////////////////////////////////
// class Hyperv

class Hyperv: Factory
{
public: 
	result_type operator()(quint16 index_);
};

namespace Scsi
{
///////////////////////////////////////////////////////////////////////////////
// struct Arrangement

struct Arrangement
{
	typedef boost::function<Libvirt::Domain::Xml::VChoice985 (quint16)>
		factory_type;

	explicit Arrangement(const factory_type& factory_):
		m_device(), m_controller(), m_factory(factory_)
	{
	}

	Libvirt::Domain::Xml::VAddress operator()(deviceList_type& controllers_);

private:
	enum
	{
		MAX_TARGETS = 256
	};

	quint16 m_device;
	quint16 m_controller;
	factory_type m_factory;
};

///////////////////////////////////////////////////////////////////////////////
// struct Bus

struct Bus
{
	Bus();

	const deviceList_type& getControllers() const
	{
		return m_controllers;
	}
	Libvirt::Domain::Xml::VAddress operator()(Libvirt::Domain::Xml::EModel model_);

private:
	Arrangement m_hyperv, m_virtio;
	deviceList_type m_controllers;
};

} // namespace Scsi
} // namespace Controller

///////////////////////////////////////////////////////////////////////////////
// struct Attachment

struct Attachment
{
	Libvirt::Domain::Xml::VAddress craftIde(quint32 index_);
	Libvirt::Domain::Xml::VAddress craftSata(quint32 index_);
	Libvirt::Domain::Xml::VAddress craftScsi
		(const boost::optional<Libvirt::Domain::Xml::EModel>& model_);
	deviceList_type getControllers() const;

private:
	enum
	{
		IDE_UNITS = 2,
		IDE_BUSES = 2,
		SATA_UNITS = 6
	};

	Controller::Moldy m_moldy;
	Controller::Scsi::Bus m_scsi;
	deviceList_type m_controllers;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	const deviceList_type& getDeviceList() const
	{
		return m_deviceList;
	}
	QString getEmulator() const;
	void addGuestChannel(const QString & path_);
	void add(const CVmRemoteDisplay* vnc_);
	void add(const Libvirt::Domain::Xml::Disk& disk_);
	void add(const Libvirt::Domain::Xml::VInterface& adapter_);
	void add(const CVmParallelPort* port_);
	void add(const CVmSerialPort* port_);
	void add(const CVmSoundDevice* sound_);
	void add(const CVmVideo* video_);
	void add(const CVmGenericPciDevice* pci_);

private:
	template<int N, class T>
	void add(const T& value_)
	{
		typename mpl::at_c<deviceList_type::value_type::types, N>::type x;
		x.setValue(value_);
		m_deviceList << x;
	}

	deviceList_type m_deviceList;
};

namespace Boot
{

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	List(const CVmSettings& settings_, Device::List& list_);

	const Attachment& getAttachment() const;

	void add(const CVmHardDisk* hdd_, const CVmRunTimeOptions* runtime_);
	void add(const CVmOpticalDisk* cdrom_);
	void add(const CVmFloppyDisk* floppy_);
	void add(const CVmGenericNetworkAdapter* network_);

private:
	template<class T>
	void build(T builder_);

private:
	Reverse m_boot;
	Device::List& m_deviceList;
	Attachment m_attachment;
};

template<class T>
void List::build(T builder_)
{
	builder_.setDisk();
	builder_.setFlags();
	builder_.setDriver();
	builder_.setSource();
	builder_.setTarget();
	builder_.setBackingChain();

	Libvirt::Domain::Xml::Disk d = static_cast<const T&>(builder_).getResult();
	switch (d.getTarget().getBus().get())
	{
	case Libvirt::Domain::Xml::EBusIde:
		d.setAddress(m_attachment.craftIde(builder_.getModel().getIndex()));
		break;
	case Libvirt::Domain::Xml::EBusSata:
		d.setAddress(m_attachment.craftSata(builder_.getModel().getIndex()));
		break;
	case Libvirt::Domain::Xml::EBusScsi:
		d.setAddress(m_attachment.craftScsi(builder_.getModel().getScsiModel()));
		break;
	default:
		break;
	}
	m_deviceList.add(d);
}

} // namespace Boot

namespace Usb
{
///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(const CVmUsbController* settings_):
		m_settings(settings_), m_controller(0)
	{
	}

	deviceList_type getDevices() const
	{
		return m_deviceList;
	}

	void add(const CVmUsbDevice* usb_);
	void addKeyboard();
	void addMouse();

private:
	template<int N, class T>
	void add(const T& value_)
	{
		typename mpl::at_c<deviceList_type::value_type::types, N>::type x;
		x.setValue(value_);
		m_deviceList << x;
	}

	void craftController(Libvirt::Domain::Xml::EModel1 model_);
	void add(Libvirt::Domain::Xml::EType10 type_);

	const CVmUsbController* m_settings;
	deviceList_type m_deviceList;
	quint16 m_controller;
};

///////////////////////////////////////////////////////////////////////////////
// struct Indicator

struct Indicator: boost::static_visitor<bool>
{
	explicit Indicator(const QString& name_);

	template<class T>
	bool operator()(const T& ) const
	{
		return false;
	}

	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 7>::type& variant_) const
	{
		return boost::apply_visitor(*this, variant_.getValue().getChoice917());
	}
	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice917::types, 0>::type& variant_) const
	{
		return boost::apply_visitor(*this, variant_.getValue().getHostdevsubsys());
	}
	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VHostdevsubsys::types, 1>::type& variant_) const
	{
		return boost::apply_visitor(*this, variant_.getValue().getSource().getSource());
	}
	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VSource1::types, 0>::type& variant_) const;
	bool operator()(Libvirt::Domain::Xml::VChoice985& device_) const
	{
		return boost::apply_visitor(*this, device_);
	}

private:
        QString m_pid, m_vid;
};

} // namespace Usb

namespace Panic
{
///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	const QList<Libvirt::Domain::Xml::Panic>& getResult() const
	{
		return m_list;
	}
	void add(quint32 os_);

private:
	QList<Libvirt::Domain::Xml::Panic> m_list;
};

} // namespace Panic

} // namespace Device

///////////////////////////////////////////////////////////////////////////////
// struct Resources

struct Resources
{
	explicit Resources(CVmConfiguration& config_): m_config(&config_)
	{
	}

	void setCpu(const Libvirt::Domain::Xml::Cpu& src_);
	bool getCpu(const VtInfo& vt_, Libvirt::Domain::Xml::Cpu& dst_);
	void setCpu(const Libvirt::Domain::Xml::Domain& vm_, const VtInfo& vt_);
	bool getCpu(const VtInfo& vt_, Libvirt::Domain::Xml::Domain& dst_);
	void setClock(const Libvirt::Domain::Xml::Clock& src_);
	bool getClock(Libvirt::Domain::Xml::Clock& dst_);
	void setMemory(const Libvirt::Domain::Xml::Memory& src_);
	bool getMemory(Libvirt::Domain::Xml::Memory& dst_);
	void setMaxMemory(const Libvirt::Domain::Xml::MaxMemory& src_);
	bool getMaxMemory(Libvirt::Domain::Xml::MaxMemory& dst_);
	void setCurrentMemory(const Libvirt::Domain::Xml::ScaledInteger& src_);
	bool getCurrentMemory(Libvirt::Domain::Xml::ScaledInteger& dst_);
	void setChipset(const Libvirt::Domain::Xml::VOs& src_);
	bool getChipset(Libvirt::Domain::Xml::Os2& dst_);

private:
	CVmHardware* getHardware()
	{
		return m_config->getVmHardwareList();
	}

	CVmConfiguration* m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct CommandLine

struct CommandLine
{
	explicit CommandLine(const CVmConfiguration& source_): m_source(&source_)
	{
	}

	CommandLine& seed(const boost::optional<Libvirt::Domain::Xml::Commandline>& original_);
	CommandLine& addDebug();
	CommandLine& addLogging();
	CommandLine& stripDebugcon();
	CommandLine& workaroundEfi2008R2();
	Libvirt::Domain::Xml::Commandline takeResult();

private:
	void stripParameter(int at_);

	QList<QString> m_result;
	const CVmConfiguration* m_source;
};

} // namespace Transponster

#endif // __REVERSE_P_H__

