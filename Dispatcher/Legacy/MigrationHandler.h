///////////////////////////////////////////////////////////////////////////////
///
/// @file MigrationHandler.h
///
/// Target task for Vm migration
///
/// Copyright (c) 2010-2016 Parallels IP Holdings GmbH
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

#ifndef __LEGACY_MIGRATIONHANDLER_H_
#define __LEGACY_MIGRATIONHANDLER_H_

#include <QObject>
#include <QProcess>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "CDspHandlerRegistrator.h"
#include "CDspDispConnection.h"
#include <prlcommon/IOService/IOCommunication/IOServerPool.h>
#include "VmConverter.h"

namespace Legacy
{
namespace Vm
{
namespace Migration
{
namespace Step
{
////////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	virtual ~Unit()
	{
	}

	virtual PRL_RESULT execute() = 0;
};

////////////////////////////////////////////////////////////////////////////////
// struct Start

struct Start: Unit
{
	explicit Start(const QString& uuid_): m_uuid(uuid_)
	{
	}

	PRL_RESULT execute();

private:
	QString m_uuid;
};

////////////////////////////////////////////////////////////////////////////////
// struct Convert

struct FirstStart: Unit
{
	FirstStart(Legacy::Vm::V2V &v2v, Unit* next_): m_v2v(v2v)
	{
		m_next.reset(next_);
	}

	PRL_RESULT execute();

private:
	const Legacy::Vm::V2V &m_v2v;
	QScopedPointer<Unit> m_next;
};

////////////////////////////////////////////////////////////////////////////////
// struct Convert

struct Convert: Unit
{
	Convert(Legacy::Vm::V2V &v2v, Unit* next_): m_v2v(v2v)
	{
		m_next.reset(next_);
	}

	PRL_RESULT execute();

private:
	const Legacy::Vm::V2V &m_v2v;
	QScopedPointer<Unit> m_next;
};

////////////////////////////////////////////////////////////////////////////////
// struct Vcmmd

struct Vcmmd: Unit
{
	Vcmmd(const QString& uuid_, const CVmConfiguration& config_, Unit* next_)
		: m_uuid(uuid_), m_config(&config_)
	{
		m_next.reset(next_);
	}

	PRL_RESULT execute();

private:
	QString m_uuid;
	const CVmConfiguration* m_config;
	QScopedPointer<Unit> m_next;
};

/////////////////////////////////////////////////////////////////////////////////
// struct Registration

struct Registration: Unit
{
	Registration(const QString& uuid_, const CVmConfiguration& config, Unit* next_)
		: m_uuid(uuid_), m_config(&config)
	{
		m_next.reset(next_);
	}

	PRL_RESULT execute();

private:
	QString m_uuid;
	const CVmConfiguration* m_config;
	QScopedPointer<Unit> m_next;
};

} // namespace Step

/////////////////////////////////////////////////////////////////////////////////
// Convoy

struct Convoy: QProcess
{
	Convoy(const SmartPtr<CDspDispConnection>& connection_,
			const SmartPtr<IOPackage>& package_)
		: m_connection(connection_), m_package(package_)
	{
	}

	bool appoint(const SmartPtr<CVmConfiguration>& config_);
	bool deport();
	void release(const IOSender::Handle& handle_, const SmartPtr<IOPackage>& package_);
	IOSender::Handle getConnection()
	{
		return m_connection->GetConnectionHandle();
	}

	void handlePackage(IOSender::Handle, const SmartPtr<IOPackage>);

private:
	SmartPtr<CDspDispConnection> m_connection;
	SmartPtr<IOPackage> m_package;
	QString m_uuid;
	SmartPtr<CVmConfiguration> m_config;
};

using namespace IOService;

/////////////////////////////////////////////////////////////////////////////////
// Handler

struct Handler: QObject
{
	Handler(IOServerPool& server_, const QSharedPointer<Convoy>& convoy_);

private slots:
	void packageReceived(IOSender::Handle, const SmartPtr<IOPackage>);
	void clientDetached(IOSender::Handle, const IOCommunication::DetachedClient);

private:
	Q_OBJECT

	IOServerPool* m_server;
	QSharedPointer<Convoy> m_convoy;
	IOSender::Handle m_client;
};

} // namespace Migration
} // namespace Vm
} // namespace Legacy

#endif // __LEGACY_MIGRATIONHANDLER_H_
