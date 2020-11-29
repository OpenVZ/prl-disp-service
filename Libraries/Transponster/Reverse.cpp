/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2020 Virtuozzo International GmbH. All rights reserved.
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

#include <QUrl>
#include "Direct.h"
#include "Reverse.h"
#include "Reverse_p.h"
#include "Direct_p.h"
#include <prlsdk/PrlOses.h>
#include <boost/mpl/for_each.hpp>
#include <Libraries/HostInfo/CHostInfo.h>
#include <prlcommon/HostUtils/HostUtils.h>
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
	CVmCpu* u = h->getCpu();
	if (NULL == u)
		return;

	boost::optional<Libvirt::Domain::Xml::Topology> f = src_.getTopology();
	if (f)
	{
		u->setNumber(f.get().getCores());
		u->setSockets(f.get().getSockets());
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

	dst_.setMode(Libvirt::Domain::Xml::EModeHostModel);

	Vm::Reverse::CpuFeaturesMask mask(*m_config);
	mask.getFeatures(vt_, dst_);

	if (!u->isEnableHotplug())
	{
		Libvirt::Domain::Xml::Topology t;
		t.setCores(u->getNumber());
		t.setThreads(1);
		t.setSockets(u->getSockets());
		dst_.setTopology(t);
	}
	CVmMemory *m = h->getMemory();
	if (m->isEnableHotplug())
	{
		QList<Libvirt::Domain::Xml::Cell > cells;
		Libvirt::Domain::Xml::Cell cell;
		boost::optional<unsigned int> id(0);
		cell.setId(id);
		int x = u->getSockets() * u->getNumber();
		cell.setCpus(2 > x ? "0" : QString("0-%1").arg(x - 1));
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

	Libvirt::Domain::Xml::Vmcoreinfo i;
	i.setState(Libvirt::Domain::Xml::EVirOnOffOn);
	f->setVmcoreinfo(i);

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

	QList<Libvirt::Domain::Xml::Timer> timers;
	Visitor::Timer v(*o, timers);
	boost::mpl::for_each<Libvirt::Domain::Xml::VTimer::types>(boost::ref(v));

	dst_.setTimerList(timers);

	Libvirt::Domain::Xml::Clock395 k;
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
	m->setEnableHotplug(true);
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

void Resources::setMemGuarantee(const Libvirt::Domain::Xml::Memtune& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	if (!src_.getMinGuarantee())
		return;

	CVmMemory* m = new CVmMemory(h->getMemory());

	const mpl::at_c<Libvirt::Domain::Xml::VMinGuarantee::types, 0>::type* v =
		boost::get<mpl::at_c<Libvirt::Domain::Xml::VMinGuarantee::types, 0>::type>
		(&src_.getMinGuarantee().get()); 

	if (v && v->getValue() == Libvirt::Domain::Xml::EVirYesNoYes)
		m->setMemGuaranteeType(PRL_MEMGUARANTEE_AUTO);
	else
	{
		const mpl::at_c<Libvirt::Domain::Xml::VMinGuarantee::types, 1>::type* s =
			boost::get<mpl::at_c<Libvirt::Domain::Xml::VMinGuarantee::types, 1>::type>
			(&src_.getMinGuarantee().get()); 

		if (s) 
		{
			m->setMemGuaranteeType(PRL_MEMGUARANTEE_PERCENTS);
			unsigned long r = h->getMemory()->getRamSize() << 10;
			if (r)
				m->setMemGuarantee(s->getValue().getOwnValue() * 100 / r);
			else
				m->setMemGuarantee(0);
		}
	}
	h->setMemory(m);
}

bool Resources::getMemGuarantee(Libvirt::Domain::Xml::Memtune& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	CVmMemory* m = h->getMemory();
	if (NULL == m)
		return false;

	if (m->getMemGuaranteeType() == PRL_MEMGUARANTEE_AUTO)
	{
		mpl::at_c<Libvirt::Domain::Xml::VMinGuarantee::types, 0>::type v;
		v.setValue(Libvirt::Domain::Xml::EVirYesNoYes);
		dst_.setMinGuarantee(Libvirt::Domain::Xml::VMinGuarantee(v));
	}
	else if (m->getMemGuaranteeType() == PRL_MEMGUARANTEE_PERCENTS)
	{
		mpl::at_c<Libvirt::Domain::Xml::VMinGuarantee::types, 1>::type v;
		Libvirt::Domain::Xml::ScaledInteger g;
		g.setOwnValue((m->getRamSize() << 10) * m->getMemGuarantee() / 100);
		v.setValue(g);
		dst_.setMinGuarantee(Libvirt::Domain::Xml::VMinGuarantee(v));
	}

	return true;
}

void Resources::setChipset(const Libvirt::Domain::Xml::VOs& src_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return;

	*(h->getChipset()) = Chipset::Marshal()
		.deserialize(boost::apply_visitor(Visitor::Chipset(), src_));
}

bool Resources::getChipset(Libvirt::Domain::Xml::Os2& dst_)
{
	CVmHardware* h = getHardware();
	if (NULL == h)
		return false;

	::Chipset* s = h->getChipset();
	if (NULL == s)
		return false;

	Libvirt::Domain::Xml::Hvmx86 m;
	m.setMachine(Chipset::Marshal().serialize(*s));
	mpl::at_c<Libvirt::Domain::Xml::VChoice305::types, 0>::type t;
	t.setValue(m);
	dst_.setType(Libvirt::Domain::Xml::VChoice305(t));

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct CommandLine

CommandLine& CommandLine::seed(const boost::optional<Libvirt::Domain::Xml::Commandline>& original_)
{
	if (original_)
		m_result = original_->getArgList();
	else
		m_result.clear();

	return *this;
}

CommandLine& CommandLine::addDebug()
{
	if (DBG_DEBUG > __log_level)
		return *this;

	// Due #PSBM-75384 - now unconditionally
	if (NULL == m_source->getVmIdentification())
		return *this;

	if (!m_result.contains("-global"))
		m_result << "-global" << "isa-debugcon.iobase=0x402";

	if (!m_result.contains("-debugcon"))
	{
		m_result << "-debugcon" << QString("file:/var/log/libvirt/qemu/%1.qdbg.log")
			.arg(m_source->getVmIdentification()->getVmName());
	}

	return *this;
}

CommandLine& CommandLine::addLogging()
{
	if (!m_result.contains("-d"))
		m_result << "-d" << "guest_errors,unimp";

	return *this;
}

void CommandLine::stripParameter(int at_)
{
	if (-1 < at_ && at_ < m_result.size())
	{
		m_result.removeAt(at_+1);
		m_result.removeAt(at_);
	}
}

CommandLine& CommandLine::stripDebugcon()
{
	// remove "-debugcon <vm-home-path>/qdbg.log" from commandline
	stripParameter(m_result.indexOf("-debugcon"));
	return *this;
}

CommandLine& CommandLine::workaroundEfi2008R2()
{
	CVmSettings* s = m_source->getVmSettings();
	if (NULL == s)
		return *this;

	CVmStartupOptions* o = s->getVmStartupOptions();
	if (NULL == o)
		return *this;

	CVmStartupBios* b = o->getBios();
	if (NULL == b)
		return *this;

	quint32 v = s->getVmCommonOptions()->getOsVersion();
	int i = m_result.indexOf("-fw_cfg");
	//EFI boot support
	if (IS_WINDOWS(v) && (v == PVS_GUEST_VER_WIN_WINDOWS7 || v == PVS_GUEST_VER_WIN_2008)
		&& b->isEfiEnabled())
	{
		if (-1 == i)
		{
			m_result << "-fw_cfg"
				<< "opt/ovmf/vbe_shim/win2008_workaround,string=1";
		}
		return *this;
	}
	stripParameter(i);

	return *this;
}

Libvirt::Domain::Xml::Commandline CommandLine::takeResult()
{
	Libvirt::Domain::Xml::Commandline output;
	output.setArgList(m_result);
	m_result.clear();

	return output;
}

namespace Device
{
///////////////////////////////////////////////////////////////////////////////
// struct Alias

const QString Alias::s_PREFIX("ua-vz-");

bool Alias::feature(const QString& value_) const
{
	return value_.startsWith(s_PREFIX);
}

QString Alias::operator()(const CHwUsbDevice& model_) const
{
	return QString(s_PREFIX).append("usb-")
		.append(USB_SYS_PATH(model_.getDeviceId()).replace('.', '_'));
}

QString Alias::operator()(const CVmGenericPciDevice model_) const
{
	QStringList u(model_.getSystemName().split(":"));
	return QString(s_PREFIX).append("pci-")
		.append(u[0]).append('-').append(u[1]).append('-').append(u[2]);
}

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
	return QString("fd") + Virtuozzo::toBase26(m_dataSource->getIndex());
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
		mpl::at_c<Libvirt::Domain::Xml::VChoice4771::types, 0>::type y;
		y.setValue(p);
		t.setGroupName(QString("virtuozzo"));
		t.setChoice4771(Libvirt::Domain::Xml::VChoice4771(y));
	}
	if (t.getGroupName())
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
		mpl::at_c<Libvirt::Domain::Xml::VChoice4774::types, 0>::type y;
		y.setValue(p);
		t.setGroupName(QString("virtuozzo"));
		t.setChoice4774(Libvirt::Domain::Xml::VChoice4774(y));
	}
	if (t.getGroupName())
		getResult().setIotune(t);
}

void Hdd::setSerial(const QString& serial_)
{
	getResult().setSerial(serial_);
}

void Hdd::setDisk()
{
	Ordinary<CVmHardDisk>::setDisk();
	if (PVE::PassthroughEnabled != m_hdd.getPassthrough())
		return;
	mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 1>::type x;
	Libvirt::Domain::Xml::Disk479 d;
	d.setDevice(Libvirt::Domain::Xml::EDevice1Lun);
	x.setValue(d);
	getResult().setDisk(Libvirt::Domain::Xml::VDisk(x));
}

void Hdd::setBackingChain()
{
	if (Flavor<CVmHardDisk>::image != m_hdd.getEmulatedType())
		return Ordinary<CVmHardDisk>::setBackingChain();

	mpl::at_c<Libvirt::Domain::Xml::VDiskBackingChain::types, 1>::type e;
	e.setValue(true);
	Libvirt::Domain::Xml::VDiskBackingChain a = e;
	foreach (CVmHddPartition* x, m_hdd.m_lstPartition)
	{
		if (x->getSystemName().isEmpty())
			continue;

		Libvirt::Domain::Xml::BackingStore y;
		y.setFormat(Libvirt::Domain::Xml::VStorageFormat
			(Flavor<CVmHardDisk>::getDriverFormat(false)));

		Libvirt::Domain::Xml::Source4 z;
		z.setFile(x->getSystemName());
		z.setStartupPolicy(Libvirt::Domain::Xml::EStartupPolicyOptional);
		mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 4>::type s;
		s.setValue(z);
		y.setDiskSource(s);

		Libvirt::Domain::Xml::VDiskBackingChainBin b;
		b.value = a;
		y.setDiskBackingChain(b);
		mpl::at_c<Libvirt::Domain::Xml::VDiskBackingChain::types, 0>::type c;
		c.setValue(y);

		a = c;
	}
	getResult().setDiskBackingChain(a);
}

void Hdd::setDriver()
{
	Ordinary<CVmHardDisk>::setDriver();
	if (Flavor<CVmHardDisk>::image == getModel().getEmulatedType() ||
		m_hdd.getStorageURL().isEmpty())
		return;

	Libvirt::Domain::Xml::Driver d;
	if (getResult().getDriver())
		d = getResult().getDriver().get();
	
	mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 0>::type f;
	f.setValue(Libvirt::Domain::Xml::EStorageFormatRaw);
	mpl::at_c<Libvirt::Domain::Xml::VType::types, 1>::type a;
	a.setValue(Libvirt::Domain::Xml::VStorageFormat(f));
	Libvirt::Domain::Xml::DriverFormat b;
	b.setName("qemu");
	b.setType(Libvirt::Domain::Xml::VType(a));
	d.setDriverFormat(b);

	getResult().setDriver(d);
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
	if (NULL != runtime_ && hdd_->getPassthrough() != PVE::PassthroughEnabled)
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
	switch (m_network.getAdapterType())
	{
		case PNT_RTL:
			return QString("rtl8139");
		case PNT_E1000:
			return QString("e1000");
		case PNT_HYPERV:
			return QString("hv-net");
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

QStringList View::getIpv6() const
{
	return NetworkUtils::ParseIps(m_network.getNetAddresses()).second;
}

QString View::getMac() const
{
	return normalizeMac(getRawMac());
}

QString View::getRawMac() const
{
	return m_network.getMacAddress();
}

QStringList View::getMacList() const
{
	QStringList r;
	foreach(const CNetPktFilter *f, m_network.m_lstPktFilter)
	{
		QString m = normalizeMac(f->getMacAddress());
		if (!m.isEmpty())
			r << m;
	}
	if (r.isEmpty())
		r << getMac();
	return r;
}

QString View::getHostMac() const
{
	return normalizeMac(m_network.getHostMacAddress());
}

QString View::getFilterName() const
{
	return NetFilter(*(m_network.getPktFilter())).getFilterRef();
}

Libvirt::Domain::Xml::FilterrefNodeAttributes View::prepareFilterref(const NetFilter& filter) const
{
	Libvirt::Domain::Xml::FilterrefNodeAttributes filter_xml;
	QList<Libvirt::Domain::Xml::Parameter> params;
	typedef QPair<QString, QString> ParamPair_t;

	foreach (ParamPair_t param, filter.getParams())
	{
		Libvirt::Domain::Xml::Parameter p;
		p.setName(param.first);
		p.setValue(param.second);
		params.append(p);
	}

	filter_xml.setFilter(filter.getFilterRef());
	filter_xml.setParameterList(params);

	return filter_xml;
}

boost::optional<Libvirt::Domain::Xml::FilterrefNodeAttributes> View::getFilterref() const
{
	NetFilter filter = NetFilter(*(m_network.getPktFilter()));

	if (filter.isCustomFilter())
		return prepareFilterref(filter);
	
	if (m_network.getFirewall()->isEnabled())
	{
		filter.setVzFilter(getRawMac());
		return prepareFilterref(filter);
	} else
		return getPredefinedFilterref();
}

boost::optional<Libvirt::Domain::Xml::FilterrefNodeAttributes> View::getPredefinedFilterref() const
{
	static QString S_IPV4_PARAMETER_NAME = "IP";
	static QString S_MAC_PARAMETER_NAME = "MAC";

	NetFilter filter = NetFilter(*(m_network.getPktFilter()));

	if (!filter.isCustomFilter())
	{
		// Disabling Prevention of IpSpoofing for dynamic IPs
		if (m_network.isConfigureWithDhcp() || getIpv4().isEmpty())
			filter.setPreventIpSpoof(false);

		QList<NetFilter::ParamPair_t> params;
		foreach(const QString& ip, getIpv4())
			params.append(qMakePair(S_IPV4_PARAMETER_NAME, ip.split('/').first()));

		foreach(const QString &mac, getMacList())
		{
			params.append(qMakePair(S_MAC_PARAMETER_NAME, mac));
		}
		filter.setParams(params);
	}

	if (filter.getFilterRef().isEmpty())
		return boost::none;

	return prepareFilterref(filter);
}

boost::optional<Libvirt::Domain::Xml::Bandwidth > View::getBandwidth() const
{
	if (!m_network.getBandwidth())
		return boost::none;

	const CVmNetBandwidthInbound *i = m_network.getBandwidth()->getInbound();
	const CVmNetBandwidthOutbound *o = m_network.getBandwidth()->getOutbound();

	if (!i && !o)
		return boost::none;

	Libvirt::Domain::Xml::Bandwidth b;
	if (i)
	{
		Libvirt::Domain::Xml::BandwidthAttributes a;

		a.setAverage(i->getAverage());
		if (i->getBurst())
			a.setBurst(i->getBurst());
		if (i->getPeak())
			a.setPeak(i->getPeak());
		if (i->getFloor())
			a.setFloor(i->getFloor());
		b.setInbound(a);
	}

	if (o)
	{
		Libvirt::Domain::Xml::BandwidthAttributes a;

		a.setAverage(o->getAverage());
		if (i->getBurst())
			a.setBurst(o->getBurst());
		if (i->getPeak())
			a.setPeak(o->getPeak());
		b.setOutbound(a);
	}

	return b;
}

boost::optional<Libvirt::Domain::Xml::VVirtualPortProfile> View::getVVirtualPortProfile() const
{
	CVmNetVirtualPortType *v = m_network.getVirtualPort();

	if (!v)
		return boost::none;

	if (v->getType() != "openvswitch")
		return boost::none;

	mpl::at_c<Libvirt::Domain::Xml::VVirtualPortProfile::types, 2>::type vv;

	Libvirt::Domain::Xml::Parameters2 p;
	if (!v->getInterfaceId().isEmpty())
	{
		mpl::at_c<Libvirt::Domain::Xml::VUUID::types, 1>::type u;
		u.setValue(::Uuid(v->getInterfaceId()).toStringWithoutBrackets());
		p.setInterfaceid(Libvirt::Domain::Xml::VUUID(u));
	}
	if (!v->getProfileId().isEmpty())
		p.setProfileid(v->getProfileId());
	boost::optional<Libvirt::Domain::Xml::Parameters2> pp = p;
	vv.setValue(pp);

	return Libvirt::Domain::Xml::VVirtualPortProfile(vv);
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

	QString a = network_.getSystemName();
	if (!a.isEmpty())
		i.setAlias(a);

	View view(network_);
	QString m = view.getMac();
	if (!m.isEmpty())
		i.setMac(m);

	i.setFilterref(view.getFilterref());
	i.setBandwidth(view.getBandwidth());
	i.setBoot(boot_);

	access_type output;
	output.setValue(i);
	return output;
}

template<>
Libvirt::Domain::Xml::Interface658 Adapter<0>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface658 output;
	output.setIpList(Ips()(network_.getNetAddresses()));

	View v(network_);
	output.setModel(v.getAdapterType());
	output.setVirtualPortProfile(v.getVVirtualPortProfile());

	Libvirt::Domain::Xml::InterfaceBridgeAttributes a;

	if (network_.getVirtualNetworkID().isEmpty())
		a.setBridge(QString("host-routed"));
	else
		a.setBridge(network_.getVirtualNetworkID());

	Libvirt::Domain::Xml::Source6 s;
	s.setInterfaceBridgeAttributes(a);
	output.setSource(s);
	output.setTarget(network_.getHostInterfaceName());
	return output;
}

template<>
Libvirt::Domain::Xml::Interface660 Adapter<1>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface660 output;
	output.setScript(QString("/bin/true"));
	output.setIpList(Ips()(network_.getNetAddresses()));
	output.setModel(View(network_).getAdapterType());
	output.setTarget(network_.getHostInterfaceName());
	return output;
}

template<>
Libvirt::Domain::Xml::Interface665 Adapter<3>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface665 output;
	Libvirt::Domain::Xml::InterfaceNetworkAttributes a;
	a.setNetwork(network_.getVirtualNetworkID());
	Libvirt::Domain::Xml::Source8 s;
	s.setInterfaceNetworkAttributes(a);
	output.setIpList(Ips()(network_.getNetAddresses()));
	output.setTarget(network_.getHostInterfaceName());
	output.setModel(View(network_).getAdapterType());
	output.setSource(s);
	return output;
}

template<>
Libvirt::Domain::Xml::Interface667 Adapter<4>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface667 output;
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
		return Adapter<1>()(network_, boot_);
	default:
		return ::Error::Simple(PRL_ERR_UNIMPLEMENTED);
	}
}

} // namespace Network

namespace Controller
{
///////////////////////////////////////////////////////////////////////////////
// struct Factory

Libvirt::Domain::Xml::Controller Factory::craft
	(const Libvirt::Domain::Xml::VChoice625& bus_, quint16 index_)
{
	Libvirt::Domain::Xml::Controller output;
	output.setIndex(index_);
	output.setChoice625(bus_);

	return output;
}

Factory::result_type Factory::wrap(const Libvirt::Domain::Xml::Controller& object_)
{
	mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 1>::type output;
	output.setValue(object_);
	return Libvirt::Domain::Xml::VChoice985(output);
}

///////////////////////////////////////////////////////////////////////////////
// class Moldy

Moldy::result_type Moldy::operator()(Libvirt::Domain::Xml::EType6 bus_, quint16 index_)
{
	mpl::at_c<Libvirt::Domain::Xml::VChoice625::types, 0>::type b;
	b.setValue(bus_);
	return wrap(craft(b, index_));
}

///////////////////////////////////////////////////////////////////////////////
// class Virtio

Virtio::result_type Virtio::operator()(quint16 index_)
{
	Libvirt::Domain::Xml::Driver1 d;
//	d.setIothread(1);
	mpl::at_c<Libvirt::Domain::Xml::VChoice625::types, 1>::type b;
	b.setValue(Libvirt::Domain::Xml::EModelVirtioScsi);
	Libvirt::Domain::Xml::Controller x = craft(b, index_);
	x.setDriver(d);

	return wrap(x);
}

///////////////////////////////////////////////////////////////////////////////
// class Hyperv

Hyperv::result_type Hyperv::operator()(quint16 index_)
{
	mpl::at_c<Libvirt::Domain::Xml::VChoice625::types, 1>::type b;
	b.setValue(Libvirt::Domain::Xml::EModelHvScsi);
	return wrap(craft(b, index_));
}

namespace Scsi
{
///////////////////////////////////////////////////////////////////////////////
// struct Arrangement

Libvirt::Domain::Xml::VAddress Arrangement::operator()(deviceList_type& controllers_)
{
	quint16 t = m_device % MAX_TARGETS;
	if (0 == t)
	{
		m_controller = controllers_.size();
		controllers_ << m_factory(m_controller);
	}
	m_device++;
	return Address().setTarget(t)(m_controller);
}

///////////////////////////////////////////////////////////////////////////////
// struct Bus

Bus::Bus(): m_hyperv(Arrangement(Hyperv())), m_virtio(Arrangement(Virtio()))
{
}

Libvirt::Domain::Xml::VAddress Bus::operator()(Libvirt::Domain::Xml::EModel model_)
{
	switch (model_)
	{
	case Libvirt::Domain::Xml::EModelHvScsi:
		return m_hyperv(m_controllers);
	default:
		return m_virtio(m_controllers);
	}
}

} // namespace Scsi
} // namespace Controller

///////////////////////////////////////////////////////////////////////////////
// struct Attachment

Libvirt::Domain::Xml::VAddress Attachment::craftIde(quint32 index_)
{
	quint16 c = index_ / IDE_UNITS / IDE_BUSES;
	quint16 b = index_ / IDE_UNITS % IDE_BUSES;
	quint16 u = index_ % IDE_UNITS;

	if (c > 0 && u == 0 && b == 0)
		m_controllers << m_moldy(Libvirt::Domain::Xml::EType6Ide, c);

	return Address().setUnit(u).setBus(b)(c);
}

Libvirt::Domain::Xml::VAddress Attachment::craftSata(quint32 index_)
{
	quint16 c = index_ / SATA_UNITS;
	quint16 u = index_ % SATA_UNITS;

	if (c > 0 && u == 0)
		m_controllers << m_moldy(Libvirt::Domain::Xml::EType6Sata, c);

	return Address().setUnit(u)(c);
}

Libvirt::Domain::Xml::VAddress Attachment::craftScsi
	(const boost::optional<Libvirt::Domain::Xml::EModel>& model_)
{
	Libvirt::Domain::Xml::EModel m = Libvirt::Domain::Xml::EModelAuto;
	if (model_)
		m = model_.get();

	return m_scsi(m);
}

deviceList_type Attachment::getControllers() const
{
	return m_controllers + m_scsi.getControllers();
}

///////////////////////////////////////////////////////////////////////////////
// struct List

void List::addGuestChannel(const QString &path_)
{
	Libvirt::Domain::Xml::Channel1 c;
	c.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceUnix);
	mpl::at_c<Libvirt::Domain::Xml::VChoice894::types, 1>::type x;
	x.setValue(path_);
	c.setChoice894(x);
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

	Libvirt::Domain::Xml::Variant725 v;
	v.setPort(vnc_->getPortNumber());
	// Websocket port auto-allocation must be explicit.
	v.setWebsocket(-1);
	v.setListen(QHostAddress(QHostAddress::LocalHostIPv6).toString());
	if (PRD_AUTO == vnc_->getMode())
		v.setAutoport(Libvirt::Domain::Xml::EVirYesNoYes);

	mpl::at_c<Libvirt::Domain::Xml::VChoice727::types, 0>::type y;
	y.setValue(v);
	Libvirt::Domain::Xml::Graphics734 g;
	g.setChoice727(Libvirt::Domain::Xml::VChoice727(y));
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

	mpl::at_c<Libvirt::Domain::Xml::VModel::types, 1>::type M;
	m.setModel(Libvirt::Domain::Xml::VModel(M));

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

void List::add(const CVmGenericPciDevice* pci_)
{
	if (PVE::DeviceDisabled == pci_->getEnabled())
		return;

	QStringList u(pci_->getSystemName().split(":"));
	if (3 > u.size())
		return;

	Libvirt::Domain::Xml::Pciaddress a;
	a.setBus(QString("0x").append(u[0]));
	a.setSlot(QString("0x").append(u[1]));
	a.setFunction(QString("0x").append(u[2]));

	Libvirt::Domain::Xml::Source13 c;
	c.setAddress(a);

	Libvirt::Domain::Xml::Hostdevsubsyspci d;
	d.setSource(c);

	mpl::at_c<Libvirt::Domain::Xml::VHostdevsubsys::types, 0>::type f;
	f.setValue(d);

	Libvirt::Domain::Xml::Hostdevsubsys g;
	g.setHostdevsubsys(f);
	g.setManaged(Libvirt::Domain::Xml::EVirYesNoYes);

	mpl::at_c<Libvirt::Domain::Xml::VChoice917::types, 0>::type h;
	h.setValue(g);

	Libvirt::Domain::Xml::Hostdev i;
	i.setChoice917(h);
	i.setAlias(Alias()(*pci_));

	add<7>(i);
}

namespace Usb
{
///////////////////////////////////////////////////////////////////////////////
// struct List

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
	Libvirt::Domain::Xml::Variant594 v;
	v.setModel(model_);
	mpl::at_c<Libvirt::Domain::Xml::VChoice625::types, 2>::type x;
	x.setValue(v);
	Libvirt::Domain::Xml::Controller y;
	y.setIndex(m_controller++);
	y.setChoice625(x);
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

///////////////////////////////////////////////////////////////////////////////
// struct Indicator

Indicator::Indicator(const QString& name_):
	m_pid(QString("0x").append(USB_PID(name_))),
	m_vid(QString("0x").append(USB_VID(name_)))
{
}

bool Indicator::operator()(const mpl::at_c<Libvirt::Domain::Xml::VSource1::types, 0>::type& variant_) const
{
	const Libvirt::Domain::Xml::Source932& u = variant_.getValue();
	return m_vid == u.getUsbproduct().getVendor() &&
		u.getUsbproduct().getProduct() == m_pid;
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

	if (0 != m_input.getCpuUnits())
		m_tune->setShares(m_input.getCpuUnits() * 1024 / 1000);
	if (m_vt.isGlobalCpuLimit())
	{
		m_tune->setGlobalPeriod(v->getDefaultPeriod());
		m_tune->setGlobalQuota(-1);
	} else
	{
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

	quint64 d = m_input.getCpuLimitType() == PRL_CPULIMIT_PERCENTS ?
		100ull : v->getMhz();
	quint64 p = static_cast<quint64>(l) * v->getDefaultPeriod();
	quint32 q = (p + d - 1) / d;

	if (m_vt.isGlobalCpuLimit())
		m_tune->setGlobalQuota(q);
	else
		m_tune->setQuota(q / (m_input.getNumber() * m_input.getSockets()));

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setNumber()
{
	if (!m_vcpu)
		return PRL_ERR_UNINITIALIZED;

	quint32 x = m_input.getNumber() * m_input.getSockets();
	m_vcpu->setOwnValue(m_input.isEnableHotplug() ? 64 : x);
	m_vcpu->setCurrent(x);

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct CpuFeaturesMask

void CpuFeaturesMask::getFeatures(const VtInfo& vt_, Libvirt::Domain::Xml::Cpu &cpu)
{
	CpuFeatures* u = vt_.getCpuFeatures();
	QSet<QString> features = CCpuHelper::getDisabledFeatures(*m_input);
	/* FIXME arat feature will be implemented in Update3. It should be disabled
	 to keep libvirt migration work. It is not working in update1 QEMU.
	 #PSBM-52808 #PSBM-51001 #PSBM-52852 #PSBM-65816 */
	features.insert("arat");
	if (!m_input->getVmHardwareList()->getCpu()->isVirtualizedHV())
		features.insert(QString("vmx"));

	QList<Libvirt::Domain::Xml::Feature> l;
	if (NULL != u)
	{
		features.unite(u->getDisabled().toSet());
		foreach(const QString& name, u->getRequired())
		{
			if (features.contains(name))
				continue;
			// invtsc and xsaves are non migratable (see libvirt cpu_map.xml)
			if (name == "invtsc")
				continue;

			Libvirt::Domain::Xml::Feature f;
			f.setName(name);
			f.setPolicy(Libvirt::Domain::Xml::EPolicyRequire);
			l.append(f);
		}
	}
	/* hypervisor feature is pure virtual, it's always absent
	   in cpufeatures mask. VM requires it, so dispatcher should
	   not disable it */
	features.remove("hypervisor");
	foreach(const QString& name, features)
	{
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

	Libvirt::Domain::Xml::Target5 t;
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

	mpl::at_c<Extract<Libvirt::Domain::Xml::VChoice985Impl>::type, 4>::type e;
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

Libvirt::Domain::Xml::Hostdev
	Device<CHwUsbDevice>::getView(const CHwUsbDevice& model_)
{
	Libvirt::Domain::Xml::Usbproduct p;
	p.setVendor(QString("0x").append(USB_VID(model_.getDeviceId())));
	p.setProduct(QString("0x").append(USB_PID(model_.getDeviceId())));

	Libvirt::Domain::Xml::Source932 a;
	a.setUsbproduct(p);
	
	mpl::at_c<Libvirt::Domain::Xml::VSource1::types, 0>::type b;
	b.setValue(a);

	Libvirt::Domain::Xml::Source14 c;
	c.setSource(b);
	c.setStartupPolicy(Libvirt::Domain::Xml::EStartupPolicyOptional);

	Libvirt::Domain::Xml::Hostdevsubsysusb u;
	u.setSource(c);
	u.setReplug(Libvirt::Domain::Xml::EVirYesNoYes);

	mpl::at_c<Libvirt::Domain::Xml::VHostdevsubsys::types, 1>::type f;
	f.setValue(u);

	Libvirt::Domain::Xml::Hostdevsubsys g;
	g.setHostdevsubsys(f);

	mpl::at_c<Libvirt::Domain::Xml::VChoice917::types, 0>::type h;
	h.setValue(g);

	Libvirt::Domain::Xml::Hostdev output;
	output.setAlias(getAlias(model_));
	output.setChoice917(h);

	return output;
}

QString Device<CHwUsbDevice>::getPlugXml(const CHwUsbDevice& model_)
{
	QDomDocument x;
	getView(model_).save(x);
	return x.toString();
}

QString Device<CHwUsbDevice>::getAlias(const CHwUsbDevice& model_)
{
	return Transponster::Device::Alias()(model_);
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

	Libvirt::Domain::Xml::Os2 os;
	mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type vos;
	if (getStartupOptions(os) || Resources(m_input).getChipset(os))
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
	foreach (const CVmGenericPciDevice* d, h->m_lstGenericPciDevices)
	{
		b.add(d);
	}
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
	x.setChoice985List(Transponster::Device::deviceList_type()
			<< b.getDeviceList()
			<< t.getAttachment().getControllers()
			<< u.getDevices());

	m_result->setDevices(x);
//	m_result->setIothreads(qMax(t.getAttachment().getControllers().size(), 1));
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

	Libvirt::Domain::Xml::Memtune g;
	if (u.getMemGuarantee(g))
		m_result->setMemtune(g);

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
	m_result->setCommandline(CommandLine(m_input).addDebug()
		.addLogging().workaroundEfi2008R2().takeResult());
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
#if 0 // do not enable vsock by default  #PSBM-107455
	Libvirt::Domain::Xml::Cid c;
	c.setAuto(Libvirt::Domain::Xml::EVirYesNoYes);
	Libvirt::Domain::Xml::Pciaddress a;
	a.setDomain(QString("0x0000"));
	a.setBus(QString("0x00"));
	a.setSlot(QString("0x0a"));
	a.setFunction(QString("0x0"));
	mpl::at_c<Libvirt::Domain::Xml::VAddress::types, 0>::type v;
	v.setValue(a);

	Libvirt::Domain::Xml::Vsock sock;
	sock.setCid(c);
	sock.setAddress(Libvirt::Domain::Xml::VAddress(v));
	x.setVsock(sock);
#endif
	m_result->setDevices(x);
	return PRL_ERR_SUCCESS;
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

PRL_RESULT Mixer::setBlank()
{
	if (m_result.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	Libvirt::Domain::Xml::VOs b = m_result->getOs();
	PRL_RESULT r = Builder::setBlank();
	m_result->setCommandline(CommandLine(m_input).seed(m_result->getCommandline())
		.addDebug().workaroundEfi2008R2().takeResult());
	m_result->setOs(boost::apply_visitor(Visitor::Mixer::Os::Unit(), b, m_result->getOs()));
	return r;
}

PRL_RESULT Mixer::setDevices()
{
	if (m_result.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	QList<Libvirt::Domain::Xml::VChoice985 > h;
	boost::optional<Libvirt::Domain::Xml::Devices> d = m_result->getDevices();
	if (d)
	{
		Visitor::Mixer::Device v;
		foreach (const Libvirt::Domain::Xml::VChoice985& e, d.get().getChoice985List())
		{
			e.apply_visitor(v);
		}
		h = v.getResult();
	}
	PRL_RESULT output = Builder::setDevices();
	if (PRL_SUCCEEDED(output))
	{
		d = m_result->getDevices();
		QList<Libvirt::Domain::Xml::VChoice985 > t = d.get().getChoice985List();
		d.get().setChoice985List(t << h);
		m_result->setDevices(d);
	}

	return output;
}

PRL_RESULT Mixer::setIdentification()
{
	return m_result.isNull() ? PRL_ERR_READ_XML_CONTENT : PRL_ERR_SUCCESS;
}

PRL_RESULT Mixer::setResources(const VtInfo& info_)
{
	if (m_result.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	// we don't need to rewrite clock and cpu model
	boost::optional<Libvirt::Domain::Xml::Cpu> u = m_result->getCpu();
	boost::optional<Libvirt::Domain::Xml::Clock> t = m_result->getClock();
	PRL_RESULT r = Builder::setResources(info_);
	m_result->setClock(t);
	if (u)
	{
		boost::optional<Libvirt::Domain::Xml::Cpu> x = m_result->getCpu();
		if (x)
			x.get().setModel(u.get().getModel());
		else
			x = u;

		m_result->setCpu(x);
	}
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

	boost::optional<Libvirt::Domain::Xml::Commandline> c = m_result->getCommandline();
	if (c)
	{
		m_result->setCommandline(CommandLine(m_input).seed(c)
			.stripDebugcon().addDebug().takeResult());
	}

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

	QList<Libvirt::Domain::Xml::VChoice985> l = a.getDeviceList();

	Visitor::Fixup::Device v(m_input.getVmHardwareList(), l);
	foreach (const Libvirt::Domain::Xml::VChoice985& e, d->getChoice985List())
	{
		boost::apply_visitor(v, e);
	}

	Libvirt::Domain::Xml::Devices devices = *d;
	devices.setChoice985List(l);
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

PRL_RESULT Pipeline::operator()(object_type object_)
{
	PRL_RESULT e = m_action(object_);
	if (PRL_FAILED(e))
		return e;

	QDomDocument x;
	if (!object_.save(x))
		return PRL_ERR_UNEXPECTED;

	setValue(x.toString());

	return PRL_ERR_SUCCESS;
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

namespace Usb
{
///////////////////////////////////////////////////////////////////////////////
// struct Operator

PRL_RESULT Operator::plug(Pipeline::object_type& object_)
{
	PRL_RESULT e = unplug(object_);
	if (PRL_FAILED(e) && e != PRL_ERR_NO_DATA)
		return e;

	boost::optional<Libvirt::Domain::Xml::Devices> d = object_.getDevices();
	if (!d)
		return PRL_ERR_INVALID_ARG;

	mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 7>::type j;
	j.setValue(Device<CHwUsbDevice>::getView(*m_model));

	Transponster::Device::deviceList_type k = d->getChoice985List();
	k.prepend(j);
	d->setChoice985List(k);
	object_.setDevices(d);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Operator::unplug(Pipeline::object_type& object_)
{
	boost::optional<Libvirt::Domain::Xml::Devices> d = object_.getDevices();
	if (!d) 
		return PRL_ERR_INVALID_ARG;

	Transponster::Device::deviceList_type a = d->getChoice985List();
	typedef Transponster::Device::deviceList_type::iterator iterator_type;

	iterator_type e = a.end();
	iterator_type p = std::find_if(a.begin(), e,
		::Transponster::Device::Usb::Indicator(m_model->getDeviceId()));
	if (e == p)
		return PRL_ERR_NO_DATA;

	a.erase(p);
	d->setChoice985List(a);
	object_.setDevices(d);

	return PRL_ERR_SUCCESS;
}

} // namespace Usb
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

bool IPv4::isEligible(const QHostAddress& address_)
{
	return !(address_.isNull() || address_ == QHostAddress(QHostAddress::Any) ||
			address_ == QHostAddress(QHostAddress::LocalHost));
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

bool IPv6::isEligible(const QHostAddress& address_)
{
	return !(address_.isNull() || address_ == QHostAddress(QHostAddress::AnyIPv6) ||
			address_ == QHostAddress(QHostAddress::LocalHostIPv6));
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
///////////////////////////////////////////////////////////////////////////////
// struct Builder

template<class T>
struct Builder
{
	typedef Libvirt::Network::Xml::Ip result_type;
	
	void setAddress(const QHostAddress& host_, const QHostAddress& mask_)
	{
		typename mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, T::index>::type a;

		m_result.setFamily(QString(T::getFamily()));
		a.setValue(host_.toString());
		m_result.setAddress(Libvirt::Network::Xml::VIpAddr(a));
		typename mpl::at_c<Libvirt::Network::Xml::VIpPrefix::types, T::index>::type p;
		p.setValue(T::getMask(mask_));
		mpl::at_c<Libvirt::Network::Xml::VChoice2401::types, 1>::type m;
		m.setValue(p);
		m_result.setChoice2401(Libvirt::Network::Xml::VChoice2401(m));
	}
	void setDhcp(const CDHCPServer& model_)
	{
		typename mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, T::index>::type a, e;
		a.setValue(model_.getIPScopeStart().toString());
		e.setValue(T::patchEnd(model_.getIPScopeStart(), model_.getIPScopeEnd()).toString());
		Libvirt::Network::Xml::Range r;
		r.setStart(a);
		r.setEnd(e);
		Libvirt::Network::Xml::Dhcp h;
		h.setRangeList(QList<Libvirt::Network::Xml::Range>() << r);
		m_result.setDhcp(h);
	}
	const result_type& getResult() const
	{
		return m_result;
	}

private:
	result_type m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor
{
	typedef QList<Libvirt::Network::Xml::Ip> result_type;

	Visitor(const CHostOnlyNetwork& model_, result_type& result_):
		m_result(&result_), m_model(&model_)
	{
	}

	template<class T>
	void operator()(const T& family_) const
	{
		Builder<T> b;
		QHostAddress a = getAddress(family_);
		if (T::isEligible(a))
		{
			b.setAddress(a, getMask(family_));
			CDHCPServer* h = getDhcp(family_);
			if (NULL != h && h->isEnabled())
				b.setDhcp(*h);

			*m_result << b.getResult();
		}
	}

	QHostAddress getMask(const Address::IPv4& ) const
	{
		return m_model->getIPNetMask();
	}
	QHostAddress getAddress(const Address::IPv4& ) const
	{
		return m_model->getHostIPAddress();
	}
	CDHCPServer* getDhcp(const Address::IPv4& ) const
	{
		return m_model->getDHCPServer();
	}
	QHostAddress getMask(const Address::IPv6& ) const
	{
		return m_model->getIP6NetMask();
	}
	QHostAddress getAddress(const Address::IPv6& ) const
	{
		QHostAddress output = m_model->getHostIP6Address();
		output.setScopeId(QString());

		return output;
	}
	CDHCPServer* getDhcp(const Address::IPv6& ) const
	{
		return m_model->getDHCPv6ServerOrig();
	}

private:
	result_type* m_result;
	const CHostOnlyNetwork* m_model;
};

} // namespace

PRL_RESULT Reverse::setHostOnly()
{
	if (PVN_BRIDGED_ETHERNET == m_input.getNetworkType())
		return PRL_ERR_SUCCESS;

	CHostOnlyNetwork* n = m_input.getHostOnlyNetwork();
	if (NULL == n)
		return PRL_ERR_SUCCESS;

	Libvirt::Network::Xml::Dns d;
	d.setEnable(Libvirt::Network::Xml::EVirYesNoNo);
	m_result.setDns(d);

	QList<Libvirt::Network::Xml::Ip> x;
	boost::mpl::for_each<boost::mpl::vector<Address::IPv4, Address::IPv6>::type>
                                (Visitor(*n, x));

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
	mpl::at_c<Libvirt::Iface::Xml::VChoice2462::types, 0>::type v;
	v.setValue(e);
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setChoice2462List(QList<Libvirt::Iface::Xml::VChoice2462>() << v);
	m_result.setBridge(b);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setBridge()
{
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setDelay(2.0);
	b.setStp(Libvirt::Iface::Xml::EVirOnOffOff);
	m_result.setBridge(b);
	Libvirt::Iface::Xml::InterfaceAddressing2466 h;
	if (!m_master.isConfigureWithDhcp())
	{
		if (!m_master.isConfigureWithDhcpIPv6())
			return PRL_ERR_SUCCESS;

		Libvirt::Iface::Xml::Protocol p;
		p.setDhcp(Libvirt::Iface::Xml::Dhcp());
		h.setProtocol(p);
	}
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
		b.setValue(Libvirt::Snapshot::Xml::Disk4021());
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
	b.setValue(Libvirt::Snapshot::Xml::Disk4022());
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
	Libvirt::Snapshot::Xml::Variant4016 o;
	mpl::at_c<Libvirt::Snapshot::Xml::VChoice4019::types, 0>::type p;
	mpl::at_c<Libvirt::Snapshot::Xml::VDisk::types, 2>::type q;

	a.setValue(Device::Clustered::Model<CVmHardDisk>(disk_).getTargetName());
	s.setFile(disk_.getSystemName() + "." + m_snapshot);
	o.setSource(s);
	p.setValue(o);
	q.setValue(Libvirt::Snapshot::Xml::VChoice4019(p));

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

///////////////////////////////////////////////////////////////////////////////
// struct Block

PRL_RESULT Block::operator()(const object_type& object_)
{
	value_type v;
	v.setName(object_.getUuid());

	QList<Libvirt::Blocksnapshot::Xml::Disk > x;
	foreach (SnapshotComponent* y, object_.m_lstSnapshotComponents)
	{
		boost::mpl::at_c<Libvirt::Blocksnapshot::Xml::VName::types, 1>::type a;
		a.setValue(y->getDevice()->getSystemName());

		Libvirt::Blocksnapshot::Xml::Disk b;
		b.setName(a);
		b.setFleece(y->getState());

		x << b;
	}
	v.setDiskList(x);
	setValue(v);

	return PRL_ERR_SUCCESS;
}

namespace Export
{
///////////////////////////////////////////////////////////////////////////////
// struct Request

PRL_RESULT Request::operator()(const object_type& object_)
{
	if (object_.isEmpty())
		return PRL_ERR_INVALID_ARG;

	value_type v;
	Libvirt::Blockexport::Xml::Address a;
	a.setHost(object_[0].get<2>().host());
	if (0 < object_[0].get<2>().port())
		a.setPort(object_[0].get<2>().port());
	else
		a.setAutoport(Libvirt::Blockexport::Xml::EVirYesNoYes);

	v.setAddress(a);

	QList<Libvirt::Blockexport::Xml::Disk > x;
	foreach (object_type::const_reference d, object_)
	{
		Libvirt::Blockexport::Xml::Disk y;
		y.setSnapshot(d.get<0>());
		y.setSnapshot(d.get<0>());

		boost::mpl::at_c<Libvirt::Blockexport::Xml::VName::types, 1>::type n;
		n.setValue(d.get<1>());
		y.setName(n);

		boost::mpl::at_c<Libvirt::Blockexport::Xml::VChoice4992::types, 1>::type c;
		c.setValue(Libvirt::Blockexport::Xml::EVirYesNoYes);
		y.setChoice4992(Libvirt::Blockexport::Xml::VChoice4992(c));

		x << y;
	}
	v.setDiskList(x);
	setValue(v);

	return PRL_ERR_SUCCESS;
}

} // namespace Export
} // namespace Snapshot

namespace Filter
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

Reverse::Reverse(const CVmGenericNetworkAdapter &adapter) : m_adapter(adapter), m_result(new Libvirt::Filter::Xml::Filter())
{
	Libvirt::Filter::Xml::FilterNodeAttributes attributes = m_result->getFilterNodeAttributes();
	attributes.setName(getVzfilterName(adapter));
	m_result->setFilterNodeAttributes(attributes);

	QList <Libvirt::Filter::Xml::VChoice5120> contents;

	contents.append(prepareNetFilters(adapter));

	CVmNetFirewall *basic_firewall = m_adapter.getFirewall();
	if (basic_firewall && basic_firewall->isEnabled())
		contents.append(prepareFirewall(*basic_firewall));

	m_result->setChoice5120List(contents);
}

QString Reverse::getVzfilterName(const CVmGenericNetworkAdapter &adapter)
{
	QString mac = Device::Network::View(adapter).getRawMac();
	return NetFilter::S_VZ_FILTER_MASK.arg(mac);
}

QString Reverse::getResult()
{
	if (m_result.isNull())
		return QString();

	QDomDocument x;
	m_result->save(x);
	m_result.reset();
	return x.toString();
}

void Reverse::setUuid(const QString &uuid)
{
	mpl::at_c<Libvirt::Filter::Xml::VUUID::types, 1>::type u;
	u.setValue(uuid);
	m_result->setUuid(Libvirt::Filter::Xml::VUUID(u));
}

boost::optional<Libvirt::Filter::Xml::VAddrIP>
Reverse::prepareIp(const QString& value)
{
	if (value.isEmpty())
		return boost::none;

	mpl::at_c<Libvirt::Filter::Xml::VAddrIP::types, 1>::type ip_holder;
	ip_holder.setValue(value);
	return Libvirt::Filter::Xml::VAddrIP(ip_holder);
}

boost::optional<Libvirt::Filter::Xml::VAddrIPv6>
Reverse::prepareIpv6(const QString& value)
{
	if (value.isEmpty())
		return boost::none;

	mpl::at_c<Libvirt::Filter::Xml::VAddrIPv6::types, 1>::type ip_holder;
	ip_holder.setValue(value);
	return Libvirt::Filter::Xml::VAddrIPv6(ip_holder);
}

boost::optional <Libvirt::Filter::Xml::VUint16range>
Reverse::preparePort(unsigned int value)
{
	if (value == 0u)
		return boost::none;

	mpl::at_c<Libvirt::Filter::Xml::VUint16range::types, 2>::type port_holder;
	port_holder.setValue((qint32)(value));
	return boost::optional<Libvirt::Filter::Xml::VUint16range>(
			Libvirt::Filter::Xml::VUint16range(port_holder));
}

Libvirt::Filter::Xml::VUint8range
Reverse::prepareIcmpv6Type(unsigned int value)
{
	mpl::at_c<Libvirt::Filter::Xml::VUint8range::types, 1>::type type_holder;
	type_holder.setValue((qint32)(value));
	return Libvirt::Filter::Xml::VUint8range(type_holder);
}

Libvirt::Filter::Xml::VChoice5120
Reverse::prepareRule(const CVmNetFirewallRule &basic_rule,
							Libvirt::Filter::Xml::EDirectionType direction,
							Libvirt::Filter::Xml::EActionType action,
							int priority
							)
{
	QString proto = basic_rule.getProtocol();
	QString local_ip = basic_rule.getLocalNetAddress();
	QString remote_ip = basic_rule.getRemoteNetAddress();
	uint local_port = basic_rule.getLocalPort();
	uint remote_port = basic_rule.getRemotePort();
	static QString S_TCP = "tcp";
	static QString S_UDP = "udp";

	Libvirt::Filter::Xml::Rule rule;
	Libvirt::Filter::Xml::RuleNodeAttributes rule_attributes;

	rule_attributes.setPriority(priority);
	rule_attributes.setDirection(direction);
	rule_attributes.setAction(action);

	rule.setRuleNodeAttributes(rule_attributes);

	// using libvirt regex to validate IPv6
	// local_ip and remote_ip should be of same type, so making
	// decision based on local_ip
	bool isIPv6 = Libvirt::Validatable<mpl::at_c<Libvirt::Filter::Xml::VAddrIPv6::types, 1>::type::inner_type>::validate(local_ip);
	bool isBoth = local_ip.isEmpty() && remote_ip.isEmpty();

	Libvirt::Filter::Xml::CommonPortAttributes port_attributes = preparePortAttributes(
				local_port, remote_port);

	if (isIPv6 || isBoth)
	{
		Libvirt::Filter::Xml::CommonIpv6AttributesP1 ip_attributes = prepareIpv6Attributes(
			local_ip, remote_ip);
		if (proto == S_TCP)
			rule.setTcpIpv6List(prepareTcpIpv6(ip_attributes, port_attributes));
		else if (proto == S_UDP)
			rule.setUdpIpv6List(prepareUdpIpv6(ip_attributes, port_attributes));
		else if (proto.isEmpty())
			rule.setAllIpv6List(prepareAllIpv6(ip_attributes));
	}

	if (!isIPv6 || isBoth)
	{
		Libvirt::Filter::Xml::CommonIpAttributesP1 ip_attributes = prepareIpAttributes(
				local_ip, remote_ip);

		if (proto == S_TCP)
			rule.setTcpList(prepareTcp(ip_attributes, port_attributes));
		else if (proto == S_UDP)
			rule.setUdpList(prepareUdp(ip_attributes, port_attributes));
		else if (proto.isEmpty())
			rule.setAllList(prepareAll(ip_attributes));
	}

	mpl::at_c<Libvirt::Filter::Xml::VChoice5120::types, 1>::type rule_holder;
	rule_holder.setValue(rule);
	return Libvirt::Filter::Xml::VChoice5120(rule_holder);
}

Libvirt::Filter::Xml::EActionType
Reverse::prepareAction(PRL_FIREWALL_POLICY policy)
{
	switch (policy)
	{
		case PFP_ACCEPT:
			return Libvirt::Filter::Xml::EActionTypeAccept;
		case PFP_DENY:
			return Libvirt::Filter::Xml::EActionTypeDrop;
		default:
			return Libvirt::Filter::Xml::EActionTypeAccept;
	}
}

QList <Libvirt::Filter::Xml::VChoice5120>
Reverse::prepareFirewall(const CVmNetFirewall &firewall)
{
	QList <Libvirt::Filter::Xml::VChoice5120> result;
	QList <CVmNetFirewallDirection*> directions;
	QList <Libvirt::Filter::Xml::EDirectionType> direction_types;
	if (firewall.getIncoming() && firewall.getIncoming()->getDirection())
	{
		directions.append(firewall.getIncoming()->getDirection());
		direction_types.append(Libvirt::Filter::Xml::EDirectionTypeIn);
	}
	if (firewall.getOutgoing() && firewall.getOutgoing()->getDirection())
	{
		directions.append(firewall.getOutgoing()->getDirection());
		direction_types.append(Libvirt::Filter::Xml::EDirectionTypeOut);
	}

	for (int i = 0; i < directions.size(); ++i)
	{
		CVmNetFirewallDirection *direction = directions[i];
		Libvirt::Filter::Xml::EDirectionType direction_type = direction_types[i];

		// Preparing rule for established connections
		result.append(prepareAllowEstablished(direction_type, result.size()));

		Libvirt::Filter::Xml::EActionType action =
				direction->getDefaultPolicy() == PFP_ACCEPT
				? Libvirt::Filter::Xml::EActionTypeDrop
				: Libvirt::Filter::Xml::EActionTypeReturn;
		foreach(const CVmNetFirewallRule &basic_rule, direction->getFirewallRules()->m_lstFirewallRules)
			result.append(prepareRule(basic_rule, direction_type, action, result.size()));

		if (direction->getDefaultPolicy() == PFP_DENY) {
			result.append(prepareDefaultDeny(direction_type, result.size()));
		}
	}

	return result;
}

Libvirt::Filter::Xml::VChoice5120 Reverse::prepareAllowEstablished(Libvirt::Filter::Xml::EDirectionType direction, int priority) {
	Libvirt::Filter::Xml::Rule rule;
	Libvirt::Filter::Xml::RuleNodeAttributes rule_attributes;

	rule_attributes.setPriority(priority++);
	rule_attributes.setDirection(direction);
	rule_attributes.setAction(Libvirt::Filter::Xml::EActionTypeReturn);

	rule.setRuleNodeAttributes(rule_attributes);

	Libvirt::Filter::Xml::CommonIpAttributesP2 ip_attributes;
	ip_attributes.setState(boost::optional<QString>(QString("ESTABLISHED,RELATED")));
	rule.setAllList(prepareAll(ip_attributes));

	mpl::at_c<Libvirt::Filter::Xml::VChoice5120::types, 1>::type rule_holder;
	rule_holder.setValue(rule);
	
	return Libvirt::Filter::Xml::VChoice5120(rule_holder);
}

QList<Libvirt::Filter::Xml::VChoice5120> Reverse::prepareDefaultDeny(Libvirt::Filter::Xml::EDirectionType direction, int priority)
{
	QList<Libvirt::Filter::Xml::VChoice5120> result;
	QList<uint> nd_messages_ids;
	nd_messages_ids.append(133);
	nd_messages_ids.append(134);
	nd_messages_ids.append(135);
	nd_messages_ids.append(136);

	// allow IcmpV6 ND messages
	foreach(const uint& nd_message_id, nd_messages_ids)
	{
		Libvirt::Filter::Xml::Rule rule;
		Libvirt::Filter::Xml::RuleNodeAttributes rule_attributes;

		rule_attributes.setPriority(priority++);
		rule_attributes.setDirection(direction);
		rule_attributes.setAction(Libvirt::Filter::Xml::EActionTypeReturn);

		rule.setRuleNodeAttributes(rule_attributes);
		rule.setIcmpv6List(prepareIcmpv6(nd_message_id));

		mpl::at_c<Libvirt::Filter::Xml::VChoice5120::types, 1>::type rule_holder;
		rule_holder.setValue(rule);
		
		result.append(Libvirt::Filter::Xml::VChoice5120(rule_holder));
	}


	Libvirt::Filter::Xml::Rule rule;
	Libvirt::Filter::Xml::RuleNodeAttributes rule_attributes;

	rule_attributes.setPriority(priority++);
	rule_attributes.setDirection(direction);
	rule_attributes.setAction(Libvirt::Filter::Xml::EActionTypeDrop);

	rule.setRuleNodeAttributes(rule_attributes);

	Libvirt::Filter::Xml::CommonIpAttributesP1 ip_attrbutes;
	rule.setAllList(prepareAll(ip_attrbutes));

	mpl::at_c<Libvirt::Filter::Xml::VChoice5120::types, 1>::type rule_holder;
	rule_holder.setValue(rule);
	result.append(Libvirt::Filter::Xml::VChoice5120(rule_holder));

	return result;
}

boost::optional<Libvirt::Filter::Xml::VChoice5120>
Reverse::prepareIpSpoofing(const CVmGenericNetworkAdapter &adapter)
{
	static QString S_NO_IP_SPOOFING = "no-ip-spoofing";
	static QString S_IPV4_PARAMETER_NAME = "IP";
	Transponster::Device::Network::View view(adapter);
	if (!adapter.getPktFilter()->isPreventIpSpoof() ||
				adapter.isConfigureWithDhcp() || view.getIpv4().empty())
		return boost::none;
	
	Libvirt::Filter::Xml::FilterrefNodeAttributes filterref;
	filterref.setFilter(S_NO_IP_SPOOFING);
	QList<Libvirt::Filter::Xml::Parameter> plist;
	Libvirt::Filter::Xml::Parameter p;
	foreach (const QString& ip, view.getIpv4()) {
		p.setName(S_IPV4_PARAMETER_NAME);
		p.setValue(ip.split('/').first());
		plist.append(p);
	}
	filterref.setParameterList(plist);
	mpl::at_c<Libvirt::Filter::Xml::VChoice5120::types, 0>::type filterref_holder;
	filterref_holder.setValue(filterref);
	return Libvirt::Filter::Xml::VChoice5120(filterref_holder);
}

boost::optional<Libvirt::Filter::Xml::VChoice5120>
Reverse::prepareIpv6Spoofing(const CVmGenericNetworkAdapter &adapter)
{
	static QString S_NO_IPV6_SPOOFING = "no-ipv6-spoofing";
	static QString S_IPV6_PARAMETER_NAME = "IPV6";
	Transponster::Device::Network::View view(adapter);
	if (!adapter.getPktFilter()->isPreventIpSpoof() ||
			adapter.isConfigureWithDhcpIPv6() || view.getIpv6().empty())
		return boost::none;

	Libvirt::Filter::Xml::FilterrefNodeAttributes filterref;
	filterref.setFilter(S_NO_IPV6_SPOOFING);
	QList<Libvirt::Filter::Xml::Parameter> plist;
	Libvirt::Filter::Xml::Parameter p;
	foreach (const QString& ip, view.getIpv6()) {
		p.setName(S_IPV6_PARAMETER_NAME);
		p.setValue(ip.split('/').first());
		plist.append(p);
	}
	filterref.setParameterList(plist);
	mpl::at_c<Libvirt::Filter::Xml::VChoice5120::types, 0>::type filterref_holder;
	filterref_holder.setValue(filterref);
	return Libvirt::Filter::Xml::VChoice5120(filterref_holder);		
}

boost::optional<Libvirt::Filter::Xml::VChoice5120>
Reverse::prepareMacSpoofing(const CVmGenericNetworkAdapter &adapter)
{
	static QString S_NO_MAC_SPOOFING = "no-mac-spoofing";
	static QString S_MAC_PARAMETER_NAME = "MAC";

	if (!adapter.getPktFilter()->isPreventMacSpoof())
		return boost::none;

	Transponster::Device::Network::View view(adapter);
	Libvirt::Filter::Xml::FilterrefNodeAttributes filterref;
	filterref.setFilter(S_NO_MAC_SPOOFING);
	QList<Libvirt::Filter::Xml::Parameter> plist;
	Libvirt::Filter::Xml::Parameter p;
	foreach(const QString &mac, view.getMacList())
	{
		p.setName(S_MAC_PARAMETER_NAME);
		p.setValue(mac);
		plist << p;
	}
	filterref.setParameterList(plist);
	mpl::at_c<Libvirt::Filter::Xml::VChoice5120::types, 0>::type filterref_holder;
	filterref_holder.setValue(filterref);
	return Libvirt::Filter::Xml::VChoice5120(filterref_holder);
}

boost::optional<Libvirt::Filter::Xml::VChoice5120>
Reverse::preparePromisc(const CVmGenericNetworkAdapter &adapter)
{
	static QString S_NO_PROMISC = "no-promisc";
	static QString S_MAC_PARAMETER_NAME = "MAC";

	if (!adapter.getPktFilter()->isPreventPromisc())
		return boost::none;

	Transponster::Device::Network::View view(adapter);
	Libvirt::Filter::Xml::FilterrefNodeAttributes filterref;
	filterref.setFilter(S_NO_PROMISC);
	QList<Libvirt::Filter::Xml::Parameter> plist;
	Libvirt::Filter::Xml::Parameter p;
	p.setName(S_MAC_PARAMETER_NAME);
	p.setValue(view.getMac());
	plist.append(p);
	filterref.setParameterList(plist);
	mpl::at_c<Libvirt::Filter::Xml::VChoice5120::types, 0>::type filterref_holder;
	filterref_holder.setValue(filterref);
	return Libvirt::Filter::Xml::VChoice5120(filterref_holder);
}


QList <Libvirt::Filter::Xml::VChoice5120>
Reverse::prepareNetFilters(const CVmGenericNetworkAdapter &adapter)
{
	QList <Libvirt::Filter::Xml::VChoice5120> result;
	
	boost::optional<Libvirt::Filter::Xml::VChoice5120> c;
	if (c = prepareIpSpoofing(adapter))
		result.append(c.get());
	if (c = prepareIpv6Spoofing(adapter)) 
		result.append(c.get());
	if (c = prepareMacSpoofing(adapter))
		result.append(c.get());
	if (c = preparePromisc(adapter))
		result.append(c.get());

	return result;
}

QList <Libvirt::Filter::Xml::Tcp> Reverse::prepareTcp(
		const Libvirt::Filter::Xml::CommonIpAttributesP1 &ip_attributes,
		const Libvirt::Filter::Xml::CommonPortAttributes &port_attributes)
{
	Libvirt::Filter::Xml::Tcp tcp;
	tcp.setCommonIpAttributesP1(ip_attributes);
	tcp.setCommonPortAttributes(port_attributes);

	QList <Libvirt::Filter::Xml::Tcp> tcp_list;
	tcp_list.append(tcp);

	return tcp_list;
}

QList <Libvirt::Filter::Xml::Udp> Reverse::prepareUdp(
		const Libvirt::Filter::Xml::CommonIpAttributesP1 &ip_attributes,
		const Libvirt::Filter::Xml::CommonPortAttributes &port_attributes)
{
	Libvirt::Filter::Xml::Udp udp;
	udp.setCommonIpAttributesP1(ip_attributes);
	udp.setCommonPortAttributes(port_attributes);

	QList <Libvirt::Filter::Xml::Udp> udp_list;
	udp_list.append(udp);

	return udp_list;
}

QList <Libvirt::Filter::Xml::TcpIpv6> Reverse::prepareTcpIpv6(
		const Libvirt::Filter::Xml::CommonIpv6AttributesP1 &ip_attributes,
		const Libvirt::Filter::Xml::CommonPortAttributes &port_attributes)
{
	Libvirt::Filter::Xml::TcpIpv6 tcp;
	tcp.setCommonIpv6AttributesP1(ip_attributes);
	tcp.setCommonPortAttributes(port_attributes);

	QList <Libvirt::Filter::Xml::TcpIpv6> tcp_list;
	tcp_list.append(tcp);

	return tcp_list;
}

QList <Libvirt::Filter::Xml::UdpIpv6> Reverse::prepareUdpIpv6(
		const Libvirt::Filter::Xml::CommonIpv6AttributesP1 &ip_attributes,
		const Libvirt::Filter::Xml::CommonPortAttributes &port_attributes)
{
	Libvirt::Filter::Xml::UdpIpv6 udp;
	udp.setCommonIpv6AttributesP1(ip_attributes);
	udp.setCommonPortAttributes(port_attributes);

	QList <Libvirt::Filter::Xml::UdpIpv6> udp_list;
	udp_list.append(udp);

	return udp_list;
}

QList<Libvirt::Filter::Xml::Icmpv6> Reverse::prepareIcmpv6(unsigned int type)
{
	Libvirt::Filter::Xml::Icmpv6 icmpv6;
	Libvirt::Filter::Xml::IcmpAttributes icmp_attributes;
	icmp_attributes.setType(prepareIcmpv6Type(type));

	icmpv6.setIcmpAttributes(icmp_attributes);

	QList <Libvirt::Filter::Xml::Icmpv6> icmpv6_list;
	icmpv6_list.append(icmpv6);

	return icmpv6_list;
}

QList <Libvirt::Filter::Xml::All>
		Reverse::prepareAll(const Libvirt::Filter::Xml::CommonIpAttributesP1 &ip_attributes)
{
	Libvirt::Filter::Xml::All all;
	all.setCommonIpAttributesP1(ip_attributes);

	QList <Libvirt::Filter::Xml::All> all_list;
	all_list.append(all);

	return all_list;
}

QList <Libvirt::Filter::Xml::All>
		Reverse::prepareAll(const Libvirt::Filter::Xml::CommonIpAttributesP2 &ip_attributes)
{
	Libvirt::Filter::Xml::All all;
	all.setCommonIpAttributesP2(ip_attributes);

	QList <Libvirt::Filter::Xml::All> all_list;
	all_list.append(all);

	return all_list;
}

QList <Libvirt::Filter::Xml::AllIpv6>
		Reverse::prepareAllIpv6(const Libvirt::Filter::Xml::CommonIpv6AttributesP1 &ip_attributes)
{
	Libvirt::Filter::Xml::AllIpv6 all;
	all.setCommonIpv6AttributesP1(ip_attributes);

	QList <Libvirt::Filter::Xml::AllIpv6> all_list;
	all_list.append(all);

	return all_list;
}

Libvirt::Filter::Xml::CommonIpAttributesP1
Reverse::prepareIpAttributes(const QString &local_ip,
								   const QString &remote_ip)
{
	Libvirt::Filter::Xml::CommonIpAttributesP1 ip_attributes;

	ip_attributes.setSrcipaddr(prepareIp(local_ip));
	ip_attributes.setDstipaddr(prepareIp(remote_ip));

	return ip_attributes;
}

Libvirt::Filter::Xml::CommonIpv6AttributesP1
	Reverse::prepareIpv6Attributes(const QString &local_ip, const QString &remote_ip)
{
	Libvirt::Filter::Xml::CommonIpv6AttributesP1 ip_attributes;

	ip_attributes.setSrcipaddr(prepareIpv6(local_ip));
	ip_attributes.setDstipaddr(prepareIpv6(remote_ip));

	return ip_attributes;
}

Libvirt::Filter::Xml::CommonPortAttributes
Reverse::preparePortAttributes(uint local_port, uint remote_port)
{
	Libvirt::Filter::Xml::CommonPortAttributes port_attributes;

	port_attributes.setSrcportstart(preparePort(local_port));
	port_attributes.setSrcportend(preparePort(local_port));

	port_attributes.setDstportstart(preparePort(remote_port));
	port_attributes.setDstportend(preparePort(remote_port));

	return port_attributes;
}
} // namespace Filter
} // namespace Transponster
