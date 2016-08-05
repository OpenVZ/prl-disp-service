///////////////////////////////////////////////////////////////////////////////
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
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlxmlmodel/VmConfig/CVmRemoteDisplay.h>

#include <memory>

#include <boost/optional.hpp>
#include <stdint.h>

#define WAIT_VNC_SERVER_TO_START_OR_STOP_PROCESS (30*1000)
#define WAIT_TO_EXIT_VNC_SERVER_AFTER_START (3*1000)

namespace Vnc
{
namespace Secure
{
///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend
{
	Frontend(const ::Vm::Config::Edit::Atomic& commit_, CDspService& service_):
		m_service(&service_), m_commit(commit_)
	{
	}

	void operator()(CVmConfiguration& object_, const CVmConfiguration& runtime_) const;

private:
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
		PRL_UINT32 minPort );

	PRL_RESULT Start (
		const QString& app,
		const QString& sID,
		CVmRemoteDisplay *remDisplay,
		PRL_UINT32 minPort );

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
	bool state(QByteArray& key_, QByteArray& certificate_) const;
	PRL_RESULT setKey(const QString& value_);
	PRL_RESULT setCertificate(const QString& value_);
private:
	void set(const char* name_, const QString& value_);

	QSettings* m_storage;
};

} // namespace Vnc

#endif //CDSPVNCSTARTER_H
