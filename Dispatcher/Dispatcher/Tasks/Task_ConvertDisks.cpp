///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_ConvertDisks.cpp
///
/// Dispatcher task for doing conversion VM image disks.
///
/// @author myakhin@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////////

#include "Task_ConvertDisks.h"


#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "CDspService.h"
#include "CDspVmSnapshotStoreHelper.h"
#include "EditHelpers/CMultiEditMergeVmConfig.h"

QMutex						Task_ConvertDisks::s_lockConvertDisks;
QMap<CVmIdent , QString >	Task_ConvertDisks::s_mapVmIdTaskId;

Task_ConvertDisks::Task_ConvertDisks(const SmartPtr<CDspClient>& pClient,
									 const SmartPtr<IOPackage>& p,
									 const QString& qsVmUuid,
									 PRL_UINT32 nDiskMask,
									 PRL_UINT32 nFlags)
: CDspTaskHelper(pClient, p),
  m_vmIdent(MakeVmIdent(qsVmUuid, pClient->getVmDirectoryUuid())),
  m_nDiskMask(nDiskMask),
  m_nFlags(nFlags),
  m_bVmConfigWasChanged(false),
  m_flgLockRegistered(false),
  m_pProcess(0),
  m_nCurHddIndex((unsigned int )-1),
  m_nCurHddItemId(0)
{
}

Task_ConvertDisks::~Task_ConvertDisks()
{
	if ( m_pProcess )
	{
		delete m_pProcess;
		m_pProcess = 0;
	}
}

PRL_RESULT Task_ConvertDisks::prepareTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		// Check on cancel
		if ( isTaskShutdown() )
		{
			cancelAndWait();
			throw PRL_ERR_SUCCESS;
		}

		// Get VM config

		m_pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(m_vmIdent, ret);
		if ( ! m_pVmConfig && PRL_SUCCEEDED(ret) )
		{
			PRL_ASSERT(0);
			ret = PRL_ERR_FAILURE;
		}
		if ( PRL_FAILED(ret) )
			throw ret;

		// Deny dangerous operations

		ret = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
				m_vmIdent.first,
				m_vmIdent.second,
				(PVE::IDispatcherCommands) getRequestPackage()->header.type, getClient());
		if (PRL_FAILED(ret))
			throw ret;

		m_flgLockRegistered = true;

		// One task per VM
		lockConvertDisks();

		// Check VM state

		VIRTUAL_MACHINE_STATE nVmState = CDspVm::getVmState(m_vmIdent);
		if ( nVmState != VMS_STOPPED && nVmState != VMS_SUSPENDED )
		{
			WRITE_TRACE( DBG_FATAL, "Convert disks: [Prepare] VM has state %s. VM uuid = %s",
							PRL_VM_STATE_TO_STRING(nVmState),
							QSTR2UTF8(m_vmIdent.first) );
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String, m_pVmConfig->getVmIdentification()->getVmName(),
										EVT_PARAM_MESSAGE_PARAM_0) );
			throw PRL_ERR_CONV_HD_WRONG_VM_STATE;
		}

		// Check disk tool

		QDir pwd = QDir( (QCoreApplication::instance())->applicationDirPath() );
		m_qsDTName = ParallelsDirs::getDiskToolPath( pwd );

		if ( ! QFile::exists(m_qsDTName) )
		{
			WRITE_TRACE(DBG_FATAL, "Convert disks: Configuration tool '%s' not found",
									QSTR2UTF8(m_qsDTName) );
			throw PRL_ERR_DISK_TOOL_NOT_FOUND;
		}

		// Success

		m_pProcess = new QProcess;
		connect( m_pProcess, SIGNAL(readyReadStandardError()), this, SLOT(onReadyReadStandardError()),
			Qt::DirectConnection );
		connect( m_pProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyReadStandardOutput()),
			Qt::DirectConnection);
		connect( m_pProcess,
			SIGNAL(finished(int , QProcess::ExitStatus)), this,
			SLOT(onFinished(int, QProcess::ExitStatus)),
			Qt::DirectConnection);
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while convert disks with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	getLastError()->setEventCode( ret );

	return ret;
}

PRL_RESULT Task_ConvertDisks::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if ( PRL_FAILED( getLastErrorCode() ) )
			throw getLastErrorCode();

		if ( isTaskShutdown() )
			throw PRL_ERR_SUCCESS;

		ret = convertDisks();
		if (PRL_FAILED(ret))
			throw ret;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while convert disks with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	setLastErrorCode( ret );

	return ret;
}

void Task_ConvertDisks::finalizeTask()
{
	if (m_bVmConfigWasChanged)
	{
		// Save VM configuration for synchronize split and disk type info
		// (safe because under exclusive operation)

		// TODO: Add merge mech in edit VM configuration

		QString qsVmHome;
		{
			CDspLockedPointer<CVmDirectoryItem> pDirItem
				= CDspService::instance()->getVmDirHelper()
					.getVmDirectoryItemByUuid(m_vmIdent.second, m_vmIdent.first);
			if (pDirItem.isValid())
				qsVmHome = pDirItem->getVmHome();
		}

		if ( ! qsVmHome.isEmpty() )
		{
			QMutexLocker lock(CDspService::instance()->getVmDirHelper().getMultiEditDispatcher());
			const IOSender::Handle
				hFakeClientHandle = QString("%1-%2").arg( m_vmIdent.second ).arg( m_vmIdent.first );

			CDspService::instance()->getVmDirHelper()
				.getMultiEditDispatcher()->registerBeginEdit(m_vmIdent.first, hFakeClientHandle, m_pVmConfig);
			PRL_RESULT ret =
					CDspService::instance()->getVmConfigManager()
						.saveConfig(m_pVmConfig, qsVmHome, getClient());
			CDspService::instance()->getVmDirHelper()
				.getMultiEditDispatcher()->registerCommit(m_vmIdent.first, hFakeClientHandle);

			if (PRL_FAILED(ret))
				setLastErrorCode( ret );
			else
			{
				CVmEvent evt( PET_DSP_EVT_VM_CONFIG_CHANGED, m_vmIdent.first, PIE_DISPATCHER );
				sendProgressEvent(evt);
			}
		}
	}

	if ( m_flgLockRegistered )
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			m_vmIdent.first,
			m_vmIdent.second,
			(PVE::IDispatcherCommands) getRequestPackage()->header.type, getClient() );

	// Send response

	if ( PRL_FAILED( getLastErrorCode() ) )
	{
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
	else
	{
		CProtoCommandPtr pCmd =
			CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), getLastErrorCode() );
		getClient()->sendResponse( pCmd, getRequestPackage() );
	}

	unlockConvertDisks();
}

void Task_ConvertDisks::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	CancelOperationSupport::cancelOperation(pUserSession, p);

	if (m_pProcess)
#ifdef _WIN_
		// FIXME: Do graceful cancel for disk tool when it'll be implemented
		m_pProcess->kill();
#else
		m_pProcess->terminate();
#endif
}

void Task_ConvertDisks::onReadyReadStandardError()
{
	if ( m_pProcess && PRL_SUCCEEDED(getLastErrorCode()) && ! operationIsCancelled() )
	{
		QString qsErrOut = UTF8_2QSTR(m_pProcess->readAllStandardError().constData());

		WRITE_TRACE(DBG_FATAL, "Convert disks: disk tool internal error: %s !",
						QSTR2UTF8(qsErrOut));

		getLastError()->addEventParameter(
			new CVmEventParameter(PVE::String, QString("%1").arg(qsErrOut), EVT_PARAM_DETAIL_DESCRIPTION) );
		setLastErrorCode(PRL_ERR_CONV_HD_EXIT_WITH_ERROR);
	}
}

void Task_ConvertDisks::onReadyReadStandardOutput()
{
	if ( ! m_pProcess )
		return;

	QString qsOutput = UTF8_2QSTR(m_pProcess->readAllStandardOutput().constData());

	sendCoversionProgressParams(qsOutput);
}

void Task_ConvertDisks::onFinished( int nExitCode, QProcess::ExitStatus nExitStatus )
{
	if ( nExitStatus == QProcess::CrashExit )
	{
		if ( PRL_SUCCEEDED(getLastErrorCode()) && ! operationIsCancelled() )
		{
			WRITE_TRACE(DBG_FATAL, "Convert disks: disk tool was crashed !");

			setLastErrorCode(PRL_ERR_CONV_HD_EXIT_WITH_ERROR);
		}
	}
	else if (nExitCode != 0)
	{
		if ( PRL_SUCCEEDED(getLastErrorCode()) && ! operationIsCancelled() )
		{
			WRITE_TRACE(DBG_FATAL, "Convert disks: disk tool error exit: %d !",
							nExitCode);

			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String, QString("%1").arg(nExitCode), EVT_PARAM_DETAIL_DESCRIPTION) );
			setLastErrorCode(PRL_ERR_CONV_HD_EXIT_WITH_ERROR);
		}
	}
}

void Task_ConvertDisks::cancelAndWait()
{
	SmartPtr<CDspTaskHelper> pTask;
	{
		QMutexLocker lock(&s_lockConvertDisks);

		if (s_mapVmIdTaskId.contains(m_vmIdent))
		{
			Uuid task_uuid = s_mapVmIdTaskId.value(m_vmIdent);
			pTask = CDspService::instance()->getTaskManager().findTaskByUuid(task_uuid);

			CDspService::instance()->getTaskManager().cancelTask(task_uuid.toString());
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "Convert disks: cannot find task for VM [uuid=%s] at cancel operation !",
							QSTR2UTF8(m_vmIdent.first));
		}
	}

	if (pTask.isValid())
		pTask->wait();
}

PRL_RESULT Task_ConvertDisks::convertDisks()
{
	PRL_UINT32 nFlags = ( PCVD_TO_PLAIN_DISK
						| PCVD_TO_EXPANDING_DISK
						| PCVD_MERGE_ALL_SNAPSHOTS);

	if ( ! (m_nFlags & nFlags)
		|| ((m_nFlags & PCVD_MERGE_ALL_SNAPSHOTS) != 0 && (m_nFlags & nFlags) != PCVD_MERGE_ALL_SNAPSHOTS)
		)
	{
		return PRL_ERR_INVALID_ARG;
	}


	if ( (m_nFlags & (PCVD_TO_PLAIN_DISK | PCVD_TO_EXPANDING_DISK))
			== (PCVD_TO_PLAIN_DISK | PCVD_TO_EXPANDING_DISK)
		)
	{
		return PRL_ERR_CONV_HD_CONFLICT;
	}

	QString qsConvMode;
	QString qsConvFlags;
	if ((m_nFlags & PCVD_MERGE_ALL_SNAPSHOTS) != 0)
	{
		qsConvMode = " merge ";
	}
	else
	{
		qsConvMode = " convert ";

		if ((m_nFlags & PCVD_TO_PLAIN_DISK) != 0)
			qsConvFlags += " --plain";
		else if ((m_nFlags & PCVD_TO_EXPANDING_DISK) != 0)
			qsConvFlags += " --expanding";
	}

	// Refresh disks configuration (from DiskDescriptor.xml)

	QList<CVmHardDisk* > lstExistingHardDisks = m_pVmConfig->getVmHardwareList()->m_lstHardDisks;
	Libvirt::Kit.vms().at(m_pVmConfig->getVmIdentification()->getVmUuid())
		.completeConfig(*m_pVmConfig);

	// Check disks configuration

	QList<CVmHardDisk* > lstHardDisks;
	foreach(CVmHardDisk* pHdd, lstExistingHardDisks)
	{
		if (   ! pHdd
			|| pHdd->getEnabled() != PVE::DeviceEnabled
			|| pHdd->getEmulatedType() != PVE::HardDiskImage
			|| pHdd->getSystemName().isEmpty()
			|| ! CFileHelper::FileExists(pHdd->getSystemName(), &getClient()->getAuthHelper())
			)
		   continue;

		PRL_UINT32 nDiskMask = 0;
		if ( pHdd->getInterfaceType() == PMS_IDE_DEVICE )
			nDiskMask = PIM_IDE_MASK_OFFSET << pHdd->getStackIndex();
		else if ( pHdd->getInterfaceType() == PMS_SCSI_DEVICE )
			nDiskMask = PIM_SCSI_MASK_OFFSET << pHdd->getStackIndex();
		else if ( pHdd->getInterfaceType() == PMS_SATA_DEVICE )
			nDiskMask = PIM_SATA_MASK_OFFSET << pHdd->getStackIndex();
		else
			continue;

		if ((m_nDiskMask & nDiskMask) == 0)
			continue;

		PRL_UINT32 nFlags = 0;
		if (pHdd->getDiskType() == PHD_PLAIN_HARD_DISK)
			nFlags |= PCVD_TO_EXPANDING_DISK;
		if (pHdd->getDiskType() == PHD_EXPANDING_HARD_DISK)
			nFlags |= PCVD_TO_PLAIN_DISK;
		bool bNeedChangeDiskType = (m_nFlags & (nFlags | PCVD_MERGE_ALL_SNAPSHOTS)) != 0;

		WRITE_TRACE( DBG_INFO, "Disk %s: flags %#x", QSTR2UTF8( QDir(pHdd->getSystemName()).dirName() ), nFlags );

		if (!bNeedChangeDiskType)
			continue;

		lstHardDisks += pHdd;
	}

	if ( lstHardDisks.isEmpty() )
		return PRL_ERR_CONV_HD_NO_ONE_DISK_FOR_CONVERSION;

	// Disks conversion

	foreach(CVmHardDisk* pHdd, lstHardDisks)
	{
		// Index needs for progress event
		m_nCurHddIndex = pHdd->getIndex();
		m_nCurHddItemId = pHdd->getItemId();

		QString qsCmd = QString("\"%1\" %2 %3 \"%4\" %5")
			.arg(m_qsDTName,
				 qsConvMode, QString("--hdd "),
				 pHdd->getSystemName(),
				 qsConvFlags);

		WRITE_TRACE(DBG_WARNING, "Convert disks: start process '%s' ...",
					QSTR2UTF8(qsCmd));
		{
			CAuthHelperImpersonateWrapper impersonate(&getClient()->getAuthHelper());
			m_pProcess->start(qsCmd);
		}
		if ( ! m_pProcess->waitForStarted() )
		{
			WRITE_TRACE(DBG_FATAL, "Convert disks: disk tool cannot be started !");

			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String, qsCmd, EVT_PARAM_MESSAGE_PARAM_0) );
			return PRL_ERR_CONV_HD_DISK_TOOL_NOT_STARTED;
		}

		WRITE_TRACE(DBG_WARNING, "Convert disks: disk tool was started !");
		m_pProcess->waitForFinished(-1);
		WRITE_TRACE(DBG_WARNING, "Convert disks: disk tool was stopped !");

		/* Remove from cache */
		CDspService::instance()->getVmConfigManager().getHardDiskConfigCache().remove(
				pHdd->getSystemName() );

		m_pProcess->disconnect();

		{
			CDspLockedPointer<CVmDirectoryItem> pDirItem
				= CDspService::instance()->getVmDirHelper()
					.getVmDirectoryItemByUuid(m_vmIdent.second, m_vmIdent.first);

			if ( pDirItem.isValid() )
			{
				PRL_RESULT ret = CDspVmSnapshotStoreHelper::SetDefaultAccessRights(
									pHdd->getSystemName(),
									getClient(),
									pDirItem.getPtr());
				if (PRL_FAILED(ret))
					return ret;
			}
		}

		if ( operationIsCancelled() && PRL_SUCCEEDED(getLastErrorCode()) )
		{
			return getCancelResult();
		}

		if ( PRL_FAILED( getLastErrorCode() ) )
		{
			return getLastErrorCode();
		}

		if ((m_nFlags & PCVD_TO_PLAIN_DISK) != 0)
			pHdd->setDiskType(PHD_PLAIN_HARD_DISK);
		if ((m_nFlags & PCVD_TO_EXPANDING_DISK) != 0)
			pHdd->setDiskType(PHD_EXPANDING_HARD_DISK);

		m_bVmConfigWasChanged = true;

		if ((m_nFlags & PCVD_MERGE_ALL_SNAPSHOTS) != 0)
		{
			deleteSnapshotsData();
		}

		CVmEvent evt( PET_DSP_EVT_CONVERSION_DISKS_PROGRESS_FINISHED,
						m_vmIdent.first, PIE_DISPATCHER );
		evt.addEventParameter(
			new CVmEventParameter(PVE::Integer,
								  QString("%1").arg(m_nCurHddIndex),
								  EVT_PARAM_VM_CONFIG_DEV_INDEX) );
		evt.addEventParameter(
			new CVmEventParameter(PVE::Integer,
								  QString("%1").arg(m_nCurHddItemId),
								  EVT_PARAM_VM_CONFIG_DEV_ITEM_ID) );
		sendProgressEvent(evt);
	}

	return PRL_ERR_SUCCESS;
}

void Task_ConvertDisks::deleteSnapshotsData()
{
	CAuthHelperImpersonateWrapper impersonate(&getClient()->getAuthHelper());

	// Delete root snapshot with its children in snapshot XML

	CDspService::instance()->getVmSnapshotStoreHelper().deleteSnapshot(m_pVmConfig, QString(), true);

	// Delete snapshot files

	QString qsSnapshotsDataPath = m_pVmConfig->getConfigDirectory() + "/"VM_GENERATED_WINDOWS_SNAPSHOTS_DIR;
	QStringList lstFilters = QStringList()
		<< "*.mem" << "*.sav" << "*.pvc" << "*.png" << "*.mem.trc" << "*.dat" << QString("*%1")
												.arg(VM_INFO_FILE_SUFFIX);

	QDir	cDir(qsSnapshotsDataPath);
	cDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
	cDir.setNameFilters( lstFilters );
	cDir.setSorting(QDir::Name | QDir::DirsLast);

	QFileInfoList cFileList = cDir.entryInfoList();
	for(int i = 0; i < cFileList.size();i++ )
	{
		QString qsPath = cFileList.at(i).filePath();
		if (cFileList.at(i).isFile())
		{
			if ( ! QFile::remove(qsPath) )
			{
				WRITE_TRACE(DBG_FATAL,
							"Convert disks: cannot delete snapshot file '%s' ! System error: %ld [%s]",
							QSTR2UTF8(qsPath),
							Prl::GetLastError(),
							QSTR2UTF8(Prl::GetLastErrorAsString()) );
			}
		}
		else
		{
			if ( ! CFileHelper::ClearAndDeleteDir(qsPath) )
			{
				WRITE_TRACE(DBG_FATAL,
							"Convert disks: cannot delete snapshot directory '%s' ! System error: %ld [%s]",
							QSTR2UTF8(qsPath),
							Prl::GetLastError(),
							QSTR2UTF8(Prl::GetLastErrorAsString()) );
			}
		}
	}
}

void Task_ConvertDisks::sendCoversionProgressParams(const QString& qsOutput)
{
	QRegExp re("\\b\\d+\\b\\s*%");
	int idx = re.lastIndexIn(qsOutput);
	if (idx == -1)
		return;

	QString qsPercent = qsOutput.mid(idx, re.matchedLength());
	qsPercent.remove("%");

	CVmEvent evt( PET_DSP_EVT_CONVERSION_DISKS_PROGRESS_CHANGED,
					m_vmIdent.first, PIE_DISPATCHER );

	evt.addEventParameter(
		new CVmEventParameter(PVE::Integer,
							  QString("%1").arg(qsPercent.toInt()),
							  EVT_PARAM_PROGRESS_CHANGED) );
	evt.addEventParameter(
		new CVmEventParameter(PVE::Integer,
							  QString("%1").arg(m_nCurHddIndex),
							  EVT_PARAM_VM_CONFIG_DEV_INDEX) );
	evt.addEventParameter(
		new CVmEventParameter(PVE::Integer,
							  QString("%1").arg(m_nCurHddItemId),
							  EVT_PARAM_VM_CONFIG_DEV_ITEM_ID) );

	sendProgressEvent(evt);
}

void Task_ConvertDisks::sendProgressEvent(const CVmEvent& evt)
{
	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspVmEvent, evt, getRequestPackage() );

	CDspService::instance()->getClientManager().sendPackageToVmClients(p, m_vmIdent);
}

void Task_ConvertDisks::lockConvertDisks()
{
	if ( isTaskShutdown() )
		return;

	QMutexLocker lock(&s_lockConvertDisks);
	s_mapVmIdTaskId.insert(m_vmIdent, getJobUuid().toString());
}

void Task_ConvertDisks::unlockConvertDisks()
{
	if ( isTaskShutdown() )
		return;

	QMutexLocker lock(&s_lockConvertDisks);
	s_mapVmIdTaskId.remove(m_vmIdent);
}
