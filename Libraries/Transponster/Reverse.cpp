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

#include "Reverse.h"
#include "Reverse_p.h"
#include "Direct_p.h"
#include <prlsdk/PrlOses.h>
#include <Libraries/HostUtils/HostUtils.h>

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

	Libvirt::Domain::Xml::Cpu952 c;
	c.setMode(Libvirt::Domain::Xml::EModeHostPassthrough);

#if (LIBVIR_VERSION_NUMBER >= 1002013)
	QList<Libvirt::Domain::Xml::Feature> l;
	if (!u->isVirtualizedHV())
		disableFeature(l, QString("vmx"));
	c.setFeatureList(l);
#endif

	mpl::at_c<Libvirt::Domain::Xml::VCpu::types, 2>::type v;
	v.setValue(c);
	dst_ = Libvirt::Domain::Xml::VCpu(v);

	return true;
}

void Resources::setCpu(const Libvirt::Domain::Xml::Vcpu& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	CVmCpu* u = new CVmCpu();
	u->setNumber(src_.getOwnValue());
	if (src_.getCpuset())
		u->setCpuMask(src_.getCpuset().get());

	h->setCpu(u);
}

bool Resources::getCpu(Libvirt::Domain::Xml::Vcpu& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	CVmCpu* u = h->getCpu();
	if (NULL == u)
		return false;

	dst_.setOwnValue(u->getNumber());
	QString m = u->getCpuMask();
	if (!m.isEmpty())
		dst_.setCpuset(m);

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

	CVmMemory* m = new CVmMemory();
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
		mpl::at_c<Libvirt::Domain::Xml::VChoice1041::types, 0>::type y;
		y.setValue(p);
		t.setChoice1041(Libvirt::Domain::Xml::VChoice1041(y));
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
		mpl::at_c<Libvirt::Domain::Xml::VChoice1045::types, 0>::type y;
		y.setValue(p);
		t.setChoice1045(Libvirt::Domain::Xml::VChoice1045(y));
	}

	getResult().setIotune(t);
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
	Builder::Hdd b(*hdd_, m_boot(*hdd_));
	if (NULL != runtime_)
	{
		b.setIoLimit(runtime_->getIoLimit());
		b.setIopsLimit(*runtime_);
	}
	build(b);
}

void List::add(const CVmOpticalDisk* cdrom_)
{
	if (cdrom_ == NULL)
		return;
	if (cdrom_->getEmulatedType() == Flavor<CVmOpticalDisk>::real)
		return;
	build(Builder::Ordinary<CVmOpticalDisk>(*cdrom_, m_boot(*cdrom_)));
}

void List::add(const CVmFloppyDisk* floppy_)
{
	if (floppy_ == NULL)
		return;
	build(Builder::Ordinary<CVmFloppyDisk>(*floppy_, m_boot(*floppy_)));
}

const Attachment& List::getAttachment() const
{
	return m_attachment;
}

} // namespace Clustered

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
	mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 1>::type y;
	y.setValue(x);
	m_controllerList << Libvirt::Domain::Xml::VChoice932(y);
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
	mpl::at_c<Libvirt::Domain::Xml::VChoice846::types, 1>::type x;
	x.setValue(QString("org.qemu.guest_agent.0"));
	c.setChoice846(x);
	add<13>(c);
}

Libvirt::Domain::Xml::Devices List::getResult() const
{
	Libvirt::Domain::Xml::Devices output;
	if (QFile::exists("/usr/bin/qemu-kvm"))
		output.setEmulator(QString("/usr/bin/qemu-kvm"));
	else if (QFile::exists("/usr/libexec/qemu-kvm"))
		output.setEmulator(QString("/usr/libexec/qemu-kvm"));

	output.setChoice932List(deviceList_type() << m_deviceList);

	output.setPanic(craftPanic());

	return output;
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

	if (PVE::SerialOutputFile != port_->getEmulatedType())
		return;

	Libvirt::Domain::Xml::Source15 a;
	a.setPath(port_->getUserFriendlyName());
	if (a.getPath().get().isEmpty())
		return;

	Libvirt::Domain::Xml::QemucdevSrcDef b;
	b.setSourceList(QList<Libvirt::Domain::Xml::Source15 >() << a);
	Libvirt::Domain::Xml::Qemucdev p;
	p.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceFile);
	p.setQemucdevSrcDef(b);
	add<12>(p);
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

	switch (network_->getEmulatedType())
	{
	case PNA_BRIDGED_ETHERNET:
		if (network_->getVirtualNetworkID().isEmpty())
			return add<4>(Network<0>()(*network_));
		else
			return add<4>(Network<3>()(*network_));

	case PNA_DIRECT_ASSIGN:
		add<4>(Network<4>()(*network_));
	default:
		return;
	}
}

void List::add(const Libvirt::Domain::Xml::Disk& disk_)
{
	add<0>(disk_);
}

Libvirt::Domain::Xml::Panic List::craftPanic() const
{
	Libvirt::Domain::Xml::Panic p;
	Libvirt::Domain::Xml::Isaaddress a;

	// The only one right value.
	// See: https://libvirt.org/formatdomain.html#elementsPanic
	a.setIobase(QString("0x505"));

	mpl::at_c<Libvirt::Domain::Xml::VAddress::types, 7>::type v;
	v.setValue(a);

	p.setAddress(Libvirt::Domain::Xml::VAddress(v));

	return p;
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
} // namespace Devices

namespace Vm
{
namespace Reverse
{
///////////////////////////////////////////////////////////////////////////////
// struct Cdrom

PRL_RESULT Cdrom::operator()()
{
	m_result = boost::none;
	typedef Device::Clustered::Builder::Ordinary<CVmOpticalDisk> builder_type;
	builder_type b(m_input);
	b.setDisk();
	b.setFlags();
	b.setSource();
	b.setTarget();
	b.setBackingChain();
	m_result = static_cast<const builder_type&>(b).getResult();

	return PRL_ERR_SUCCESS;
}

QString Cdrom::getResult()
{
	if (!m_result)
		return QString();

	QDomDocument x;
	m_result->save(x);
	m_result = boost::none;
	return x.toString();
}

///////////////////////////////////////////////////////////////////////////////
// struct Reverse

Vm::Vm(const CVmConfiguration& input_): m_input(input_)
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

PRL_RESULT Vm::setBlank()
{
	m_result = boost::none;
	if (PVT_VM != m_input.getVmType())
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	m_result = Libvirt::Domain::Xml::Domain();
	m_result->setType(Libvirt::Domain::Xml::ETypeKvm);
	mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type vos;

	//EFI boot support
	if (m_input.getVmSettings()->getVmStartupOptions()->getBios()->isEfiEnabled())
	{
		Libvirt::Domain::Xml::Loader l;
		l.setReadonly(Libvirt::Domain::Xml::EReadonlyYes);
		l.setType(Libvirt::Domain::Xml::EType2Pflash);

		// package OVMF.x86_64
		l.setOwnValue(QString("/usr/share/OVMF/OVMF_CODE.fd"));

		Libvirt::Domain::Xml::Os2 os;
		os.setLoader(l);
		vos.setValue(os);
	}

	m_result->setOs(vos);
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

PRL_RESULT Vm::setSettings()
{
	if (!m_result)
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

PRL_RESULT Vm::setDevices()
{
	if (!m_result)
		return PRL_ERR_UNINITIALIZED;

	CVmHardware* h = m_input.getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;
	CVmSettings* s = m_input.getVmSettings();
	if (NULL == s)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	Device::List b;
	Device::Clustered::List t(Boot::Reverse(s->
		getVmStartupOptions()->getBootDeviceList()), b);
	foreach (const CVmHardDisk* d, h->m_lstHardDisks)
	{
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

	Device::Usb::List u(s->getUsbController());
	foreach (const CVmUsbDevice* d, h->m_lstUsbDevices)
	{
		u.add(d);
	}
	u.addMouse();

	Libvirt::Domain::Xml::Devices x = b.getResult();
	QList<Libvirt::Domain::Xml::VChoice932> n(x.getChoice932List());
	n << t.getAttachment().getControllers() << u.getDevices();
	x.setChoice932List(n);
	m_result->setDevices(x);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vm::setResources()
{
	if (!m_result)
		return PRL_ERR_UNINITIALIZED;

	CVmHardware* h = m_input.getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	Resources u(m_input);
	Libvirt::Domain::Xml::Memory m;
	if (u.getMemory(m))
		m_result->setMemory(m);

	Libvirt::Domain::Xml::VCpu c;
	if (u.getVCpu(c))
		m_result->setCpu(c);

	Libvirt::Domain::Xml::Vcpu v;
	if (u.getCpu(v))
		m_result->setVcpu(v);

	Libvirt::Domain::Xml::Clock t;
	if (u.getClock(t))
		m_result->setClock(t);

	return PRL_ERR_SUCCESS;
}

QString Vm::getResult()
{
	if (!m_result)
		return QString();

	QDomDocument x;
	m_result->save(x);
	m_result = boost::none;
	return x.toString();
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
	mpl::at_c<Libvirt::Network::Xml::VChoice1166::types, 1>::type m;
	m.setValue(p);
	output.setChoice1166(Libvirt::Network::Xml::VChoice1166(m));

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
	mpl::at_c<Libvirt::Iface::Xml::VChoice1230::types, 0>::type v;
	v.setValue(e);
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setChoice1230List(QList<Libvirt::Iface::Xml::VChoice1230>() << v);
	m_result.setBridge(b);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setBridge()
{
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setDelay(2.0);
	b.setStp(Libvirt::Iface::Xml::EVirOnOffOff);
	m_result.setBridge(b);
	Libvirt::Iface::Xml::InterfaceAddressing1260 h;
	if (!m_master.isConfigureWithDhcp())
	{
		if (!m_master.isConfigureWithDhcpIPv6())
			return PRL_ERR_SUCCESS;
			
		Libvirt::Iface::Xml::Protocol p;
		p.setDhcp(Libvirt::Iface::Xml::Dhcp());
		h.setProtocol2(p);
	}
	mpl::at_c<Libvirt::Iface::Xml::VChoice1266::types, 0>::type a;
	a.setValue(Libvirt::Iface::Xml::Dhcp());
	h.setProtocol(Libvirt::Iface::Xml::VChoice1266(a));
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
		b.setValue(Libvirt::Snapshot::Xml::Disk1726());
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

Reverse::Reverse(const QString& uuid_, const CVmConfiguration& input_):
	m_uuid(uuid_), m_hardware(input_.getVmHardwareList())
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
	m_result.setDescription(QString("Snapshot by dispatcher"));
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
		b.setValue(Libvirt::Snapshot::Xml::Disk1727());
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

