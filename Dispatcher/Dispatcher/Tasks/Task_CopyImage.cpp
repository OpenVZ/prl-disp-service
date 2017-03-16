///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CopyImage.cpp
///
/// Dispatcher task for doing copy image of virtual device.
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

//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include "Libraries/PrlCommonUtils/CFileHelper.h"
//#include "Libraries/DiskImage/DiskImage.h"  // DiskImage commented out by request from CP team
#include "CFileHelperDepPart.h"
#include "CDspService.h"
#include "Task_CopyImage.h"


Task_CopyImage::Task_CopyImage( const SmartPtr<CDspClient>& pClient,
				const SmartPtr<IOPackage>& p)
: CDspTaskHelper(pClient, p),
  m_bExclusiveOpWasReg(false)
{
}

Task_CopyImage::~Task_CopyImage()
{
}

PRL_RESULT Task_CopyImage::prepareTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		// Parse command

		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( getRequestPackage() );
		if ( ! cmd->IsValid() )
			throw PRL_ERR_FAILURE;

		CProtoCopyImageCommand
			*pCopyImageCmd = CProtoSerializer::CastToProtoCommand<CProtoCopyImageCommand>(cmd);
		m_qsVmUuid = pCopyImageCmd->GetVmUuid();
		QString qsImageConfig = pCopyImageCmd->GetImageConfig();
		QString qsNewImage = pCopyImageCmd->GetNewImageName();
		QString qsTargetPath = pCopyImageCmd->GetTargetPath();

        CVmHardDisk vmHardDisk;
        if ( ! StringToElement(&m_vmHardDisk, qsImageConfig) )
            throw PRL_ERR_BAD_PARAMETERS;

		if ( m_vmHardDisk.getEmulatedType() != PVE::HardDiskImage )
			throw PRL_ERR_CI_DEVICE_IS_NOT_VIRTUAL;

		m_qsSourcePath = m_vmHardDisk.getSystemName();
		if (qsNewImage.isEmpty())
		{
			QFileInfo fi(m_qsSourcePath);
			m_qsTargetPath = QString("%1/%2").arg(qsTargetPath, fi.fileName());
		}
		else
			m_qsTargetPath = QDir(qsTargetPath).absoluteFilePath(qsNewImage);

		// Check access

		bool bSetNotValid = false;
		ret = CDspService::instance()->getAccessManager()
			.checkAccess( getClient(), PVE::DspCmdDirCopyImage, m_qsVmUuid, &bSetNotValid, getLastError() );
		if ( PRL_FAILED(ret) )
			throw ret;

		// Check VM state

		VIRTUAL_MACHINE_STATE vmState
			= CDspVm::getVmState( m_qsVmUuid, getClient()->getVmDirectoryUuid() );
		if ( vmState != VMS_STOPPED && vmState != VMS_SUSPENDED )
			throw PRL_ERR_CI_CANNOT_COPY_IMAGE_NON_STOPPED_VM;

		// Check permissions

		if ( ! checkPathPermissions(m_qsSourcePath) )
			throw PRL_ERR_CI_PERMISSIONS_DENIED;

		// Lock exclusive operation

		ret = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
				m_qsVmUuid,
				getClient()->getVmDirectoryUuid(),
				PVE::DspCmdDirCopyImage,
				getClient() );
		if( PRL_FAILED( ret ) )
			throw ret;

		m_bExclusiveOpWasReg = true;

	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while copy image with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	getLastError()->setEventCode( ret );

	return ret;
}

PRL_RESULT Task_CopyImage::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		ret = CFileHelperDepPart::CopyFileWithNotifications(
				m_qsSourcePath,
				m_qsTargetPath,
				&getClient()->getAuthHelper(),
				this,
				m_vmHardDisk.getDeviceType(),
				m_vmHardDisk.getIndex());
		if (PRL_FAILED(ret))
			throw ret;

	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while copy image with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	setLastErrorCode( ret );

	return ret;
}

void Task_CopyImage::finalizeTask()
{
	// Cleanup at error case

	if ( ! m_qsTargetPath.isEmpty()
		&& QFileInfo(m_qsTargetPath).exists()
		&& PRL_FAILED( getLastErrorCode() ))
	{
		CFileHelper::RemoveEntry(m_qsTargetPath, &getClient()->getAuthHelper());
	}

	if (m_bExclusiveOpWasReg)
	{
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
			m_qsVmUuid,
			getClient()->getVmDirectoryUuid(),
			PVE::DspCmdDirCopyImage,
			getClient() );
	}

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
}

bool Task_CopyImage::checkPathPermissions(QString qsPath)
{
	QDir	cDir(qsPath);
	cDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
	cDir.setSorting(QDir::Name | QDir::DirsLast);

	QFileInfoList cFileList = cDir.entryInfoList();
	for(int i = 0; i < cFileList.size(); ++i)
	{
		QFileInfo fi = cFileList.at(i);
		if( QFileInfo(qsPath) == fi )
			continue;

		if (fi.isDir())
			if ( ! checkPathPermissions(fi.filePath()) )
				return false;

		if ( ! CFileHelper::FileCanRead(fi.filePath(), &getClient()->getAuthHelper()) )
			return false;
	}

	return true;
}
