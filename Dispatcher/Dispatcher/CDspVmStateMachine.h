/*
 * Copyright (c) 2016 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
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
#include <Libraries/PrlCommonUtils/CFirewallHelper.h>

namespace Vm
{
namespace Tray
{
typedef boost::function1<void, CVmConfiguration& > action_type;

} // namespace Tray

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
// struct NoAgent

struct NoAgent
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Reboot

struct Reboot
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Running

// State flag that VM is active
struct Running
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: Details::Frontend<Frontend>
{
	Frontend(const QString& uuid_, const SmartPtr<CDspClient>& user_,
			const QSharedPointer< ::Network::Routing>& routing_);

	template <VIRTUAL_MACHINE_STATE N>
	struct State: Details::Trace<State<N> >, boost::mpl::integral_c<VIRTUAL_MACHINE_STATE, N>
	{
		template <class Event, class FSM>
		void on_entry(Event const& event_, FSM& fsm_)
		{
			Details::Trace<State>::on_entry(event_, fsm_);
			WRITE_TRACE(DBG_INFO, "VM '%s' changed state to %s",
				qPrintable(fsm_.getName()), PRL_VM_STATE_TO_STRING(N));
		}
	};

	typedef State<VMS_STOPPED> Stopped;
	typedef State<VMS_SUSPENDED> Suspended;
	typedef State<VMS_PAUSED> Paused;
	typedef State<VMS_MOUNTED> Mounted;
	typedef State<VMS_UNKNOWN> Unknown;

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

		const QString getName() const
		{
			return NULL == m_big ? QString() : m_big->getName();
		}

		void changeTray(const Tray::action_type& action_)
		{
			m_big->getConfigEditor()(action_);
		}

		template<class T>
		void pullToolsVersion(const T&)
		{
			pullToolsVersion();
		}

		template <class T>
		void pullToolsVersionAfterReboot(const T&)
		{
			pullToolsVersion(new ::Vm::Guest::Actor(m_big->getConfigEditor()));
		}

		// Pseudo-state
		struct Already: msmf::entry_pseudo_state<0>
		{
		};

		// Started
		struct Started: Details::Trace<Started>
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
			msmf::Row<Started, Conventional<VMS_STOPPING>, Stopping>,
			msmf::Row<Already, Conventional<VMS_STOPPING>, Stopping>,
			msmf::Row<Rebooted, Conventional<VMS_STOPPING>, Stopping>,
			a_irow<Started, Tray::action_type, &Running_::changeTray>,
			a_irow<Already, Tray::action_type, &Running_::changeTray>,
			a_irow<Rebooted, Tray::action_type, &Running_::changeTray>,
			msmf::Row<Stopping, Reboot, Rebooted>,
			msmf::Row
			<
				Started,
				Reboot,
				Rebooted
			>,
			msmf::Row
			<
				Already,
				NoAgent,
				Started
			>,
			msmf::Row
			<
				Started,
				NoAgent,
				msmf::none
			>,
			msmf::Row
			<
				Rebooted,
				Reboot,
				msmf::none
			>,
			a_irow<
				Started,
				Agent,
				&Running_::pullToolsVersion<Agent>
			>,
			a_row<
				Rebooted,
				Agent,
				Started,
				&Running_::pullToolsVersionAfterReboot<Agent>
			>,
			a_row<
				Rebooted,
				Switch,
				Started,
				&Running_::pullToolsVersionAfterReboot<Switch>
			>,
			a_row
			<
				Already,
				Conventional<VMS_RUNNING>,
				Started,
				&Running_::pullToolsVersion<Conventional<VMS_RUNNING> >
			>
		>
		{
		};

	private:
		void pullToolsVersion(::Vm::Guest::Actor* network_ = NULL)
		{
			WRITE_TRACE(DBG_INFO, "action guest tools on running for VM '%s'",
				qPrintable(getName()));

			::Vm::Guest::Actor *a = new ::Vm::Guest::Actor(m_big->getConfigEditor());
			::Vm::Guest::Watcher *p = new ::Vm::Guest::Watcher(m_big->getUuid());

			a->connect(p, SIGNAL(destroyed()), SLOT(deleteLater()));
			a->connect(p, SIGNAL(guestToolsStarted(const QString)),
					SLOT(setToolsVersionSlot(const QString)));
			if (NULL != network_)
			{
				network_->connect(p, SIGNAL(destroyed()), SLOT(deleteLater()));
				network_->connect(p, SIGNAL(guestToolsStarted(const QString)),
						SLOT(configureNetworkSlot(const QString)));
			}
			m_big->m_toolsState = p->getFuture();

			// usually guest agent is ready within 2..3 seconds after event
			// starting watcher earlier results in connect error logged
			p->startTimer(5000);
		}

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
		void operator()(const Event&, Frontend& fsm_, FromState&, Paused&)
		{
			WRITE_TRACE(DBG_INFO, "disabling guarantees for VM '%s'", qPrintable(fsm_.m_name));
			Vcmmd::Api(fsm_.getUuid()).deactivate();
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
			fsm_.getConfigEditor()(boost::bind
				(&configure<FromState, ToState>, _1, boost::ref(from_), boost::ref(to_)));
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
			s.connect(boost::bind(&Vm::Config::Repairer<Vm::Config::revise_types>::type::do_,
				_1, boost::cref(runtime)));
			s.connect(boost::bind(&Cluster::configure<FromState, ToState>, _1,
				boost::ref(from_), boost::ref(to_)));
			s.connect(boost::bind<void>(Vnc::Secure::Frontend(e, fsm_.getService()),
				_1, boost::cref(runtime)));
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

	msmf::Row<Unknown,    Conventional<VMS_PAUSED>,     Paused,
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

	msmf::Row<Suspended,  Conventional<VMS_PAUSED>,     Paused,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesUp, Lock, Notification> > >,

	msmf::Row<Suspended,  Conventional<VMS_RUNNING>,    Running,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, RoutesUp, Runtime, Lock, Notification> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Running,    Conventional<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, Cluster, BackupDisable, Unlock, Notification> > >,

	msmf::Row<Running,    Conventional<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, Cluster, BackupDisable, Unlock, Notification> > >,

	msmf::Row<Running,    Conventional<VMS_PAUSED>,     Paused,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Cluster, Notification> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Paused,     Conventional<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, BackupDisable, Unlock, Notification> > >,

	msmf::Row<Paused,     Conventional<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, BackupDisable, Unlock, Notification> > >,

	msmf::Row<Paused,     Conventional<VMS_RUNNING>,    Running::entry_pt<Running::Already>,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, Cluster, Notification> > >,

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

	msmf::Row<Reverting,  Conventional<VMS_PAUSED>,     Paused,
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

	PRL_VM_TOOLS_STATE getToolsState()
	{
		if (m_toolsState.has_value())
			return m_toolsState.get();
		boost::optional<CVmConfiguration> c = getConfig();
		if (!c)
			return PTS_NOT_INSTALLED;
		QString v = c->getVmSettings()->getVmTools()->getAgentVersion();
		return v.isEmpty() ? PTS_NOT_INSTALLED : PTS_POSSIBLY_INSTALLED;
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
	boost::future<PRL_VM_TOOLS_STATE> m_toolsState;
};

///////////////////////////////////////////////////////////////////////////////
// struct Machine

typedef boost::msm::back::state_machine<Frontend> Machine;

} // namespace State
} // namespace Vm

#endif // __CDSP_VM_STATE_MACHINE_H__
