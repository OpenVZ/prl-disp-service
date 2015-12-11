////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2012-2015 Parallels IP Holdings GmbH
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

#include "CProtoSerializer.h"
#include "CDspClientManager.h"
#include "CDspService.h"
#include "CFileHelperDepPart.h"
#include "CDspHaClusterHelper.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/Std/PrlAssert.h"

using namespace Parallels;

Task_MoveVm::Task_MoveVm ( SmartPtr<CDspClient>& user, const SmartPtr<IOPackage>& p)
:CDspTaskHelper( user, p ),
m_nSteps(0)
{
	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	CProtoVmMoveCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMoveCommand>(cmd);

	m_sVmDirUuid = getClient()->getVmDirectoryUuid();
	m_sVmUuid = pCmd->GetVmUuid();
	m_sNewVmHome = pCmd->GetNewHomePath();
	m_nFlags = pCmd->GetCommandFlags();
}

Task_MoveVm::~Task_MoveVm()
{
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

	VIRTUAL_MACHINE_STATE vmState;

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

	sNewVmBundle = QString("%1/%2%3").arg(m_sNewVmHome).
		arg(m_pVmConfig->getVmIdentification()->getVmName()).arg(VMDIR_DEFAULT_BUNDLE_SUFFIX);

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
		CDspLockedPointer< CVmDirectoryItem > pVmDirItem =
			CDspService::instance()->getVmDirManager().getVmDirItemByUuid(m_sVmDirUuid, m_sVmUuid);

		if (!pVmDirItem) {
			WRITE_TRACE(DBG_FATAL, "Can't found VmDirItem for dirUuid %s, vmUuid = %s",
					QSTR2UTF8(m_sVmDirUuid), QSTR2UTF8(m_sVmUuid));
			nRetCode = PRL_ERR_OPERATION_FAILED;
			goto exit;
		}
		m_sOldVmConfigPath = pVmDirItem->getVmHome();
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

	vmState = CDspVm::getVmState( m_sVmUuid, m_sVmDirUuid );
	if ((VMS_SUSPENDED != vmState) && (VMS_STOPPED != vmState)) {
		WRITE_TRACE(DBG_FATAL, "Error: can't move Vm home, Vm state is forbidden! (state = %#x, '%s')",
				vmState, PRL_VM_STATE_TO_STRING( vmState ) );
		getLastError()->addEventParameter( new CVmEventParameter (
				PVE::String,
				m_pVmConfig->getVmIdentification()->getVmName(),
				EVT_PARAM_MESSAGE_PARAM_0 ) );
		getLastError()->addEventParameter( new CVmEventParameter (
				PVE::String,
				VMS_SUSPENDED == vmState ? "VMS_SUSPENDED" : "VMS_SUSPENDING_SYNC",
				EVT_PARAM_MESSAGE_PARAM_1 ) );
		nRetCode = PRL_ERR_DISP_VM_COMMAND_CANT_BE_EXECUTED;
		goto exit;
	}

	/* lock Vm exclusive parameters */
	m_pVmInfo = SmartPtr<CVmDirectory::TemporaryCatalogueItem>(new CVmDirectory::TemporaryCatalogueItem(
			m_sVmUuid, m_sNewVmConfigPath, m_pVmConfig->getVmIdentification()->getVmName()));

	nRetCode = CDspService::instance()->getVmDirManager()
			.lockExistingExclusiveVmParameters(m_sVmDirUuid, m_pVmInfo.getImpl());
	if (PRL_FAILED(nRetCode))
	{
		switch (nRetCode)
		{
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			WRITE_TRACE(DBG_FATAL, "path '%s' already registered", QSTR2UTF8(m_pVmInfo->vmXmlPath));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_MESSAGE_PARAM_1));
			break;
		default:
			WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
				QSTR2UTF8(m_pVmInfo->vmUuid), QSTR2UTF8(m_pVmInfo->vmName), QSTR2UTF8(m_pVmInfo->vmXmlPath));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
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

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QFileInfo fiOldVmBundle(m_sOldVmBundle);
	QFileInfo fiNewBundle(m_sNewVmBundle);
	QDir sourceDir(fiOldVmBundle.absoluteFilePath());
	QDir dir;
	QList<QFileInfo> dirList;
	QList<QFileInfo> itemList;
	QString sTarget;
	QString src_mp, dst_mp;

	WRITE_TRACE(DBG_DEBUG, "Move VM from %s to %s", QSTR2UTF8(m_sOldVmBundle),
			QSTR2UTF8(m_sNewVmBundle));

	// create destination directory if it doesn't exist
	if (!CFileHelper::DirectoryExists(m_sNewVmHome, &getClient()->getAuthHelper())) {
		if (!CFileHelper::WriteDirectory(m_sNewVmHome, &getClient()->getAuthHelper())) {
			nRetCode = PRL_ERR_OPERATION_FAILED;
			goto exit;
		}
	}

	src_mp = CFileHelper::GetMountPoint(m_sOldVmBundle);
	dst_mp = CFileHelper::GetMountPoint(QFileInfo(m_sNewVmBundle).absolutePath());
	if (src_mp.isEmpty() || dst_mp.isEmpty()) {
		nRetCode = PRL_ERR_OPERATION_FAILED;
		goto exit;
	}

	// if new location is on the same partition - just rename directory
	if (src_mp == dst_mp) {
		if (!CSimpleFileHelper::AtomicMoveFile(m_sOldVmBundle, m_sNewVmBundle)) {
			nRetCode = PRL_ERR_OPERATION_FAILED;
			goto exit;
		}
		m_nSteps |= MOVE_VM_SAME_PARTITION;

		nRetCode = postMoveActions();
		if (PRL_FAILED(nRetCode))
			goto exit;
		return PRL_ERR_SUCCESS;
	}

	// copy source dir content to target
	if (!sourceDir.mkpath(m_sNewVmBundle)) {
		WRITE_TRACE(DBG_FATAL, "Can't create directory %s", QSTR2UTF8(m_sNewVmBundle));
		nRetCode = PRL_ERR_OPERATION_FAILED;
		goto exit;
	}
	if (m_isFsSupportPermsAndOwner) {
		if (!fiNewBundle.permission(fiOldVmBundle.permissions()))
			WRITE_TRACE(DBG_FATAL,
				"Can not set permissions for %s", QSTR2UTF8(fiOldVmBundle.absoluteFilePath()));
		if (!CFileHelper::setOwnerByTemplate(
			m_sNewVmBundle, fiOldVmBundle.absoluteFilePath(), getClient()->getAuthHelper(), false))
				WRITE_TRACE(DBG_FATAL,
					"Can not set owner for %s", QSTR2UTF8(fiOldVmBundle.absoluteFilePath()));
	}
	dirList.append(fiOldVmBundle);
	while (dirList.size()) {
		dir.setPath(dirList.takeFirst().absoluteFilePath());
		itemList = dir.entryInfoList(
			QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
		for (int i = 0; i < itemList.size(); i++) {
			QFileInfo fi = itemList.at(i);
			nRetCode = handle_dir_item(fi, sourceDir, dirList);
			if (PRL_FAILED(nRetCode))
				goto exit;
		}
	}

	nRetCode = postMoveActions();
	if (PRL_FAILED(nRetCode))
		goto exit;

	// remove old bundle
	CFileHelper::ClearAndDeleteDir(m_sOldVmBundle);

#ifdef _LIBVIRT_
	if (!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
	{
		Libvirt::Result r = Libvirt::Kit.vms().at(getVmUuid()).setConfig(*m_pVmConfig);

		if (r.isFailed())
		{
			getLastError()->fromString(r.error().convertToEvent().toString());
			nRetCode = PRL_ERR_OPERATION_FAILED;
			goto exit;
		}
	}
#endif // _LIBVIRT_

	return PRL_ERR_SUCCESS;
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_MoveVm::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	if (PRL_FAILED(getLastErrorCode())) {
		if (!m_sNewVmBundle.isEmpty()) {
			if (m_nSteps & MOVE_VM_SAME_PARTITION)
				CFileHelper::AtomicMoveFile(m_sNewVmBundle, m_sOldVmBundle);
			else
				CFileHelper::ClearAndDeleteDir(m_sNewVmBundle);
		}
		if (m_nSteps & MOVE_VM_NEW_HOME_SET)
			setVmHome(m_sOldVmConfigPath);
	}

	// delete temporary registration
	if (m_nSteps & MOVE_VM_EXCL_OP_REGISTERED)
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			m_sVmUuid, m_sVmDirUuid, PVE::DspCmdDirVmMove, getClient());

	if (m_nSteps & MOVE_VM_EXCL_PARAMS_LOCKED)
		CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(m_pVmInfo.getImpl());

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

PRL_RESULT Task_MoveVm::handle_dir_item(const QFileInfo& fi, const QDir& sourceDir, QList<QFileInfo>& dirList)
{
	QString sTarget = m_sNewVmBundle + "/" + sourceDir.relativeFilePath(fi.absoluteFilePath());
	if ( sTarget.contains("../") ) {
		WRITE_TRACE(DBG_FATAL, "Path %s is out of Vm bundle %s",
					QSTR2UTF8(fi.absoluteFilePath()), QSTR2UTF8(m_sNewVmBundle));
		return PRL_ERR_OPERATION_FAILED;
	}
	if (fi.isSymLink()) {
		QString symLinkTarget;
#ifdef _WIN_
		symLinkTarget = QFile::symLinkTarget(fi.absoluteFilePath());
#else
		// QFile::symLinkTarget read _absolute_ path only
		char buf[PATH_MAX + 1];
		ssize_t sz = ::readlink(QSTR2UTF8(fi.absoluteFilePath()), buf, sizeof(buf));
		if (sz == -1) {
			WRITE_TRACE(DBG_FATAL, "readlink(%s) error %m", QSTR2UTF8(fi.absoluteFilePath()));
			return PRL_ERR_OPERATION_FAILED;
		}
		buf[(sz >= (ssize_t)sizeof(buf)) ? (ssize_t)sizeof(buf)-1 : sz] = '\0';
		symLinkTarget = QString("%1").arg(buf);
#endif
		WRITE_TRACE(DBG_DEBUG, "Create symlink %s on %s", QSTR2UTF8(sTarget), QSTR2UTF8(symLinkTarget));
		if (!QFile::link(symLinkTarget, sTarget)) {
			WRITE_TRACE(DBG_FATAL, "Can't create link %s on %s",
						QSTR2UTF8(sTarget), QSTR2UTF8(symLinkTarget));
			return PRL_ERR_OPERATION_FAILED;
		}
	} else {
		if (fi.isDir()) {
			WRITE_TRACE(DBG_DEBUG, "Create directory %s", QSTR2UTF8(sTarget));
			if (!sourceDir.mkpath(sTarget)) {
				WRITE_TRACE(DBG_FATAL, "Can't create directory %s", QSTR2UTF8(sTarget));
				return PRL_ERR_OPERATION_FAILED;
			}
			dirList.append(fi);
		} else {
			WRITE_TRACE(DBG_DEBUG, "Copy file %s to %s",
					QSTR2UTF8(fi.absoluteFilePath()), QSTR2UTF8(sTarget));
			PRL_RESULT retCode = CFileHelperDepPart::CopyFileWithNotifications(
						fi.absoluteFilePath(),
						sTarget,
						&getClient()->getAuthHelper(),
						this,
						PDE_GENERIC_DEVICE,
						0);
			if ( PRL_FAILED(retCode) ) {
				WRITE_TRACE(DBG_FATAL, "Copy %s to %s failed",
					QSTR2UTF8(fi.absoluteFilePath()), QSTR2UTF8(sTarget));
				return retCode;
			}
		}
		if (m_isFsSupportPermsAndOwner) {
			if (!QFileInfo(sTarget).permission(fi.permissions()))
				WRITE_TRACE(DBG_FATAL, "Can not set permissions for %s", QSTR2UTF8(sTarget));
		}
	}
	if (m_isFsSupportPermsAndOwner) {
		if (!CFileHelper::setOwnerByTemplate(
				sTarget, fi.absoluteFilePath(), getClient()->getAuthHelper(), false))
			WRITE_TRACE(DBG_FATAL, "Can not set owner for %s", QSTR2UTF8(sTarget));
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_MoveVm::setVmHome(const QString& path)
{
	PRL_RESULT res;
	CDspLockedPointer< CVmDirectoryItem > pVmDirItem =
		CDspService::instance()->getVmDirManager().getVmDirItemByUuid(m_sVmDirUuid, m_sVmUuid);

	if (!pVmDirItem) {
		WRITE_TRACE(DBG_FATAL, "Can't found VmDirItem for dirUuid %s, vmUuid = %s",
				QSTR2UTF8(m_sVmDirUuid), QSTR2UTF8(m_sVmUuid));
		return PRL_ERR_OPERATION_FAILED;
	}

	m_pVmConfig->getVmIdentification()->setHomePath(path);
	pVmDirItem->setVmHome(path);

	res = CDspService::instance()->getVmDirManager().updateVmDirItem(pVmDirItem);
	if (PRL_FAILED(res) ) {
		WRITE_TRACE(DBG_FATAL, "Can't update Container %s VmCatalogue by error: %s",
			QSTR2UTF8(m_sVmUuid), PRL_RESULT_TO_STRING(res));
	}
	return res;
}

PRL_RESULT Task_MoveVm::handleClusterResource()
{
	PRL_RESULT res = PRL_ERR_SUCCESS;
	CVmHighAvailability *ha = m_pVmConfig->getVmSettings()->getHighAvailability();
	if (!ha->isEnabled())
		return PRL_ERR_SUCCESS;

	bool dstShared = CFileHelper::isSharedFS(m_sNewVmBundle);
	SmartPtr<CDspHaClusterHelper> helper = CDspService::instance()->getHaClusterHelper();
	QString name = m_pVmConfig->getVmIdentification()->getVmName();

	if (CFileHelper::isSharedFS(QFileInfo(m_sOldVmBundle).path())) {
		if (dstShared) {
			res = helper->updateClusterResourcePath(name, m_sNewVmBundle);
		} else {
			res = helper->removeClusterResource(name);
		}
	} else if (dstShared) {
		res = helper->addClusterResource(name, ha, m_sNewVmBundle);
	}
	return res;
}

PRL_RESULT Task_MoveVm::postMoveActions()
{
	PRL_RESULT res = setVmHome(m_sNewVmConfigPath);
	if (PRL_FAILED(res))
		return res;
	m_nSteps |= MOVE_VM_NEW_HOME_SET;

	return handleClusterResource();
}
