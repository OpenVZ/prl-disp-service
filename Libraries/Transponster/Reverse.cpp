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
#include <QUrl>
#include <Libraries/PrlNetworking/netconfig.h>
#include <Libraries/CpuFeatures/CCpuHelper.h>

namespace Transponster
{
///////////////////////////////////////////////////////////////////////////////
// struct Resources

void Resources::setCpu(const Libvirt::Domain::Xml::Cpu& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	Vm::Reverse::CpuFeaturesMask m(*m_config);
	m.setDisabledFeatures(src_);

	if (src_.getNuma())
	{
		qint32 maxNumaRam = 0;
		foreach (const Libvirt::Domain::Xml::Cell& cell, src_.getNuma().get())
			maxNumaRam += cell.getMemory()>>10;

		if (maxNumaRam)
			h->getMemory()->setMaxNumaRamSize(maxNumaRam);
	}
}

bool Resources::getCpu(const VtInfo& vt_, Libvirt::Domain::Xml::Cpu& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	CVmCpu* u = h->getCpu();
	if (NULL == u)
		return false;

	if(0 == u->getNumber())
		return false;

	dst_.setMode(Libvirt::Domain::Xml::EModeCustom);

	Vm::Reverse::CpuFeaturesMask mask(*m_config);
	mask.getFeatures(vt_, dst_);

	Libvirt::Domain::Xml::Model z;
	if (dst_.getModel())
		z = *dst_.getModel();
	z.setOwnValue(vt_.getCpuModel());
	dst_.setModel(z);

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
		dst_.setNuma(cells);
	}

	return true;
}

void Resources::setCpu(const Libvirt::Domain::Xml::Domain& vm_, const VtInfo& vt_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	Vm::Direct::Cpu b(vm_, h->getCpu(), vt_);
	if (PRL_FAILED(Director::cpu(b)))
		return;
	CVmCpu *cpu = b.getResult();
	if (cpu != NULL)
	{
		cpu->setVirtualizePMU(
				vm_.getFeatures() &&
				vm_.getFeatures()->getPmu() &&
				vm_.getFeatures()->getPmu()->getState() &&
				vm_.getFeatures()->getPmu()->getState().get() ==
					Libvirt::Domain::Xml::EVirOnOffOn);
	}
	h->setCpu(cpu);
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
	dst_.setNumatune(b.getNuma());

	boost::optional<Libvirt::Domain::Xml::Features> f = dst_.getFeatures();
	if (u->isVirtualizePMU())
	{
		if (!f)
			f = Libvirt::Domain::Xml::Features();

		Libvirt::Domain::Xml::Pmu pmu;
		pmu.setState(Libvirt::Domain::Xml::EVirOnOffOn);
		f->setPmu(pmu);
	}
	else if (f)
		f->setPmu(boost::none);
	dst_.setFeatures(f);

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

	mpl::at_c<Libvirt::Domain::Xml::VTimer::types, 3>::type v;

	quint32 os = m_config->getVmSettings()->getVmCommonOptions()->getOsVersion();
	if (IS_WINDOWS(os) && os >= PVS_GUEST_VER_WIN_2008)
		v.setValue(Libvirt::Domain::Xml::EName2Hypervclock);
	else
		v.setValue(Libvirt::Domain::Xml::EName2Kvmclock);

	Libvirt::Domain::Xml::Timer t;
	t.setTimer(Libvirt::Domain::Xml::VTimer(v));
	t.setPresent(Libvirt::Domain::Xml::EVirYesNoYes);

	QList<Libvirt::Domain::Xml::Timer> timers;
	timers.append(t);
	dst_.setTimerList(timers);

	Libvirt::Domain::Xml::Clock377 k;
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
		return true;

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

const Libvirt::Domain::Xml::EDevice Flavor<CVmHardDisk>::kind = Libvirt::Domain::Xml::EDeviceDisk;
const boost::none_t Flavor<CVmHardDisk>::snapshot = boost::none;

const Libvirt::Domain::Xml::EDevice Flavor<CVmOpticalDisk>::kind = Libvirt::Domain::Xml::EDeviceCdrom;
const Libvirt::Domain::Xml::ESnapshot Flavor<CVmOpticalDisk>::snapshot = Libvirt::Domain::Xml::ESnapshotNo;

const Libvirt::Domain::Xml::EDevice Flavor<CVmFloppyDisk>::kind = Libvirt::Domain::Xml::EDeviceFloppy;
const Libvirt::Domain::Xml::ESnapshot Flavor<CVmFloppyDisk>::snapshot = Libvirt::Domain::Xml::ESnapshotNo;

///////////////////////////////////////////////////////////////////////////////
// struct Model

template<>
QString Model<CVmFloppyDisk>::getTargetName() const
{
	return QString("fd") + Parallels::toBase26(m_dataSource->getIndex());
}

template<>
boost::optional<Libvirt::Domain::Xml::EBus> Model<CVmFloppyDisk>::getBus() const
{
	return Libvirt::Domain::Xml::EBusFdc;
}

namespace Builder
{
///////////////////////////////////////////////////////////////////////////////
// struct Ordinary

template <>
void Ordinary<CVmOpticalDisk>::setSource()
{
	m_result.setDiskSource(getSource());
}

template<>
void Ordinary<CVmHardDisk>::setSource()
{
	Libvirt::Domain::Xml::VDiskSource x = getSource();
	if (!x.empty())
		return m_result.setDiskSource(x);

	if (PVE::BootCampHardDisk == getModel().getEmulatedType())
	{
		Libvirt::Domain::Xml::Source3 s;
		s.setVolume(getModel().getRealDeviceName());
		mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 3>::type x;
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
		mpl::at_c<Libvirt::Domain::Xml::VChoice1057::types, 0>::type y;
		y.setValue(p);
		t.setChoice1057(Libvirt::Domain::Xml::VChoice1057(y));
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
		mpl::at_c<Libvirt::Domain::Xml::VChoice1061::types, 0>::type y;
		y.setValue(p);
		t.setChoice1061(Libvirt::Domain::Xml::VChoice1061(y));
	}

	getResult().setIotune(t);
}

void Hdd::setSerial(const QString& serial_)
{
	getResult().setSerial(serial_);
}

} // namespace Builder

} // namespace Clustered

namespace Boot
{
///////////////////////////////////////////////////////////////////////////////
// struct List

List::List(const CVmSettings& settings_, Device::List& list_):
	m_boot(settings_.getVmStartupOptions()->getBootDeviceList()),
	m_deviceList(list_)
{
}

void List::add(const CVmHardDisk* hdd_, const CVmRunTimeOptions* runtime_)
{
	if (hdd_ == NULL)
		return;
	if (hdd_->getEnabled() != PVE::DeviceEnabled)
		return;
	Clustered::Builder::Hdd b(*hdd_, m_boot(*hdd_));
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
	if (cdrom_->getEmulatedType() == Clustered::Flavor<CVmOpticalDisk>::real)
		return;
	if (cdrom_->getEnabled() != PVE::DeviceEnabled)
		return;
	build(Clustered::Builder::Ordinary<CVmOpticalDisk>(*cdrom_, m_boot(*cdrom_)));
}

void List::add(const CVmFloppyDisk* floppy_)
{
	if (floppy_ == NULL)
		return;
	if (floppy_->getEnabled() != PVE::DeviceEnabled)
		return;
	build(Clustered::Builder::Ordinary<CVmFloppyDisk>(*floppy_, m_boot(*floppy_)));
}

void List::add(const CVmGenericNetworkAdapter* network_)
{
	if (network_ == NULL)
		return;

	Prl::Expected<Libvirt::Domain::Xml::VInterface, ::Error::Simple> a =
		Network::build(*network_, m_boot(*network_));

	if (a.isSucceed())
		m_deviceList.add(a.value());
}

const Attachment& List::getAttachment() const
{
	return m_attachment;
}

} //namespace Boot

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

namespace Network
{

///////////////////////////////////////////////////////////////////////////////
// struct View

QString View::getAdapterType() const
{
	switch (m_network.getAdapterType()) {
	case PNT_RTL:
		return QString("rtl8139");
	case PNT_E1000:
		return QString("e1000");
	default:
		return QString("virtio");
	}
}

QString View::normalizeMac(const QString &mac_)
{
	QString normalized(mac_);
	return normalized.replace(QRegExp("([^:]{2})(?!:|$)"), "\\1:");
}

QStringList View::getIpv4() const
{
	return NetworkUtils::ParseIps(m_network.getNetAddresses()).first;
}

QString View::getMac() const
{
	return normalizeMac(m_network.getMacAddress());
}

QString View::getFilterName() const
{
	CNetPktFilter *filter = m_network.getPktFilter();
	QStringList filters;

	if (filter->isPreventIpSpoof() &&
		// For enforced static IPs only
		m_network.isAutoApply() &&
		!m_network.isConfigureWithDhcp() &&
		!getIpv4().isEmpty())
		filters << "no-ip-spoofing";
	if (filter->isPreventMacSpoof())
		filters << "no-mac-spoofing";
	if (filter->isPreventPromisc())
		filters << "no-promisc";

	return filters.join("-");
}

boost::optional<Libvirt::Domain::Xml::FilterrefNodeAttributes> View::getFilterref() const
{
	QString filter = getFilterName();
	if (filter.isEmpty())
		return boost::none;

	Libvirt::Domain::Xml::FilterrefNodeAttributes filterref;
	filterref.setFilter(filter);

	QList<Libvirt::Domain::Xml::Parameter> params;
	Libvirt::Domain::Xml::Parameter p;
	foreach(const CNetPktFilter *filter, m_network.m_lstPktFilter)
	{
		QString mac = normalizeMac(filter->getMacAddress());
		if (mac.isEmpty())
			continue;

		p.setName("MAC");
		p.setValue(mac);
		params << p;
	}
	if (params.isEmpty())
	{
		p.setName("MAC");
		p.setValue(getMac());
		params << p;
	}

	foreach(const QString& e, getIpv4())
	{
		// IPv4 only, for now
		p.setName("IP");
		p.setValue(e.split('/').first());
		params << p;
	}

	filterref.setParameterList(params);
	return filterref;
}

///////////////////////////////////////////////////////////////////////////////
// struct Adapter

template<int N>
Libvirt::Domain::Xml::VInterface Adapter<N>::operator()
	(const CVmGenericNetworkAdapter& network_, const boot_type& boot_)
{
	typename Libvirt::Details::Value::Grab<access_type>::type i = prepare(network_);

	if (network_.getConnected() != PVE::DeviceConnected)
		i.setLink(Libvirt::Domain::Xml::EStateDown);

	i.setAlias(network_.getSystemName());
	View view(network_);
	QString m = view.getMac();
	if (!m.isEmpty())
		i.setMac(m);

	i.setFilterref(view.getFilterref());
	i.setBoot(boot_);

	access_type output;
	output.setValue(i);
	return output;
}

template<>
Libvirt::Domain::Xml::Interface620 Adapter<0>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface620 output;
	output.setIpList(Ips()(network_.getNetAddresses()));
	output.setModel(View(network_).getAdapterType());
	Libvirt::Domain::Xml::Source6 s;
	if (network_.getVirtualNetworkID().isEmpty())
		s.setBridge(QString("host-routed"));
	else
		s.setBridge(network_.getVirtualNetworkID());
	output.setSource(s);
	output.setTarget(network_.getHostInterfaceName());
	return output;
}

template<>
Libvirt::Domain::Xml::Interface628 Adapter<3>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface628 output;
	Libvirt::Domain::Xml::Source8 s;
	s.setNetwork(network_.getVirtualNetworkID());
	output.setIpList(Ips()(network_.getNetAddresses()));
	output.setTarget(network_.getHostInterfaceName());
	output.setModel(View(network_).getAdapterType());
	output.setSource(s);
	return output;
}

template<>
Libvirt::Domain::Xml::Interface630 Adapter<4>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface630 output;
	Libvirt::Domain::Xml::Source9 s;
	s.setDev(network_.getSystemName());
	output.setIpList(Ips()(network_.getNetAddresses()));
	output.setModel(View(network_).getAdapterType());
	output.setTarget(network_.getHostInterfaceName());
	output.setSource(s);
	return output;
}

Prl::Expected<Libvirt::Domain::Xml::VInterface, ::Error::Simple>
	build(const CVmGenericNetworkAdapter& network_, const boot_type& boot_)
{
	switch (network_.getEmulatedType())
	{
	case PNA_BRIDGE:
		return Adapter<0>()(network_, boot_);
	case PNA_BRIDGED_NETWORK:
		/* Legacy case before PNA_BRIDGE introduced*/
		if (network_.getVirtualNetworkID().isEmpty())
			return Adapter<0>()(network_, boot_);
		return Adapter<3>()(network_, boot_);
	case PNA_DIRECT_ASSIGN:
		return Adapter<4>()(network_, boot_);
	case PNA_ROUTED:
	{
		CVmGenericNetworkAdapter routed(network_);
		routed.setSystemName(QString("host-routed"));
		routed.setVirtualNetworkID(QString("host-routed"));
		return Adapter<0>()(routed, boot_);
	}
	default:
		return ::Error::Simple(PRL_ERR_UNIMPLEMENTED);
	}
}

} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Attachment

void Attachment::craftController(const Libvirt::Domain::Xml::VChoice590& bus_, quint16 index_)
{
	Libvirt::Domain::Xml::Controller x;
	x.setIndex(index_);
	x.setChoice590(bus_);
	mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 1>::type y;
	y.setValue(x);
	m_controllerList << Libvirt::Domain::Xml::VChoice941(y);
}

Libvirt::Domain::Xml::VAddress Attachment::craftIde(quint32 index_)
{
	quint16 c = index_ / IDE_UNITS / IDE_BUSES;
	quint16 b = index_ / IDE_UNITS % IDE_BUSES;
	quint16 u = index_ % IDE_UNITS;

	if (c > 0 && u == 0 && b == 0)
	{
		mpl::at_c<Libvirt::Domain::Xml::VChoice590::types, 0>::type v;
		v.setValue(Libvirt::Domain::Xml::EType6Ide);
		craftController(v, c);
	}

	return Address().setUnit(u).setBus(b)(c);
}

Libvirt::Domain::Xml::VAddress Attachment::craftSata(quint32 index_)
{
	quint16 c = index_ / SATA_UNITS;
	quint16 u = index_ % SATA_UNITS;

	if (c > 0 && u == 0)
	{
		mpl::at_c<Libvirt::Domain::Xml::VChoice590::types, 0>::type v;
		v.setValue(Libvirt::Domain::Xml::EType6Sata);
		craftController(v, c);
	}

	return Address().setUnit(u)(c);
}

Libvirt::Domain::Xml::VAddress Attachment::craftScsi
	(quint32 index_, const boost::optional<Libvirt::Domain::Xml::EModel>& model_)
{
	Libvirt::Domain::Xml::EModel m = Libvirt::Domain::Xml::EModelAuto;
	if (model_)
		m = model_.get();
	quint16 c = index_ / SCSI_TARGETS;
	quint16 t = index_ % SCSI_TARGETS;

	if (t == 0)
	{
		mpl::at_c<Libvirt::Domain::Xml::VChoice590::types, 1>::type v;
		v.setValue(m);
		craftController(v, c);
	}

	return Address().setTarget(t)(c);
}

///////////////////////////////////////////////////////////////////////////////
// struct List

void List::addGuestChannel(const QString &path_)
{
	Libvirt::Domain::Xml::Channel1 c;
	c.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceUnix);
	mpl::at_c<Libvirt::Domain::Xml::VChoice851::types, 1>::type x;
	x.setValue(path_);
	c.setChoice851(x);
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

	Libvirt::Domain::Xml::Variant688 v;
	v.setPort(vnc_->getPortNumber());
	v.setListen(vnc_->isEncrypted() ?
		QHostAddress(QHostAddress::LocalHostIPv6).toString() :
		vnc_->getHostName());
	if (PRD_AUTO == vnc_->getMode())
		v.setAutoport(Libvirt::Domain::Xml::EVirYesNoYes);

	mpl::at_c<Libvirt::Domain::Xml::VChoice690::types, 0>::type y;
	y.setValue(v);
	Libvirt::Domain::Xml::Graphics697 g;
	g.setChoice690(Libvirt::Domain::Xml::VChoice690(y));
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
		a.setAccel2d(Libvirt::Domain::Xml::EVirYesNoNo);
		a.setAccel3d(Libvirt::Domain::Xml::EVirYesNoYes);
		m.setAcceleration(a);
	}
	Libvirt::Domain::Xml::Video v;
	v.setModel(m);
	add<9>(v);
}

void List::add(const Libvirt::Domain::Xml::Disk& disk_)
{
	add<0>(disk_);
}

void List::add(const Libvirt::Domain::Xml::VInterface& adapter_)
{
	add<4>(adapter_);
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
	Libvirt::Domain::Xml::Variant575 v;
	v.setModel(model_);
	mpl::at_c<Libvirt::Domain::Xml::VChoice590::types, 2>::type x;
	x.setValue(v);
	Libvirt::Domain::Xml::Controller y;
	y.setIndex(m_controller++);
	y.setChoice590(x);
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

Cpu::Cpu(const CVmCpu& input_, const VtInfo& vt_): m_input(input_), m_vt(vt_)
{
	if (NULL == vt_.getQemuKvm())
		return;

	if (NULL != m_vt.getQemuKvm()->getVCpuInfo())
	{
		m_vcpu = Libvirt::Domain::Xml::Vcpu();
		m_tune = Libvirt::Domain::Xml::Cputune();
		m_numa = Libvirt::Domain::Xml::Numatune();
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

PRL_RESULT Cpu::setNode()
{
	QString m = m_input.getNodeMask();
	if (m.isEmpty())
		return PRL_ERR_SUCCESS;

	if (!m_numa)
		return PRL_ERR_UNINITIALIZED;

	Libvirt::Domain::Xml::Memory1 d;
	d.setMode(Libvirt::Domain::Xml::EMode3Strict);
	mpl::at_c<Libvirt::Domain::Xml::VMemory::types, 0>::type v;
	v.setValue(m);
	d.setMemory(Libvirt::Domain::Xml::VMemory(v));

	m_numa->setMemory(d);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setUnits()
{
	if (!m_tune)
		return PRL_ERR_UNINITIALIZED;

	CVCpuInfo* v(m_vt.getQemuKvm()->getVCpuInfo());
	if (!v)
		return PRL_ERR_UNINITIALIZED;

	m_tune->setShares(m_input.getCpuUnits() * 1024 / 1000);
	if (m_vt.isGlobalCpuLimit()) {
		m_tune->setGlobalPeriod(v->getDefaultPeriod());
		m_tune->setGlobalQuota(-1);
	} else {
		m_tune->setPeriod(v->getDefaultPeriod());
		m_tune->setQuota(-1);
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setLimit()
{
	qint32 l = m_input.getCpuLimitValue();
	if (0 == l)
		return PRL_ERR_SUCCESS;

	if (!m_tune)
		return PRL_ERR_UNINITIALIZED;

	CVCpuInfo* v(m_vt.getQemuKvm()->getVCpuInfo());
	if (!v)
		return PRL_ERR_UNINITIALIZED;

	quint32 q;
	if (m_input.getCpuLimitType() == PRL_CPULIMIT_PERCENTS)
		q = v->getDefaultPeriod() * l / 100;
	else
		q = v->getDefaultPeriod() * l / v->getMhz();

	if (m_vt.isGlobalCpuLimit())
		m_tune->setGlobalQuota(q);
	else
		m_tune->setQuota(q / m_input.getNumber());

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setNumber()
{
	if (!m_vcpu)
		return PRL_ERR_UNINITIALIZED;

	m_vcpu->setOwnValue(m_input.isEnableHotplug() ? 32 : m_input.getNumber());
	m_vcpu->setCurrent(m_input.getNumber());

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct CpuFeaturesMask

void CpuFeaturesMask::getFeatures(const VtInfo& vt_, Libvirt::Domain::Xml::Cpu &cpu)
{
	QSet<QString> features = CCpuHelper::getDisabledFeatures(*m_input);

	if (!m_input->getVmHardwareList()->getCpu()->isVirtualizedHV())
		features.insert(QString("vmx"));

	QList<Libvirt::Domain::Xml::Feature> l;
	foreach(const QString& name, vt_.getRequiredCpuFeatures())
	{
		if (features.contains(name))
			continue;
		/* invtsc and xsaves are non migratable (see libvirt cpu_map.xml) */
		if (name == "invtsc" || name == "xsaves")
			continue;
		Libvirt::Domain::Xml::Feature f;
		f.setName(name);
		Libvirt::Domain::Xml::EPolicy p(Libvirt::Domain::Xml::EPolicyRequire);
		/* FIXME arat feature will be implemented in Update3. It should be disabled
		   to keep libvirt migration work. It is not working in update1 QEMU.
		   #PSBM-52808 #PSBM-51001 #PSBM-52852 */
		if (name == "arat")
			p = Libvirt::Domain::Xml::EPolicyDisable;
		f.setPolicy(p);
		l.append(f);
	}
	foreach(QString name, features)
	{
		/* hypervisor feature is pure virtual, it's always absent
		   in cpufeatures mask. VM requires it, so dispatcher should
		   not disable it */
		if (name == "hypervisor")
			continue;
		Libvirt::Domain::Xml::Feature f;
		f.setName(name);
		f.setPolicy(Libvirt::Domain::Xml::EPolicyDisable);
		l.append(f);
	}

	cpu.setFeatureList(l);
}

void CpuFeaturesMask::setDisabledFeatures(const Libvirt::Domain::Xml::Cpu &cpu)
{
	foreach (const Libvirt::Domain::Xml::Feature& f, cpu.getFeatureList())
	{
		if (f.getName().compare(QString("vmx"), Qt::CaseInsensitive))
		{
			if (f.getPolicy() == Libvirt::Domain::Xml::EPolicyDisable)
				m_input->getVmHardwareList()->getCpu()->setVirtualizedHV(false);
			break;
		}
	}

	CCpuHelper::update(*m_input);
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
	b.setDriver();
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

QString Device<CVmHardDisk>::getTargetName(const CVmHardDisk& model_)
{
	Transponster::Device::Clustered::Builder::Hdd b(model_);
	return b.getModel().getTargetName();
}

QString Device<CVmFloppyDisk>::getUpdateXml(const CVmFloppyDisk& model_)
{
	return Transponster::Device::Clustered::Builder
		::ChangeableMedia<CVmFloppyDisk>::getUpdateXml(model_);
}

QString Device<CVmOpticalDisk>::getUpdateXml(const CVmOpticalDisk& model_)
{
	return Transponster::Device::Clustered::Builder
		::ChangeableMedia<CVmOpticalDisk>::getUpdateXml(model_);
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

namespace
{

QString getMode(PRL_SERIAL_PORT_SOCKET_OPERATION_MODE mode_)
{
	return mode_ == PSP_SERIAL_SOCKET_SERVER ?
		"bind" : "connect";
}

} // namespace

Prl::Expected<Libvirt::Domain::Xml::Qemucdev, ::Error::Simple>
	Device<CVmSerialPort>::getLibvirtXml(const CVmSerialPort& model_)
{
	Libvirt::Domain::Xml::Qemucdev output;
	Libvirt::Domain::Xml::Source15 a;

	QString p(model_.getUserFriendlyName());
	if (p.isEmpty())
		return ::Error::Simple(PRL_ERR_INVALID_ARG);

	QList<Libvirt::Domain::Xml::Source15> l;
	Libvirt::Domain::Xml::QemucdevSrcDef b;
	switch (model_.getEmulatedType())
	{
	case PVE::SerialOutputFile:
		output.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceFile);
		a.setPath(p);
		a.setAppend(Libvirt::Domain::Xml::EVirOnOffOn);
		l << a;
		break;
	case PVE::RealSerialPort:
		output.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceDev);
		a.setPath(p);
		a.setAppend(Libvirt::Domain::Xml::EVirOnOffOn);
		l << a;
		break;
	case PVE::SerialSocket:
		output.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceUnix);
		a.setPath(p);
		a.setAppend(Libvirt::Domain::Xml::EVirOnOffOn);
		a.setMode(getMode(model_.getSocketMode()));
		l << a;
		break;
	case PVE::SerialTCP:
		output.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceTcp);
		a.setMode(getMode(model_.getSocketMode()));
		{
			QUrl u(QString("tcp://%1").arg(p));
			a.setHost(u.host());
			a.setService(QString::number(u.port()));
			Libvirt::Domain::Xml::Protocol r;
			r.setType(Libvirt::Domain::Xml::EType13Raw);
			b.setProtocol(r);
		}
		l << a;
		break;
	case PVE::SerialUDP:
		output.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceUdp);
		{
			QUrl u(QString("udp://%1").arg(p));
			a.setHost(u.host());
			a.setService(QString::number(u.port()));
			Libvirt::Domain::Xml::Protocol r;
			r.setType(Libvirt::Domain::Xml::EType13Raw);
			b.setProtocol(r);
		}
		// need sources in both modes
		a.setMode(getMode(PSP_SERIAL_SOCKET_SERVER));
		l << a;
		a.setMode(getMode(PSP_SERIAL_SOCKET_CLIENT));
		l << a;
		break;
	default:
		return ::Error::Simple(PRL_ERR_UNIMPLEMENTED);
	}

	b.setSourceList(l);
	output.setQemucdevSrcDef(b);

	return output;
}

Prl::Expected<QString, ::Error::Simple>
	Device<CVmGenericNetworkAdapter>::getPlugXml(const CVmGenericNetworkAdapter& model_)
{
	Prl::Expected<Libvirt::Domain::Xml::VInterface, ::Error::Simple> a =
		Transponster::Device::Network::build(model_);
	if (a.isFailed())
		return a.error();

	mpl::at_c<Extract<Libvirt::Domain::Xml::VChoice941Impl>::type, 4>::type e;
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
	Libvirt::Domain::Xml::Os2 os;
	if (getStartupOptions(os))
		vos.setValue(os);

	m_result->setOs(vos);
	return PRL_ERR_SUCCESS;
}

bool Builder::getStartupOptions(Libvirt::Domain::Xml::Os2& os_) const
{
	bool changed = false;
	CVmStartupOptions* o = m_input.getVmSettings()->getVmStartupOptions();
	if (o->isAllowSelectBootDevice())
	{
		Libvirt::Domain::Xml::Bootmenu menu;
		menu.setEnable(Libvirt::Domain::Xml::EVirYesNoYes);
		os_.setBootmenu(menu);
		changed = true;
	}
	CVmStartupBios* b = m_input.getVmSettings()->getVmStartupOptions()->getBios();
	//EFI boot support
	if (b != NULL && b->isEfiEnabled())
	{
		Libvirt::Domain::Xml::Loader l;
		l.setReadonly(Libvirt::Domain::Xml::EReadonlyYes);
		l.setType(Libvirt::Domain::Xml::EType2Pflash);

		// package OVMF.x86_64
		l.setOwnValue(QString("/usr/share/OVMF/OVMF_CODE.fd"));

		os_.setLoader(l);

		QString x = b->getNVRAM();
		if (!x.isEmpty())
		{
			Libvirt::Domain::Xml::Nvram n;
			n.setOwnValue(x);
			n.setFormat(Libvirt::Domain::Xml::EFormatQcow2);
			os_.setNvram(n);
		}
		changed = true;
	}
	return changed;
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
	Transponster::Device::Boot::List t(*s, b);
	foreach (const CVmHardDisk* d, h->m_lstHardDisks)
	{
		if (d->getConnected() != PVE::DeviceConnected)
			continue;
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
		CVmGenericNetworkAdapter copy = PrlNet::fixMacFilter(*d, h->m_lstNetworkAdapters);
		t.add(&copy);
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

	b.addGuestChannel("org.qemu.guest_agent.0");
	b.addGuestChannel("org.qemu.guest_agent.1");

	Libvirt::Domain::Xml::Devices x;
	if (m_result->getDevices())
		x = m_result->getDevices().get();

	x.setEmulator(b.getEmulator());
	x.setChoice941List(Transponster::Device::deviceList_type()
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

	Libvirt::Domain::Xml::Cpu c;
	if (u.getCpu(vt_, c))
		m_result->setCpu(c);

	Libvirt::Domain::Xml::Clock t;
	if (u.getClock(t))
		m_result->setClock(t);

	u.getCpu(vt_, *m_result);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Builder::setIdentification()
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

	Libvirt::Domain::Xml::Memballoon m;
	m.setStats(5);
	m.setAutodeflate(Libvirt::Domain::Xml::EVirOnOffOn);
	x.setMemballoon(m);

	m_result->setDevices(x);
	return PRL_ERR_SUCCESS;
}

void Vm::setCommandline()
{
	Libvirt::Domain::Xml::Commandline q;
	q.setArgList(QList<QString>() << "-d" << "guest_errors,unimp");
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
		hv.setVpindex(Libvirt::Domain::Xml::EVirOnOffOn);
		hv.setStimer(Libvirt::Domain::Xml::EVirOnOffOn);
		hv.setReset(Libvirt::Domain::Xml::EVirOnOffOn);
		hv.setRuntime(Libvirt::Domain::Xml::EVirOnOffOn);
		hv.setSynic(Libvirt::Domain::Xml::EVirOnOffOn);
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

PRL_RESULT Mixer::setResources(const VtInfo& info_)
{
	// we don't need to rewrite clock
	boost::optional<Libvirt::Domain::Xml::Clock> t = m_result->getClock();
	PRL_RESULT r = Builder::setResources(info_);
	m_result->setClock(t);
	return r;
}

///////////////////////////////////////////////////////////////////////////////
// struct Fixer

Fixer::Fixer(const CVmConfiguration& input_, char* xml_): Builder(input_)
{
	shape(xml_, m_result);
}

PRL_RESULT Fixer::setBlank()
{
	if (m_result.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	CVmStartupBios* b = m_input.getVmSettings()->getVmStartupOptions()->getBios();
	if (b == NULL || b->getNVRAM().isEmpty())
		return PRL_ERR_SUCCESS;

	m_result->setOs(boost::apply_visitor
		(Visitor::Fixup::Os(b->getNVRAM()), m_result->getOs()));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Fixer::setIdentification()
{
	if (m_result.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	PRL_RESULT r;
	QString n = m_result->getIds().getName();
	if (PRL_SUCCEEDED(r = Builder::setIdentification()))
	{
		Libvirt::Domain::Xml::Ids x = m_result->getIds();
		x.setName(n);
		m_result->setIds(x);
	}
	return r;
}

PRL_RESULT Fixer::setDevices()
{
	if (m_result.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	const Libvirt::Domain::Xml::Devices* d = m_result->getDevices().get_ptr();
	if (NULL == d)
		return PRL_ERR_UNINITIALIZED;

	Transponster::Device::List a;
	foreach(const CVmSerialPort* s, m_input.getVmHardwareList()->m_lstSerialPorts)
		a.add(s);

	QList<Libvirt::Domain::Xml::VChoice941> l = a.getDeviceList();

	Visitor::Fixup::Device v(m_input.getVmHardwareList(), l);
	foreach (const Libvirt::Domain::Xml::VChoice941& e, d->getChoice941List())
	{
		boost::apply_visitor(v, e);
	}

	Libvirt::Domain::Xml::Devices devices = *d;
	devices.setChoice941List(l);
	m_result->setDevices(devices);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Fixer::setResources(const VtInfo&)
{
	m_result->setCurrentMemory(boost::none);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Pipeline

Pipeline::Pipeline(char* xml_)
{
	shape(xml_, m_result);
}

PRL_RESULT Pipeline::operator()(boost::function1<PRL_RESULT, Libvirt::Domain::Xml::Domain&> action_)
{
	if (m_result.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	return action_(*m_result);
}

QString Pipeline::getResult()
{
	if (m_result.isNull())
		return QString();

	QDomDocument x;
	m_result->save(x);
	m_result.reset();
	return x.toString();
}

///////////////////////////////////////////////////////////////////////////////
// struct Clock

PRL_RESULT Clock::operator()(Libvirt::Domain::Xml::Domain& dst_)
{
	boost::optional<Libvirt::Domain::Xml::Clock> t = dst_.getClock();
	if (!t)
		return PRL_ERR_READ_XML_CONTENT;

	Libvirt::Domain::Xml::Clock c = t.get();
	boost::apply_visitor(Visitor::Adjustment(c, m_offset), t->getClock());

	dst_.setClock(c);
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
	mpl::at_c<Libvirt::Network::Xml::VChoice1186::types, 1>::type m;
	m.setValue(p);
	output.setChoice1186(Libvirt::Network::Xml::VChoice1186(m));

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
	mpl::at_c<Libvirt::Iface::Xml::VChoice1250::types, 0>::type v;
	v.setValue(e);
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setChoice1250List(QList<Libvirt::Iface::Xml::VChoice1250>() << v);
	m_result.setBridge(b);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setBridge()
{
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setDelay(2.0);
	b.setStp(Libvirt::Iface::Xml::EVirOnOffOff);
	m_result.setBridge(b);
	Libvirt::Iface::Xml::InterfaceAddressing1280 h;
	if (!m_master.isConfigureWithDhcp())
	{
		if (!m_master.isConfigureWithDhcpIPv6())
			return PRL_ERR_SUCCESS;

		Libvirt::Iface::Xml::Protocol p;
		p.setDhcp(Libvirt::Iface::Xml::Dhcp());
		h.setProtocol2(p);
	}
	mpl::at_c<Libvirt::Iface::Xml::VChoice1286::types, 0>::type a;
	a.setValue(Libvirt::Iface::Xml::Dhcp());
	h.setProtocol(Libvirt::Iface::Xml::VChoice1286(a));
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

namespace Device
{
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
} // namespace Device

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
		b.setValue(Libvirt::Snapshot::Xml::Disk1744());
		Libvirt::Snapshot::Xml::Disk x;
		x.setName(Libvirt::Snapshot::Xml::VName(a));
		x.setDisk(Libvirt::Snapshot::Xml::VDisk(b));
		output << x;
	}
	return output;
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// struct Internal

boost::optional<Libvirt::Snapshot::Xml::Disk> Internal::operator()(const CVmHardDisk& disk_) const
{
	Libvirt::Snapshot::Xml::Disk x;
	mpl::at_c<Libvirt::Snapshot::Xml::VName::types, 0>::type a;
	a.setValue(Device::Clustered::Model<CVmHardDisk>(disk_).getTargetName());
	mpl::at_c<Libvirt::Snapshot::Xml::VDisk::types, 1>::type b;
	b.setValue(Libvirt::Snapshot::Xml::Disk1745());
	x.setName(Libvirt::Snapshot::Xml::VName(a));
	x.setDisk(Libvirt::Snapshot::Xml::VDisk(b));
	return x;
}

///////////////////////////////////////////////////////////////////////////////
// struct External

boost::optional<Libvirt::Snapshot::Xml::Disk> External::operator()(const CVmHardDisk& disk_) const
{
	if (!m_disks.contains(disk_.getSerialNumber()))
		return boost::none;

	Libvirt::Snapshot::Xml::Disk x;
	mpl::at_c<Libvirt::Snapshot::Xml::VName::types, 0>::type a;
	Libvirt::Snapshot::Xml::Source s;
	Libvirt::Snapshot::Xml::Variant1739 o;
	mpl::at_c<Libvirt::Snapshot::Xml::VChoice1742::types, 0>::type p;
	mpl::at_c<Libvirt::Snapshot::Xml::VDisk::types, 2>::type q;

	a.setValue(Device::Clustered::Model<CVmHardDisk>(disk_).getTargetName());
	s.setFile(disk_.getSystemName() + "." + m_snapshot);
	o.setSource(s);
	p.setValue(o);
	q.setValue(Libvirt::Snapshot::Xml::VChoice1742(p));

	x.setName(Libvirt::Snapshot::Xml::VName(a));
	x.setDisk(Libvirt::Snapshot::Xml::VDisk(q));
	return x;
}

///////////////////////////////////////////////////////////////////////////////
// struct Reverse

Reverse::Reverse(const QString& uuid_, const QString& description_,
	const CVmConfiguration& input_): m_uuid(uuid_),
	m_description(description_), m_hardware(input_.getVmHardwareList()),
	m_policy(boost::bind(Internal(), _1))
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

		boost::optional<Libvirt::Snapshot::Xml::Disk> r = m_policy(*d);

		if (r)
			e << r.get();
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

