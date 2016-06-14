///////////////////////////////////////////////////////////////////////////////
///
/// @file CVmMigrateTargetServer.cpp
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

#include <CVmMigrateTargetServer.h>
#include <CVmMigrateTargetDisk.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/IOService/IOCommunication/IORoutingTableHelper.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include <prlsdk/PrlDisk.h>
#include <prlxmlmodel/DiskDescriptor/CDiskXML.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

typedef SmartPtr<CVmConfiguration> config_type;

namespace
{
void cleanup_hdd_tasks(Migrate::Vm::Target::Disk::taskList_type& lstDiskObjTask)
{
	foreach (CVmMigrateTargetDisk* t, lstDiskObjTask)
	{
		delete t;
		WRITE_TRACE(DBG_DEBUG,"[Disk migration] task %p destroyed", t);
	}
	lstDiskObjTask.clear();
}

void get_harddisk_images(const config_type& config_, std::list<QString>& list)
{
	try {
		foreach (CVmHardDisk* d, config_->getVmHardwareList()->m_lstHardDisks)
		{
			if (d->getEnabled() && d->getEmulatedType() == PVE::HardDiskImage)
				list.push_back(d->getSystemName());
		}
	}
	catch ( std::bad_alloc& ) {
		WRITE_TRACE(DBG_FATAL,"[Disk migration] buiding image list exception");
		list.clear();
	}
}

PRL_RESULT InitializeHardDiskCfg
	(const config_type& config_, Migrate::Vm::Target::Disk::taskList_type* lstDiskTasks)
{
	PRL_RESULT err = PRL_ERR_SUCCESS;

	std::list<QString> lst_imagepath;
	get_harddisk_images(config_, lst_imagepath);

	/*
	* Check for existing items otherwise one item will be reserved for
	* lstDiskTasks and function return with PRL_ERR_OUT_OF_MEMORY
	* because of (1!=0) comparsion
	*/
	if ( 0 == lst_imagepath.size() ) {
		WRITE_TRACE(DBG_FATAL,"[Disk migration] no hard disks");
		return PRL_ERR_SUCCESS;
	}

	QScopedPointer<VirtualDisk::Qcow2> curr_disk;
	QString value;

	foreach (const QString& p, lst_imagepath)
	{
		QFile f(QDir(p).absoluteFilePath("DiskDescriptor.xml"));
		CDiskXML d(&f);
		value = d.getParameters()->getUserData()->getMigrationInstanceId();
		CFileHelper::ClearAndDeleteDir(p);
		VirtualDisk::Qcow2::create(p,
			VirtualDisk::qcow2PolicyList_type(1, d.getParameters()->getSize() * 512));
		curr_disk.reset(new VirtualDisk::Qcow2());
		err = curr_disk->open(p, PRL_DISK_WRITE);

		if (PRL_FAILED(err))
			break;

		err = PRL_ERR_ENTRY_ALREADY_EXISTS;
		if (lstDiskTasks->find(value) != lstDiskTasks->end())
			break;

		CVmMigrateTargetDisk *disk_task = new (std::nothrow)CVmMigrateTargetDisk();
		err = PRL_ERR_OUT_OF_MEMORY;
		if (disk_task == NULL)
			break;

		try {
			lstDiskTasks->insert(value, disk_task);
		} catch ( std::bad_alloc& ) {
			delete disk_task;
			break;
		}

		/*
		* NOTE: curr_disk should not be stored somewhere or accessed
		* after init() call. Since disk_task->init() call requires
		* disk reopening after recreation of BATS, more over
		* it is strongly required to open disk object with fair condition,
		* defined by VM configuration. So there is only one call for this
		* purposes HddOpenDisk(). But this call does not operate on
		* previously created IDisk object. Reopening through IDisk::Close/Open
		* sequence does not perform a lot additional actions around cache mode etc.
		*/

		if (PRL_FAILED(err = disk_task->init(curr_disk.take())))
			break;
	}
	return err;
}

//Notify clients about VM migration finished event
void NotifyClientWithCancel(
		const SmartPtr<IOPackage>&, // &p,
		const QString &, // sVmDirectoryUuid,
		const QString &sVmUuid)
{
	WRITE_TRACE(DBG_DEBUG, "VM %s migration cancelled (notifier)", qPrintable(sVmUuid));
	QCoreApplication::exit(-1);
}

} // namespace

/////////////////////////////////////////////////////////////////////////////
// class CVmMigrateTargetServer

CVmMigrateTargetServer::CVmMigrateTargetServer():
	m_ioServer(IORoutingTableHelper::GetServerRoutingTable(PSL_LOW_SECURITY), IOSender::Dispatcher),
	m_connection(m_ioServer)
{
	// Handle new client
	bool bConnected = QObject::connect( &m_ioServer,
						SIGNAL(onClientAttached(IOSender::Handle,
							const SmartPtr<IOPackage>)),
						SLOT(clientAttached(IOSender::Handle,
							const SmartPtr<IOPackage>)),
						Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	// Handle client disconnect
	bConnected = QObject::connect( &m_ioServer,
							SIGNAL(onClientDisconnected(IOSender::Handle)),
							SLOT(clientDisconnected(IOSender::Handle)),
							Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	// Connect to handle traffic report package
	bConnected = QObject::connect( &m_ioServer,
							SIGNAL(onPackageReceived(IOSender::Handle,
								const SmartPtr<IOPackage>)),
							SLOT(handlePackage(IOSender::Handle,
								const SmartPtr<IOPackage>)),
							Qt::DirectConnection );
	PRL_ASSERT(bConnected);
}

void CVmMigrateTargetServer::clientAttached ( IOSender::Handle h, const SmartPtr<IOPackage> p )
{
	WRITE_TRACE(DBG_DEBUG, "Client connected : handle=%s [%s]", qPrintable(h), p->buffers[0].getImpl());

	//Store VM migration connection handle
	if (!m_connection.setHandle(h))
	{
		WRITE_TRACE(DBG_FATAL, "VM migration connection already was attached!");
		return (void)m_ioServer.disconnectClient(h);
	}

	if ( p->header.type != VmMigrateStartCmd || p->header.buffersNumber != 1 )
	{
		WRITE_TRACE(DBG_FATAL, "Wrong attach package!");
		return m_connection.disconnect();
	}

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	if ( !pCmd->IsValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Wrong attach package: [%s]!", p->buffers[0].getImpl());
		return m_connection.disconnect();
	}
	ProcessStartMigrationPackage(pCmd, p);
}

void CVmMigrateTargetServer::handlePackage ( IOSender::Handle h, const SmartPtr<IOPackage> p )
{
	if (h != m_connection.getHandle())
	{
		WRITE_TRACE(DBG_FATAL, "Unauthorized request from '%s' connection received: [%s]",\
			qPrintable(h), p->header.buffersNumber == 1 ? p->buffers[0].getImpl() : "no data");
		return (void)m_ioServer.disconnectClient(h);
	}
	if (m_subject.isNull()) {
		WRITE_TRACE(DBG_FATAL, "m_pVmMigrateTarget is invalid, p->header.type %d", p->header.type);
		return;
	}
	WRITE_TRACE(DBG_DEBUG, "package %d", p->header.type);
	switch (p->header.type)
	{
	case FileCopyFinishCmd:
	case VmMigrateFinishCmd:
		WRITE_TRACE(DBG_DEBUG, "disconnect all because of finished");
		QObject::disconnect( &m_ioServer,
				SIGNAL(onClientDisconnected(IOSender::Handle)),
				this,
				SLOT(clientDisconnected(IOSender::Handle)));
		QObject::disconnect( &m_ioServer,
				SIGNAL(onClientAttached(IOSender::Handle, const SmartPtr<IOPackage>)),
				this,
				SLOT(clientAttached(IOSender::Handle, const SmartPtr<IOPackage>)));
		QObject::disconnect( &m_ioServer,
				SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
				this,
				SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)));

		WRITE_TRACE(DBG_DEBUG, "Asked local dispatcher to finalize migration");
		if (!send(IOPackage::duplicateInstance(p)))
		{
			m_subject.reset();
			NotifyClientWithCancel(p, QString(), m_sVmUuid);
		}
		break;
	default:
		if (PRL_FAILED((*m_subject)(p)))
		{
			m_subject.reset();
			QCoreApplication::exit(-1);
		}
	}
}

void CVmMigrateTargetServer::handlePackage(const SmartPtr<IOPackage> package_)
{
	WRITE_TRACE(DBG_DEBUG, "Got answer from local dispatcher");
	if (package_->header.type == VmMigrateFinishCmd)
	{
		WRITE_TRACE(DBG_DEBUG, "Migration is over");
		migrateFinish(package_);
		m_subject.reset();
		QCoreApplication::exit(0);
	}
	else if (package_->header.type == DispToDispResponseCmd)
	{
		WRITE_TRACE(DBG_DEBUG, "Target's dispatcher creates the response for the source");
		m_connection.send(IOPackage::duplicateInstance(package_));
		m_subject.reset();
		QCoreApplication::exit(-1);
	}
	else
	{
		WRITE_TRACE(DBG_DEBUG, "Failed to migrate Vm");
		CDispToDispCommandPtr d = CDispToDispProtoSerializer
			::CreateDispToDispResponseCommand(PRL_ERR_OPERATION_WAS_CANCELED, package_);
		(void)m_connection.send(DispatcherPackage::createInstance(
			d->GetCommandId(), d->GetCommand()->toString(), package_));

		m_subject.reset();
		QCoreApplication::exit(-1);
	}
}

bool CVmMigrateTargetServer::partialVmStart()
{
	return true;
}

void CVmMigrateTargetServer::clientDisconnected(IOSender::Handle h)
{
	WRITE_TRACE(DBG_WARNING, "%s", __FUNCTION__);

	/* VmDirectoryUuid is not used */
	WRITE_TRACE(DBG_FATAL, "Client was disconnected unexpectedly");

	//Cleanup migration connection handle
	if (h == m_connection.getHandle())
	{
		m_subject.reset();
		QCoreApplication::exit(-1);
	}
}

void CVmMigrateTargetServer::ProcessStartMigrationPackage(CDispToDispCommandPtr pCmd, const SmartPtr<IOPackage> &p)
{
	// !!! Attn : Vm config is empty now. Please add new code after ResetVmConfig() only
	CVmMigrateStartCommand *pVmMigrateStartCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(pCmd);

	CDispCommonPreferences d;
	d.fromString(pVmMigrateStartCmd->GetDispatcherConfig());

	using namespace Migrate::Vm::Target;
	Object::Unit u(*pVmMigrateStartCmd, d);
	PRL_RESULT e = u();
	if (PRL_FAILED(e))
	{
		CDispToDispCommandPtr d = CDispToDispProtoSerializer
					::CreateDispToDispResponseCommand(e, p);
		return (void)m_connection.send(DispatcherPackage::createInstance(
			d->GetCommandId(), d->GetCommand()->toString(), p));
	}
	WRITE_TRACE(DBG_DEBUG, "File copier will use this directory as a VM home: '%s'", qPrintable(u.getHome()));
	m_connection.setCopier(u.getUuid(), u.getHome(),
		d.getDispToDispPreferences()->getSendReceiveTimeout() * 1000);
	Object::Saviour s(pVmMigrateStartCmd->GetMigrationFlags(), pVmMigrateStartCmd->GetVmPrevState());
	m_subject.reset(new Subject(u, s, m_connection));
	m_sVmUuid = u.getUuid();

	CDispToDispCommandPtr pReply;
 	if ((pVmMigrateStartCmd->GetVersion() >= MIGRATE_DISP_PROTO_V3) && !partialVmStart()) {
		pReply = CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
			PRL_ERR_VM_START_FAILED, p);
		m_connection.send(DispatcherPackage::createInstance(
			pReply->GetCommandId(), pReply->GetCommand()->toString(), p));
		m_connection.disconnect();
		return;
	}

	pReply = CDispToDispProtoSerializer::CreateVmMigrateReply("");

	m_connection.send(DispatcherPackage::createInstance
		(pReply->GetCommandId(), pReply->GetCommand()->toString(), p));
}

void CVmMigrateTargetServer::migrateFinish(const SmartPtr<IOPackage>& p)
{
	if (m_subject.isNull())
		return;

	(*m_subject)(p);
}

bool CVmMigrateTargetServer::connectToDisp()
{
	if (!startListening())
		return false;

	m_client.reset(new IOClient(
			       IORoutingTableHelper::GetClientRoutingTable(PSL_HIGH_SECURITY),
			       IOSender::VmConverter,
			       ParallelsDirs::getDispatcherLocalSocketPath(),
			       0,
			       true));

	if (!QObject::connect(m_client.data(),
		     SIGNAL(onDetachedClientReceived(const SmartPtr<IOPackage>,
				     const IOCommunication::DetachedClient)),
		     SLOT(handleClient(const SmartPtr<IOPackage>,
				       const IOCommunication::DetachedClient)),
		     Qt::QueuedConnection))
		WRITE_TRACE(DBG_FATAL, "can't connect detached client handler");

	if (!QObject::connect(m_client.data(),
			SIGNAL(onPackageReceived(const SmartPtr<IOPackage>)),
			SLOT(handlePackage(const SmartPtr<IOPackage>))))
		WRITE_TRACE(DBG_FATAL, "can't connect package handler");


	m_client->connectClient();
	if (m_client->waitForConnection() != IOSender::Connected)
	{
		WRITE_TRACE(DBG_FATAL, "Unable to connect");
		return false;
	}

	return send(DispatcherPackage::createInstance(PVE::DspVmAuth, QString::number(::getpid())));
}

bool CVmMigrateTargetServer::send(const SmartPtr<IOPackage>& package_)
{
	IOSendJob::Handle j = m_client->sendPackage(package_);

	IOSendJob::Result r = m_client->waitForSend(j);
	if (r != IOSendJob::Success)
	{
		WRITE_TRACE(DBG_FATAL, "Handshake procedure failed: waitForSend() " \
			    "broken with code %d", r);
		return false;
	}

	r = m_client->getSendResult(j);
	if (r != IOSendJob::Success)
	{
		WRITE_TRACE(DBG_FATAL, "Handshake procedure failed: getSendResult() " \
			    "broken with code %d", r);
		return false;
	}

	WRITE_TRACE(DBG_DEBUG, "Migration app has been successfully connected");
	return true;
}

void CVmMigrateTargetServer::handleClient
	(const SmartPtr<IOPackage> p, const IOCommunication::DetachedClient detachedClient)
{
	if (p.isValid()
		&& p->header.type == VmMigrateStartCmd
		&& getIOServer().attachClient(detachedClient))
		return;

	WRITE_TRACE(DBG_FATAL, "Unable to attach client with package %u", p->header.type);
	QCoreApplication::exit(-1);
}

bool CVmMigrateTargetServer::startListening ()
{
	IOSender::State state =  m_ioServer.listen();
	return state == IOSender::Connected;
}

void CVmMigrateTargetServer::stopListening ()
{
	m_ioServer.disconnectServer();
}

bool CVmMigrateTargetServer::migrateCancel()
{
	if (m_connection.getHandle().isEmpty())//We do not have connection yet
	{
		WRITE_TRACE(DBG_FATAL, "No source side connection - couldn't to send cancel migration request");
		return false;
	}

	CDispToDispCommandPtr pCancelCmd = CDispToDispProtoSerializer
				::CreateDispToDispCommandWithoutParams(VmMigrateCancelCmd);
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance( pCancelCmd );
	SmartPtr<IOPackage> pResponse = m_connection.exchange(p);
	if (!pResponse.isValid())
	{
		return true;
	}
	PRL_ASSERT(DispToDispResponseCmd == pResponse->header.type);

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(pResponse);
	CDispToDispResponseCommand *pResponseCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
	PRL_ASSERT(pResponseCmd);
	return true;
}

namespace Migrate
{
namespace Vm
{
namespace Target
{
///////////////////////////////////////////////////////////////////////////////
// struct Connection

bool Connection::send(const package_type& package_)
{
	if (m_handle.isEmpty())
		return false;

	IOSendJob::Handle j = m_server->sendPackage(m_handle, package_);
	return IOSendJob::Success == m_server->waitForSend(j);
}

package_type Connection::exchange(const package_type& package_)
{
	if (m_handle.isEmpty())
		return package_type();

	IOSendJob::Handle j = m_server->sendPackage(m_handle, package_);
	IOSendJob::Result e = m_server->waitForSend(j);
	if (IOSendJob::Success != e)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to send the request. "
					"IOSendJob::Error=%u", e);
		return package_type();
	}
	e = m_server->waitForResponse(j);
	if (IOSendJob::Success != e)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to wait response on the request. "
					"IOSendJob::Error=%u", e);
		return package_type();
	}
	IOSendJob::Response r = m_server->takeResponse(j);
	if (IOSendJob::Success != r.responseResult)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to extract response on the request. "
					"IOSendJob::Error=%u", r.responseResult);
		return package_type();
	}
	return r.responsePackages[0];
}

void Connection::disconnect()
{
	if (!m_handle.isEmpty())
	{
		m_server->disconnectClient(m_handle);
		m_handle.clear();
	}
	if (m_copier.isValid())
	{
		m_copier->cancelOperation();
		m_copier = copier_type();
	}
	m_sender = sender_type();
}

bool Connection::setCopier(const QString& uuid_, const QString& home_, quint32 timeout_)
{
	if (m_handle.isEmpty())
		return false;

	m_sender = sender_type(new CVmFileListCopySenderServer(*m_server, m_handle));
	m_copier = copier_type(new CVmFileListCopyTarget(m_sender.data(), uuid_, home_, NULL, timeout_));
	m_copier->SetCancelNotifySender(NotifyClientWithCancel);
	return true;
}

bool Connection::setHandle(const QString& value_)
{
	if (!m_handle.isEmpty())
		return false;

	m_handle = value_;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct Subject

Subject::Subject(const Object::Unit& object_, const Object::Saviour& saviour_,
	Connection& connection_):
	m_disk(new Disk::Igniter(object_.getConfig()), saviour_.saveDisk()),
	m_default(connection_.getCopier()), m_tracking(object_.getConfig())
{
	QSharedPointer<Saviour::The> s(new Saviour::The(
		connection_.getCopier(), new Saviour::Disk(m_disk,
					saviour_.saveObject(object_))));

	m_map[VmMigrateCancelCmd] = command_type(new Cancel(connection_.getCopier()));
	m_map[VmMigrateMemPageCmd] = command_type(new Delegate<Memory::Main>
		(connection_, Memory::Main()));
	m_map[VmMigrateVideoMemViewCmd] = command_type(new Delegate<Memory::Video>
		(connection_, Memory::Video()));
	m_map[VmMigrateDiskBlockCmd] = command_type(new Delegate<Disk::Block>
		(connection_, Disk::Block(m_disk)));

	Finish::Local f(object_.getUuid(), s);
	m_map[VmMigrateFinishCmd] = command_type(new Finish::Return<Return<Finish::Global> >
		(Return<Finish::Global>(connection_, Finish::Global(f))));
	m_map[VmMigrateFreeGuestCmd] = command_type(new Return<Saviour::Command>
		(connection_, Saviour::Command(s)));
	m_map[VmMigrateFillDiskMapCmd] = command_type(new Delegate<Disk::Map::Fill>
		(connection_, Disk::Map::Fill(m_tracking)));
	m_map[VmMigrateMakeDiskMapCmd] = command_type(new Return<Disk::Map::Make>
		(connection_, Disk::Map::Make(m_tracking)));

	m_map[FileCopyFinishCmd] = command_type(new Finish::Return<File::Finish>
		(File::Finish(f, m_default)));
	m_map[FileCopyCancelCmd] = command_type(new File::Cancel(connection_.getCopier()));
}

Subject::~Subject()
{
}

PRL_RESULT Subject::operator()(const package_type& package_)
{
	map_type::iterator p;
	p = m_map.find(package_->header.type);
	if (m_map.end() != p)
		return p.value()->execute(package_);

	return m_default.execute(package_);
}

namespace Saviour
{
///////////////////////////////////////////////////////////////////////////////
// struct Decorator

PRL_RESULT Decorator::execute()
{
	if (m_decorated.isNull())
	{
		return PRL_ERR_SUCCESS;
	}

	return m_decorated->execute();
}

///////////////////////////////////////////////////////////////////////////////
// struct Kill

PRL_RESULT Kill::execute()
{
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Continue
/*
template<VmLocalEvent E>
PRL_RESULT Continue<E>::execute()
{
	return PRL_ERR_SUCCESS;
}
*/
///////////////////////////////////////////////////////////////////////////////
// struct Start

PRL_RESULT Start::execute()
{
	PRL_RESULT output = Decorator::execute();
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk

PRL_RESULT Disk::execute()
{
	PRL_RESULT e = m_subject->wait();
	if (PRL_FAILED(e))
		return e;

	e = Decorator::execute();
	return e;
}

///////////////////////////////////////////////////////////////////////////////
// struct Config

PRL_RESULT Config::execute()
{
	PRL_RESULT r = Decorator::execute();
	return r;
}

///////////////////////////////////////////////////////////////////////////////
// struct Factory

Component* Factory::operator()() const
{
	return new Saviour::Kill();
}

///////////////////////////////////////////////////////////////////////////////
// struct Log

Log::Log(const QString& home_, const QString& name_, Component* decorated_):
	Decorator(decorated_), m_home(home_), m_name(name_)
{
}

PRL_RESULT Log::execute()
{
	PRL_RESULT output = Decorator::execute();
	// only now will open parallels.log on shared Vm bundle,
	// so source side alredy closed this file
	// and we will not get open error (https://jira.sw.ru/browse/PSBM-11247)
	// on shared PCS
	SetLogFileName(qPrintable(m_home), qPrintable(m_name));
	Logger::ResetLogFile();
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct The

The::The(const copier_type& copier_, Component* saviour_):
	m_result(PRL_ERR_UNINITIALIZED), m_copier(copier_), m_saviour(saviour_)
{
}

void The::operator()()
{
	if (PRL_ERR_UNINITIALIZED != m_result)
		return;

	if (m_saviour.isNull() || PRL_SUCCEEDED(m_saviour->execute()))
	{
		m_result = PRL_ERR_SUCCESS;
	}
	else
	{
		CVmEvent e;
		e.setEventCode(PRL_ERR_VM_MIGRATE_RESUME_FAILED);
		m_copier->SetError(e);
		m_result = PRL_ERR_VM_MIGRATE_CONTINUE_START_FAILED;
	}
	m_saviour.reset();
}

} // namespace Saviour

namespace Finish
{
///////////////////////////////////////////////////////////////////////////////
// struct Local

PRL_RESULT Local::execute(const package_type&)// package_)
{
	getSaviour()();
	return getSaviour().getResult();
}

} // namespace Finish

namespace Memory
{
///////////////////////////////////////////////////////////////////////////////
// struct Main

PRL_RESULT Main::operator()(const package_type&)
{
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Video

PRL_RESULT Video::operator()(const package_type&)
{
	return PRL_ERR_SUCCESS;
}

} // namespace Memory

namespace Disk
{
///////////////////////////////////////////////////////////////////////////////
// struct Igniter

bool Igniter::operator()(taskList_type& taskList_)
{
	PRL_RESULT e = InitializeHardDiskCfg(m_config, &taskList_);
	if (PRL_SUCCEEDED(e))
		return true;

	cleanup_hdd_tasks(taskList_);
	WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while preparing HDD for "
				"migration with code [%#x][%s]", __FUNCTION__,
				e, PRL_RESULT_TO_STRING(e));

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// struct Saviour

Saviour::~Saviour()
{
}

PRL_RESULT Saviour::handle(const taskList_type& request_)
{
	if (m_successor.isNull())
		return PRL_ERR_SUCCESS;

	return m_successor->handle(request_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Wait

PRL_RESULT Wait::handle(const taskList_type& request_)
{
	foreach (CVmMigrateTargetDisk* t, request_)
	{
		t->wait_queued();
	}
	return Saviour::handle(request_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Result

PRL_RESULT Result::handle(const taskList_type& request_)
{
	foreach (CVmMigrateTargetDisk* t, request_)
	{
		PRL_RESULT e = t->getError();
		if (PRL_FAILED(e))
		{
			WRITE_TRACE(DBG_FATAL, "Some of virtual hdd disk migration "
				"failed with  with code [%#x][%s]", e,
				PRL_RESULT_TO_STRING(e));
			return e;
		}
	}
	return Saviour::handle(request_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Tracker

PRL_RESULT Tracker::handle(const taskList_type& request_)
{
	return Saviour::handle(request_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Subject

Subject::Subject(Igniter* igniter_, Saviour* saviour_):
	m_igniter(igniter_), m_saviour(saviour_)
{
}

Subject::~Subject()
{
	cleanup_hdd_tasks(m_taskList);
}

PRL_RESULT Subject::push(const QString& uuid_, const CVmMigrateDiskBlock_t& header_,
	const SmartPtr<char>& data_)
{
	// There is no defined place where virtual disk subsystem should be initialized
        // The one requirement is that virtual disk files did not opened for write on
	// source node shall be transmmitted to target node before any attempt to access them
	QScopedPointer<Igniter> g(m_igniter.take());
	if (!g.isNull() && !(*g)(m_taskList))
	{
		WRITE_TRACE(DBG_FATAL, "[%s] Wrong function param: Iginter:", __FUNCTION__);
		return PRL_ERR_FAILURE;
	}

	taskList_type::iterator p =  m_taskList.find(uuid_);
	if (m_taskList.end() == p)
	{
		WRITE_TRACE(DBG_FATAL, "[%s] Wrong package destination: %s lba: 0x%llX count: %u",
			__FUNCTION__, QSTR2UTF8(uuid_), header_.lba, header_.nsect);
		return PRL_ERR_FAILURE;
	}
	if (p.value()->isRunning())
	{
		// Write data asynchronously to this thread and do not care about result
		// from this thread point of view
		p.value()->add_data(header_, data_);
		return PRL_ERR_SUCCESS;
	}
	/*
	 * Ops!  Disk task eventloop has finished work, mostly due some error
	 * Drop data.
	 * Note: also we can remove task form list but this is requires synhronization with
	 * main thread, so let it as is and spam about it until source node will cancel migration
	 */
	WRITE_TRACE(DBG_FATAL, "[%s] Dropped package for %s lba: 0x%llX count: %u",
			__FUNCTION__, QSTR2UTF8(uuid_), header_.lba, header_.nsect);
	return PRL_ERR_FAILURE;
}

PRL_RESULT Subject::wait()
{
	PRL_RESULT output = PRL_ERR_SUCCESS;
	if (!m_saviour.isNull())
	{
		output = m_saviour->handle(m_taskList);
		m_saviour.reset();
	}
	// NB. after this call all the disk devices should be closed.
	cleanup_hdd_tasks(m_taskList);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Block

PRL_RESULT Block::operator()(const package_type& package_)
{
	quint32 z;
	SmartPtr<char> a, b;
	IOPackage::EncodingType t;
	package_->getBuffer(0, t, a, z);
	package_->getBuffer(1, t, b, z);
	if (!a.isValid() || !b.isValid())
	{
		return PRL_ERR_FAILURE;
	}
	CVmMigrateDiskBlock_t* h = (CVmMigrateDiskBlock_t*)a.getImpl();
	return m_subject->push(Uuid::fromGuid(h->disk_id).toString(), *h, b);
}

namespace Map
{
///////////////////////////////////////////////////////////////////////////////
// struct Fill

PRL_RESULT Fill::operator()(const package_type&)
{
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Make

PRL_RESULT Make::execute(const package_type&)
{
	return PRL_ERR_SUCCESS;
}

} // namespace Map
} // namespace Disk

namespace File
{
///////////////////////////////////////////////////////////////////////////////
// struct Handler

PRL_RESULT Handler::execute(const package_type& package_)
{
	m_chain->handlePackage(package_);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Finish

PRL_RESULT Finish::execute(const package_type& package_)
{
	PRL_RESULT output = Finish::Local::execute(package_);

	// handle package on FileCopy protocol after migrateFinish()
	// call so this function will set error on FileCopy level on
	// failure (https://jira.sw.ru/browse/PSBM-10300)
	m_handler.execute(package_);
	return output;
}

} // namespace File

namespace Object
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

Unit::Unit(CVmMigrateStartCommand& request_, const CDispCommonPreferences& dispatcher_):
	m_config(new CVmConfiguration(request_.GetVmConfig())), m_dispatcher(&dispatcher_)
{
	CVmIdentification* i = m_config->getVmIdentification();
	if (NULL == i)
		return;

	m_network.fromString(request_.GetNetworkConfig());

	m_home = QFileInfo(m_config->getVmIdentification()->getHomePath()).dir().absolutePath();
	m_uuid = i->getVmUuid();
}

PRL_RESULT Unit::operator()()
{
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Saviour

Saviour::Saviour(quint32 flags_, VIRTUAL_MACHINE_STATE return_):
	m_copy(true), m_flags(flags_), m_return(return_)
{
	m_copy = 0 == (flags_ & PVM_DONT_COPY_VM);
}

Target::Disk::Saviour* Saviour::saveDisk() const
{
	Disk::Saviour* a = new Disk::Result();
	//if (m_flags & PVMT_SEND_DISK_MAP)
	//	a->setSuccessor(new Disk::Tracker());

	Disk::Saviour* output = new Disk::Wait();
	output->setSuccessor(a);
	return output;
}

Target::Saviour::Component* Saviour::saveObject(const Unit& reference_) const
{
	Target::Saviour::Factory f(VMS_RUNNING == m_return, m_flags, reference_.getHome());
	Target::Saviour::Component* s = f();
	if (NULL == s)
		return NULL;

	QString p = QFileInfo(QDir(reference_.getHome()), VMDIR_DEFAULT_VM_CONFIG_FILE)
		.absoluteFilePath();
	if (!m_copy)
		return NULL;

	return new Target::Saviour::Config(p, reference_.getConfig(), s);
}

} // namespace Object
} // namespace Target
} // namespace Vm
} // namespace Migrate
