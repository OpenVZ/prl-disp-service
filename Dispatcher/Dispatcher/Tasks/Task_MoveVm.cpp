////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2012-2017, Parallels International GmbH
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
/// @file
///	Task_MoveVm.cpp
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author krasnov@
///
////////////////////////////////////////////////////////////////////////////////

#include "Task_MoveVm.h"
#include "Task_MoveVm_p.h"
#include "CDspTaskManager.h"
#include <boost/foreach.hpp>
#include "Task_CreateImage.h"
#include "CDspTemplateFacade.h"
#include "CDspTemplateScanner.h"
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include "CDspClientManager.h"
#include "CDspService.h"
#include "CFileHelperDepPart.h"
#include "CDspHaClusterHelper.h"
#include "CDspVmBackupInfrastructure_p.h"
#include <prlcommon/HostUtils/PCSUtils.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/PrlCommonUtils/CVmMigrateHelper.h"

using namespace Parallels;

Task_MoveVm::Task_MoveVm(const SmartPtr<CDspClient>& user, const SmartPtr<IOPackage>& p,
	Vm::Directory::Ephemeral& ephemeral_):
	CDspTaskHelper(user, p), m_nSteps(), m_ephemeral(&ephemeral_)
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	CProtoVmMoveCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMoveCommand>(cmd);

	m_sVmUuid = pCmd->GetVmUuid();
	m_sNewVmHome = pCmd->GetNewHomePath();
	m_nFlags = pCmd->GetCommandFlags();
}

void Task_MoveVm::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	CDspTaskHelper::cancelOperation(pUserSession, p);
}

QString Task_MoveVm::getVmUuid()
{
	return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

PRL_RESULT Task_MoveVm::prepareTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QFileInfo fiNewHome(m_sNewVmHome);
	QFileInfo fiNewBundle;
	QString sNewVmBundle;
	CDspLockedPointer<CVmEvent> pParams = getTaskParameters();

	WRITE_TRACE(DBG_DEBUG, "Move Vm %s to %s, flags %u", QSTR2UTF8(m_sVmUuid), QSTR2UTF8(m_sNewVmHome), m_nFlags);

	m_isFsSupportPermsAndOwner = CFileHelper::isFsSupportPermsAndOwner(m_sNewVmHome);
	/*
	 * Checking
	 */
	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - VM bundle move rejected!");
		nRetCode = PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
		goto exit;
	}

	if( m_sNewVmHome.isEmpty() )
	{
		WRITE_TRACE(DBG_FATAL, "New Vm home is empty");
		nRetCode = PRL_ERR_INVALID_PARAM;
		goto exit;
	}
	if( !fiNewHome.isAbsolute() )
	{
		WRITE_TRACE(DBG_FATAL, "New Vm home path is not absolute : %s", QSTR2UTF8(m_sNewVmHome));
		nRetCode = PRL_ERR_VMDIR_PATH_IS_NOT_ABSOLUTE;
		goto exit;
	}

	m_pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(getClient(), m_sVmUuid, nRetCode);
	if ( PRL_FAILED(nRetCode) ) {
		WRITE_TRACE(DBG_FATAL,
			"CDspVmDirHelper::getVmConfigByUuid(%s) error : %d", QSTR2UTF8(m_sVmUuid), nRetCode);
		goto exit;
	} else if ( !m_pVmConfig ) {
		nRetCode = PRL_ERR_VM_GET_CONFIG_FAILED;
		WRITE_TRACE(DBG_FATAL, "Couldn't to get VM config for UUID '%s'", QSTR2UTF8(m_sVmUuid));
		goto exit;
	}

	m_pVmConfig->setRelativePath();

	sNewVmBundle = QString("%1/%2").arg(m_sNewVmHome).
		arg(Vm::Config::getVmHomeDirName(m_pVmConfig->getVmIdentification()->getVmUuid()));

	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, m_sVmUuid, EVT_PARAM_DISP_TASK_VM_UUID ) );
	pParams->addEventParameter(
		new CVmEventParameter( PVE::String, sNewVmBundle, EVT_PARAM_DISP_TASK_MOVE_NEW_HOME_PATH) );

	fiNewBundle.setFile(sNewVmBundle);
	if (fiNewBundle.exists())
	{
		WRITE_TRACE(DBG_FATAL, "New Vm path '%s' already exists", QSTR2UTF8(sNewVmBundle));
		nRetCode = PRL_ERR_FILE_OR_DIR_ALREADY_EXISTS;
		goto exit;
	}

	m_sNewVmBundle =  fiNewBundle.absoluteFilePath();
	m_sNewVmConfigPath = QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sNewVmBundle);
	/* check access */
	nRetCode = CDspService::instance()->getAccessManager().checkAccess(
			getClient(), PVE::DspCmdDirVmMove, m_sVmUuid, NULL, getLastError());
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while CDspAccessManager::checkAccess() with code [%#x][%s]",
			__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	{
		QPair<const QString, CDspLockedPointer<CVmDirectoryItem> > x =
			CDspService::instance()->getVmDirHelper()
				.getVmDirectoryItemByUuid(getClient(), m_sVmUuid);
		if (x.first.isEmpty() || x.second->getVmType() != PVT_VM) {
			WRITE_TRACE(DBG_FATAL, "Can't found VmDirItem for dirUuid %s, vmUuid = %s",
					QSTR2UTF8(m_sVmDirUuid), QSTR2UTF8(m_sVmUuid));
			nRetCode = PRL_ERR_OPERATION_FAILED;
			goto exit;
		}
		m_sVmDirUuid = x.first;
		m_sOldVmConfigPath = x.second->getVmHome();
		m_sOldVmBundle = QFileInfo(m_sOldVmConfigPath).absolutePath();
	}

	/*
	 * Locking
	 */
	nRetCode = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
			m_sVmUuid, m_sVmDirUuid, PVE::DspCmdDirVmMove, getClient());
	if ( PRL_FAILED(nRetCode) ) {
		WRITE_TRACE(DBG_FATAL, "[%s] registerExclusiveVmOperation failed. Reason: %#x (%s)",
			__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}
	m_nSteps |= MOVE_VM_EXCL_OP_REGISTERED;
	/* lock Vm exclusive parameters */
	m_pVmInfo = SmartPtr<CVmDirectory::TemporaryCatalogueItem>(new CVmDirectory::TemporaryCatalogueItem(
			m_sVmUuid, m_sNewVmConfigPath, m_pVmConfig->getVmIdentification()->getVmName()));

	nRetCode = CDspService::instance()->getVmDirManager()
			.lockExistingExclusiveVmParameters(m_sVmDirUuid, m_pVmInfo.getImpl());
	if (PRL_FAILED(nRetCode))
	{
		CDspTaskFailure f(*this);
		f.setCode(nRetCode);
		switch (nRetCode)
		{
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			WRITE_TRACE(DBG_FATAL, "path '%s' already registered", QSTR2UTF8(m_pVmInfo->vmXmlPath));
			f(m_pVmInfo->vmName, m_pVmInfo->vmXmlPath);
			break;
		default:
			WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
				QSTR2UTF8(m_pVmInfo->vmUuid), QSTR2UTF8(m_pVmInfo->vmName), QSTR2UTF8(m_pVmInfo->vmXmlPath));
			f.setToken(m_pVmInfo->vmUuid).setToken(m_pVmInfo->vmXmlPath)
				.setToken(m_pVmInfo->vmName)();
		}
		goto exit;
	}
	m_nSteps |= MOVE_VM_EXCL_PARAMS_LOCKED;
	return PRL_ERR_SUCCESS;

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_MoveVm::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT output = PRL_ERR_SUCCESS;
	WRITE_TRACE(DBG_DEBUG, "Move VM from %s to %s", qPrintable(m_sOldVmBundle),
			qPrintable(m_sNewVmBundle));

	// create destination directory if it doesn't exist
	if (!CFileHelper::DirectoryExists(m_sNewVmHome, &getClient()->getAuthHelper())) {
		if (!CFileHelper::WriteDirectory(m_sNewVmHome, &getClient()->getAuthHelper()))
			output = PRL_ERR_OPERATION_FAILED;
	}
	if (PRL_SUCCEEDED(output))
	{
		Chain::Move::Import x(*m_ephemeral,
			Chain::Move::Copy::Online::Nexus(
				Chain::Move::Rename(
					Chain::Move::Copy::Offline(m_isFsSupportPermsAndOwner))));
		output = x(Chain::Move::Request(*this, m_sNewVmConfigPath));
	}
	setLastErrorCode(output);
	return output;
}

void Task_MoveVm::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	if (m_nSteps & MOVE_VM_EXCL_PARAMS_LOCKED)
		CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(m_pVmInfo.getImpl());

	// delete temporary registration
	if (m_nSteps & MOVE_VM_EXCL_OP_REGISTERED)
	{
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			m_sVmUuid, m_sVmDirUuid, PVE::DspCmdDirVmMove, getClient());
	}
	// send response
	if ( PRL_FAILED( getLastErrorCode() ) ) {
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	} else {
		// send event to GUI for changing the config params
		CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, m_sVmUuid, PIE_DISPATCHER );
		SmartPtr<IOPackage> pkg =
			DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage() );
		CDspService::instance()->getClientManager().sendPackageToAllClients( pkg );

		CProtoCommandPtr pCmd =
			CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), getLastErrorCode() );
		CProtoCommandDspWsResponse*
			pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

		CDspService::instance()->getVmDirHelper().appendAdvancedParamsToVmConfig(getClient(), m_pVmConfig);
		pResponseCmd->SetVmConfig( m_pVmConfig->toString() );

		getClient()->sendResponse( pCmd, getRequestPackage() );
	}
}

namespace Command
{
namespace Move
{
///////////////////////////////////////////////////////////////////////////////
// struct Attribute

void Attribute::operator()(const QFileInfo& source_, const QFileInfo& target_)
{
	QString s = source_.absoluteFilePath();
	QString t = target_.absoluteFilePath();
	if (!target_.permission(source_.permissions()))
	{
		WRITE_TRACE(DBG_FATAL, "Can not set permissions for %s",
			qPrintable(s));
	}  
	if (!CFileHelper::setOwnerByTemplate(t, s, *m_auth, false))
	{
		WRITE_TRACE(DBG_FATAL, "Can not set owner for %s",
			qPrintable(t));
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Remove

Remove::result_type Remove::operator()()
{
	if (!CFileHelper::ClearAndDeleteDir(m_path))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot remove directory '%s'", qPrintable(m_path));
		return PRL_ERR_CANT_REMOVE_ENTRY;
	}

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Copy

Copy::result_type Copy::operator()(const QFileInfo& source_, const QFileInfo& target_)
{
	QList<QFileInfo>().swap(m_queue);
	PRL_RESULT e = process(source_, target_);
	if (PRL_FAILED(e))
		return e;

	QDir s(source_.absoluteFilePath());
	QDir t(target_.absoluteFilePath());
	while (!m_queue.isEmpty())
	{
		QDir x(m_queue.takeFirst().absoluteFilePath());
		foreach (const QFileInfo& i, x.entryInfoList(
			QDir::AllDirs | QDir::Files | QDir::Hidden |
			QDir::System | QDir::NoDotAndDotDot))
		{
			PRL_RESULT e = process(i, t.filePath(s
				.relativeFilePath(i.absoluteFilePath())));
			if (PRL_FAILED(e))
				return e;
		}
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Copy::process(const QFileInfo& source_, const QFileInfo& target_)
{
	QString s(source_.absoluteFilePath());
	QString t(target_.absoluteFilePath());
	CAuthHelper* a = &m_task->getClient()->getAuthHelper();
	if (source_.isDir())
	{
		WRITE_TRACE(DBG_DEBUG, "Create directory %s", QSTR2UTF8(t));
		if (!CFileHelper::WriteDirectory(t, a))
		{
			WRITE_TRACE(DBG_FATAL, "Can't create directory %s", QSTR2UTF8(t));
			return PRL_ERR_OPERATION_FAILED;
		}
		m_queue << source_;
	}
	else
	{
		WRITE_TRACE(DBG_DEBUG, "Copy file %s to %s", qPrintable(s), qPrintable(t));
		PRL_RESULT e = CFileHelperDepPart::CopyFileWithNotifications(
				s, t, a, m_task, PDE_GENERIC_DEVICE, 0);
		if (PRL_FAILED(e))
		{
			WRITE_TRACE(DBG_FATAL, "Copy %s to %s failed",
				qPrintable(s), qPrintable(t));
			return e;
		}
	}
	if (m_attribute)
		m_attribute.get()(source_, target_);

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Rename

Rename::result_type Rename::operator()()
{
	if (!CSimpleFileHelper::AtomicMoveFile(m_source, m_target))
		return PRL_ERR_OPERATION_FAILED;

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Directory

Directory::Directory(const CVmIdent& ident_):
	m_service(CDspService::instance()), m_ident(ident_)
{
}

Directory::result_type Directory::operator()(const QFileInfo& value_)
{
	CDspLockedPointer<CVmDirectoryItem> d = getManager()
		.getVmDirItemByUuid(m_ident);
	if (!d.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Can't found VmDirItem for dirUuid %s, vmUuid = %s",
				qPrintable(m_ident.second), qPrintable(m_ident.first));
		return PRL_ERR_OPERATION_FAILED;
	}

	d->setVmHome(value_.absoluteFilePath());
	PRL_RESULT output = getManager().updateVmDirItem(d);
	if (PRL_FAILED(output))
	{
		WRITE_TRACE(DBG_FATAL, "Can't update VM %s VmCatalogue: %s",
			qPrintable(m_ident.first), PRL_RESULT_TO_STRING(output));
	}
	return output;
}

CDspVmDirManager& Directory::getManager() const
{
	return m_service->getVmDirManager();
}

///////////////////////////////////////////////////////////////////////////////
// struct Config

Config::Config(const QString& directory_, const SmartPtr<CVmConfiguration>& config_):
	m_service(CDspService::instance()), m_directory(directory_), m_config(config_)
{
}

Config::result_type Config::operator()(const QFileInfo& value_)
{
	QString p = value_.absoluteFilePath();
	CVmIdentification* i = m_config->getVmIdentification();
	i->setHomePath(p);
	return getManager().saveConfig(m_config, p,
		CDspClient::makeServiceUser(m_directory));
}

CDspVmConfigManager& Config::getManager() const
{
	return m_service->getVmConfigManager();
}

///////////////////////////////////////////////////////////////////////////////
// struct Cluster

Cluster::Cluster(const SmartPtr<CVmConfiguration>& config_):
	m_service(CDspService::instance()), m_config(config_)
{
}

Cluster::result_type Cluster::operator()(const QString& source_, const QString& target_)
{
	CVmHighAvailability *a = m_config->getVmSettings()->getHighAvailability();
	if (!a->isEnabled())
		return PRL_ERR_SUCCESS;

	QString n = m_config->getVmIdentification()->getVmName();
	bool o = CFileHelper::isSharedFS(QFileInfo(source_).path());
	if (CFileHelper::isSharedFS(target_))
	{
		if (o)  
			return getHelper().updateClusterResourcePath(n, target_);

		return getHelper().addClusterResource(n, a, target_);
	}
	if (o)  
		return getHelper().removeClusterResource(n);

	return PRL_ERR_SUCCESS;
}

CDspHaClusterHelper& Cluster::getHelper() const
{
	return *m_service->getHaClusterHelper();
}

///////////////////////////////////////////////////////////////////////////////
// struct Libvirt

Libvirt::result_type Libvirt::operator()()
{
	SmartPtr<CVmConfiguration> x = m_task->getVmConfig();
	if (!x.isValid())
		return PRL_ERR_UNEXPECTED;

	if (x->getVmSettings()->getVmCommonOptions()->isTemplate())
		return PRL_ERR_SUCCESS;

#ifdef _LIBVIRT_
	::Libvirt::Result r = ::Libvirt::Kit.vms().at(m_task->getVmUuid()).setConfig(*x);
	if (r.isFailed())
	{
		CVmEvent e = r.error().convertToEvent();
		e.setEventCode(PRL_ERR_OPERATION_FAILED);
		return CDspTaskFailure(*m_task)(e);
	}
#endif // _LIBVIRT_

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Storage

PRL_RESULT Storage::import()
{
	PRL_RESULT e = m_catalog->import(m_source.fileName(),
		boost::bind(&Storage::do_, this, _1));
	if (PRL_FAILED(e))
		return e;

	return m_catalog->commit();
}

PRL_RESULT Storage::query()
{
	SmartPtr<CVmConfiguration> x = m_task->getVmConfig();
	if (!x.isValid())
		return PRL_ERR_UNINITIALIZED;

	QScopedPointer<const ::Template::Storage::Entry::Unit> u;
	PRL_RESULT e = m_catalog->find(m_source.fileName(), u);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "Cannot read the template folder");
		return e;
	}
	SmartPtr<CVmConfiguration> y = u->getConfig();
	if (!y.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot read the template config");
		return PRL_ERR_OPERATION_FAILED;
	}
	*x = *y;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Storage::sweep()
{
	PRL_RESULT e = m_catalog->unlink(m_source.fileName());
	if (PRL_FAILED(e))
		return e;

	return m_catalog->commit();
}

PRL_RESULT Storage::do_(const QFileInfo& target_)
{
	SmartPtr<CVmConfiguration> x = m_task->getVmConfig();
	if (!x.isValid())
		return PRL_ERR_UNINITIALIZED;

	Command::Move::Copy y(*m_task);
	result_type e = y(m_source, target_);
	if (PRL_FAILED(e))
		return e;

	QDir z(target_.absoluteFilePath());
	return x->saveToFile(z.filePath(VMDIR_DEFAULT_VM_CONFIG_FILE), true, true);
}

} // namespace Move
} // namespace Command

namespace Chain
{
namespace Move
{
///////////////////////////////////////////////////////////////////////////////
// struct Request

Request::Request(Task_MoveVm& context_, const QString& target_):
	base_type(QFileInfo(), QFileInfo(target_)), m_context(&context_)
{
	const SmartPtr<CVmConfiguration>& x = context_.getVmConfig();
	if (x.isValid())
		first = QFileInfo(x->getVmIdentification()->getHomePath());
}

CVmIdent Request::getObject() const
{
	return MakeVmIdent(m_context->getVmUuid(), m_context->getVmDirectory());
}

namespace Copy
{
///////////////////////////////////////////////////////////////////////////////
// struct Offline

Offline::result_type Offline::operator()(const request_type& request_)
{
	Instrument::Command::Batch b;
	QString s = request_.getSourcePrivate();
	QString t = request_.getTargetPrivate();

	Command::Move::Copy x(request_.getContext());
	if (m_attribute)
	{
		x.setAttribute(Command::Move::Attribute
			(request_.getContext().getClient()->getAuthHelper()));
	}
	b.addItem(boost::bind(x, QFileInfo(s), QFileInfo(t)),
		Command::Move::Remove(t));
	Command::Move::Directory d(request_.getObject());
	b.addItem(boost::bind(d, request_.getTargetConfig()),
		boost::bind<void>(d, request_.getSourceConfig()));
	Command::Move::Config c(request_.getContext().getVmDirectory(),
		request_.getContext().getVmConfig());
	b.addItem(boost::bind(c, request_.getTargetConfig()),
		boost::bind<void>(c, request_.getSourceConfig()));
	b.addItem(boost::bind(Command::Move::Cluster
		(request_.getContext().getVmConfig()), s, t));
	b.addItem(Command::Move::Remove(s));
	b.addItem(Command::Move::Libvirt(request_.getContext()));

	return b.execute();
}

namespace Online
{
namespace Special
{
///////////////////////////////////////////////////////////////////////////////
// struct Disk

PRL_RESULT Disk::execute(const QDir& target_)
{
	if (m_source.isEmpty())
		return PRL_ERR_UNINITIALIZED;

	namespace mb = ::Libvirt::Instrument::Agent::Vm::Block;
	QHash<QString, CVmHardDisk> m;
	foreach (const CVmHardDisk& i, m_source)
	{
		m[target_.absoluteFilePath(i.getSystemName())] = i;
	}
	mb::Completion c;
	mb::Activity a = ::Libvirt::Kit.vms().at(m_thread->getVmUuid())
			.getVolume().copy(m, c);
	c.wait();
	return a.stop();
}

PRL_RESULT Disk::prepare(const QDir& target_, CDspTaskManager& dispatcher_)
{
	typedef CDspTaskFuture<Task_CreateImage> future_type;

	QList<future_type> t;
	SmartPtr<CDspClient> c = m_thread->getClient();
	SmartPtr<IOPackage> p(DispatcherPackage::createInstance
		(PVE::DspCmdDirCreateImage, new CProtoCreateImageCommand()));
	foreach (CVmHardDisk i, m_source)
	{
		i.setSystemName(target_.absoluteFilePath(i.getSystemName()));
		i.setUserFriendlyName(i.getSystemName());
		t << dispatcher_.schedule(new Task_CreateImage(
			c, p, m_thread->getVmConfig(),
			ElementToString(&i, XML_VM_CONFIG_EL_HARD_DISK),
			true, false));
	}
	PRL_RESULT output = PRL_ERR_SUCCESS;
	BOOST_FOREACH(future_type& f, t)
	{
		PRL_RESULT e = f.wait().getTask()->getLastErrorCode();
		if (PRL_FAILED(e) && PRL_SUCCEEDED(output))
			output = e;
	}

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Facade

PRL_RESULT Facade::prepare(const QDir& target_)
{
	return m_disk.prepare(target_, CDspService::instance()->getTaskManager());
}

} // namespace Special

///////////////////////////////////////////////////////////////////////////////
// struct Regular

Regular::result_type Regular::operator()(const QDir& target_)
{
	foreach (itemList_type::const_reference f, m_folders)
	{
		if (!CFileHelper::WriteDirectory
			(target_.absoluteFilePath(f.second), getAuth()))
			return PRL_ERR_MAKE_DIRECTORY;
	}
	int i = 0;
	foreach (itemList_type::const_reference f, m_files)
	{
		PRL_RESULT e = CFileHelperDepPart::CopyFileWithNotifications(
			f.first.absoluteFilePath(),
			target_.absoluteFilePath(f.second),
			getAuth(), m_thread, PDE_CLUSTERED_DEVICE, i++);
		if (PRL_FAILED(e))
			return e;
	}

	return PRL_ERR_SUCCESS;
}

CAuthHelper* Regular::getAuth() const
{
	return &(m_thread->getClient()->getAuthHelper());
}

///////////////////////////////////////////////////////////////////////////////
// struct Estimate

Estimate::Estimate(): m_regular(PRL_ERR_UNINITIALIZED), m_special(PRL_ERR_UNINITIALIZED)
{
}

PRL_RESULT Estimate::operator()(const Request& request_)
{
	typedef Regular::itemList_type itemList_type;
	itemList_type d, f;
	QString s(request_.getSourcePrivate());

	m_regular = regular_type(PRL_ERR_FAILURE);
	m_special = special_type(PRL_ERR_FAILURE);
	PRL_RESULT e = CVmMigrateHelper::GetEntryListsVmHome(s, d, f);
	if (PRL_FAILED(e))
		return e;

	m_special = Special::Facade(request_.getContext());
	foreach (CVmHardDisk* d, ::Backup::Object::Model
		(request_.getContext().getVmConfig()).getImages())
	{
		if (PVE::DeviceConnected != d->getConnected())
			continue;

		QString n(d->getSystemName());
		if (QFileInfo(n).isAbsolute())
		{
			if (!n.startsWith(s))
				continue;

			n = QDir(s).relativeFilePath(n);
		}
		m_special.value().account(*d);
		itemList_type::iterator p = std::find_if(f.begin(), f.end(),
			boost::bind(&itemList_type::value_type::second, _1) ==
				boost::cref(n));
		if (f.end() != p)
			f.erase(p);
	}
	m_regular = Regular(d, f, request_.getContext());

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Nexus

Nexus::result_type Nexus::operator()(const request_type& request_)
{
	VIRTUAL_MACHINE_STATE s = CDspVm::getVmState(request_.getObject());
	if (!(VMS_RUNNING == s || s == VMS_PAUSED))
		return Unit<request_type>::operator()(request_);

	Estimate x;
	PRL_RESULT e = x(request_);
	if (PRL_FAILED(e))
		return e;

	Instrument::Command::Batch b;
	QString t = request_.getTargetPrivate();
	b.addItem(boost::bind(x.getRegular().value(), QDir(t)),
		Command::Move::Remove(t));
	b.addItem(boost::bind(&Special::Facade::prepare, x.getSpecial().value(), QDir(t)));
	Command::Move::Directory d(request_.getObject());
	b.addItem(boost::bind(d, request_.getTargetConfig()),
		boost::bind<void>(d, request_.getSourceConfig()));
	Command::Move::Config c(request_.getContext().getVmDirectory(),
		request_.getContext().getVmConfig());
	b.addItem(boost::bind(c, request_.getTargetConfig()),
		boost::bind<void>(c, request_.getSourceConfig()));
	b.addItem(boost::bind(Command::Move::Cluster(request_.getContext().getVmConfig()),
		request_.getSourcePrivate(), t));
	b.addItem(boost::bind(&Special::Facade::execute, x.getSpecial().value(), QDir(t)));
	b.addItem(Command::Move::Remove(request_.getSourcePrivate()));
	b.addItem(Command::Move::Libvirt(request_.getContext()));

	return b.execute();
}

} // namespace Online
} // namespace Copy

///////////////////////////////////////////////////////////////////////////////
// struct Rename

Rename::result_type Rename::operator()(const request_type& request_)
{
	QString s = request_.getSourcePrivate();
	QString x = CFileHelper::GetMountPoint(s);
	if (x.isEmpty())
		return PRL_ERR_OPERATION_FAILED;

	QString t = request_.getTargetPrivate();
	QDir p(t);
	if (!p.cdUp())
		return PRL_ERR_UNEXPECTED;

	QString y = CFileHelper::GetMountPoint(p.absolutePath());
	if (y.isEmpty())
		return PRL_ERR_OPERATION_FAILED;

	if (x != y || pcs_fs(qPrintable(y)))
		return Unit<request_type>::operator()(request_);

	Instrument::Command::Batch b;
	b.addItem(Command::Move::Rename(s, t), boost::bind<void>(Command::Move::Rename(t, s)));
	Command::Move::Directory d(request_.getObject());
	b.addItem(boost::bind(d, request_.getTargetConfig()),
		boost::bind<void>(d, request_.getSourceConfig()));
	Command::Move::Config c(request_.getContext().getVmDirectory(),
		request_.getContext().getVmConfig());
	b.addItem(boost::bind(c, request_.getTargetConfig()),
		boost::bind<void>(c, request_.getSourceConfig()));
	b.addItem(boost::bind(Command::Move::Cluster(request_.getContext().getVmConfig()), s, t));
	b.addItem(Command::Move::Libvirt(request_.getContext()));

	return b.execute();
}

///////////////////////////////////////////////////////////////////////////////
// struct Import

namespace tf = Template::Facade;
namespace ts = Template::Storage;

Import::result_type Import::operator()(const request_type& request_)
{
	SmartPtr<CVmConfiguration> x = request_.getContext().getVmConfig();
	if (!x.isValid())
		return PRL_ERR_UNINITIALIZED;

	ts::Dao::pointer_type p;
	ts::Dao d(request_.getContext().getClient()->getAuthHelper());
	PRL_RESULT e = d.findForEntry(request_.getSourcePrivate(), p);
	if (PRL_SUCCEEDED(e))
		return PRL_ERR_VM_REQUEST_NOT_SUPPORTED;

	QString a = request_.getTargetPrivate();
	e = d.findForEntry(a, p);
	if (PRL_FAILED(e))
		return Unit<request_type>::operator()(request_);

	if (!x->getVmSettings()->getVmCommonOptions()->isTemplate())
		return PRL_ERR_VM_REQUEST_NOT_SUPPORTED;

	tf::Workbench w;
	QString r = p->getRoot().absolutePath();
	tf::Host h(*m_ephemeral, w);
	e = h.insert(r);
	if (PRL_FAILED(e) && e != PRL_ERR_VM_DIR_CONFIG_ALREADY_EXISTS)
		return e;

	boost::optional<QString> u(m_ephemeral->find(r));
	if (!u) 
		return PRL_ERR_UNEXPECTED;

	Template::Scanner::Folder f(tf::Folder(u.get(), w), p.take());
	f.run();
	e = d.findByRoot(r, p);
	if (PRL_FAILED(e))
		return e;

	Instrument::Command::Batch b;
	tf::Registrar y(u.get(), *x, w);
	x->getVmIdentification()->setHomePath(request_.getTargetConfig().absoluteFilePath());
	b.addItem(boost::bind(&tf::Registrar::begin, &y), boost::bind(&tf::Registrar::rollback, &y));
	Command::Move::Storage s(request_.getContext(), request_.getSourcePrivate(), p.take());
	b.addItem(boost::bind(&Command::Move::Storage::import, s),
		boost::bind(&Command::Move::Storage::sweep, s));
	b.addItem(boost::bind(&Command::Move::Storage::query, s));
	b.addItem(Command::Move::Remove(request_.getSourcePrivate()));
	b.addItem(boost::bind(&CDspVmDirHelper::deleteVmDirectoryItem,
		&w.getDirectoryHelper(), request_.getContext().getVmDirectory(),
			request_.getContext().getVmUuid()));
	b.addItem(boost::bind(&tf::Registrar::execute, &y));
	b.addItem(boost::bind(&tf::Registrar::commit, &y));

	return b.execute();
}

} // namespace Move
} // namespace Chain

