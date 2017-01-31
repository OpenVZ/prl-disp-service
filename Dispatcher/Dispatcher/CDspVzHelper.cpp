///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVzHelper.cpp
///
/// @author igor
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

#include "CDspVzHelper.h"
#include "CDspService.h"
#include "Tasks/Task_VzManager.h"
#include "CVmValidateConfig.h"

#ifdef _CT_
#include "Tasks/Task_VzStateMonitor.h"
# ifdef _WIN_
#   include "Tasks/Task_MigrateCt_win.h"
#   include "Tasks/Task_CopyCtTemplate_win.h"
# else
#   include "Tasks/Task_MigrateCtSource.h"
#   include "Tasks/Task_CopyCtTemplate.h"
# endif
#endif

#ifdef _LIN_
#include "Tasks/Task_ExecVm.h"
#endif
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"

#include "Dispatcher/Dispatcher/Cache/CacheImpl.h"
#include "Libraries/Virtuozzo/CVzPloop.h"


/*************** CDspVzHelper::CConfigCache *************/

QString get_conf_fname(const QString &sPath)
{
	return QString(sPath + "/ve.conf");
}

CDspVzHelper::CConfigCache::CConfigCache()
	:m_configCache( new Cache<CVmConfiguration>(/*1d ttl */ 1*24*60*60))
{
}

SmartPtr<CVmConfiguration> CDspVzHelper::CConfigCache::get_config(const QString &sHome,
		SmartPtr<CDspClient> pUserSession)
{
#ifdef _LIN_
	return m_configCache->getFromCache(get_conf_fname(sHome), pUserSession);
#else
	Q_UNUSED(sHome);
	Q_UNUSED(pUserSession);
	return SmartPtr<CVmConfiguration>();
#endif
}

void CDspVzHelper::CConfigCache::update(const QString &sHome,
		const SmartPtr<CVmConfiguration> &pConfig,
		SmartPtr<CDspClient> pUserSession)
{
#ifdef _LIN_
	m_configCache->updateCache(get_conf_fname(sHome), pConfig, pUserSession);
#else
	Q_UNUSED(sHome);
	Q_UNUSED(pConfig);
	Q_UNUSED(pUserSession);
#endif
}

void CDspVzHelper::CConfigCache::remove(const QString &sHome)
{
#ifdef _LIN_
	m_configCache->updateCache(get_conf_fname(sHome),
			SmartPtr<CVmConfiguration>(),
			SmartPtr<CDspClient>());
#else
	Q_UNUSED(sHome);
#endif
}

/**************** CDspVzHelper ************************/
CDspVzHelper::CDspVzHelper(CDspService& service_, const Backup::Task::Launcher& backup_):
	m_service(&service_), m_backup(backup_)
{
}

CDspVzHelper::~CDspVzHelper()
{
}

void CDspVzHelper::initVzStateMonitor()
{
        SmartPtr<CDspClient> pUser( new CDspClient(IOSender::Handle()) );
        pUser->getAuthHelper().AuthUserBySelfProcessOwner();

        const SmartPtr<IOPackage> p =
                DispatcherPackage::createInstance( PVE::DspCmdCtlDispatherFakeCommand );

#ifdef _CT_
	m_service->getTaskManager().schedule(new Task_VzStateMonitor( pUser, p ));
#endif
}

PRL_RESULT CDspVzHelper::insertVmDirectoryItem(SmartPtr<CVmConfiguration> &pConfig)
{
	PRL_ASSERT(pConfig);
	if (!pConfig) {
		WRITE_TRACE(DBG_FATAL, "Can't insert Container to VmDirectory: invalid config");
		return PRL_ERR_VM_GET_CONFIG_FAILED;
	}

	if (pConfig->getVmIdentification()->getVmUuid().isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Can't insert Container to VmDirectory: invalid ctid");
		return PRL_ERR_INVALID_PARAM;
	}

	CVmDirectoryItem *pItem = new CVmDirectoryItem;
	pItem->setCtId(pConfig->getVmIdentification()->getCtId());
	pItem->setVmUuid(pConfig->getVmIdentification()->getVmUuid());
	pItem->setVmName(pConfig->getVmIdentification()->getVmName());
	pItem->setVmHome(pConfig->getVmIdentification()->getHomePath());
	pItem->setVmType(PVT_CT);
	pItem->setValid( PVE::VmValid );
	pItem->setRegistered(PVE::VmRegistered);

	//
	// insert new item in user's VM Directory
	//
	PRL_RESULT res = m_service->getVmDirHelper().insertVmDirectoryItem(
				m_service->getVmDirManager().getVzDirectoryUuid(), pItem);
	if (PRL_FAILED(res))
	{
		delete pItem;
		if (res == PRL_ERR_ENTRY_ALREADY_EXISTS)
			// Ignore the PRL_ERR_ENTRY_ALREADY_EXISTS
			res = PRL_ERR_SUCCESS;
		else
			WRITE_TRACE(DBG_FATAL, ">>> Can't insert Container %s to VmDirectory by error %#x, %s",
				QSTR2UTF8(pConfig->getVmIdentification()->getVmUuid()),
				res, PRL_RESULT_TO_STRING( res ) );
	}

	return res;
}

PRL_RESULT CDspVzHelper::fillVzDirectory(CVmDirectory *pDir)
{

	QStringList lst;

	getVzlibHelper().get_envid_list(lst);

	foreach(QString uuid, lst)
	{
		SmartPtr<CVmConfiguration> pConfig = getVzlibHelper().
						get_env_config_by_ctid(uuid);
		if (!pConfig)
			continue;
		CVmDirectoryItem *pItem = new CVmDirectoryItem;
		pItem->setCtId(pConfig->getVmIdentification()->getCtId());
		pItem->setVmUuid(pConfig->getVmIdentification()->getVmUuid());
		pItem->setVmType(PVT_CT);
		pItem->setRegistered(PVE::VmRegistered);
		pItem->setVmName(pConfig->getVmIdentification()->getVmName());
		pItem->setVmHome(pConfig->getVmIdentification()->getHomePath());

		pDir->addVmDirectoryItem(pItem);
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspVzHelper::check_env_state(PRL_UINT32 nCmd, const QString &sUuid, CVmEvent *errEvt)
{
        VIRTUAL_MACHINE_STATE nState;
        PRL_RESULT res = getVzlibHelper().get_env_status(sUuid, nState);
        if (PRL_FAILED(res))
                return res;

        res = PRL_ERR_DISP_VM_COMMAND_CANT_BE_EXECUTED;

        switch (nCmd) {
        case PVE::DspCmdVmStop:
	case PVE::DspCmdVmPause:
                if (nState == VMS_RUNNING)
                        return PRL_ERR_SUCCESS;
				res = PRL_ERR_DISP_VM_IS_NOT_STARTED;
                break;
        case PVE::DspCmdVmStartEx:
        case PVE::DspCmdVmStart:
                if (nState != VMS_RUNNING)
                        return PRL_ERR_SUCCESS;
				res = PRL_ERR_DISP_VM_IS_NOT_STOPPED;
                break;
        case PVE::DspCmdDirUnregVm:
        case PVE::DspCmdDirVmDelete:
                if (nState == VMS_STOPPED || nState == VMS_SUSPENDED)
                        return PRL_ERR_SUCCESS;
                break;
        case PVE::DspCmdVmSuspend:
                if (nState == VMS_RUNNING)
                        return PRL_ERR_SUCCESS;
                break;
        case PVE::DspCmdVmResume:
                if (nState == VMS_SUSPENDED)
                        return PRL_ERR_SUCCESS;
                break;
	case PVE::DspCmdVmGuestRunProgram:
	case PVE::DspCmdVmLoginInGuest:
		if (nState == VMS_RUNNING)
			return PRL_ERR_SUCCESS;
		break;
	default:
                return PRL_ERR_SUCCESS;
        }

	if (errEvt != NULL) {
		errEvt->setEventCode( res );
		errEvt->addEventParameter( new CVmEventParameter (
					PVE::String,
					sUuid,
					EVT_PARAM_MESSAGE_PARAM_0 ) );
		if (res == PRL_ERR_DISP_VM_COMMAND_CANT_BE_EXECUTED)
			errEvt->addEventParameter( new CVmEventParameter (
					PVE::String,
					PRL_VM_STATE_TO_STRING( nState ),
					EVT_PARAM_MESSAGE_PARAM_1 ) );
	}

	WRITE_TRACE(DBG_FATAL, "Command %s can't be executed because Container"
			" '%s' is in '%s' state", PVE::DispatcherCommandToString(nCmd),
			QSTR2UTF8(sUuid), PRL_VM_STATE_TO_STRING(nState));

	return res;
}


bool CDspVzHelper::checkAccess(SmartPtr<CDspClient> &pUserSession)
{
	// FIXME:
	if (!pUserSession->getAuthHelper().isLocalAdministrator())
		return false;
	return true;
}

void CDspVzHelper::UpdateHardDiskInformation(SmartPtr<CVmConfiguration> &config)
{
	foreach(CVmHardDisk *d, config->getVmHardwareList()->m_lstHardDisks)
	{
		if (d->getEmulatedType() != PVE::HardDiskImage)
			continue;

		PloopImage::Image i(d->getUserFriendlyName());

		i.updateDiskInformation(*d);
	}
}

SmartPtr<CVmConfiguration> CDspVzHelper::getCtConfig(
		SmartPtr<CDspClient> pUserSession,
		const QString &sUuid,
		const QString &sHome,
		bool bFull)
{
	SmartPtr<CVmConfiguration> pConfig;
	pConfig = getConfigCache().get_config(sHome, pUserSession);
	if (!pConfig)
	{
		/* get From Disk */
		pConfig = getVzlibHelper().get_env_config(sUuid);
		if (!pConfig)
			return SmartPtr<CVmConfiguration>();
		/* update Cache */
		getConfigCache().update(sHome, pConfig, pUserSession);
	}

	if (bFull) {

		pConfig->getVmIdentification()->setVmFilesLocation(
				CFileHelper::GetVmFilesLocationType(pConfig->getVmIdentification()->getHomePath())
				);
		appendAdvancedParamsToCtConfig(pConfig);

		CVmEvent *evt = new CVmEvent;
		fillCtInfo(pUserSession, sUuid, *evt);
		pConfig->getVmSettings()->getVmRuntimeOptions()
			->getInternalVmInfo()->setParallelsEvent(evt);
		UpdateHardDiskInformation(pConfig);
	}

	return pConfig;
}

SmartPtr<CVmConfiguration> CDspVzHelper::getCtConfig(
		SmartPtr<CDspClient> pUserSession,
		const QString &sUuid,
		bool bFull)
{
	QString sHome;
	{
		CDspLockedPointer<CVmDirectoryItem> pDir =
			m_service->getVmDirHelper().getVmDirectoryItemByUuid(
					CDspVmDirManager::getVzDirectoryUuid(),
					sUuid);
		if (!pDir)
			return SmartPtr<CVmConfiguration>();
		sHome = pDir->getVmHome();
	}

	return getCtConfig(pUserSession, sUuid, sHome, bFull);
}

PRL_RESULT CDspVzHelper::getCtConfigList(SmartPtr<CDspClient> pUserSession,
		quint32 nFlags,
		QList<SmartPtr<CVmConfiguration> > &lstConfig)
{
	QString sServerUuid = m_service->getDispConfigGuard().getDispConfig()
			->getVmServerIdentification()->getServerUuid();

	CDspLockedPointer<CVmDirectory>	pDir = m_service->getVmDirManager()
							.getVzDirectory();
	if ( !pDir)
	{
		WRITE_TRACE(DBG_FATAL, "Virtuozzo Directory not found, skip Ct processing");
		return PRL_ERR_SUCCESS;
	}
	if (checkAccess(pUserSession))
	{
		foreach( CVmDirectoryItem* pDirItem, pDir->m_lstVmDirectoryItems )
		{
			SmartPtr<CVmConfiguration> pConfig;

			if (nFlags & PGVLF_GET_ONLY_IDENTITY_INFO)
				pConfig = CDspVmDirHelper::CreateVmConfigFromDirItem(sServerUuid, pDirItem);
			else
				pConfig = getCtConfig(pUserSession, pDirItem->getVmUuid(),
						pDirItem->getVmHome(), true);
			if (pConfig)
				lstConfig += pConfig;
		}
	}
	return PRL_ERR_SUCCESS;
}

QList<SmartPtr<CVmConfiguration> > CDspVzHelper::getAutoResumeCtList(
		 QList<SmartPtr<CVmConfiguration> > &configs)
{
	QList<SmartPtr<CVmConfiguration> > resumeList;

	foreach(SmartPtr<CVmConfiguration> pConfig, configs)
	{
		/* There no API to get the list of CT suspended in PRAM
		 * use low-level logic
		 */
		QFile f(getVzrebootMarkFile(pConfig));
		if (f.exists())
			resumeList += pConfig;
	}

	return resumeList;
}

QList<PRL_ALLOWED_VM_COMMAND> CDspVzHelper::getAllowedCommands()
{
        QList< PRL_ALLOWED_VM_COMMAND > lstAllowed;

        // FIXME:
	lstAllowed += PAR_VM_START_ACCESS;
	lstAllowed += PAR_VM_STOP_ACCESS;
	lstAllowed += PAR_VM_SUSPEND_ACCESS;
	lstAllowed += PAR_VM_RESUME_ACCESS;
	lstAllowed += PAR_VM_DELETE_ACCESS;
	lstAllowed += PAR_VM_GETCONFIG_ACCESS;
	lstAllowed += PAR_VM_GET_VMINFO_ACCESS;
	lstAllowed += PAR_VM_COMMIT_ACCESS;
	lstAllowed += PAR_VM_BEGINEDIT_ACCESS;

        return lstAllowed;
}

PRL_RESULT CDspVzHelper::fillCtInfo(SmartPtr<CDspClient> pUserSession,
		const QString& sVmUuid,
		CVmEvent& outVmEvent)
{
	PRL_RESULT res = PRL_ERR_SUCCESS;
	VIRTUAL_MACHINE_STATE vmState = VMS_UNKNOWN;

	(void) sVmUuid;

	res = getVzlibHelper().get_env_status(sVmUuid, vmState);

	outVmEvent.addEventParameter(
			new CVmEventParameter( PVE::Integer
				, QString("%1").arg( PRL_FAILED( res ) )
				, EVT_PARAM_VMINFO_VM_IS_INVALID )
			);
	outVmEvent.addEventParameter(
			new CVmEventParameter( PVE::Integer
				, QString("%1").arg( res )
				, EVT_PARAM_VMINFO_VM_VALID_RC )
			);

	outVmEvent.addEventParameter( new CVmEventParameter(PVE::Integer
				, QString("%1").arg(vmState)
				, EVT_PARAM_VMINFO_VM_STATE ));

	CVmSecurity Security;

	if (checkAccess(pUserSession))
		Security.getAccessControlList()->setAccessControl(getAllowedCommands());
	Security.setOwnerPresent( false );
	Security.setOwner(pUserSession->getAuthHelper().getUserName());
	Security.setAccessForOthers(PAO_VM_NOT_SHARED);

	outVmEvent.addEventParameter(
			new CVmEventParameter( PVE::String,
				Security.toString(),
				EVT_PARAM_VMINFO_VM_SECURITY )
			);

	/* FIXME */
	bool bIsVncServerStarted = isCtVNCServerRunning(sVmUuid);
	outVmEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt
				, QString("%1").arg(bIsVncServerStarted)
				, EVT_PARAM_VMINFO_IS_VNC_SERVER_STARTED ));

	VIRTUAL_MACHINE_ADDITION_STATE addidionalState = VMAS_NOSTATE;
	outVmEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt
				, QString("%1").arg(addidionalState)
				, EVT_PARAM_VMINFO_VM_ADDITION_STATE ));

	bool bIsVmWaitingForAnswer = false;
	outVmEvent.addEventParameter( new CVmEventParameter(PVE::UnsignedInt
				, QString("%1").arg(bIsVmWaitingForAnswer)
				, EVT_PARAM_VMINFO_IS_VM_WAITING_FOR_ANSWER ));

	return PRL_ERR_SUCCESS;
}

void CDspVzHelper::sendCtInfo(const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& pkg )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	QString uuid = cmd->GetVmUuid();

	CVmEvent evt;
	PRL_RESULT rc = fillCtInfo(pUserSession, uuid, evt);
	if( PRL_FAILED(rc) )
	{
		WRITE_TRACE(DBG_WARNING, "fillCtInfo failed: error #%x, %s", rc, PRL_RESULT_TO_STRING(rc) );
		pUserSession->sendSimpleResponse( pkg, rc );
		return;
	}

	////////////////////////////////////////////////////////////////////////
	// prepare response
	////////////////////////////////////////////////////////////////////////
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->SetVmEvent( evt.toString() );

	SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, pkg );
	m_service->getIOServer().sendPackage( sender, responsePkg );

	return;
}

void CDspVzHelper::sendCtConfigSample(const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& pkg )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	CVmConfiguration cfg;
	cfg.fromString(cmd->GetFirstStrParam());
	QString sSample = cfg.getCtSettings()->getConfigSample();

	int ret;
	SmartPtr<CVmConfiguration> pConfig = CVzHelper::get_env_config_sample(sSample, ret);
	if (PRL_FAILED(ret)) {
		pUserSession->sendSimpleResponse( pkg, ret );
		return;
	}

	pConfig->getCtSettings()->setConfigSample(sSample);

	////////////////////////////////////////////////////////////////////////
	// prepare response
	////////////////////////////////////////////////////////////////////////
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse *pResponseCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

	pResponseCmd->SetVmConfig( pConfig->toString() );

	SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, pkg );
	m_service->getIOServer().sendPackage( sender, responsePkg );

	return;
}

void CDspVzHelper::sendCtConfig(const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& pkg )
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( pkg );
	if ( ! cmd->IsValid() )
	{
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return;
	}

	sendCtConfigByUuid((const IOSender::Handle&)sender, pUserSession, pkg, cmd->GetVmUuid());
}

bool CDspVzHelper::sendCtConfigByUuid(const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& pkg,
		QString vm_uuid )
{

	SmartPtr<CVmConfiguration> pConfig = getCtConfig(pUserSession, vm_uuid, true);
	if (!pConfig) {
		pUserSession->sendSimpleResponse( pkg, PRL_ERR_FAILURE );
		return false;
	}

	////////////////////////////////////////////////////////////////////////
	// prepare response
	////////////////////////////////////////////////////////////////////////
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse *pResponseCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

	pResponseCmd->SetVmConfig( pConfig->toString() );

	SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, pkg );
	m_service->getIOServer().sendPackage( sender, responsePkg );

	return true;
}

void CDspVzHelper::beginEditConfig(const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& pkg )
{
	sendCtConfig(sender, pUserSession, pkg);
}

void CDspVzHelper::resetCtUptime(const QString &vm_uuid, SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	PRL_ASSERT( pUser.getImpl() );
	PRL_ASSERT( p.getImpl() );

	WRITE_TRACE(DBG_FATAL, "Resetting CT '%s' uptime", QSTR2UTF8(vm_uuid));

	int res = getVzlibHelper().reset_env_uptime(vm_uuid);
	if (res) {
		WRITE_TRACE(DBG_FATAL, "Error on resetting uptime of CT '%s'",
						QSTR2UTF8(vm_uuid));
		pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
	} else
		pUser->sendSimpleResponse( p, PRL_ERR_SUCCESS );
}

void CDspVzHelper::sendCtTemplateList(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	PRL_ASSERT( pUser.getImpl() );
	PRL_ASSERT( p.getImpl() );

	WRITE_TRACE(DBG_FATAL, "Retrieving Containers templates list");

	QList<SmartPtr<CtTemplate> > templates;
	int res = getVzTemplateHelper().get_templates(templates);
	if (res) {
		WRITE_TRACE(DBG_FATAL, "Error on retrieving list of Container"
				" templates");
		pUser->sendSimpleResponse(p, PRL_ERR_FAILURE);
		return;
	}

	QStringList lstCtTemplates;
	foreach(SmartPtr<CtTemplate> pTemplate, templates)
		lstCtTemplates.append(pTemplate->toString());

	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(p, PRL_ERR_SUCCESS);
	CProtoCommandDspWsResponse *pResponseCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetParamsList(lstCtTemplates);

	pUser->sendResponse(pCmd, p);
}

void CDspVzHelper::removeCtTemplate(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	PRL_ASSERT( pUser.getImpl() );
	PRL_ASSERT( p.getImpl() );

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if (!cmd->IsValid()) {
		WRITE_TRACE(DBG_FATAL, "ParseCommand failed");
		pUser->sendSimpleResponse( p, PRL_ERR_UNRECOGNIZED_REQUEST);
		return;
	}

	CProtoCommandWithTwoStrParams* pParam =
		CProtoSerializer::CastToProtoCommand<CProtoCommandWithTwoStrParams>(cmd);
	if (!pParam) {
		WRITE_TRACE(DBG_FATAL, "CProtoCommandWithOneStrParam failed");
		pUser->sendSimpleResponse( p, PRL_ERR_INVALID_ARG);
		return;
	}

	QString sName = pParam->GetFirstStrParam();
	QString sOsTmplName = pParam->GetSecondStrParam();
	if (sOsTmplName.isEmpty())
		WRITE_TRACE(DBG_INFO, "Removing Containers OS template \"%s\"",
			QSTR2UTF8(sName));
	else
		WRITE_TRACE(DBG_INFO, "Removing Containers application"
			" template \"%s\" for OS template \"%s\"",
			QSTR2UTF8(sName), QSTR2UTF8(sOsTmplName));

	PRL_RESULT res = getVzTemplateHelper().remove_template(sName,
			sOsTmplName);
	if (PRL_FAILED(res))
		WRITE_TRACE(DBG_FATAL, "Error on removing Container template"
				" \"%s\", res = %x", QSTR2UTF8(sName), res);
	pUser->sendSimpleResponse(p, res);
}

void CDspVzHelper::copyCtTemplate(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	PRL_ASSERT( pUser.getImpl() );
	PRL_ASSERT( p.getImpl() );
#ifdef _CT_
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if (!cmd->IsValid()) {
		WRITE_TRACE(DBG_FATAL, "ParseCommand failed");
		pUser->sendSimpleResponse( p, PRL_ERR_UNRECOGNIZED_REQUEST);
		return;
	}
	m_service->getTaskManager().schedule(new Task_CopyCtTemplateSource(pUser, cmd, p));
#endif
}

void CDspVzHelper::registerGuestSession(const IOSender::Handle& sender,
			SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() )
	{
		pUser->sendSimpleResponse( p, PRL_ERR_UNRECOGNIZED_REQUEST);
		return;
	}

	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( p, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse
		*pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->AddStandardParam(Uuid::createUuid().toString());

	SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, p );
	m_service->getIOServer().sendPackage( sender, responsePkg );
}

void CDspVzHelper::guestRunProgram(const IOSender::Handle& sender,
			SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p)
{
#ifdef _LIN_
	Q_UNUSED(sender);

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() )
	{
		pUser->sendSimpleResponse( p, PRL_ERR_UNRECOGNIZED_REQUEST);
		return;
	}
	// there no guest sessions for Container,
	// use VmUuid in the response.
	QString sUuid = cmd->GetVmUuid();

	CVmEvent evt;
	PRL_RESULT res = check_env_state(PVE::DspCmdVmGuestRunProgram, sUuid, &evt);
	if (PRL_FAILED(res)) {
		pUser->sendResponseError( &evt, p );
		return;
	}
	m_service->getTaskManager().schedule(new Task_ExecVm(pUser, p, Exec::Ct()));
#else
	Q_UNUSED(pUser);
	m_service->sendSimpleResponseToClient(sender, p, PRL_ERR_UNIMPLEMENTED);
#endif
}

// Handle Virtuozzo Container Command
// return is commend were processed sign
bool CDspVzHelper::handlePackage(const IOSender::Handle& h,
		SmartPtr<CDspClient> &pUserSession,
		const SmartPtr<IOPackage>& p)
{
	PRL_RESULT rc;
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand( p );
	if ( ! pCmd->IsValid() ) {
		WRITE_TRACE(DBG_FATAL, "Invalid command %d",
				p->header.type);
		return false;
	}

	PRL_VM_TYPE nType;
	QString vm_uuid;
	bool bOk = false;
	if (p->header.type == PVE::DspCmdDirVmCreate)
	{
		CProtoVmCreateCommand *pVmCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCreateCommand>(pCmd);
		CVmConfiguration cfg;

		cfg.fromString(pVmCmd->GetVmConfig());

		bOk = (PRL_SUCCEEDED(cfg.getValidRc()));
		nType = cfg.getVmType();
	}
	else if (p->header.type == PVE::DspCmdGetDefaultVmConfig)
	{
		CVmConfiguration cfg;

		cfg.fromString(pCmd->GetFirstStrParam());

		bOk = (PRL_SUCCEEDED(cfg.getValidRc()));
		nType = cfg.getVmType();
	}
	else if (p->header.type == PVE::DspCmdGetCtTemplateList ||
			p->header.type == PVE::DspCmdRemoveCtTemplate ||
			p->header.type == PVE::DspCmdCopyCtTemplate )
	{
		bOk = true;
		nType = PVT_CT;
	} else if (p->header.type == PVE::DspCmdDirVmEditCommit ||
		 p->header.type == PVE::DspCmdVmSectionValidateConfig ||
		 p->header.type == PVE::DspCmdVmCommitEncryption)
	{
		// VM uuid stored in the CVmConfiguration
		CVmConfiguration cfg;

		cfg.fromString(pCmd->GetFirstStrParam());

		vm_uuid = cfg.getVmIdentification()->getVmUuid();
		bOk = m_service->getVmDirManager().getVmTypeByUuid(vm_uuid, nType);
	}
	else if (p->header.type == PVE::DspCmdDirRegVm)
	{
	        QString sPath = pCmd->GetFirstStrParam();
		sPath += "/" VZ_CT_CONFIG_FILE;

		if (!QFile::exists(sPath))
			return false;
		bOk = true;
		nType = PVT_CT;
	}
	else
	{
		if (p->header.type == PVE::DspCmdCtReinstall)
			vm_uuid = pCmd->GetFirstStrParam();
		else
			vm_uuid = pCmd->GetVmUuid();

		if (vm_uuid.isEmpty())
			LOG_MESSAGE(DBG_INFO, "=> empty vmuuid for %s",
					PVE::DispatcherCommandToString(p->header.type));
		bOk = m_service->getVmDirManager().getVmTypeByUuid(vm_uuid, nType);
	}

	if (!(bOk && nType == PVT_CT))
		return false;

	WRITE_TRACE(DBG_FATAL, "Processing command '%s' %d for CT uuid='%s' ",
			PVE::DispatcherCommandToString(p->header.type),
			p->header.type,
			QSTR2UTF8( vm_uuid ) );

	if (!checkAccess(pUserSession))
	{
		m_service->
			sendSimpleResponseToClient( h, p, PRL_ERR_ACCESS_DENIED);
		return true;
	}

	/* Three way switch:
	 *  1) supported command
	 *  2) unsupporded command
	 *  3) generic command (not processed)
	 */
	switch( p->header.type )
	{
		// Supported Actions
		case PVE::DspCmdDirVmCreate:
		case PVE::DspCmdVmStart:
		case PVE::DspCmdVmStartEx:
		case PVE::DspCmdVmStop:
		case PVE::DspCmdVmSuspend:
		case PVE::DspCmdVmResume:
		case PVE::DspCmdDirVmDelete:
		case PVE::DspCmdDirVmEditCommit:
		case PVE::DspCmdDirRegVm:
		case PVE::DspCmdDirUnregVm:
		case PVE::DspCmdVmGuestSetUserPasswd:
		case PVE::DspCmdVmAuthWithGuestSecurityDb:
		case PVE::DspCmdDirVmClone:
		case PVE::DspCmdDirCreateImage:
		case PVE::DspCmdVmResizeDisk:
		case PVE::DspCmdVmMount:
		case PVE::DspCmdVmUmount:
		case PVE::DspCmdVmGetSnapshotsTree:
		case PVE::DspCmdVmCreateSnapshot:
		case PVE::DspCmdVmDeleteSnapshot:
		case PVE::DspCmdVmSwitchToSnapshot:
		case PVE::DspCmdVmStartVNCServer:
		case PVE::DspCmdVmStopVNCServer:
		case PVE::DspCmdDirVmMove:
		case PVE::DspCmdVmRestartGuest:
		case PVE::DspCmdVmGetPackedProblemReport:
		case PVE::DspCmdVmGetProblemReport:
		case PVE::DspCmdVmCommitEncryption:
		case PVE::DspCmdCtReinstall:
		case PVE::DspCmdVmPause:
			m_service->getTaskManager().schedule(new Task_VzManager( pUserSession, p));
			break;
		case PVE::DspCmdVmGuestRunProgram:
			guestRunProgram(h, pUserSession, p);
			break;
		case PVE::DspCmdGetVmInfo:
			sendCtInfo(h, pUserSession, p);
			break;
		case PVE::DspCmdDirVmEditBegin:
			beginEditConfig(h, pUserSession, p);
			break;
		case PVE::DspCmdVmGetConfig:
			sendCtConfig(h, pUserSession, p);
			break;
		case PVE::DspCmdVmGetStatistics:
			CDspStatCollectingThread::SendVmGuestStatistics(pCmd->GetVmUuid(),
				pUserSession, p );
			break;
		case PVE::DspCmdPerfomanceStatistics:
			CDspStatCollectingThread::ProcessPerfStatsCommand(pUserSession, p);
			break;
		case PVE::DspCmdVmSubscribeToGuestStatistics:
			rc = CDspStatCollectingThread::SubscribeToVmGuestStatistics(
					pCmd->GetVmUuid(), pUserSession);
			pUserSession->sendSimpleResponse(p, rc);
			break;
		case PVE::DspCmdVmUnsubscribeFromGuestStatistics:
			rc = CDspStatCollectingThread::UnsubscribeFromVmGuestStatistics(
					pCmd->GetVmUuid(), pUserSession);
			pUserSession->sendSimpleResponse(p, rc);
			break;
		case PVE::DspCmdVmResetUptime:
			resetCtUptime(vm_uuid, pUserSession, p);
			break;
		case PVE::DspCmdGetDefaultVmConfig:
			sendCtConfigSample(h, pUserSession, p);
			break;
		case PVE::DspCmdGetCtTemplateList:
			sendCtTemplateList(pUserSession, p);
			break;
		case PVE::DspCmdRemoveCtTemplate:
			removeCtTemplate(pUserSession, p);
			break;
		case PVE::DspCmdCopyCtTemplate:
			copyCtTemplate(pUserSession, p);
			break;
		case PVE::DspCmdVmLoginInGuest:
			registerGuestSession(h, pUserSession, p );
			break;
		case PVE::DspCmdVmGuestLogout:
			pUserSession->sendSimpleResponse(p, PRL_ERR_SUCCESS );
			break;
		// Fake PRL_ERR_SCCESS answer untill implemented
		case PVE::DspCmdVmSectionValidateConfig:
			CVmValidateConfig::validateSectionConfig(pUserSession, p);
			break;
		case PVE::DspCmdGetVmToolsInfo:
		case PVE::DspCmdGetVmVirtDevInfo:
			pUserSession->sendSimpleResponse( p, PRL_ERR_SUCCESS );
			break;
		case PVE::DspCmdDirVmMigrate:
#ifdef _CT_
			m_service->getTaskManager()
				.schedule(new Task_MigrateCtSource(pUserSession, pCmd, p));
#endif
			break;
		case PVE::DspCmdVmMigrateCancel:
			m_service->getVmMigrateHelper()
				.cancelMigration(pUserSession, p, pCmd->GetVmUuid());
			break;
		case PVE::DspCmdCreateVmBackup:
			m_backup.startCreateCtBackupSourceTask(pUserSession, p);
			break;
		case PVE::DspCmdBeginVmBackup:
			m_backup.launchBeginCtBackup(pUserSession, p);
			break;
		case PVE::DspCmdEndVmBackup:
			m_backup.launchEndVeBackup(pUserSession, p);
			break;
		case PVE::DspCmdRestoreVmBackup:
			m_backup.startRestoreVmBackupTargetTask(pUserSession, p);
			break;
		case PVE::DspCmdRemoveVmBackup:
			m_backup.startRemoveVmBackupSourceTask(pUserSession, p);
			break;
		case PVE::DspCmdGetBackupTree:
			m_backup.startGetBackupTreeSourceTask(pUserSession, p);
			break;
		case PVE::DspCmdVmGuestGetNetworkSettings:
			m_service->getTaskManager().schedule(new Task_VzManager( pUserSession, p));
			break;
		case PVE::DspCmdVmReset:
		case PVE::DspCmdVmSetConfig:
		case PVE::DspCmdDirLockVm:
		case PVE::DspCmdDirUnlockVm:
		case PVE::DspCmdDirVerifyVmConfig:
			m_service->
				sendSimpleResponseToClient( h, p, PRL_ERR_UNIMPLEMENTED);
			break;
		case PVE::DspCmdUserCancelOperation:
			m_service->getShellServiceHelper().cancelOperation(pUserSession, p);
			break;
		// Unsupported actions
		case PVE::DspCmdDirCopyImage:
		case PVE::DspCmdVmDropSuspendedState:
		case PVE::DspCmdVmDevConnect:
		case PVE::DspCmdVmDevGetState:
		case PVE::DspCmdVmDevDisconnect:
		case PVE::DspCmdVmDevChangeMedia:
		case PVE::DspCmdVmTaskRun:
		case PVE::DspCmdVmTaskGetState:
		case PVE::DspCmdVmTaskCancel:
		case PVE::DspCmdVmAnswer:
		case PVE::DspCmdVmGetLogData:
		case PVE::DspCmdVmGetMonitorState:
		case PVE::DspCmdVmInitiateDevStateNotifications:
		case PVE::DspCmdVmInstallTools:
		case PVE::DspCmdVmCancelCompact:
		case PVE::DspCmdVmGetSuspendedScreen:
		case PVE::DspCmdVmRunCompressor:
		case PVE::DspCmdVmCancelCompressor:
		case PVE::DspCmdVmFinishCompressorInternal:
		case PVE::DspCmdVmUpdateSnapshotData:
		case PVE::DspCmdVmInstallUtility:
		case PVE::DspCmdVmUpdateToolsSection:
		case PVE::DspCmdVmCreateDiskSnapshot:
		case PVE::DspCmdVmGuestChangeSID:
		case PVE::DspCmdVmCompact:
		case PVE::DspCmdVmSuspendCancel:
		case PVE::DspCmdVmChangeLogLevel:
		case PVE::DspCmdVmGuestSuspendHardDisk:
		case PVE::DspCmdVmGuestResumeHardDisk:
		case PVE::DspCmdVmChangeSid:
		case PVE::DspCmdVmInternal:
		case PVE::DspCmdVmConvertDisks:
		case PVE::DspCmdDirRestoreVm:
			m_service->
				sendSimpleResponseToClient( h, p, PRL_ERR_ACTION_NOT_SUPPORTED_FOR_CT);
			break;
		// Not handled (processed by other component)
		default:
			return false;
	}
	return true;
}

// Handle Dispatcher-Dispatcher command for Virtuozzo Container & Template
bool CDspVzHelper::handleToDispatcherPackage(
			SmartPtr<CDspDispConnection> pDispConnection,
			const SmartPtr<IOPackage>& p)
{
	PRL_ASSERT( pDispConnection.getImpl() );
	PRL_ASSERT( p.getImpl() );

	if (p->header.type != CopyCtTemplateCmd)
		return false;

#ifdef _CT_
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	if ( ! pCmd->IsValid() ) {
		WRITE_TRACE(DBG_FATAL, "Invalid command %d", p->header.type);
		return false;
	}
	m_service->getTaskManager()
		.schedule(new Task_CopyCtTemplateTarget(pDispConnection, pCmd, p));
#endif
	return true;
}

#ifdef _CT_
bool CDspVzHelper::addCtVNCServer(const QString &uuid, vncServer_type vncServer_)
{
	if (!vncServer_.isValid())
		return false;

	QMutexLocker locker(&m_tblCtVNCServerMtx);
	QHash<QString, vncServer_type>::iterator p = m_tblCtVNCServer.find(uuid);
	if (m_tblCtVNCServer.end() != p)
	{
		if (p.value()->IsRunning())
			return false;
		m_tblCtVNCServer.erase(p);
	}
	if (!vncServer_->IsRunning())
		return false;

	return m_tblCtVNCServer.end() != m_tblCtVNCServer.insert(uuid, vncServer_);
}

PRL_RESULT CDspVzHelper::removeCtVNCServer(const QString &uuid, bool onCtStop)
{
	QMutexLocker locker(&m_tblCtVNCServerMtx);

	PRL_RESULT rc = PRL_ERR_SUCCESS;
	QHash<QString, vncServer_type>::iterator it = m_tblCtVNCServer.find(uuid);

	if (it == m_tblCtVNCServer.end()) {
		if (onCtStop)
			return PRL_ERR_SUCCESS;
		WRITE_TRACE(DBG_FATAL, "Can not find VNC server for Container ID %s",
				QSTR2UTF8(uuid));
		return PRL_ERR_VNC_SERVER_NOT_STARTED;
	}

	vncServer_type vncs = it.value();
	rc = vncs->Terminate();
	if ((rc != PRL_ERR_SUCCESS) && (rc != PRL_ERR_VNC_SERVER_NOT_STARTED)) {
		WRITE_TRACE(DBG_FATAL, "Can't stop VNC");
		rc = PRL_ERR_FAILED_TO_STOP_VNC_SERVER;
	}
	m_tblCtVNCServer.erase(it);
	return rc;
}

void CDspVzHelper::removeAllCtVNCServer()
{
	QMutexLocker locker(&m_tblCtVNCServerMtx);

	QHash<QString, vncServer_type>::iterator it;
	for (it = m_tblCtVNCServer.begin(); it != m_tblCtVNCServer.end(); it = m_tblCtVNCServer.begin()) {
		vncServer_type vncs = it.value();
		vncs->Terminate();
		m_tblCtVNCServer.erase(it);
	}
}

bool CDspVzHelper::isCtVNCServerRunning(const QString &uuid)
{
	QMutexLocker locker(&m_tblCtVNCServerMtx);
	QHash<QString, vncServer_type>::iterator it = m_tblCtVNCServer.find(uuid);

	if (it == m_tblCtVNCServer.end())
		return false;

	if (it.value()->IsRunning())
		return true;

	m_tblCtVNCServer.erase(it);
	return false;
}

void CDspVzHelper::appendAdvancedParamsToCtConfig(SmartPtr<CVmConfiguration> pOutConfig)
{
	QString sUuid = pOutConfig->getVmIdentification()->getVmUuid();

	/* update VNC port */
	CVmRemoteDisplay *remDisplay = pOutConfig->getVmSettings()->getVmRemoteDisplay();
	if ( remDisplay->getMode() == PRD_AUTO ) {
		QMutexLocker locker(&m_tblCtVNCServerMtx);
		PRL_UINT32 port = 0;

		QHash<QString, vncServer_type>::iterator it = m_tblCtVNCServer.find(sUuid);

		if (it != m_tblCtVNCServer.end())
			port = it.value()->GetPort();

		remDisplay->setPortNumber(port);
	} else if ( remDisplay->getMode() == PRD_DISABLED ) {
		remDisplay->setPortNumber(0);
	}
	remDisplay->setWebSocketPortNumber(remDisplay->getPortNumber());
	remDisplay->setEncrypted(Vnc::Encryption(*CDspService::instance()->getQSettings().getPtr()).enabled());
}

#ifdef _LIN_
void CDspVzHelper::syncCtsUptime()
{
	typedef QPair<QString, QString> tuple_type;
	QList<tuple_type> a;
	QString u = m_service->getVmDirManager().getVzDirectoryUuid();
	CDspVmDirManager& m = m_service->getVmDirManager();
	{
		CDspLockedPointer<CVmDirectory> d = m.getVmDirectory(u);
		if (!d.isValid())
			return;
		foreach(CVmDirectoryItem* i, d->m_lstVmDirectoryItems)
		{
			a.push_back(qMakePair(i->getVmUuid(), i->getVmHome()));
		}
	}
	foreach(const tuple_type& t, a)
	{
		tribool_type run = CVzHelper::is_env_running(t.first);
		if (!run)
			continue;

		if (0 == CVzHelper::sync_env_uptime(t.first))
			m_configCache.remove(t.second);
	}
}
#endif // _LIN_

#endif
