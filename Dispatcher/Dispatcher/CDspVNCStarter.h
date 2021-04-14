///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	CDspVNCStarter.h
///
/// @brief
///	Definition of the class CDspVNCStarter
///
/// @brief
///	Manages VNC server process: starts, terminates.
///
/// @author sergeyt
///	romanp@
///
/// @history
///
///////////////////////////////////////////////////////////////////////////////

#ifndef CDSPVNCSTARTER_H
#define CDSPVNCSTARTER_H

#include <QProcess>
#include <QObject>
#include <memory>
#include <CDspVmConfigManager.h>
#include <prlcommon/Interfaces/VirtuozzoNamespace.h>
#include <prlxmlmodel/VmConfig/CVmRemoteDisplay.h>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <stdint.h>

#define WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS (30*1000)
#define WAIT_TO_EXIT_VNC_SERVER_AFTER_START (3*1000)
#define WAIT_VNC_SERVER_TO_WRITE_AFTER_START (120*1000)

namespace Vnc
{
typedef QPair<quint32, quint32> range_type;

namespace Secure
{
struct Driver;

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: boost::noncopyable
{
	Frontend(const ::Vm::Config::Edit::Atomic& commit_, CDspService& service_);
	~Frontend();

	void setup(CVmConfiguration& object_, const CVmConfiguration& runtime_) const;
	void restore(CVmConfiguration& object_, const CVmConfiguration& runtime_) const;

private:
	void draw(CVmRemoteDisplay& object_, const CVmRemoteDisplay* runtime_,
		const range_type& playground_) const;

	Driver* m_rfb;
	Driver* m_websocket;
	CDspService* m_service;
	::Vm::Config::Edit::Atomic m_commit;
};

} // namespace Secure

namespace Starter
{
struct Unit;
} // namespace Starter
} // namespace Vnc

///////////////////////////////////////////////////////////////////////////////
// class CDspVNCStarter

class CDspVNCStarter : public QObject
{
	Q_OBJECT
public:
	CDspVNCStarter ();
	~CDspVNCStarter ();

	bool IsRunning();
	PRL_RESULT Terminate();

	PRL_RESULT Start (
		const QString& sVmUuid,
		CVmRemoteDisplay *remDisplay,
		::Vnc::range_type port );

	PRL_RESULT Start (
		const QString& app,
		const QString& sID,
		CVmRemoteDisplay *remDisplay,
		::Vnc::range_type port );

	PRL_UINT32 GetPort();
public slots:
	void doOnExit();
private:
	QMutex m_mutex;
	Vnc::Starter::Unit* m_impl;
	boost::optional<uintptr_t> m_ticket;
};

namespace Vnc
{
///////////////////////////////////////////////////////////////////////////////
// class Guard

class Guard: public QObject
{
	Q_OBJECT
public:
	bool attach(QProcess* process_);
	bool event(QEvent* event_);
signals:
	void stop();
private slots:
	void finish(int, QProcess::ExitStatus)
	{
		quit();
	}
private:
	void quit();

	std::auto_ptr<QProcess> m_process;
};

///////////////////////////////////////////////////////////////////////////////
// struct Encryption

struct Encryption
{
	explicit Encryption(QSettings& storage_);

	bool enabled() const;
	bool state(QByteArray& key_, QByteArray& certificate_, QString& fmt_) const;
	PRL_RESULT setKey(const QString& value_);
	PRL_RESULT setCertificate(const QString& value_);
private:
	void set(const char* name_, const QString& value_);

	QSettings* m_storage;
};

} // namespace Vnc

#endif //CDSPVNCSTARTER_H
