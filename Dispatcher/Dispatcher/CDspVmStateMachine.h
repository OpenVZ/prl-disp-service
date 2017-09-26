/*
 * Copyright (c) 2016-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __CDSP_VM_STATE_MACHINE_H__
#define __CDSP_VM_STATE_MACHINE_H__

#include "CDspService.h"
#include "CDspVmStateSender.h"
#include "CVcmmdInterface.h"
#include "CDspVmNetworkHelper.h"
#include "CDspBackupDevice.h"
#include "Tasks/Task_BackgroundJob.h"
#include "Interfaces/Debug.h"
#include <Libraries/StatesUtils/StatesHelper.h>
#include <prlsdk/PrlEnums.h>
#include <prlcommon/Logging/Logging.h>
#include <boost/signals2/signal.hpp>
#include <boost/msm/front/internal_row.hpp>
#include "CDspLibvirtExec.h"
#include "CDspVmGuest.h"
#include "CDspVmStateMachine_p.h"
#include <boost/phoenix/core/value.hpp>
#include <Libraries/PrlCommonUtils/CFirewallHelper.h>

namespace Vm
{
namespace Configuration
{
typedef boost::function<void(CVmConfiguration& )> update_type;

} // namespace Configuration

namespace State
{
namespace msmf = boost::msm::front;

///////////////////////////////////////////////////////////////////////////////
// struct Conventional

template <VIRTUAL_MACHINE_STATE N>
struct Conventional: boost::mpl::integral_c<VIRTUAL_MACHINE_STATE, N>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Switch

struct Switch
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Agent

struct Agent
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Reboot

struct Reboot
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Upgrade

struct Upgrade
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Running

// State flag that VM is active
struct Running
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Started

struct Started: Details::Trace<Started>
{
	typedef Config::Edit::Atomic editor_type;
	typedef Details::Trace<Started> trace_type;
	typedef ::Vm::Guest::Connector connector_type;
	typedef Configuration::update_type update_type;
	typedef boost::optional<PRL_VM_TOOLS_STATE> tools_type;
	typedef boost::function<void (connector_type& )> decorator_type;
	typedef boost::phoenix::expression::value<tools_type>::type toolsEvent_type;
	typedef boost::function<connector_type* (QWeakPointer<QAtomicInt>)> factory_type;

	Started(): m_guard()
	{
	}
	Started(const editor_type& editor_, const factory_type& factory_);

	template <class M>
	void on_entry(Conventional<VMS_RUNNING> const& event_, M& fsm_)
	{
		trace_type::on_entry(event_, fsm_);
		connect_(boost::bind(&connector_type::setRetries, _1, 1));
	}
	template <class M>
	void on_entry(Switch const& event_, M& fsm_)
	{
		trace_type::on_entry(event_, fsm_);
		connect_();
	}
	template <class M>
	void on_entry(Agent const& event_, M& fsm_)
	{
		trace_type::on_entry(event_, fsm_);
		connect_();
	}
	template <class Event, class M>
	void on_entry(Event const& event_, M& fsm_)
	{
		trace_type::on_entry(event_, fsm_);
	}
	template<typename Event, typename M>
	void on_exit(const Event& event_, M& fsm_)
	{
		trace_type::on_exit(event_, fsm_);
		setTools(boost::none);
	}
	const tools_type& getTools() const
	{
		return m_tools;
	}

	struct Action
	{
		template<class M>
		void operator()(Agent const&, M&, Started& state_, Started&)
		{
			if (NULL == state_.m_guard)
				state_.connect_(decorator_type());
		}
		template<class M>
		void operator()(toolsEvent_type const& event_, M&, Started& state_, Started&)
		{
			state_.setTools(event_());
		}
		template<class M>
		void operator()(update_type const& event_, M&, Started& state_, Started&)
		{
			if (!state_.m_editor.isNull())
				(*state_.m_editor)(event_);
		}
	};
	struct internal_transition_table: boost::mpl::vector
		<
			msmf::Internal<Agent, Action>,
			msmf::Internal<update_type, Action>,
			msmf::Internal<toolsEvent_type, Action>
		>
	{
	};

private:
	void setTools(const tools_type& value_);
	void connect_();
        void connect_(const decorator_type& decorator_);

	tools_type m_tools;
	QAtomicInt* m_guard;
	factory_type m_factory;
	QSharedPointer<QAtomicInt> m_incarnation;
	QSharedPointer<editor_type> m_editor;
};

///////////////////////////////////////////////////////////////////////////////
// struct Generic

template <VIRTUAL_MACHINE_STATE N>
struct Generic: Details::Trace<Generic<N> >, boost::mpl::integral_c<VIRTUAL_MACHINE_STATE, N>
{
	template <class Event, class FSM>
	void on_entry(Event const& event_, FSM& fsm_)
	{
		Details::Trace<Generic>::on_entry(event_, fsm_);
		WRITE_TRACE(DBG_INFO, "VM '%s' changed state to %s",
			qPrintable(fsm_.getName()), PRL_VM_STATE_TO_STRING(N));
	}
};

namespace Paused
{
typedef Generic<VMS_PAUSED> Ordinary;

///////////////////////////////////////////////////////////////////////////////
// struct Unknown

struct Unknown: Ordinary
{
};

} // namespace Paused

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Details::Frontend<Frontend>
{
	Frontend(const QString& uuid_, const SmartPtr<CDspClient>& user_,
			const QSharedPointer< ::Network::Routing>& routing_);

	typedef Generic<VMS_STOPPED> Stopped;
	typedef Generic<VMS_SUSPENDED> Suspended;
	typedef Generic<VMS_MOUNTED> Mounted;
	typedef Generic<VMS_UNKNOWN> Unknown;

	// Running_
	struct Running_: Details::Frontend<Running_>, boost::mpl::integral_c<VIRTUAL_MACHINE_STATE, VMS_RUNNING>
	{
		typedef boost::mpl::vector< ::Vm::State::Running> flag_list;

		Running_(): m_big()
		{
		}
		explicit Running_(State::Frontend& big_): m_big(&big_)
		{
		}

		void apply(const Configuration::update_type& update_)
		{
			m_big->getConfigEditor()(update_);
		}

		void upgrade(const Upgrade&)
		{
			CVmConfiguration runtime;
			Libvirt::Instrument::Agent::Vm::Unit v = Libvirt::Kit.vms().at(m_big->getUuid());
			if (v.getConfig(runtime, true).isFailed())
			{
				WRITE_TRACE(DBG_INFO, "failed to read runtime config from for VM '%s'",
					qPrintable(m_big->m_name));
				return;
			}
			WRITE_TRACE(DBG_INFO, "updating config from runtime for VM '%s'",
					qPrintable(m_big->m_name));
			Config::Edit::Atomic e = m_big->getConfigEditor();
			e(boost::bind<void>(&Vnc::Secure::Frontend::setup,
				Vnc::Secure::Frontend(e, m_big->getService()),
				_1, boost::cref(runtime)));
		}

		// Pseudo-state
		struct Already: msmf::entry_pseudo_state<0>
		{
		};

		// Rebooted
		typedef struct Rebooted: Details::Trace<Rebooted>
		{
		} initial_state;

		// Stopping
		struct Stopping: Details::Trace<Stopping>
		{
		};

		struct transition_table: boost::mpl::vector
		<
			msmf::Row
			<
				Started,
				Conventional<VMS_STOPPING>,
				Stopping
			>,
			msmf::Row
			<
				Rebooted,
				Conventional<VMS_STOPPING>,
				Stopping
			>,
			a_irow<Started, Upgrade, &Running_::upgrade>,
			a_irow<Rebooted, Upgrade, &Running_::upgrade>,
			a_irow<Rebooted, Configuration::update_type, &Running_::apply>,
			msmf::Row
			<
				Stopping,
				Reboot,
				Rebooted
			>,
			msmf::Row
			<
				Started,
				Reboot,
				Rebooted
			>,
			msmf::Row
			<
				Rebooted,
				Conventional<VMS_RUNNING>,
				msmf::none
			>,
			msmf::Row
			<
				Rebooted,
				Reboot,
				msmf::none
			>,
			msmf::Row
			<
				Rebooted,
				Agent,
				Started
			>,
			msmf::Row
			<
				Already,
				Conventional<VMS_RUNNING>,
				Started
			>
		>
		{
		};

	private:
		State::Frontend *m_big;
	};
	
	typedef boost::msm::back::state_machine<Running_> Running;

	// State
	struct Reverting: Stopped
	{
	};

	typedef Unknown initial_state;

	// Guard
	struct HasState
	{
		template<class Event, class FromState>
		bool operator()(const Event&, Frontend& fsm_, FromState&, Stopped&)
		{
			return !CStatesHelper(fsm_.getHome()).savFileExists();
		}
		template<class Event, class FromState>
		bool operator()(const Event&, Frontend& fsm_, FromState&, Suspended&)
		{
			return CStatesHelper(fsm_.getHome()).savFileExists();
		}
	};

	// Action
	struct Guarantee
	{
		template<class Event, class FromState>
		void operator()(const Event&, Frontend& fsm_, FromState&, Running&)
		{
			WRITE_TRACE(DBG_INFO, "enabling guarantees for VM '%s'", qPrintable(fsm_.m_name));
			Vcmmd::Api(fsm_.getUuid()).activate();
		}

		template<class Event, class FromState>
		void operator()(const Event&, Frontend& fsm_, FromState&, Paused::Ordinary&)
		{
			WRITE_TRACE(DBG_INFO, "disabling guarantees for VM '%s'", qPrintable(fsm_.m_name));
			Vcmmd::Api(fsm_.getUuid()).deactivate();
		}

		template<class Event, class FromState>
		void operator()(const Event& event_, Frontend& fsm_, FromState& source_, Paused::Unknown& target_)
		{
			Paused::Ordinary& o = target_;
			this->operator()(event_, fsm_, source_, o);
		}

		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			WRITE_TRACE(DBG_INFO, "unregistering guarantees for VM '%s'", qPrintable(fsm_.m_name));
			Vcmmd::Api(fsm_.getUuid()).deinit();
		}
	};

	// Action
	struct Traffic
	{
		template<class Event, class FromState>
		void operator()(const Event&, Frontend& fsm_, FromState&, Running&)
		{
			boost::optional<CVmConfiguration> y = fsm_.getConfig();
			if (!y)
				return;

			WRITE_TRACE(DBG_INFO, "configuring network rates for VM '%s'", qPrintable(fsm_.m_name));
			Network::Traffic::Accounting x(fsm_.getUuid());
			foreach (CVmGenericNetworkAdapter *a, y->getVmHardwareList()->m_lstNetworkAdapters)
			{
				x(QSTR2UTF8(a->getHostInterfaceName()));
			}
			if (PRL_FAILED(Task_NetworkShapingManagement::setNetworkRate(y.get())))
			{
				CVmEvent m;
				m.setEventType(PET_DSP_EVT_ERROR_MESSAGE);
				m.setEventCode(PRL_ERR_VZ_OPERATION_FAILED);
				m.addEventParameter(new CVmEventParameter(PVE::String,
					"Failed to configure the network rate for VM. See logs for more details."
					, EVT_PARAM_MESSAGE_PARAM_0));
				SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, m);
				fsm_.getService().getClientManager().sendPackageToAllClients(p);
			}
		}
	};

	// Action
	struct Notification
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			CDspLockedPointer<CDspVmStateSender> s = CDspService::instance()->getVmStateSender();
			if (s.isValid())
			{
				s->onVmStateChanged(FromState::value, ToState::value, fsm_.getUuid(),
					fsm_.m_user->getVmDirectoryUuid(), false);
			}
		}
	};

	// Action
	struct RoutesUp
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			boost::optional<CVmConfiguration> y = fsm_.getConfig();
			if (!y)
				return;

			WRITE_TRACE(DBG_INFO, "configuring routes for VM '%s'", qPrintable(fsm_.m_name));
			fsm_.m_routing->up(y.get());
			SmartPtr<CVmConfiguration> x(&y.get(), SmartPtrPolicy::DoNotReleasePointee);
			CFirewallHelper fw(x);
			if (PRL_FAILED(fw.Execute()))
				WRITE_TRACE(DBG_FATAL, "failed to enable firewall rules for VM '%s'", qPrintable(fsm_.m_name));
		}
	};

	// Action
	struct RoutesDown
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			boost::optional<CVmConfiguration> y = fsm_.getConfig();
			if (!y)
				return;

			WRITE_TRACE(DBG_INFO, "disabling routes for VM '%s'", qPrintable(fsm_.m_name));
			fsm_.m_routing->down(y.get());
			SmartPtr<CVmConfiguration> x(&y.get(), SmartPtrPolicy::DoNotReleasePointee);
			CFirewallHelper fw(x, true);
			if (PRL_FAILED(fw.Execute()))
				WRITE_TRACE(DBG_FATAL, "failed to disable firewall rules for VM '%s'", qPrintable(fsm_.m_name));
		}
	};

	// Action
	struct Cluster
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState& from_, ToState& to_)
		{
			Config::Edit::Atomic a = fsm_.getConfigEditor();
			if (VMS_MIGRATING != CDspVm::getVmState(a.getObject()))
			{
				a(boost::bind(&configure<FromState, ToState>,
					_1, boost::ref(from_), boost::ref(to_)));
			}
		}

		template<class FromState, class ToState>
		static void configure(CVmConfiguration& config_, FromState&, ToState&)
		{
			bool f = boost::is_same<FromState, Running>::value;
			bool t = boost::is_same<ToState, Running>::value;
			// ignore transitions where Running doesn't present on neither side
			if (!f && !t)
				return;
			CVmSettings *s = config_.getVmSettings();
			if (s)
			{
				ClusterOptions *o = s->getClusterOptions();
				if (o)
					o->setRunning(!f && t);
			}
		}
	};

	// Action
	struct Runtime
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState& from_, ToState& to_)
		{
			Libvirt::Instrument::Agent::Vm::Unit v = Libvirt::Kit.vms().at(fsm_.getUuid());
			qint64 p = qMax<qint64>(fsm_.getService().getDispConfigGuard()
				.getDispWorkSpacePrefs()->getVmGuestCollectPeriod(), quint64(1));
			v.setMemoryStatsPeriod(p);

			CVmConfiguration runtime;
			if (v.getConfig(runtime, true).isFailed())
			{
				WRITE_TRACE(DBG_INFO, "failed to read runtime config from for VM '%s'", qPrintable(fsm_.m_name));
				return;
			}

			WRITE_TRACE(DBG_INFO, "updating config from runtime for VM '%s'", qPrintable(fsm_.m_name));
			Config::Edit::Atomic e = fsm_.getConfigEditor();
			boost::signals2::signal<void (CVmConfiguration& )> s;
			s.connect(boost::bind<void>(&Vnc::Secure::Frontend::setup,
				Vnc::Secure::Frontend(e, fsm_.getService()),
				_1, boost::cref(runtime)));
			s.connect(boost::bind(&Vm::Config::Repairer<Vm::Config::revise_types>::type::do_,
				_1, boost::cref(runtime)));
			s.connect(boost::bind(&Cluster::configure<FromState, ToState>, _1,
				boost::ref(from_), boost::ref(to_)));
			e(s);
		}
	};

	struct BackupDisable
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			boost::optional<CVmConfiguration> c = fsm_.getConfig();
			if (!c)
				return;
			WRITE_TRACE(DBG_INFO, "disabling backups, attached to VM '%s'", qPrintable(fsm_.m_name));
			SmartPtr<CVmConfiguration> x(&c.get(), SmartPtrPolicy::DoNotReleasePointee);
			::Backup::Device::Service(x).disable();
		}
	};

	struct Lock
	{
		template<class E, class S, class D>
		void operator()(const E&, Frontend& fsm_, S&, D&)
		{
			CDspClient& u = fsm_.getUser();
			fsm_.getService().getVmDirHelper().registerExclusiveVmOperation
				(fsm_.getUuid(), u.getVmDirectoryUuid(), PVE::DspCmdVmStartEx, u.getSessionUuid());
		}
	};

	struct Unlock
	{
		template<class E, class S, class D>
		void operator()(const E&, Frontend& fsm_, S&, D&)
		{
			CDspClient& u = fsm_.getUser();
			fsm_.getService().getVmDirHelper().unregisterExclusiveVmOperation
				(fsm_.getUuid(), u.getVmDirectoryUuid(), PVE::DspCmdVmStartEx, u.getSessionUuid());
		}
	};

	struct transition_table : boost::mpl::vector<
	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	//
	msmf::Row<Unknown,    Conventional<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> >, HasState>,

	msmf::Row<Unknown,    Conventional<VMS_STOPPED>,    Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> >, HasState>,

	msmf::Row<Unknown,    Conventional<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> > >,

	msmf::Row<Unknown,    Conventional<VMS_PAUSED>,     Paused::Unknown,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesUp, Lock, Notification> > >,

	msmf::Row<Unknown,    Conventional<VMS_RUNNING>,    Running::entry_pt<Running::Already>,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, RoutesUp, Runtime, Lock, Notification> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Stopped,    Conventional<VMS_RUNNING>,    Running,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, RoutesUp, Runtime, Lock, Notification> > >,

	msmf::Row<Stopped,    Conventional<VMS_MOUNTED>,    Mounted,    Notification >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Suspended,  Conventional<VMS_STOPPED>,    Stopped,    Notification >,

	msmf::Row<Suspended,  Conventional<VMS_PAUSED>,     Paused::Ordinary,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesUp, Lock, Notification> > >,

	msmf::Row<Suspended,  Conventional<VMS_RUNNING>,    Running,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, RoutesUp, Runtime, Lock, Notification> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Running,    Conventional<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, Unlock, Cluster, BackupDisable, Notification> > >,

	msmf::Row<Running,    Conventional<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, Unlock, Cluster, BackupDisable, Notification> > >,

	msmf::Row<Running,    Conventional<VMS_PAUSED>,     Paused::Ordinary,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Cluster, Notification> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Paused::Ordinary,     Conventional<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, BackupDisable, Unlock, Notification> > >,

	msmf::Row<Paused::Ordinary,     Conventional<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, BackupDisable, Unlock, Notification> > >,

	msmf::Row<Paused::Ordinary,     Conventional<VMS_RUNNING>,    Running::entry_pt<Running::Already>,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, Cluster, Notification> > >,

	msmf::Row<Paused::Unknown,     Conventional<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, BackupDisable, Unlock, Notification> > >,

	msmf::Row<Paused::Unknown,     Conventional<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, BackupDisable, Unlock, Notification> > >,

	msmf::Row<Paused::Unknown,     Conventional<VMS_RUNNING>,    Running::entry_pt<Running::Already>,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, Cluster, Runtime, Notification> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Mounted,    Conventional<VMS_STOPPED>,    Stopped,    Notification >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Stopped,    Switch,                Reverting >,
	msmf::Row<Suspended,  Switch,                Reverting >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Reverting,  Conventional<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> >, HasState>,

	msmf::Row<Reverting,  Conventional<VMS_STOPPED>,    Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> >, HasState>,

	msmf::Row<Reverting,  Conventional<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> > >,

	msmf::Row<Reverting,  Conventional<VMS_PAUSED>,     Paused::Ordinary,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesUp, Runtime, Lock, Notification> > >,

	msmf::Row<Reverting,  Conventional<VMS_RUNNING>,    Running,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, RoutesUp, Runtime, Lock, Notification> > >

	//      +-----------+----------------------+-----------+--------+
	> {};

	const QString& getName() const
	{
		return m_name;
	}
	const QString& getUuid() const
	{
		return m_uuid;
	}
	const QString getHome() const;
	void setHome(const QString& path_)
	{
		m_home = QFileInfo(path_);
	}

	void setName(const QString& value_);
	boost::optional<CVmConfiguration> getConfig() const;
	Config::Edit::Atomic getConfigEditor() const;

protected:
	CDspClient& getUser() const
	{
		return *m_user;
	}
	CDspService& getService() const
	{
		return *m_service;
	}

private:
	QString m_uuid;
	CDspService* m_service;
	SmartPtr<CDspClient> m_user;
	QSharedPointer< ::Network::Routing> m_routing;
	QString m_name;
	boost::optional<QFileInfo> m_home;
};

///////////////////////////////////////////////////////////////////////////////
// struct Machine

typedef boost::msm::back::state_machine<Frontend> Machine;

} // namespace State
} // namespace Vm

#endif // __CDSP_VM_STATE_MACHINE_H__
