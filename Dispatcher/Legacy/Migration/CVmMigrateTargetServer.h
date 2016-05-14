///////////////////////////////////////////////////////////////////////////////
///
/// @file CVmMigrateTargetServer.h
///
/// Copyright (c) 2016 Parallels IP Holdings GmbH
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

#ifndef CVmMigrateTargetServer_H
#define CVmMigrateTargetServer_H

#include <prlcommon/IOService/IOCommunication/IOServer.h>
#include <Libraries/VmFileList/CVmFileListCopy.h>
#include <prlxmlmodel/NetworkConfig/CParallelsNetworkConfig.h>
#include <prlxmlmodel/DispConfig/CDispatcherConfig.h>

#include <CVmMigrateTargetDisk.h>
#include <CVmMigrateTargetServer_p.h>

class CVmMigrateTargetServer : public QObject
{
	Q_OBJECT

public:
	/**
	 * Class default constructor - creates and initializes IO server object
	 * that should processing VM migration requests
	 */
	CVmMigrateTargetServer();

	/**
	 * Returns reference to internal IO server object
	 */
	IOServer& getIOServer()
	{
		return m_ioServer;
	}

	/**
	 * Startups server
	 * @return sign whether startup procedure was successful
	 */
	bool connectToDisp();
	bool startListening();
	void stopListening();

	/**
	 * Lets to cancel migrate procedure
	 * @param ppointer to the cancel migration request package object
	 */
	bool migrateCancel();
	void migrateFinish(const SmartPtr<IOPackage>& pRequest);
	bool isRunning() const
	{
		return !m_subject.isNull();
	}

signals:
	void finished(int);

private slots://Couple of slots to process IO server events
	/**
	 * Slot that processes client attach event
	 * @param handle of attached connection
	 * @param pointer initial VM migration request package object
	 */
	void clientAttached(IOSender::Handle h, const SmartPtr<IOPackage> p);

	/**
	 * Slot that handles VM migration client disconnect event
	 * @param handle to disconnected connection
	 */
	void clientDisconnected(IOSender::Handle h);

	/**
	 * Slot that processes received from VM migration connection package
	 * @param handle to VM migration connection
	 * @param pointer to received package object
	 */
	void handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
	void handlePackage(const SmartPtr<IOPackage>);
	void handleClient(const SmartPtr<IOPackage>, const IOCommunication::DetachedClient);

private:
	/**
	 * Processes start VM migration request
	 * @param pointer to the start VM migration command object
	 * @param pointer to the start VM migration request package
	 */
	void ProcessStartMigrationPackage(CDispToDispCommandPtr pCmd, const SmartPtr<IOPackage>& p);

	/* partial Vm initialization for hot-mode migration */
	bool partialVmStart();
	bool send(const SmartPtr<IOPackage>&);

private:
	QString m_sVmUuid;
	/**
	 * IO server object
	 */
	IOServer m_ioServer;
	QScopedPointer<IOClientInterface> m_client;

	Migrate::Vm::Target::Connection m_connection;
	QScopedPointer<Migrate::Vm::Target::Subject> m_subject;
};

#endif//CVmMigrateTargetServer_H
