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

#include "Direct.h"
#include "Reverse.h"
#include "Reverse_p.h"
#include "Direct_p.h"
#include <prlsdk/PrlOses.h>
#include <prlcommon/HostUtils/HostUtils.h>

namespace Transponster
{
///////////////////////////////////////////////////////////////////////////////
// struct Resources

void Resources::setVCpu(const Libvirt::Domain::Xml::VCpu& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	boost::apply_visitor(Visitor::Cpu(*h), src_);
}

namespace
{

void disableFeature(QList<Libvirt::Domain::Xml::Feature>& features_, const QString& name_)
{
	Libvirt::Domain::Xml::Feature f;
	f.setName(name_);
	f.setPolicy(Libvirt::Domain::Xml::EPolicyDisable);
	features_.append(f);
}

} // namespace

bool Resources::getVCpu(Libvirt::Domain::Xml::VCpu& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	CVmCpu* u = h->getCpu();
	if (NULL == u)
		return false;
	
	if(0 == u->getNumber())
		return false;

	Libvirt::Domain::Xml::Cpu955 c;
	
	c.setMode(Libvirt::Domain::Xml::EModeHostPassthrough);

#if (LIBVIR_VERSION_NUMBER >= 1002013)
	QList<Libvirt::Domain::Xml::Feature> l;
	if (!u->isVirtualizedHV())
		disableFeature(l, QString("vmx"));
	c.setFeatureList(l);
#endif

	CVmMemory *m = h->getMemory();
	if (m->isEnableHotplug()) {

		QList<Libvirt::Domain::Xml::Cell > cells;
		Libvirt::Domain::Xml::Cell cell;
		boost::optional<unsigned int> id(0); 
		cell.setId(id);

		QString mask = "0";
		for (unsigned int i=1; i<u->getNumber(); i++)
			mask += "," + QString::number(i);

		cell.setCpus(mask);
		cell.setMemory(m->getMaxNumaRamSize()<<10);
		cells.append(cell);
		c.setNuma(cells);
	}

	mpl::at_c<Libvirt::Domain::Xml::VCpu::types, 2>::type v;
	v.setValue(c);
	dst_ = Libvirt::Domain::Xml::VCpu(v);

	return true;
}

void Resources::setCpu(const Libvirt::Domain::Xml::Domain& vm_, const VtInfo& vt_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	Vm::Direct::Cpu b(vm_, h->getCpu(), vt_);
	if (PRL_SUCCEEDED(Director::cpu(b)))
		h->setCpu(b.getResult());
}

bool Resources::getCpu(const VtInfo& vt_, Libvirt::Domain::Xml::Domain& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	CVmCpu* u = h->getCpu();
	if (NULL == u)
		return false;

	Vm::Reverse::Cpu b(*u, vt_);
	if (PRL_FAILED(Director::cpu(b)))
		return false;

	dst_.setVcpu(b.getVcpu());
	dst_.setCputune(b.getTune());

	return true;
}

void Resources::setClock(const Libvirt::Domain::Xml::Clock& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	::Clock* c = new ::Clock();
	boost::apply_visitor(Visitor::Clock(*c), src_.getClock());
	h->setClock(c);
}

bool Resources::getClock(Libvirt::Domain::Xml::Clock& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	::Clock* c = h->getClock();
	if (NULL == c)
		return false;

	CVmSettings* s = m_config->getVmSettings();
	if (NULL == s)
		return false;

	CVmCommonOptions* o = s->getVmCommonOptions();
	if (NULL == o)
		return false;

	QList<Libvirt::Domain::Xml::Timer> timers;
	quint32 os = m_config->getVmSettings()->getVmCommonOptions()->getOsVersion();
	if (IS_WINDOWS(os) && os >= PVS_GUEST_VER_WIN_2008)
	{
		Libvirt::Domain::Xml::Timer t;
		mpl::at_c<Libvirt::Domain::Xml::VTimer::types, 3>::type v;
		v.setValue(Libvirt::Domain::Xml::EName2Hypervclock);
		t.setTimer(Libvirt::Domain::Xml::VTimer(v));
		t.setPresent(Libvirt::Domain::Xml::EVirYesNoYes);
		timers.append(t);
	}
	dst_.setTimerList(timers);

	Libvirt::Domain::Xml::Clock374 k;
	k.setOffset(Libvirt::Domain::Xml::EOffsetUtc);
	if (0 != c->getTimeShift())
	{
		mpl::at_c<Libvirt::Domain::Xml::VAdjustment::types, 1>::type va;
		va.setValue(QString::number(c->getTimeShift()));
		k.setAdjustment(Libvirt::Domain::Xml::VAdjustment(va));
	}
	mpl::at_c<Libvirt::Domain::Xml::VClock::types, 0>::type a;
	a.setValue(k);
	dst_.setClock(a);
	return true;
}

void Resources::setMemory(const Libvirt::Domain::Xml::Memory& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	CVmMemory* m = new CVmMemory(h->getMemory());
	m->setRamSize(src_.getScaledInteger().getOwnValue() >> 10);
	h->setMemory(m);
}

bool Resources::getMemory(Libvirt::Domain::Xml::Memory& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	CVmMemory* m = h->getMemory();
	if (NULL == m)
		return false;

	Libvirt::Domain::Xml::ScaledInteger v;
	v.setOwnValue(m->getRamSize() << 10);
	dst_.setScaledInteger(v);
	return true;
}

void Resources::setMaxMemory(const Libvirt::Domain::Xml::MaxMemory& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	CVmMemory* m = new CVmMemory(h->getMemory());
	m->setMaxRamSize(src_.getScaledInteger().getOwnValue() >> 10);
	m->setMaxSlots(src_.getSlots());
	h->setMemory(m);
}

bool Resources::getMaxMemory(Libvirt::Domain::Xml::MaxMemory& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	CVmMemory* m = h->getMemory();
	if (NULL == m)
		return false;

	if (!m->isEnableHotplug())
		return false;

	Libvirt::Domain::Xml::ScaledInteger v;
	v.setOwnValue(m->getMaxRamSize() << 10);
	dst_.setScaledInteger(v);
	if (m->getMaxSlots())
		dst_.setSlots(m->getMaxSlots());
	return true;
}

void Resources::setCurrentMemory(const Libvirt::Domain::Xml::ScaledInteger& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	CVmMemory* m = new CVmMemory(h->getMemory());
	m->setRamSize(src_.getOwnValue() >> 10);
	h->setMemory(m);
}

bool Resources::getCurrentMemory(Libvirt::Domain::Xml::ScaledInteger& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	CVmMemory* m = h->getMemory();
	if (NULL == m)
		return false;

	dst_.setOwnValue(m->getRamSize() << 10);
	return true;
}

void Resources::setChipset(const Libvirt::Domain::Xml::Sysinfo& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	(void)src_;
	Chipset* c = new Chipset();
	h->setChipset(c);
}

namespace Device
{

namespace Clustered
{
///////////////////////////////////////////////////////////////////////////////
// struct Flavor

const Flavor<CVmHardDisk>::emulated_type Flavor<CVmHardDisk>::real = PVE::RealHardDisk;
const Flavor<CVmHardDisk>::emulated_type Flavor<CVmHardDisk>::image = PVE::HardDiskImage;
const Libvirt::Domain::Xml::EDevice Flavor<CVmHardDisk>::kind = Libvirt::Domain::Xml::EDeviceDisk;
const boost::none_t Flavor<CVmHardDisk>::snapshot = boost::none;

template<>
struct Flavor<CVmOpticalDisk>
{
	typedef PVE::CdromEmulatedType emulated_type;

	static const Libvirt::Domain::Xml::EDevice kind;
	static const emulated_type real;
	static const emulated_type image;
	static const bool readonly = true;
	static const Libvirt::Domain::Xml::ESnapshot snapshot;

	static const char* getTarget()
	{
		return "sd";
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
const Flavor<CVmOpticalDisk>::emulated_type Flavor<CVmOpticalDisk>::real = PVE::RealCdRom;
const Flavor<CVmOpticalDisk>::emulated_type Flavor<CVmOpticalDisk>::image = PVE::CdRomImage;
const Libvirt::Domain::Xml::EDevice Flavor<CVmOpticalDisk>::kind = Libvirt::Domain::Xml::EDeviceCdrom;
const Libvirt::Domain::Xml::ESnapshot Flavor<CVmOpticalDisk>::snapshot = Libvirt::Domain::Xml::ESnapshotNo;

template<>
struct Flavor<CVmFloppyDisk>
{
	typedef PVE::FloppyEmulatedType emulated_type;

	static const Libvirt::Domain::Xml::EDevice kind;
	static const emulated_type real;
	static const emulated_type image;
	static const bool readonly = true;
	static const Libvirt::Domain::Xml::ESnapshot snapshot;

	static const char* getTarget()
	{
		return "fd";
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
const Flavor<CVmFloppyDisk>::emulated_type Flavor<CVmFloppyDisk>::real = PVE::RealFloppyDisk;
const Flavor<CVmFloppyDisk>::emulated_type Flavor<CVmFloppyDisk>::image = PVE::FloppyDiskImage;
const Libvirt::Domain::Xml::EDevice Flavor<CVmFloppyDisk>::kind = Libvirt::Domain::Xml::EDeviceFloppy;
const Libvirt::Domain::Xml::ESnapshot Flavor<CVmFloppyDisk>::snapshot = Libvirt::Domain::Xml::ESnapshotNo;

///////////////////////////////////////////////////////////////////////////////
// struct Model

template<>
boost::optional<Libvirt::Domain::Xml::EBus> Model<CVmFloppyDisk>::getBus() const
{
	return Libvirt::Domain::Xml::EBusFdc;
}

namespace Builder
{
///////////////////////////////////////////////////////////////////////////////
// struct Ordinary

template<>
void Ordinary<CVmHardDisk>::setSource()
{
	Libvirt::Domain::Xml::VDiskSource x = getSource();
	if (!x.empty())
		return m_result.setDiskSource(x);

	if (PVE::BootCampHardDisk == getModel().getEmulatedType())
	{
		Libvirt::Domain::Xml::Source4 s;
		s.setVolume(getModel().getRealDeviceName());
		mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 4>::type x;
		x.setValue(s);
		return m_result.setDiskSource(x);

	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Hdd

void Hdd::setIoLimit(const CVmIoLimit* global_)
{
	Libvirt::Domain::Xml::Iotune t(getResult().getIotune() ?
						*getResult().getIotune() :
						Libvirt::Domain::Xml::Iotune());
	quint32 p = 0;
	if (NULL != global_)
		p = global_->getIoLimitValue()? : p;

	if (m_hdd.getIoLimit() != NULL)
		p = m_hdd.getIoLimit()->getIoLimitValue()? : p;

	if (p != 0)
	{
		mpl::at_c<Libvirt::Domain::Xml::VChoice1050::types, 0>::type y;
		y.setValue(p);
		t.setChoice1050(Libvirt::Domain::Xml::VChoice1050(y));
	}

	getResult().setIotune(t);
}

void Hdd::setIopsLimit(const CVmRunTimeOptions& runtime_)
{
	Libvirt::Domain::Xml::Iotune t(getResult().getIotune() ?
						*getResult().getIotune() :
						Libvirt::Domain::Xml::Iotune());
	quint32 p = 0;
	p = runtime_.getIopsLimit()? : p;
	p = m_hdd.getIopsLimit()? : p;

	if (p != 0)
	{
		mpl::at_c<Libvirt::Domain::Xml::VChoice1054::types, 0>::type y;
		y.setValue(p);
		t.setChoice1054(Libvirt::Domain::Xml::VChoice1054(y));
	}

	getResult().setIotune(t);
}

void Hdd::setSerial(const QString& serial_)
{
	getResult().setSerial(serial_);
}

} // namespace Builder

///////////////////////////////////////////////////////////////////////////////
// struct List

List::List(const Boot::Reverse& boot_, Device::List& list_)
	: m_boot(boot_), m_deviceList(list_)
{
}

void List::add(const CVmHardDisk* hdd_, const CVmRunTimeOptions* runtime_)
{
	if (hdd_ == NULL)
		return;
	if (hdd_->getEnabled() != PVE::DeviceEnabled)
		return;
	Builder::Hdd b(*hdd_, m_boot(*hdd_));
	if (NULL != runtime_)
	{
		b.setIoLimit(runtime_->getIoLimit());
		b.setIopsLimit(*runtime_);
	}
	if (!hdd_->getSerialNumber().isEmpty())
		b.setSerial(hdd_->getSerialNumber());
	build(b);
}

void List::add(const CVmOpticalDisk* cdrom_)
{
	if (cdrom_ == NULL)
		return;
	if (cdrom_->getEmulatedType() == Flavor<CVmOpticalDisk>::real)
		return;
	if (cdrom_->getEnabled() != PVE::DeviceEnabled)
		return;
	build(Builder::Ordinary<CVmOpticalDisk>(*cdrom_, m_boot(*cdrom_)));
}

void List::add(const CVmFloppyDisk* floppy_)
{
	if (floppy_ == NULL)
		return;
	if (floppy_->getEnabled() != PVE::DeviceEnabled)
		return;
	build(Builder::Ordinary<CVmFloppyDisk>(*floppy_, m_boot(*floppy_)));
}

const Attachment& List::getAttachment() const
{
	return m_attachment;
}

} // namespace Clustered

///////////////////////////////////////////////////////////////////////////////
// struct Ips

QList<Libvirt::Domain::Xml::Ip> Ips::operator()(const QList<QString>& ips_)
{
	QList<Libvirt::Domain::Xml::Ip> ips;
	QStringList ipv4, ipv6;
	boost::tie(ipv4, ipv6) = NetworkUtils::ParseIps(ips_);
	foreach(const QString& e, ipv4)
	{
		QPair<QHostAddress, int> am = QHostAddress::parseSubnet(e);
		ips.append(craft<Transponster::Network::Address::IPv4>(e.split('/').first(), am.second));
	}
	foreach(const QString e, ipv6)
	{
		QPair<QHostAddress, int> am = QHostAddress::parseSubnet(e);
		ips.append(craft<Transponster::Network::Address::IPv6>(e.split('/').first(), am.second));
	}
	return ips;
}

namespace
{
QString generateAdapterType(PRL_VM_NET_ADAPTER_TYPE type_)
{
	switch (type_) {
	case PNT_RTL:
		return QString("ne2k_pci");
	case PNT_E1000:
		return QString("e1000");
	default:
		return QString("virtio");
	}
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// struct Network

template<int N>
Libvirt::Domain::Xml::VInterface Network<N>::operator()
	(const CVmGenericNetworkAdapter& network_)
{
	typename Libvirt::Details::Value::Grab<access_type>::type i = prepare(network_);

	i.setAlias(network_.getSystemName());
	QString m = network_.getMacAddress().replace(QRegExp("([^:]{2})(?!:|$)"), "\\1:");
	if (!m.isEmpty())
		i.setMac(m);

	access_type output;
	output.setValue(i);
	return output;
}

template<>
Libvirt::Domain::Xml::Interface617 Network<0>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface617 output;
	output.setIpList(Ips()(network_.getNetAddresses()));
	output.setModel(generateAdapterType(network_.getAdapterType()));
	Libvirt::Domain::Xml::Source6 s;
	s.setBridge(network_.getSystemName());
	output.setSource(s);
	output.setTarget(network_.getHostInterfaceName());
	return output;
}

template<>
Libvirt::Domain::Xml::Interface625 Network<3>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface625 output;
	Libvirt::Domain::Xml::Source8 s;
	s.setNetwork(network_.getVirtualNetworkID());
	output.setIpList(Ips()(network_.getNetAddresses()));
	output.setTarget(network_.getHostInterfaceName());
	output.setModel(generateAdapterType(network_.getAdapterType()));
	output.setSource(s);
	return output;
}

template<>
Libvirt::Domain::Xml::Interface627 Network<4>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface627 output;
	Libvirt::Domain::Xml::Source9 s;
	s.setDev(network_.getSystemName());
	output.setIpList(Ips()(network_.getNetAddresses()));
	output.setModel(generateAdapterType(network_.getAdapterType()));
	output.setTarget(network_.getHostInterfaceName());
	output.setSource(s);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Attachment

void Attachment::craftController(const Libvirt::Domain::Xml::VChoice587& bus_, quint16 index_)
{
	Libvirt::Domain::Xml::Controller x;
	x.setIndex(index_);
	x.setChoice587(bus_);
	mpl::at_c<Libvirt::Domain::Xml::VChoice935::types, 1>::type y;
	y.setValue(x);
	m_controllerList << Libvirt::Domain::Xml::VChoice935(y);
}

Libvirt::Domain::Xml::VAddress Attachment::craftIde()
{
	quint16 c = m_ide / IDE_UNITS / IDE_BUSES;
	quint16 b = m_ide / IDE_UNITS % IDE_BUSES;
	quint16 u = m_ide++ % IDE_UNITS;

	if (c > 0 && u == 0 && b == 0)
	{
		mpl::at_c<Libvirt::Domain::Xml::VChoice587::types, 0>::type v;
		v.setValue(Libvirt::Domain::Xml::EType6Ide);
		craftController(v, c);
	}

	return Address().setUnit(u).setBus(b)(c);
}

Libvirt::Domain::Xml::VAddress Attachment::craftSata()
{
	quint16 c = m_sata / SATA_UNITS;
	quint16 u = m_sata++ % SATA_UNITS;

	if (c > 0 && u == 0)
	{
		mpl::at_c<Libvirt::Domain::Xml::VChoice587::types, 0>::type v;
		v.setValue(Libvirt::Domain::Xml::EType6Sata);
		craftController(v, c);
	}

	return Address().setUnit(u)(c);
}

Libvirt::Domain::Xml::VAddress Attachment::craftScsi(const boost::optional<Libvirt::Domain::Xml::EModel>& model_)
{
	Libvirt::Domain::Xml::EModel m = Libvirt::Domain::Xml::EModelAuto;
	if (model_)
		m = model_.get();
	quint16 c = m_scsi[m] / SCSI_TARGETS;
	quint16 t = m_scsi[m]++ % SCSI_TARGETS;

	if (t == 0)
	{
		mpl::at_c<Libvirt::Domain::Xml::VChoice587::types, 1>::type v;
		v.setValue(m);
		craftController(v, c);
	}

	return Address().setTarget(t)(c);
}

///////////////////////////////////////////////////////////////////////////////
// struct List

List::List()
{
	Libvirt::Domain::Xml::Channel1 c;
	c.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceUnix);
	mpl::at_c<Libvirt::Domain::Xml::VChoice847::types, 1>::type x;
	x.setValue(QString("org.qemu.guest_agent.0"));
	c.setChoice847(x);
	add<13>(c);
}

QString List::getEmulator() const
{
	if (QFile::exists("/usr/bin/qemu-kvm"))
		return "/usr/bin/qemu-kvm";
	else if (QFile::exists("/usr/libexec/qemu-kvm"))
		return "/usr/libexec/qemu-kvm";
	return "";
}


void List::add(const CVmParallelPort* port_)
{
	if (NULL == port_)
		return;

	Libvirt::Domain::Xml::Source15 a;
	a.setPath(port_->getUserFriendlyName());
	Libvirt::Domain::Xml::QemucdevSrcDef b;
	b.setSourceList(QList<Libvirt::Domain::Xml::Source15 >() << a);
	Libvirt::Domain::Xml::Qemucdev p;
	p.setQemucdevSrcDef(b);
	add<11>(p);
}

void List::add(const CVmSerialPort* port_)
{
	if (NULL == port_)
		return;

	Prl::Expected<Libvirt::Domain::Xml::Qemucdev, ::Error::Simple> p =
		Vm::Reverse::Device<CVmSerialPort>::getLibvirtXml(*port_);
	if (p.isSucceed())
		add<12>(p.value());
}

void List::add(const CVmSoundDevice* sound_)
{
	if (NULL != sound_)
	{
		Libvirt::Domain::Xml::Sound s;
		s.setAlias(sound_->getUserFriendlyName());
		add<6>(s);
	}
}

void List::add(const CVmRemoteDisplay* vnc_)
{
	if (NULL == vnc_ || vnc_->getMode() == PRD_DISABLED)
		return;

	Libvirt::Domain::Xml::Variant685 v;
	v.setPort(vnc_->getPortNumber());
	v.setListen(vnc_->getHostName());
	if (PRD_AUTO == vnc_->getMode())
		v.setAutoport(Libvirt::Domain::Xml::EVirYesNoYes);

	mpl::at_c<Libvirt::Domain::Xml::VChoice687::types, 0>::type y;
	y.setValue(v);
	Libvirt::Domain::Xml::Graphics694 g;
	g.setChoice687(Libvirt::Domain::Xml::VChoice687(y));
	QString p = vnc_->getPassword();
	if (!p.isEmpty())
		g.setPasswd(p);

	mpl::at_c<Libvirt::Domain::Xml::VGraphics::types, 1>::type z;
	z.setValue(g);
	add<8>(Libvirt::Domain::Xml::VGraphics(z));
}

void List::add(const CVmVideo* video_)
{
	if (NULL == video_)
		return;

	Libvirt::Domain::Xml::Model1 m;
	m.setVram(video_->getMemorySize() << 10);
	if (P3D_DISABLED != video_->getEnable3DAcceleration())
	{
		Libvirt::Domain::Xml::Acceleration a;
		a.setAccel3d(Libvirt::Domain::Xml::EVirYesNoYes);
		m.setAcceleration(a);
	}
	Libvirt::Domain::Xml::Video v;
	v.setModel(m);
	add<9>(v);
}

void List::add(const CVmGenericNetworkAdapter* network_)
{
	if (NULL == network_)
		return;

	Prl::Expected<Libvirt::Domain::Xml::VInterface, ::Error::Simple> a =
		Vm::Reverse::Device<CVmGenericNetworkAdapter>::getLibvirtXml(*network_);
	if (!a.isFailed())
		add<4>(a.value());
}

void List::add(const Libvirt::Domain::Xml::Disk& disk_)
{
	add<0>(disk_);
}

namespace Usb
{

void List::add(const CVmUsbDevice* usb_)
{
	if (usb_ == NULL || m_settings == NULL)
		return;
	if (usb_->getUsbType() != PUDT_OTHER)
		return;
	if (m_settings->isUhcEnabled())
		craftController(Libvirt::Domain::Xml::EModel1Piix3Uhci);
	if (m_settings->isEhcEnabled())
		craftController(Libvirt::Domain::Xml::EModel1Ehci);
	if (m_settings->isXhcEnabled())
		craftController(Libvirt::Domain::Xml::EModel1NecXhci);
}

void List::craftController(Libvirt::Domain::Xml::EModel1 model_)
{
	Libvirt::Domain::Xml::Variant572 v;
	v.setModel(model_);
	mpl::at_c<Libvirt::Domain::Xml::VChoice587::types, 2>::type x;
	x.setValue(v);
	Libvirt::Domain::Xml::Controller y;
	y.setIndex(m_controller++);
	y.setChoice587(x);
	add<1>(y);
}

void List::add(Libvirt::Domain::Xml::EType10 type_)
{
	Libvirt::Domain::Xml::Input x;
	x.setType(type_);
	x.setBus(m_controller ? Libvirt::Domain::Xml::EBus1Usb : Libvirt::Domain::Xml::EBus1Ps2);
	add<5>(x);
}

void List::addKeyboard()
{
	add(Libvirt::Domain::Xml::EType10Keyboard);
}

void List::addMouse()
{
	// if a USB controller is present, then add a USB tablet device
	// otherwise - add a ps/2 mouse
	if (m_controller)
		add(Libvirt::Domain::Xml::EType10Tablet);
	else
		add(Libvirt::Domain::Xml::EType10Mouse);
}

} // namespace Usb

namespace Panic
{
///////////////////////////////////////////////////////////////////////////////
// struct List

void List::add(quint32 os_)
{
	Libvirt::Domain::Xml::Panic p;

	if (IS_WINDOWS(os_) && os_ >= PVS_GUEST_VER_WIN_2012)
	{
		p.setModel(Libvirt::Domain::Xml::EModel10Hyperv);
	}
	else
	{
		p.setModel(Libvirt::Domain::Xml::EModel10Isa);
		Libvirt::Domain::Xml::Isaaddress a;

		// The only one right value.
		// See: https://libvirt.org/formatdomain.html#elementsPanic
		a.setIobase(QString("0x505"));

		mpl::at_c<Libvirt::Domain::Xml::VAddress::types, 7>::type v;
		v.setValue(a);

		p.setAddress(Libvirt::Domain::Xml::VAddress(v));
	}

	m_list << p;
}

} // namespace Panic
} // namespace Devices

namespace Vm
{
namespace Reverse
{
///////////////////////////////////////////////////////////////////////////////
// struct Cpu

Cpu::Cpu(const CVmCpu& input_, const VtInfo& vt_): m_input(input_), m_vt()
{
	if (NULL == vt_.getQemuKvm())
		return;

	m_vt = vt_.getQemuKvm()->getVCpuInfo();
	if (NULL != m_vt)
	{
		m_vcpu = Libvirt::Domain::Xml::Vcpu();
		m_tune = Libvirt::Domain::Xml::Cputune();
	}
}

PRL_RESULT Cpu::setMask()
{
	QString m = m_input.getCpuMask();
	if (m.isEmpty())
		return PRL_ERR_SUCCESS;

	if (!m_vcpu)
		return PRL_ERR_UNINITIALIZED;

	m_vcpu->setCpuset(m);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setUnits()
{
	if (!m_tune)
		return PRL_ERR_UNINITIALIZED;

	m_tune->setShares(m_input.getCpuUnits() * 1024 / 1000);
	m_tune->setPeriod(m_vt->getDefaultPeriod());
	m_tune->setQuota(-1);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setLimit()
{
	qint32 l = m_input.getCpuLimit();
	if (0 == l)
		return PRL_ERR_SUCCESS;

	if (!m_tune)
		return PRL_ERR_UNINITIALIZED;

	m_tune->setQuota(m_vt->getDefaultPeriod() * l / (100 * m_input.getNumber()));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setNumber()
{
	if (!m_vcpu)
		return PRL_ERR_UNINITIALIZED;

	m_vcpu->setOwnValue(m_input.isEnableHotplug()? m_vt->getMaxVCpu(): m_input.getNumber());
	m_vcpu->setCurrent(m_input.getNumber());

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

QString Device<Dimm>::getPlugXml(const Dimm& model_)
{
	Libvirt::Domain::Xml::Memory2 d;
	d.setModel(Libvirt::Domain::Xml::EModel7Dimm);

	Libvirt::Domain::Xml::Target4 t;
	Libvirt::Domain::Xml::ScaledInteger v;
	v.setOwnValue(model_.getSize());
	t.setSize(v);
	t.setNode(model_.getNodeId());
	d.setTarget(t);
	QDomDocument x;
	d.save(x);
	return x.toString();
}

QString Device<CVmHardDisk>::getPlugXml(const CVmHardDisk& model_)
{
	typedef Transponster::Device::Clustered::Builder::Hdd
		builder_type;
	builder_type b(model_);
	b.setDisk();
	b.setFlags();
	b.setSource();
	b.setTarget();
	b.setBackingChain();
	QString s = model_.getSerialNumber();
	if (!s.isEmpty())
		b.setSerial(s);

	QDomDocument x;
	static_cast<const builder_type&>(b).getResult().save(x);

	return x.toString();
}

QString	Device<CVmOpticalDisk>::getUpdateXml(const CVmOpticalDisk& model_)
{
	typedef Transponster::Device::Clustered::Builder::Ordinary<CVmOpticalDisk>
		builder_type;
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

Prl::Expected<QString, ::Error::Simple>
	Device<CVmSerialPort>::getPlugXml(const CVmSerialPort& model_)
{
	Prl::Expected<Libvirt::Domain::Xml::Qemucdev, ::Error::Simple> p =
		getLibvirtXml(model_);
	if (p.isFailed())
		return p.error();

	QDomDocument x;
	if (!p.value().save(x))
		return ::Error::Simple(PRL_ERR_INVALID_ARG);

	return x.toString();
}

Prl::Expected<Libvirt::Domain::Xml::Qemucdev, ::Error::Simple>
	Device<CVmSerialPort>::getLibvirtXml(const CVmSerialPort& model_)
{
	if (PVE::SerialOutputFile != model_.getEmulatedType())
		return ::Error::Simple(PRL_ERR_UNIMPLEMENTED);

	Libvirt::Domain::Xml::Source15 a;
	a.setPath(model_.getUserFriendlyName());
	if (a.getPath().get().isEmpty())
		return ::Error::Simple(PRL_ERR_INVALID_ARG);
	a.setAppend(Libvirt::Domain::Xml::EVirOnOffOn);

	Libvirt::Domain::Xml::QemucdevSrcDef b;
	b.setSourceList(QList<Libvirt::Domain::Xml::Source15 >() << a);
	Libvirt::Domain::Xml::Qemucdev output;
	output.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceFile);
	output.setQemucdevSrcDef(b);

	return output;
}

Prl::Expected<QString, ::Error::Simple>
	Device<CVmGenericNetworkAdapter>::getPlugXml(const CVmGenericNetworkAdapter& model_)
{
	Prl::Expected<Libvirt::Domain::Xml::VInterface, ::Error::Simple> a = 
		getLibvirtXml(model_);
	if (a.isFailed())
		return a.error();

	mpl::at_c<Extract<Libvirt::Domain::Xml::VChoice935Impl>::type, 4>::type e;
	e.setValue(a.value());
	QDomDocument x;
	e.produce(x);
	return x.toString();
}

Prl::Expected<QString, ::Error::Simple>
	Device<CVmGenericNetworkAdapter>::getUpdateXml(const CVmGenericNetworkAdapter& model_)
{
	return getPlugXml(model_);
}

Prl::Expected<Libvirt::Domain::Xml::VInterface, ::Error::Simple>
	Device<CVmGenericNetworkAdapter>::getLibvirtXml(const CVmGenericNetworkAdapter& model_)
{
	switch (model_.getEmulatedType())
	{
	case PNA_BRIDGED_ETHERNET:
		if (model_.getVirtualNetworkID().isEmpty())
			return Transponster::Device::Network<0>()(model_);
		else
			return Transponster::Device::Network<3>()(model_);
	case PNA_DIRECT_ASSIGN:
		return Transponster::Device::Network<4>()(model_);
	case PNA_ROUTED:
	{
		CVmGenericNetworkAdapter routed(model_);
		routed.setSystemName(QString("host-routed"));
		return Transponster::Device::Network<0>()(routed);
	}
	default:
		return ::Error::Simple(PRL_ERR_UNIMPLEMENTED);
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Builder

Builder::Builder(const CVmConfiguration& input_): m_input(input_)
{
	QString x;
	CVmHardware* h;
	CVmIdentification* i = m_input.getVmIdentification();
	if (NULL == i)
		goto bad;

	x = QFileInfo(i->getHomePath()).absolutePath();
	if (x.isEmpty())
		goto bad;

	h = m_input.getVmHardwareList();
	if (NULL == h)
		goto bad;

	h->RevertDevicesPathToAbsolute(x);
	return;
bad:
	m_input.setVmType(PVT_CT);
}

PRL_RESULT Builder::setBlank()
{
	if (m_result.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	 if (Libvirt::Domain::Xml::ETypeKvm != m_result->getType())
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	if (PVT_VM != m_input.getVmType())
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type vos;

	//EFI boot support
	CVmStartupBios* b = m_input.getVmSettings()->getVmStartupOptions()->getBios();
	if (b != NULL && b->isEfiEnabled())
	{
		Libvirt::Domain::Xml::Loader l;
		l.setReadonly(Libvirt::Domain::Xml::EReadonlyYes);
		l.setType(Libvirt::Domain::Xml::EType2Pflash);

		// package OVMF.x86_64
		l.setOwnValue(QString("/usr/share/OVMF/OVMF_CODE.fd"));

		Libvirt::Domain::Xml::Os2 os;
		os.setLoader(l);

		QString x = b->getNVRAM();
		if (!x.isEmpty())
		{
			Libvirt::Domain::Xml::Nvram n;
			n.setOwnValue(x);
			os.setNvram(n);
		}

		vos.setValue(os);
	}

	m_result->setOs(vos);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Builder::setSettings()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	CVmSettings* s = m_input.getVmSettings();
	if (NULL == s)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	CVmCommonOptions* o = s->getVmCommonOptions();
	if (NULL == o)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	QString d = o->getVmDescription();
	if (!d.isEmpty())
		m_result->setDescription(d);

	CVmRunTimeOptions* r(s->getVmRuntimeOptions());
	if (NULL == r)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	Libvirt::Domain::Xml::Blkiotune b;
	b.setWeight(HostUtils::convertIoprioToWeight(r->getIoPriority()));
	m_result->setBlkiotune(b);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Builder::setDevices()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	CVmHardware* h = m_input.getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;
	CVmSettings* s = m_input.getVmSettings();
	if (NULL == s)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	Transponster::Device::List b;
	Transponster::Device::Clustered::List t(Boot::Reverse(s->
		getVmStartupOptions()->getBootDeviceList()), b);
	foreach (const CVmHardDisk* d, h->m_lstHardDisks)
	{
		// "disconnected" flag is not supported for HDDs
		if (d->getConnected() != PVE::DeviceConnected)
			return PRL_ERR_DISK_INVALID_PARAMETERS;
		t.add(d, s->getVmRuntimeOptions());
	}
	foreach (const CVmFloppyDisk* d, h->m_lstFloppyDisks)
	{
		t.add(d);
	}
	foreach (const CVmOpticalDisk* d, h->m_lstOpticalDisks)
	{
		t.add(d);
	}
	b.add(s->getVmRemoteDisplay());
	foreach (const CVmVideo* d, h->m_lstVideo)
	{
		b.add(d);
	}
	foreach (const CVmSerialPort* d, h->m_lstSerialPorts)
	{
		b.add(d);
	}
//	foreach (const CVmParallelPort* d, h->m_lstParallelPorts)
//	{
//		b.add(d);
//	}
	foreach (const CVmGenericNetworkAdapter* d, h->m_lstNetworkAdapters)
	{
		b.add(d);
	}
	foreach (const CVmSoundDevice* d, h->m_lstSoundDevices)
	{
		b.add(d);
	}

	Transponster::Device::Usb::List u(s->getUsbController());
	foreach (const CVmUsbDevice* d, h->m_lstUsbDevices)
	{
		u.add(d);
	}
	u.addMouse();

	Libvirt::Domain::Xml::Devices x;
	if (m_result->getDevices())
		x = m_result->getDevices().get();

	x.setEmulator(b.getEmulator());
	x.setChoice935List(Transponster::Device::deviceList_type()
			<< b.getDeviceList()
			<< t.getAttachment().getControllers()
			<< u.getDevices());

	m_result->setDevices(x);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Builder::setResources(const VtInfo& vt_)
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	CVmHardware* h = m_input.getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	Resources u(m_input);
	Libvirt::Domain::Xml::Memory m;
	if (u.getMemory(m))
		m_result->setMemory(m);

	Libvirt::Domain::Xml::ScaledInteger cur_m;
	if (u.getCurrentMemory(cur_m))
		m_result->setCurrentMemory(cur_m);

	Libvirt::Domain::Xml::MaxMemory max_m;
	if (u.getMaxMemory(max_m))
		m_result->setMaxMemory(max_m);

	Libvirt::Domain::Xml::VCpu c;
	if (u.getVCpu(c))
		m_result->setCpu(c);

	Libvirt::Domain::Xml::Clock t;
	if (u.getClock(t))
		m_result->setClock(t);

	u.getCpu(vt_, *m_result);
	return PRL_ERR_SUCCESS;
}

QString Builder::getResult()
{
	if (m_result.isNull())
		return QString();

	QDomDocument x;
	m_result->save(x);
	m_result.reset();
	return x.toString();
}

///////////////////////////////////////////////////////////////////////////////
// struct Vm

Vm::Vm(const CVmConfiguration& input_): Builder(input_)
{
	if (PVT_VM != m_input.getVmType())
		return;

	m_result.reset(new Libvirt::Domain::Xml::Domain());
	m_result->setType(Libvirt::Domain::Xml::ETypeKvm);
}

PRL_RESULT Vm::setBlank()
{
	PRL_RESULT r = Builder::setBlank();
	if (PRL_FAILED(r))
		return r;

	m_result->setOnCrash(Libvirt::Domain::Xml::ECrashOptionsPreserve);
	setFeatures();
	setCommandline();
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vm::setIdentification()
{
	if (!m_result)
		return PRL_ERR_UNINITIALIZED;

	CVmIdentification* i = m_input.getVmIdentification();
	if (NULL == i)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	mpl::at_c<Libvirt::Domain::Xml::VUUID::types, 1>::type u;
	u.setValue(::Uuid(i->getVmUuid()).toStringWithoutBrackets());
	Libvirt::Domain::Xml::Ids x;
	x.setUuid(Libvirt::Domain::Xml::VUUID(u));
	x.setName(i->getVmName());
	m_result->setIds(x);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vm::setDevices()
{
	CVmSettings* s = m_input.getVmSettings();
	if (NULL == s)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;
	CVmCommonOptions* o = s->getVmCommonOptions();
	if (NULL == o)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	PRL_RESULT r = Builder::setDevices();
	if (PRL_FAILED(r))
		return r;

	Libvirt::Domain::Xml::Devices x;
	if (m_result->getDevices())
		x = m_result->getDevices().get();

	Transponster::Device::Panic::List p;
	p.add(o->getOsVersion());
	x.setPanicList(p.getResult());

	m_result->setDevices(x);
	return PRL_ERR_SUCCESS;
}

void Vm::setCommandline()
{
	Libvirt::Domain::Xml::Commandline q;
	q.setArgList(QList<QString>() << "-d" << "guest_errors,unimp,cpu_reset");
	m_result->setCommandline(q);
}

void Vm::setFeatures()
{
	Libvirt::Domain::Xml::Features f;
	f.setPae(true);
	f.setAcpi(true);
	f.setApic(Libvirt::Domain::Xml::Apic());

	m_result->setFeatures(f);

	CVmSettings* s = m_input.getVmSettings();
	if (NULL == s)
		return;

	CVmCommonOptions* o = s->getVmCommonOptions();
	if (NULL == o)
		return;

	quint32 os = o->getOsVersion();
	if (IS_WINDOWS(os) && os >= PVS_GUEST_VER_WIN_2008)
	{
		Libvirt::Domain::Xml::Hyperv hv;
		hv.setRelaxed(Libvirt::Domain::Xml::EVirOnOffOn);
		hv.setVapic(Libvirt::Domain::Xml::EVirOnOffOn);
		Libvirt::Domain::Xml::Spinlocks s;
		s.setState(Libvirt::Domain::Xml::EVirOnOffOn);
		s.setRetries(8191);
		hv.setSpinlocks(s);
		f.setHyperv(hv);
	}
	m_result->setFeatures(f);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mixer

Mixer::Mixer(const CVmConfiguration& input_, char* xml_): Builder(input_)
{
	shape(xml_, m_result);
}

PRL_RESULT Mixer::setIdentification()
{
	return PRL_ERR_SUCCESS;
}

} // namespace Reverse
} // namespace Vm

namespace Network
{
namespace Address
{
namespace
{
int cast(const quint8* start_, const quint8* end_)
{
	int output = 0;
	while (start_ < end_)
	{
		switch (*start_)
		{
		case 255:
			output += 8;
			++start_;
			continue;
		default:
			return 0;
		case 254:
			++output;
		case 252:
			++output;
		case 248:
			++output;
		case 240:
			++output;
		case 224:
			++output;
		case 192:
			++output;
		case 128:
			++output;
		case 0:
			break;
		}
		break;
	}
	return output;
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// struct IPv4

QHostAddress IPv4::patchEnd(const QHostAddress& start_, const QHostAddress& end_)
{
	quint32 a = start_.toIPv4Address();
	quint32 b = end_.toIPv4Address();
	return QHostAddress(a + qMin(0xffffU, b - a));
}

int IPv4::getMask(const QHostAddress& mask_)
{
	quint32 x = qToBigEndian(mask_.toIPv4Address());
	return cast((quint8* )&x, 4 + (quint8* )&x);
}

const char* IPv4::getFamily()
{
	return "ipv4";
}

///////////////////////////////////////////////////////////////////////////////
// struct IPv6

QHostAddress IPv6::patchEnd(const QHostAddress& start_, const QHostAddress& end_)
{
	Q_IPV6ADDR a = start_.toIPv6Address();
	Q_IPV6ADDR b = end_.toIPv6Address();
	if (std::equal(&a.c[0], &a.c[14], &b.c[0]))
		return end_;

	a.c[14] = 0xff;
	a.c[15] = 0xff;
	return QHostAddress(a);
}

int IPv6::getMask(const QHostAddress& mask_)
{
	Q_IPV6ADDR x = mask_.toIPv6Address();
	return cast(x.c, x.c + 16);
}

const char* IPv6::getFamily()
{
	return "ipv6";
}

} // namespace Address

///////////////////////////////////////////////////////////////////////////////
// struct Reverse

Reverse::Reverse(const CVirtualNetwork& input_): m_input(input_)
{
}

PRL_RESULT Reverse::setUuid()
{
	mpl::at_c<Libvirt::Network::Xml::VUUID::types, 1>::type u;
	u.setValue(::Uuid(m_input.getUuid()).toStringWithoutBrackets());
	m_result.setUuid(Libvirt::Network::Xml::VUUID(u));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setName()
{
	m_result.setName(m_input.getNetworkID());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setType()
{
	if (PVN_BRIDGED_ETHERNET == m_input.getNetworkType())
	{
		Libvirt::Network::Xml::Forward f;
		f.setMode(Libvirt::Network::Xml::EModeBridge);
		m_result.setForward(f);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setBridge()
{
	CVZVirtualNetwork* z = m_input.getVZVirtualNetwork();
	if (NULL != z)
	{
		Libvirt::Network::Xml::Bridge b;
		b.setName(z->getBridgeName());
		m_result.setBridge(b);
	}
	else
	{
		Libvirt::Network::Xml::Bridge b;
		b.setStp(Libvirt::Network::Xml::EVirOnOffOff);
		m_result.setBridge(b);
		if (!m_input.getBoundCardMac().isEmpty())
			m_result.setMac(m_input.getBoundCardMac());
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setVlan()
{
	// <vlan> tag is not for linux bridge
	if (PVN_BRIDGED_ETHERNET == m_input.getNetworkType())
		return PRL_ERR_SUCCESS;

	unsigned short x = m_input.getVLANTag();
	if (Libvirt::Validatable<Libvirt::Network::Xml::PId>::validate(x))
	{
		Libvirt::Network::Xml::Tag t;
		t.setId(x);
		m_result.setVlan(QList<Libvirt::Network::Xml::Tag>() << t);
	}
	return PRL_ERR_SUCCESS;
}

namespace
{
template<class T>
Libvirt::Network::Xml::Ip craft(const CDHCPServer& src_,
			const QHostAddress& host_, const QHostAddress& mask_)
{
	typename mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, T::index>::type a, e;
	a.setValue(src_.getIPScopeStart().toString());
	e.setValue(T::patchEnd(src_.getIPScopeStart(), src_.getIPScopeEnd()).toString());
	Libvirt::Network::Xml::Range r;
	r.setStart(a);
	r.setEnd(e);
	Libvirt::Network::Xml::Dhcp h;
	h.setRangeList(QList<Libvirt::Network::Xml::Range>() << r);
	Libvirt::Network::Xml::Ip output;
	output.setFamily(QString(T::getFamily()));
	output.setDhcp(h);
	a.setValue(host_.toString());
	output.setAddress(Libvirt::Network::Xml::VIpAddr(a));
	typename mpl::at_c<Libvirt::Network::Xml::VIpPrefix::types, T::index>::type p;
	p.setValue(T::getMask(mask_));
	mpl::at_c<Libvirt::Network::Xml::VChoice1175::types, 1>::type m;
	m.setValue(p);
	output.setChoice1175(Libvirt::Network::Xml::VChoice1175(m));

	return output;
}

} // namespace

PRL_RESULT Reverse::setHostOnly()
{
	CHostOnlyNetwork* n = m_input.getHostOnlyNetwork();
	if (NULL == n)
		return PRL_ERR_SUCCESS;

	QList<Libvirt::Network::Xml::Ip> x;
	CDHCPServer* v4 = n->getDHCPServer();
	if (NULL != v4 && v4->isEnabled())
		x << craft<Address::IPv4>(*v4, n->getHostIPAddress(), n->getIPNetMask());

	CDHCPServer* v6 = n->getDHCPv6ServerOrig();
	if (NULL != v6 && v6->isEnabled())
		x << craft<Address::IPv6>(*v6, n->getHostIP6Address(), n->getIP6NetMask());

	if (!x.isEmpty())
		m_result.setIpList(x);

	return PRL_ERR_SUCCESS;
}

QString Reverse::getResult() const
{
	QDomDocument x;
	m_result.save(x);
	return x.toString();
}

} // namespace Network

namespace Interface
{
namespace Bridge
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

PRL_RESULT Reverse::setMaster()
{
	Libvirt::Iface::Xml::BasicEthernetContent e;
	e.setMac(m_master.getMacAddress());
	e.setName(m_master.getDeviceName());
	mpl::at_c<Libvirt::Iface::Xml::VChoice1239::types, 0>::type v;
	v.setValue(e);
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setChoice1239List(QList<Libvirt::Iface::Xml::VChoice1239>() << v);
	m_result.setBridge(b);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setBridge()
{
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setDelay(2.0);
	b.setStp(Libvirt::Iface::Xml::EVirOnOffOff);
	m_result.setBridge(b);
	Libvirt::Iface::Xml::InterfaceAddressing1269 h;
	if (!m_master.isConfigureWithDhcp())
	{
		if (!m_master.isConfigureWithDhcpIPv6())
			return PRL_ERR_SUCCESS;
			
		Libvirt::Iface::Xml::Protocol p;
		p.setDhcp(Libvirt::Iface::Xml::Dhcp());
		h.setProtocol2(p);
	}
	mpl::at_c<Libvirt::Iface::Xml::VChoice1275::types, 0>::type a;
	a.setValue(Libvirt::Iface::Xml::Dhcp());
	h.setProtocol(Libvirt::Iface::Xml::VChoice1275(a));
	mpl::at_c<Libvirt::Iface::Xml::VInterfaceAddressing::types, 0>::type v;
	v.setValue(h);
	m_result.setInterfaceAddressing(v);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setInterface()
{
	m_result.setName(m_name);
	return PRL_ERR_SUCCESS;
}

QString Reverse::getResult() const
{
	QDomDocument x;
	m_result.save(x);
	return x.toString();
}

} // namespace Bridge
} // namespace Interface

namespace Boot
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

Reverse::Reverse(const QList<device_type* >& list_)
{
	unsigned o = 0;
	std::list<device_type* > x = list_.toStdList();
	x.sort(boost::bind(&device_type::getBootingNumber, _1) <
		boost::bind(&device_type::getBootingNumber, _2));
	foreach(device_type* d, x)
	{
		if (!d->isInUse())
			continue;

		key_type k = qMakePair(d->getType(), d->getIndex());
		m_map[k] = ++o;
	}
}

Reverse::order_type Reverse::operator()(const CVmDevice& device_) const
{
	key_type k = qMakePair(device_.getDeviceType(), device_.getIndex());
	map_type::const_iterator p = m_map.find(k);
	if (m_map.end() == p)
		return order_type();

	return p.value();
}

} // namespace Boot

namespace Snapshot
{
namespace
{
template<class T>
QList<Libvirt::Snapshot::Xml::Disk> getAbsentee(const QList<T* >& list_)
{
	QList<Libvirt::Snapshot::Xml::Disk> output;
	foreach (const T* d, list_)
	{
		if (!d->getEnabled())
			continue;

		mpl::at_c<Libvirt::Snapshot::Xml::VName::types, 0>::type a;
		a.setValue(Device::Clustered::Model<T>(*d).getTargetName());
		mpl::at_c<Libvirt::Snapshot::Xml::VDisk::types, 0>::type b;
		b.setValue(Libvirt::Snapshot::Xml::Disk1737());
		Libvirt::Snapshot::Xml::Disk x;
		x.setName(Libvirt::Snapshot::Xml::VName(a));
		x.setDisk(Libvirt::Snapshot::Xml::VDisk(b));
		output << x;
	}
	return output;
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// struct Reverse

Reverse::Reverse(const QString& uuid_, const QString& description_,
	const CVmConfiguration& input_): m_uuid(uuid_),
	m_description(description_), m_hardware(input_.getVmHardwareList())
{
	CVmIdentification* i = input_.getVmIdentification();
	if (NULL == i)
		return;

	QString x = QFileInfo(i->getHomePath()).absolutePath();
	if (!x.isEmpty())
		m_hardware.RevertDevicesPathToAbsolute(x);
}

PRL_RESULT Reverse::setIdentity()
{
	m_result.setName(m_uuid);
	m_result.setDescription(m_description);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setInstructions()
{
	// disk devices
	QList<Libvirt::Snapshot::Xml::Disk> e;
	e << getAbsentee(m_hardware.m_lstFloppyDisks);
	e << getAbsentee(m_hardware.m_lstOpticalDisks);
	foreach (const CVmHardDisk* d, m_hardware.m_lstHardDisks)
	{
		if (!d->getEnabled() || d->getEmulatedType() != PVE::HardDiskImage)
			continue;

		mpl::at_c<Libvirt::Snapshot::Xml::VName::types, 0>::type a;
		a.setValue(Device::Clustered::Model<CVmHardDisk>(*d).getTargetName());
		mpl::at_c<Libvirt::Snapshot::Xml::VDisk::types, 1>::type b;
		b.setValue(Libvirt::Snapshot::Xml::Disk1738());
		Libvirt::Snapshot::Xml::Disk x;
		x.setName(Libvirt::Snapshot::Xml::VName(a));
		x.setDisk(Libvirt::Snapshot::Xml::VDisk(b));
		e << x;
	}
	m_result.setDisks(e);
	if (!m_result.getMemory())
	{
		// no memory
		mpl::at_c<Libvirt::Snapshot::Xml::VMemory::types, 0>::type a;
		a.setValue(Libvirt::Snapshot::Xml::ESnapshotNo);
		m_result.setMemory(Libvirt::Snapshot::Xml::VMemory(a));
	}
	return PRL_ERR_SUCCESS;
}

QString Reverse::getResult() const
{
	QDomDocument x;
	m_result.save(x);
	return x.toString();
}

void Reverse::setMemory()
{
	mpl::at_c<Libvirt::Snapshot::Xml::VMemory::types, 0>::type a;
	a.setValue(Libvirt::Snapshot::Xml::ESnapshotInternal);
	m_result.setMemory(Libvirt::Snapshot::Xml::VMemory(a));
}

} // namespace Snapshot

} // namespace Transponster

