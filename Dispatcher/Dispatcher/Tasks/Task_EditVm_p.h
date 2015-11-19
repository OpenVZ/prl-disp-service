///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_EditVm_p.h
///
/// Edit VM configuration
///
/// @author shrike
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#ifndef __TASK_EDITVM_P_H__
#define __TASK_EDITVM_P_H__

#include "CDspTaskHelper.h"
#include "CDspLibvirt.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"

class Task_EditVm;
namespace Edit
{
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Action

struct Action
{
	virtual ~Action();

	virtual bool execute(CDspTaskFailure& feedback_) = 0;
	void setNext(Action* value_)
	{
		m_next.reset(value_);
	}

private:
	QScopedPointer<Action> m_next;
};

///////////////////////////////////////////////////////////////////////////////
// struct Request

struct Request
{
	typedef SmartPtr<CVmConfiguration> config_type;

	Request(Task_EditVm& task_, const config_type& start_,
		const config_type& final_);

	const CVmIdent& getObject() const
	{
		return m_object;
	}
	const CVmConfiguration& getStart() const
	{
		return *m_start;
	}
	const CVmConfiguration& getFinal() const
	{
		return *m_final;
	}

private:
	CVmIdent m_object;
	Task_EditVm* m_task;
	const config_type m_start;
	const config_type m_final;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor
{
	explicit Visitor(const Request& input_): m_input(&input_)
	{
	}

	template<class T>
	void operator()(T factory_)
	{
		Action* a = factory_(*m_input);
		if (NULL == a)
			return;

		a->setNext(m_result.take());
		m_result.reset(a);
	}

	Action* getResult()
	{
		return m_result.take();
	}

private:
	const Request* m_input;
	QScopedPointer<Action> m_result;
};

namespace Runtime
{
namespace Cdrom
{
///////////////////////////////////////////////////////////////////////////////
// struct Action

struct Action: Vm::Action
{
	Action(const QString& vm_, const CVmOpticalDisk& device_):
		m_vm(vm_), m_device(device_)
	{
	}

	bool execute(CDspTaskFailure& feedback_);

private:
	QString m_vm;
	CVmOpticalDisk m_device;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	Action* operator()(const Request& input_) const;
};

} // namespace Cdrom

namespace Memory
{
///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	Vm::Action* operator()(const Request& input_) const;
};

} // namespace Memory

namespace Disk
{

namespace Limit
{

namespace Policy
{

struct Io
{
static Libvirt::Result setLimit(Libvirt::Tools::Agent::Vm::Runtime& device_,
	const CVmHardDisk* disk_, quint32 limit_)
{
	return device_.setIoLimit(disk_, limit_);
}
};

struct Iops
{
static Libvirt::Result setLimit(Libvirt::Tools::Agent::Vm::Runtime& device_,
	const CVmHardDisk* disk_, quint32 limit_)
{
	return device_.setIopsLimit(disk_, limit_);
}
};

} // namespace Policy

///////////////////////////////////////////////////////////////////////////////
// struct Unit

template <typename T>
struct Unit: Vm::Action
{
	Unit(const QString& vm_, const CVmHardDisk& device_, quint32 limit_):
		m_vm(vm_), m_device(device_), m_limit(limit_)
	{
	}

	bool execute(CDspTaskFailure& feedback_)
	{
		Libvirt::Tools::Agent::Vm::Runtime d = Libvirt::Kit.vms().at(m_vm).getRuntime();
		Libvirt::Result e(T::setLimit(d, &m_device, m_limit));
		if (e.isFailed())
		{
			feedback_(e.error().convertToEvent());
			return false;
		}
		return Vm::Action::execute(feedback_);
	}

private:
	QString m_vm;
	CVmHardDisk m_device;
	quint32 m_limit;
};

} // namespace Limit

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	Vm::Action* operator()(const Request& input_) const;
private:
	bool isDiskIoUntunable(const CVmHardDisk* disk_) const;
};

} // namespace Disk

namespace Blkiotune
{

///////////////////////////////////////////////////////////////////////////////
// struct Action

struct Action: Vm::Action
{
	Action(const QString& vm_, quint32 ioprio_): m_vm(vm_), m_ioprio(ioprio_)
	{
	}

	bool execute(CDspTaskFailure& feedback_);

private:
	QString m_vm;
	quint32 m_ioprio;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	Vm::Action* operator()(const Request& input_) const;
};

} // namespace Blkiotune

namespace Network
{
typedef CVmGlobalNetwork general_type;
typedef CVmGenericNetworkAdapter device_type;

///////////////////////////////////////////////////////////////////////////////
// struct Dao

struct Dao
{
	typedef QList<device_type* > list_type;

	explicit Dao(const list_type& dataSource_);

	const list_type& getEligible() const
	{
		return m_eligible;
	}
	device_type* findDefaultGwIp4Bridge() const;
	device_type* findDefaultGwIp6Bridge() const;
	device_type* find(const QString& name_, quint32 index_) const;

private:
	list_type m_eligible;
	list_type m_dataSource;
};

///////////////////////////////////////////////////////////////////////////////
// struct Bridge

struct Bridge
{
	explicit Bridge(const QString& mac_): m_mac(mac_)
	{
	}

	const QString& getMac() const
	{
		return m_mac;
	}

private:
	QString m_mac;
};

///////////////////////////////////////////////////////////////////////////////
// struct Routed

struct Routed
{
	Routed(const QString& mac_, const device_type* defaultGwIp4Bridge_,
		const device_type* defaultGwIp6Bridge_);

	const QString& getMac() const
	{
		return m_mac;
	}
	std::pair<QString, QString> getIp4Defaults() const;
	std::pair<QString, QString> getIp6Defaults() const;

private:
	QString m_mac;
	const device_type* m_defaultGwIp4Bridge;
	const device_type* m_defaultGwIp6Bridge;
};

///////////////////////////////////////////////////////////////////////////////
// struct Address

struct Address
{
	explicit Address(const device_type& device_);

	QStringList operator()(const Routed& mode_);
	QStringList operator()(const Bridge& mode_);

private:
	QStringList m_v4;
	QStringList m_v6;
	const device_type* m_device;
};

namespace Difference
{
///////////////////////////////////////////////////////////////////////////////
// struct SearchDomain

struct SearchDomain
{
	SearchDomain(const general_type& general_, const Dao& devices_);

	QStringList calculate(const general_type& general_, const Dao& devices_);

private:
	QStringList m_general;
	QList<device_type* > m_devices;
};

///////////////////////////////////////////////////////////////////////////////
// struct Device

struct Device
{
	Device(const general_type& general_, const Dao& devices_);

	QStringList calculate(const general_type& general_, const Dao& devices_);

private:
	static bool isEqual(const device_type* first_, const device_type* second_);

	Dao m_devices;
	const general_type* m_general;
	const device_type* m_defaultGwIp4Bridge;
	const device_type* m_defaultGwIp6Bridge;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	Vm(const general_type& general_, const Dao& devices_);

	QStringList calculate(const general_type& general_, const Dao& devices_);

private:
	Device m_device;
	boost::optional<QString> m_hostname;
	boost::optional<SearchDomain> m_searchDomain;
};

} // namespace Difference

///////////////////////////////////////////////////////////////////////////////
// struct Action

struct Action: Vm::Action
{
	Action(const QString& vm_, const QStringList& args_):
		m_vm(vm_), m_args(args_)
	{
	}

	bool execute(CDspTaskFailure& feedback_);

private:
	QString m_vm;
	QStringList m_args;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	Action* operator()(const Request& input_) const;
};

} // namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	Action* operator()(const Request& input_) const;
};

} // namespace Runtime
} // namespace Vm
} // namespace Edit

#endif	// __TASK_EDITVM_P_H__
