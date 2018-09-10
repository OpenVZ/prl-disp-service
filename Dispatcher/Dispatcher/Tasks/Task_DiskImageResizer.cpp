////////////////////////////////////////////////////////////////////////////////
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
/// @author sergeyt
///	igor@
///
////////////////////////////////////////////////////////////////////////////////

#include <QDateTime>

#include "Task_DiskImageResizer.h"
#include "Task_CommonHeaders.h"

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include "CDspClientManager.h"
#include "CDspAccessManager.h"

#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "Libraries/StatesUtils/StatesHelper.h"

#include "Libraries/PrlCommonUtils/CFileHelper.h"


using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#define MAX_TIME_INTERVAL 500

#define MB2SECT(x)	((PRL_UINT64) (x) << 11)
#define SECT2MB(x)	((x) >> 11)
#define BYTE2MB(x)	((x) >> 20)

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct Info

struct Info
{
	explicit Info(QIODevice& resizer_): m_resizer(&resizer_)
	{
	}

	PRL_RESULT operator()(CDiskImageInfo& builder_);

private:
	QIODevice* m_resizer;
};

PRL_RESULT Info::operator()(CDiskImageInfo& builder_)
{
	builder_.setResizeSupported(true);
	forever
	{
		QByteArray a = m_resizer->readLine(4096).trimmed();
		if (a.isEmpty())
			break;
		
		if (a.contains("Unsupported filesystem"))
			builder_.setResizeSupported(false);
		else if (a.contains("The last partition cannot be resized "))
			builder_.setResizeSupported(false);

		QList<QByteArray> b = a.split(':');
		if (2 != b.size())
			continue;
		b[1] = b[1].trimmed();
		b[1].truncate(b[1].indexOf('M'));
		if (b[0].startsWith("Size"))
			builder_.setCurrentSize(b[1].toULongLong());
		else if (b[0].startsWith("Minimum without resizing the last partition"))
		{}
		else if (b[0].startsWith("Minimum"))
			builder_.setMinSize(qMax<qlonglong>(100, b[1].toLongLong()));
	}
	return PRL_ERR_SUCCESS;
}

} // namespace

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

PRL_RESULT Task_DiskImageResizer::seed(CProtoVmResizeDiskImageCommand& source_)
{
	m_disk = NULL;
	PRL_RESULT c = PRL_ERR_SUCCESS;
	m_pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(
						getClient(), source_.GetVmUuid(), c);
	if (!m_pVmConfig.isValid())
	{
		if (PRL_SUCCEEDED(c))
			c = PRL_ERR_FAILURE;

		WRITE_TRACE(DBG_FATAL, "Cannot read the VM config");
		return c;
	}
	CVmHardware* h = m_pVmConfig->getVmHardwareList();
	CVmIdentification* i = m_pVmConfig->getVmIdentification();
	if (NULL == h || i == NULL)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	const QList<CVmHardDisk* >& a = h->m_lstHardDisks;
	QList<CVmHardDisk* >::const_iterator e = a.end();
	QList<CVmHardDisk* >::const_iterator p = std::find_if(a.begin(), e,
			boost::bind(&CVmHardDisk::getSystemName, _1) == source_.GetDiskImage() &&
			boost::bind(&CVmHardDisk::getEmulatedType, _1) == PVE::HardDiskImage);
	if (e == p)
		return PRL_ERR_FILE_NOT_FOUND;

	h->RevertDevicesPathToAbsolute(QFileInfo(i->getHomePath()).absolutePath());
	m_disk = *p;
	m_Flags = source_.GetFlags();
	m_NewSize = source_.GetSize();

	return PRL_ERR_SUCCESS;
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
		ret = seed(*pVmCmd);
		if (PRL_FAILED(ret))
			throw ret;

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
		if (NULL != m_disk)
		{
			WRITE_TRACE(DBG_FATAL, "Error occurred on DiskResizer diskimage='%s' size=%d [%#x][%s]",
				QSTR2UTF8(m_disk->getSystemName()), m_NewSize,
				ret, PRL_RESULT_TO_STRING( ret ) );
		}
		if (!(m_OpFlags & TASK_SKIP_SEND_RESPONSE))
			getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
	else
	{
		// DiskDescriptor.xml is changed
		Notify(PET_DSP_EVT_VM_CONFIG_CHANGED);

		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(
				getRequestPackage(), PRL_ERR_SUCCESS);
		if (m_Flags & PRIF_DISK_INFO)
		{
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd)
				->AddStandardParam(m_info.toString());
		}
		if (!(m_OpFlags & TASK_SKIP_SEND_RESPONSE))
			getClient()->sendResponse(pCmd, getRequestPackage());
	}
	m_disk = NULL;
}

PRL_RESULT Task_DiskImageResizer::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	CVmIdent i(getVmUuid(), getClient()->getVmDirectoryUuid());
	CAuthHelperImpersonateWrapperPtr g(CAuthHelperImpersonateWrapper
		::create(&getClient()->getAuthHelper()));
	if (g->wasImpersonated())
	{
		switch (CDspVm::getVmState(i))
		{
		case VMS_PAUSED:
		case VMS_RUNNING:
		{
			Libvirt::Result e;
			if (0 < (m_Flags & (PRIF_DISK_INFO | PRIF_RESIZE_LAST_PARTITION)))
			{
				e = ::Error::Simple(PRL_ERR_VM_REQUEST_NOT_SUPPORTED,
					"The only supported operation for an online VM is image resize.");
			}
			else if (PVE::DeviceEnabled == m_disk->getEnabled() &&
				m_disk->getConnected() == PVE::DeviceConnected)
			{
				e = Libvirt::Kit.vms().at(i.first)
					.getRuntime().setImageSize(*m_disk, m_NewSize << 20);
			}
			else
				ret = PRL_ERR_INVALID_HANDLE;

			if (e.isFailed())
				ret = CDspTaskFailure(*this)(e.error().convertToEvent());
			break;
		}
		default:
			ret = run_disk_tool();
		}
		g.reset();
	}
	else
		ret = PRL_ERR_IMPERSONATE_FAILED;

	if (PRL_FAILED(ret))
	{
		WRITE_TRACE(DBG_FATAL, "Error while resizing the hdd image [%#x][%s]",
				ret, PRL_RESULT_TO_STRING(ret));
	}
	PRL_RESULT e = CDspService::instance()->getAccessManager()
			.setOwnerAndAccessRightsToPathAccordingVmRights(
				m_disk->getSystemName(),
				getClient(),
				CDspService::instance()->getVmDirHelper().getVmDirectoryItemByUuid(
					i.second, i.first).getPtr(),
				true);
	if(PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "Can't change owner of the disk image [%s] (0x%x)",
			QSTR2UTF8(m_disk->getSystemName()), e);
	}
	setLastErrorCode(ret);

	return ret;
}

PRL_RESULT Task_DiskImageResizer::IsHasRightsForResize( CVmEvent& evtOutError )
{
	if (!CFileHelper::FileCanWrite(m_disk->getSystemName(), &getClient()->getAuthHelper()))
	{
		evtOutError.setEventCode( PRL_ERR_ACCESS_TO_VM_HDD_DENIED );
		evtOutError.addEventParameter(
			new CVmEventParameter( PVE::String,
			m_pVmConfig->getVmIdentification()->getVmName(), EVT_PARAM_MESSAGE_PARAM_0));
		evtOutError.addEventParameter(
			new CVmEventParameter( PVE::String,
			m_disk->getSystemName(), EVT_PARAM_MESSAGE_PARAM_1));
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
	lstArgs += m_disk->getSystemName();
	if (infoMode) {
		lstArgs += "--info";
	}
	else {
		lstArgs += QString("--size"); lstArgs += QString("%1M").arg(m_NewSize);
	}
	if (m_Flags & PRIF_RESIZE_LAST_PARTITION)
		lstArgs += QString("--resize_partition");

	QProcess resizer_proc;
	resizer_proc.setProcessChannelMode(QProcess::MergedChannels);

	Notify(PET_DSP_EVT_DISK_RESIZE_STARTED);

	resizer_proc.start(resizer_cmd, lstArgs);

	PRL_RESULT ret = PRL_ERR_SUCCESS;
	if (resizer_proc.waitForStarted())
	{
		/* Progress event loop untill error arived or programm exited */
		while (resizer_proc.state() == QProcess::Running)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, MAX_TIME_INTERVAL);

			HostUtils::Sleep(MAX_TIME_INTERVAL);
		}
	}
	/* remove from cache */
	if (!infoMode)
		CDspService::instance()->getVmConfigManager().getHardDiskConfigCache().remove(m_disk->getSystemName());

	if (infoMode) {
		ret = Info(resizer_proc)(m_info);
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
