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
#include "Libraries/PrlCommonUtils/CFileHelper.h"


using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#define MAX_TIME_INTERVAL 500

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
	m_bflLocked(PRL_FALSE),
	m_Flags(0)
{
	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration());
	setLastErrorCode(PRL_ERR_SUCCESS);
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
		// DiskDescriptor.xml is changed
		Notify(PET_DSP_EVT_VM_CONFIG_CHANGED);

		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(
				getRequestPackage(), PRL_ERR_SUCCESS);

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

	QProcess resizer_proc;

	Notify(PET_DSP_EVT_DISK_RESIZE_STARTED);

	resizer_proc.start(resizer_cmd, lstArgs);

	PRL_RESULT ret = PRL_ERR_SUCCESS;
	PRL_BOOL bContinue = resizer_proc.waitForStarted();

	/* Progress event loop untill error arived or programm exited */
	while (bContinue)
	{
		if (resizer_proc.state() == QProcess::NotRunning)
			bContinue = false;

		QCoreApplication::processEvents(QEventLoop::AllEvents, MAX_TIME_INTERVAL);

		HostUtils::Sleep(MAX_TIME_INTERVAL);
	}

	/* remove from cache */
	if (!infoMode)
		CDspService::instance()->getVmConfigManager().getHardDiskConfigCache().remove( m_DiskImage );

	if (infoMode) {
		ret = PRL_ERR_SUCCESS;
	} else if (resizer_proc.exitCode() != 0 && !operationIsCancelled()) {
		ret = resizer_proc.exitCode();
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(ret);

		QString qsErrorOutput = UTF8_2QSTR(resizer_proc.readAllStandardError().constData());
		pEvent->addEventParameter(new CVmEventParameter(PVE::String,
			qsErrorOutput, EVT_PARAM_DETAIL_DESCRIPTION));

		WRITE_TRACE(DBG_FATAL, "HDD resize utility exited with output: cmd='%s %s'"
					" result=%d\nerr=%s\nout=%s\n\n",
			QSTR2UTF8(resizer_cmd),
			QSTR2UTF8(lstArgs.join(" ")),
			resizer_proc.exitCode(),
			QSTR2UTF8(qsErrorOutput),
			resizer_proc.readAllStandardOutput().data());
		ret = PRL_ERR_OPERATION_FAILED;
	}

	return ret;
}
