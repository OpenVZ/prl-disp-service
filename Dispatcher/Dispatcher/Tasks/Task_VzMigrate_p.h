///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VzMigrate_p.h
///
/// vzmigrate related internal stuff
///
/// Copyright (c) 2016-2018, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __Task_VzMigrate_p_H_
#define __Task_VzMigrate_p_H_

#include <QVector>
#include <QProcess>
#include "CDspInstrument.h"
#include <prlcommon/Logging/Logging.h>
#include "Task_MigrateVmTunnel_p.h"
#include <prlcommon/Std/noncopyable.h>
#include "Task_DispToDispConnHelper.h"

class CDspTaskHelper;
namespace Migrate
{
namespace Ct
{
namespace mvp = Migrate::Vm::Pump;
namespace mvpp = mvp::Pull;
namespace mvppv = mvpp::Visitor;

namespace Flop
{
///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor: boost::static_visitor<PRL_RESULT>
{
	typedef Migrate::Vm::Flop::unexpected_type variant_type;

	PRL_RESULT operator()(boost::mpl::at_c<variant_type::types, 0>::type value_) const
	{
		return value_;
	}
	PRL_RESULT operator()(const boost::mpl::at_c<variant_type::types, 1>::type& value_) const
	{
		if (value_.isValid())
			return value_->getEventCode();

		return PRL_ERR_UNEXPECTED;
	}
};

} // namespace Flop

///////////////////////////////////////////////////////////////////////////////
// struct Pump

struct Pump: QObject
{
	Pump(const mvpp::Queue& queue_, const mvppv::Dispatch& dispatch_):
		m_queue(queue_), m_dispatch(dispatch_)
	{
	}

	void reactReceipt(const mvp::Fragment::bin_type& package_);

public slots:
	void reactBytesWritten(qint64 value_);

private:
	Q_OBJECT

	void setState(const mvpp::target_type& value_);

	mvpp::Queue m_queue;
	mvpp::state_type m_state;
	mvppv::Dispatch m_dispatch;
};

///////////////////////////////////////////////////////////////////////////////
// struct Hub

struct Hub: QObject
{
	typedef QPair<QLocalSocket* , Pump* > pump_type;
	typedef QHash<int, pump_type> pumps_type;
	typedef Prl::Expected<void, QPair<PRL_RESULT, QString> >
		result_type;

	Hub();

	void stopPump(int alias_);
	void startPump(int alias_, int socket_);
	result_type operator()(const mvp::Fragment::bin_type& package_);

private:
	pumps_type m_pumps;
};

namespace Handler
{
typedef QPair<mvp::Fragment::bin_type, boost::phoenix::expression::reference<QString>::type>
	input_type;

///////////////////////////////////////////////////////////////////////////////
// struct Error

struct Error: Instrument::Chain::Lsp<Error, input_type>
{
	explicit Error(const redo_type& redo_):
		Instrument::Chain::Lsp<Error, input_type>(redo_)
	{
	}

	bool filter(argument_type request_) const;
	result_type handle(argument_type request_);
	static result_type handleUnknown(argument_type request_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Fragment

struct Fragment: Instrument::Chain::Lsp<Fragment, input_type>
{
	Fragment(Hub::pumps_type& pumps_, const redo_type& redo_):
		Instrument::Chain::Lsp<Fragment, input_type>(redo_),
		m_pumps(&pumps_)
	{
	}

	bool filter(argument_type request_) const;
	result_type handle(argument_type request_);

private:
	Hub::pumps_type* m_pumps;
};

} // namespace Handler
} // namespace Ct
} // namespace Migrate

///////////////////////////////////////////////////////////////////////////////
// struct Task_HandleDispPackage

struct Task_HandleDispPackage: QThread
{
	Task_HandleDispPackage(
		IOSendJobInterface *pSendJobInterface,
		IOSendJob::Handle &hJob,
		QVector<int> &nFd);

	void run();
	/* wake up response waitings for task termination */
	inline IOSendJob::Result urgentResponseWakeUp()
	{
		return m_pSendJobInterface->urgentResponseWakeUp(m_hJob);
	}

public slots:
	void spin();

signals:
	void onDispPackageHandlerFailed(PRL_RESULT nRetCode, const QString &sErrInfo);

private:
	Q_OBJECT

	static Prl::Expected<IOSendJob::Response, PRL_RESULT>
		pull(IOSendJobInterface* gateway_, IOSendJob::Handle strand_);

	IOSendJobInterface *m_pSendJobInterface;
	IOSendJob::Handle m_hJob;
	QVector<int> &m_nFd;
	QVector<bool> m_bActiveFd;
	Migrate::Ct::Hub m_hub;
	QFutureWatcher<Prl::Expected<IOSendJob::Response, PRL_RESULT> >* m_watcher;
};

#endif // __Task_VzMigrate_p_H_

