////////////////////////////////////////////////////////////////////////////////
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
/// @author sergeyt
///	igor@
///
////////////////////////////////////////////////////////////////////////////////

#include <QDateTime>

#include "Task_DiskImageResizer.h"
#include "Task_CommonHeaders.h"

#include "CProtoSerializer.h"
#include "CDspClientManager.h"
#include "CDspAccessManager.h"
#include "Tasks/Task_CommitUnfinishedDiskOp.h"

#include "Libraries/PrlCommonUtilsBase/SysError.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/StatesUtils/StatesHelper.h"

#include "XmlModel/DiskImageInfo/CDiskImageInfo.h"
// #include "VI/Sources/ImageTool/resizer/ImageToolSharedInfo.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

#ifndef _WIN_
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
#endif

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#define MAX_TIME_INTERVAL 500

// Values for shared memory name generation
#ifdef _WIN_
        #define IPCMEM_DEFAULT_PATH "Global\\"
#else
        #define IPCMEM_DEFAULT_PATH "/tmp/"
#endif

#define IPCMEM_DEFAULT_PREFIX "prl_shared_mem_"
#define MB2SECT(x)	((PRL_UINT64) (x) << 11)
#define SECT2MB(x)	((x) >> 11)
#define BYTE2MB(x)	((x) >> 20)

Task_DiskImageResizer::Task_DiskImageResizer(SmartPtr<CDspClient>& user,
				const SmartPtr<IOPackage>& p,
				PRL_UINT32 nOpFlags)
:
	CDspTaskHelper(user, p),
	m_OpFlags(nOpFlags),
	m_CurProgress(0),
// VirtualDisk commented out by request from CP team
//	m_pDisk(NULL),
//	m_ImageToolInfo(NULL),
	m_bflLocked(PRL_FALSE),
//	m_pMemory(NULL),
	m_Flags(0)
{
	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration());
	setLastErrorCode(PRL_ERR_SUCCESS);
}

Task_DiskImageResizer::~Task_DiskImageResizer()
{
//	DeinitIPCMemory();
// VirtualDisk commented out by request from CP team
//	if (m_pDisk != NULL)
//		m_pDisk->Release();

}

QString Task_DiskImageResizer::getVmUuid()
{
	return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

QString Task_DiskImageResizer::ConvertToFullPath(const QString &sPath)
{
        if (!QFileInfo(sPath).isAbsolute())
        {
                QString sVmHomeDirPath = QFileInfo(m_pVmConfig->getVmIdentification()->getHomePath()).absolutePath();
                if (!QFileInfo(sVmHomeDirPath).isDir())
                        sVmHomeDirPath = QFileInfo(sVmHomeDirPath).absolutePath();
                QFileInfo _fi(sVmHomeDirPath + '/' + sPath);
                return (_fi.absoluteFilePath());
        }

        return (sPath);
}

PRL_RESULT Task_DiskImageResizer::prepareTask()
{
	// check params in existing VM
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmResizeDisk,
					UTF8_2QSTR(getRequestPackage()->buffers[0].getImpl()));
		if (!cmd->IsValid())
			throw (PRL_ERR_FAILURE);

		CProtoVmResizeDiskImageCommand *pVmCmd =
			CProtoSerializer::CastToProtoCommand<CProtoVmResizeDiskImageCommand>(cmd);
		m_DiskImage = ConvertToFullPath(pVmCmd->GetDiskImage());
		m_NewSize = pVmCmd->GetSize();
		m_Flags = pVmCmd->GetFlags();

		m_pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(
								getClient(), pVmCmd->GetVmUuid(), ret);
		if (!m_pVmConfig)
		{
			PRL_ASSERT(PRL_FAILED(ret));
			if (!PRL_FAILED(ret))
				ret = PRL_ERR_FAILURE;

			WRITE_TRACE(DBG_FATAL, "Couldn't to extract VM config for ");
			throw ret;
		}
		ret = CDspService::instance()->getAccessManager().checkAccess(getClient(),
										PVE::DspCmdVmResizeDisk,
										getVmUuid(),
										NULL, getLastError());
		if (PRL_FAILED(ret))
			throw ret;

		if (!(m_OpFlags & TASK_SKIP_LOCK)) {
			// Deny multiple resizing
			ret = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
					getVmUuid(),
					getClient()->getVmDirectoryUuid(),
					PVE::DspCmdVmResizeDisk, getClient(),
					this->getJobUuid());
			if (PRL_FAILED(ret))
				throw ret;
			m_bflLocked = PRL_TRUE;
		}

//		ret = InitIPCMemory();
//		if (PRL_FAILED(ret))
//			throw ret;

		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred on Task_DiskImageResizer prepare [%#x][%s]",
			code, PRL_RESULT_TO_STRING( code));
	}

	setLastErrorCode(ret);

	return ret;
}

void Task_DiskImageResizer::Notify(PRL_EVENT_TYPE evType)
{
	CVmEvent event(evType, getVmUuid(), PIE_DISPATCHER);

	if (evType == PET_DSP_EVT_DISK_RESIZE_PROGRESS_CHANGED)
	{
		event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
				QString::number(m_CurProgress),
				EVT_PARAM_PROGRESS_CHANGED));
	}

	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(
			PVE::DspVmEvent, event, getRequestPackage());

	CDspService::instance()->getClientManager().sendPackageToVmClients(
			p, getClient()->getVmDirectoryUuid(), getVmUuid());
}

void Task_DiskImageResizer::finalizeTask()
{
	PRL_RESULT ret = getLastErrorCode();

	// Remove operation lock
	if (m_bflLocked)
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			getVmUuid(),
			getClient()->getVmDirectoryUuid(),
			PVE::DspCmdVmResizeDisk, getClient() );

	/**
	* Send response
	*/
	if (PRL_FAILED(ret))
	{
		WRITE_TRACE(DBG_FATAL, "Error occurred on DiskResizer diskimage='%s' size=%d [%#x][%s]",
			QSTR2UTF8(m_DiskImage), m_NewSize,
			ret, PRL_RESULT_TO_STRING( ret ) );


		if (!(m_OpFlags & TASK_SKIP_SEND_RESPONSE))
			getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
	else
	{
		// DiskDescriptor.xml is chnged
		Notify(PET_DSP_EVT_VM_CONFIG_CHANGED);

		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(
				getRequestPackage(), PRL_ERR_SUCCESS);
/* commented out by request from CP team
 * 		if (m_ImageToolInfo != NULL)
		{
			// Add image info to the response
			CDiskImageInfo di;

			ResizeSharedInfo *resizeInfo = &m_ImageToolInfo->m_SpecInfo.resizeInfo;
			di.setSuspended(resizeInfo->m_Suspended);
			di.setSnapshotCount(resizeInfo->m_SnapCount);
			di.setResizeSupported(resizeInfo->m_resizeSupported);
			di.setCurrentSize(resizeInfo->m_currentSize);
			di.setMinSize(resizeInfo->m_minSize < 100 ? 100 : resizeInfo->m_minSize);
			di.setMaxSize(resizeInfo->m_maxSize);

			CProtoCommandDspWsResponse *pResponseCmd =
					CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
			pResponseCmd->AddStandardParam(di.toString());
		} */

		if (!(m_OpFlags & TASK_SKIP_SEND_RESPONSE))
			getClient()->sendResponse(pCmd, getRequestPackage());
	}
}

PRL_RESULT Task_DiskImageResizer::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	bool flgImpersonated = false;
	try
	{
		// Let current thread impersonate the security context of a logged-on user
		if (!getClient()->getAuthHelper().Impersonate())
		{
			getLastError()->setEventCode(PRL_ERR_IMPERSONATE_FAILED);
			throw PRL_ERR_IMPERSONATE_FAILED;
		}
		flgImpersonated = true;
		ret = run_disk_tool();
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error while resizing the hdd image [%#x][%s]",
				code, PRL_RESULT_TO_STRING( code ) );
	}

	// Terminates the impersonation of a user, return to Dispatcher access rights
	if (flgImpersonated && !getActualClient()->getAuthHelper().RevertToSelf())
	{
		WRITE_TRACE(DBG_FATAL, "RevertToSelf failed: %s",
			PRL_RESULT_TO_STRING(PRL_ERR_REVERT_IMPERSONATE_FAILED));
	}
	PRL_RESULT e = CDspService::instance()->getAccessManager()
			.setOwnerAndAccessRightsToPathAccordingVmRights(
				m_DiskImage,
				getClient(),
				CDspService::instance()->getVmDirHelper().getVmDirectoryItemByUuid(
					getClient(),
					m_pVmConfig->getVmIdentification()->getVmUuid()).getPtr(),
				true);
	if(PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "Can't change owner of the disk image [%s] (0x%x)",
			QSTR2UTF8(m_DiskImage), e);
	}
	setLastErrorCode(ret);

	return ret;
}

PRL_RESULT Task_DiskImageResizer::IsHasRightsForResize( CVmEvent& evtOutError )
{
	if (!CFileHelper::FileCanWrite(m_DiskImage, &getClient()->getAuthHelper()))
	{
		evtOutError.setEventCode( PRL_ERR_ACCESS_TO_VM_HDD_DENIED );
		evtOutError.addEventParameter(
			new CVmEventParameter( PVE::String,
			m_pVmConfig->getVmIdentification()->getVmName(), EVT_PARAM_MESSAGE_PARAM_0));
		evtOutError.addEventParameter(
			new CVmEventParameter( PVE::String,
			m_DiskImage, EVT_PARAM_MESSAGE_PARAM_1));
		return evtOutError.getEventCode();
	}

	return PRL_ERR_SUCCESS;
}

/* void dump_it_data(struct ImageToolSharedInfo *ImageToolInfo)
{
	ResizeSharedInfo *resizeInfo = &ImageToolInfo->m_SpecInfo.resizeInfo;
	WRITE_TRACE(DBG_FATAL, "ResizeStructSize=%d Completion=%d Break=%d Error=%d"
				" Split=%d Suspended=%d SnapCount=%d Compactable=%d"
				" fsNotSupported=%d currentSize=%llu minSize=%lluu",
		ImageToolInfo->m_SpecSize, ImageToolInfo->m_Completion, ImageToolInfo->m_Break, ImageToolInfo->m_Error,
		resizeInfo->m_Split, resizeInfo->m_Suspended, resizeInfo->m_SnapCount, resizeInfo->m_Compactable,
		resizeInfo->m_fsNotSupported, resizeInfo->m_currentSize, resizeInfo->m_minSize
		   );
} */

PRL_RESULT Task_DiskImageResizer::run_disk_tool()
{

	QStringList lstArgs;
	bool infoMode = (m_Flags & PRIF_DISK_INFO);

	// Taking application path as 'current'
	QDir pwd = QDir( (QCoreApplication::instance())->applicationDirPath() );
	QString resizer_cmd = ParallelsDirs::getDiskToolPath( pwd );

	if (!QFile::exists(resizer_cmd))
	{
		WRITE_TRACE(DBG_FATAL, "resizer tool '%s' not found",
				QSTR2UTF8(resizer_cmd));
		return PRL_ERR_DISK_RESIZER_NOT_FOUND;
	}

	lstArgs += "resize";
	lstArgs += QString("--hdd");
	lstArgs += QFileInfo(m_DiskImage).isDir() ? m_DiskImage : QFileInfo(m_DiskImage).canonicalPath();
	if (infoMode) {
		lstArgs += "--info";
	}
	else {
		lstArgs += QString("--size"); lstArgs += QString("%1M").arg(m_NewSize);
	}
	if (m_Flags & PRIF_RESIZE_LAST_PARTITION)
		lstArgs += QString("--resize_partition");

	lstArgs += QString("--comm"); lstArgs += m_IPCName;

	if ( !CDspService::instance()->isServerMode() )
		lstArgs += "--tr-err"; // translate errors: see bug #482607

	QProcess resizer_proc;

	Notify(PET_DSP_EVT_DISK_RESIZE_STARTED);

	resizer_proc.start(resizer_cmd, lstArgs);

	PRL_RESULT ret = PRL_ERR_SUCCESS;
	PRL_BOOL bContinue = resizer_proc.waitForStarted();

	/* Progress event loop untill error arived or programm exited */
	while (bContinue)
	{
		if (operationIsCancelled())
		{
			Cancel(getCancelResult());
			ret = getCancelResult();
			break;
		}
		if (resizer_proc.state() == QProcess::NotRunning)
			bContinue = false;

/*		dump_it_data(m_ImageToolInfo);
		if (PRL_FAILED(m_ImageToolInfo->m_Error))
		{
			WRITE_TRACE(DBG_FATAL, "resizing utility exits with error [%x]", m_ImageToolInfo->m_Error);
			ret = m_ImageToolInfo->m_Error;
			break;
		}

		// Completion to percents
		PRL_UINT32 completion;
		if (m_ImageToolInfo->m_Completion == PRL_DISK_PROGRESS_COMPLETED)
			completion = 100;
		else
			completion = (100 * m_ImageToolInfo->m_Completion) / PRL_DISK_PROGRESS_MAX;

		if (completion != m_CurProgress)
		{
			m_CurProgress = completion;
			Notify(PET_DSP_EVT_DISK_RESIZE_PROGRESS_CHANGED);
		} */
		QCoreApplication::processEvents(QEventLoop::AllEvents, MAX_TIME_INTERVAL);

		HostUtils::Sleep(MAX_TIME_INTERVAL);
	}
	resizer_proc.waitForFinished(60 * 1000);

	/* remove from cache */
	if (!infoMode)
		CDspService::instance()->getVmConfigManager().getHardDiskConfigCache().remove( m_DiskImage );

	if (infoMode) {
		/* explicitly set m_fsNotSupported on error */
		/*if (PRL_FAILED(m_ImageToolInfo->m_Error))
			m_ImageToolInfo->m_SpecInfo.resizeInfo.m_resizeSupported = 0; */
		ret = PRL_ERR_SUCCESS;
	} else if (resizer_proc.exitCode() != 0 && !operationIsCancelled()) {
	/*	ret = m_ImageToolInfo->m_Error;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(ret);

		QString qsErrorOutput = UTF8_2QSTR(resizer_proc.readAllStandardError().constData());
		pEvent->addEventParameter(new CVmEventParameter(PVE::String,
			qsErrorOutput, EVT_PARAM_DETAIL_DESCRIPTION));

		WRITE_TRACE(DBG_FATAL, "HDD resize utility exited with output: cmd='%s %s'"
					" it_result=0x%x [%d]\nerr=%s\nout=%s\n\n",
			QSTR2UTF8(resizer_cmd),
			QSTR2UTF8(lstArgs.join(" ")),
			m_ImageToolInfo->m_Error,
			resizer_proc.exitCode(),
			QSTR2UTF8(qsErrorOutput),
			resizer_proc.readAllStandardOutput().data()); */
		ret = PRL_ERR_OPERATION_FAILED;
	}

	return ret;
}

void Task_DiskImageResizer::Cancel(unsigned int uErrorCode)
{
	// Cancel operation
	setLastErrorCode(uErrorCode);

	WRITE_TRACE(DBG_FATAL, "Cancel disk image resizing initiated...");
	// Set cancelation to shared memory
	// m_ImageToolInfo->m_Break = PRL_TRUE;
}

// Shared memory interprocess communication
/*
PRL_RESULT Task_DiskImageResizer::InitIPCMemory()
{
	PRL_RESULT ret = PRL_ERR_FILE_OR_DIR_ALREADY_EXISTS;

	// Cleanup before init
	DeinitIPCMemory();

	QDateTime tm = QDateTime::currentDateTime();
	// Create shared memory object
	m_IPCName = QString(IPCMEM_DEFAULT_PATH) + QString(IPCMEM_DEFAULT_PREFIX) +
						getVmUuid() + QString("%1").arg(tm.toTime_t());
#ifndef _WIN_
	int fd;
	fd = open(m_IPCName.toUtf8().data(), O_RDWR | O_CREAT | O_EXCL, 0600);
	if (fd == -1)
	{
		WRITE_TRACE(DBG_FATAL, "Error creating file %s errno=%d",
			QSTR2UTF8(m_IPCName), HostUtils::GetLastError());
		ret = PRL_ERR_OPERATION_FAILED;
		goto ErrorExit;
	}

	close(fd);
#endif

	m_pMemory = IPCMemory::Attach(m_IPCName, sizeof(struct ImageToolSharedInfo), ret, IPCMemory::CurrentUser);

	if (PRL_FAILED(ret) || m_pMemory->Data() == NULL)
	{
		WRITE_TRACE(DBG_FATAL, "Error creating IPC memory object: %s 0x%x",
				PRL_RESULT_TO_STRING(ret), ret);
#ifndef _WIN_
		QFile::remove(m_IPCName);
#endif
		goto ErrorExit;
	}
	m_ImageToolInfo = (struct ImageToolSharedInfo *) m_pMemory->Data();
	// Initialize shared data
	memset(m_ImageToolInfo, 0, sizeof(struct ImageToolSharedInfo));
	m_ImageToolInfo->m_Version = IMAGE_TOOL_PROTOCOL_VERSION_CURRENT;
	m_ImageToolInfo->m_SpecSize = sizeof(struct ResizeSharedInfo);

	return PRL_ERR_SUCCESS;

ErrorExit:
	m_IPCName.clear();
	return ret;
}

void Task_DiskImageResizer::DeinitIPCMemory()
{
	// Deinit memory
	if (m_pMemory)
		m_pMemory->Detach();
	m_pMemory = NULL;

#ifndef _WIN_
	if (!m_IPCName.isEmpty())
		QFile::remove(m_IPCName);
#endif
} */

PRL_RESULT Task_DiskImageResizer::updateSplittedState( bool& isSplitted )
{
	CAuthHelperImpersonateWrapper impersonate(&getClient()->getAuthHelper());

	QList<CVmHardDisk*> lstHdd;
	foreach(CVmHardDisk* pHdd, m_pVmConfig->getVmHardwareList()->m_lstHardDisks)
	{
		if ( ! pHdd
			|| pHdd->getSystemName().isEmpty()
			|| ! CFileHelper::IsPathsEqual( pHdd->getSystemName(), m_DiskImage )
			)
		   continue;
		lstHdd << pHdd;
		break;
	}
	PRL_ASSERT( lstHdd.size() );
	if( !lstHdd.size() )
		return PRL_ERR_FAILURE;

	PRL_RESULT res = CDspVmDirHelper::UpdateHardDiskInformation( lstHdd, getClient() );
	isSplitted = lstHdd[0]->isSplitted();
	return res;
}

