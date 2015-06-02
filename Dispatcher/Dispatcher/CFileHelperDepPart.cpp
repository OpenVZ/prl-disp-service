///////////////////////////////////////////////////////////////////////////////
///
/// @file CFileHelperDepPart.cpp
///
/// Dispatcher's subset of file system processing methods
///
/// @author sergeyt@
/// @owner sergeym@
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

#include "CFileHelperDepPart.h"

#include "ParallelsPlatform.h"
#include "ParallelsQt.h"

#include "CDspTaskHelper.h"
#include "CDspAccessManager.h"
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "CDspUserHelper.h"
#include "CProtoSerializer.h"

#include "Tasks/Task_CloneVm.h"
#include "Libraries/Logging/Logging.h"
#include "Libraries/Std/PrlTime.h"
//#include "Libraries/AbstractFile/CommonFile.h"  // AbstractFile commented out by request from CP team
#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/PrlCommonUtilsBase/CSimpleFileHelper.h"

#ifdef _LIN_
# include <sys/types.h>
# include <fcntl.h>
# include <malloc.h>
#endif

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

static void NotifyCopyEvent(CDspTaskHelper * lpcTaskHelper, _PRL_EVENT_TYPE type, PRL_DEVICE_TYPE devType, int iDevNum)
{

	//////////////////////////////////////////////////////////////////////////
	// send notifications about finish of file copying
	//////////////////////////////////////////////////////////////////////////

	// Create event for client
	CVmEvent event(	type, Uuid().toString(), PIE_DISPATCHER);

	///////////////////////
	// Add event parameters
	///////////////////////

	event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
		 QString::number(devType),
			EVT_PARAM_DEVICE_TYPE));

	event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
			QString::number(iDevNum),
		 EVT_PARAM_DEVICE_INDEX));

	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspVmEvent, event,
							  lpcTaskHelper->getRequestPackage());

	lpcTaskHelper->getClient()->sendPackage( p );

	//////////////////////////////////////////////////////////////////////////
	// end of sending notifications about finish of file copying
	//////////////////////////////////////////////////////////////////////////
}

/////////////////////////////////////////////////////////////////////////////
//
// @func CFileHelper::GetFileName(QString & strFilePath)
//
// @brief
//			file copy with cancel check
//
// @params
//			QString & strSrcFile - file from copy
//			QString & strDestFile - file to copy
//			CDspTaskHelper * lpcTaskHelper - object form get information of cancel
//			PVE::DeviceType devType - type of device
//			int iDevNum - device number
// @return
//			PRL_RESULT - operation result
//
//
/////////////////////////////////////////////////////////////////////////////
//
PRL_RESULT CFileHelperDepPart::CopyFileWithNotifications(const QString & strSrcFile,
																  const QString & strDestFile,
																  CAuthHelper* pDestOwner,
																  CDspTaskHelper * lpcTaskHelper,
																  PRL_DEVICE_TYPE devType,
																  int iDevNum
																  )
{
	quint64  sz = 0;
	PRL_RESULT r;

	NotifyCopyEvent(lpcTaskHelper, PET_VM_INF_START_BUNCH_COPYING, devType, iDevNum);
	r =  CopyFileWithNotifications ( strSrcFile, strDestFile, pDestOwner
		, lpcTaskHelper, devType, iDevNum
		, sz, sz);
	if (PRL_ERR_SUCCESS == r)
		NotifyCopyEvent(lpcTaskHelper, PET_VM_INF_END_BUNCH_COPYING, devType, iDevNum);
	return r;
}


PRL_RESULT CFileHelperDepPart::CopyFileWithNotifications(const QString & /*strSrcFile*/,
										  const QString & /*strDestFile*/,
										  CAuthHelper* pDestOwner,
										  CDspTaskHelper * /*lpcTaskHelper*/,
										  PRL_DEVICE_TYPE /*devType*/,
										  int /*iDevNum*/,
										  quint64 /*uiTotalCopySize*/,
										  quint64 /*uiTotalCompleteSize*/
										  )
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( pDestOwner );

	PRL_RESULT retCode=PRL_ERR_OPERATION_FAILED;
// AbstractFile commented out by request from CP team
//	char * szData = 0;
//	// Calculated suze of temporary buffer
//	quint64	ui64BufSize = 0;
//
//	ICommonFile *pSrcFile = NULL, *pDstFile = NULL;
//	try
//	{
//		// open source file
//		pSrcFile = ICommonFile::CreateSpecFile(AF_INVALID_ID);
//
//		// Check is we created file abstraction correctly
//		if (!pSrcFile)
//			throw PRL_ERR_OUT_OF_MEMORY;
//
//		pSrcFile->Open(strSrcFile, FA_READ, FSHR_READ, FO_EXISTING, 0);
//
//		if (!pSrcFile->IsValid())
//		{
//			WRITE_TRACE(DBG_FATAL, "Couldn't to open source file '%s' due error %u", QSTR2UTF8(strSrcFile), pSrcFile->GetError());
//			throw (PRL_ERR_OPERATION_FAILED);
//		}
//
//		// get size of file
//		quint64 ui64FileSize = pSrcFile->GetSize();
//
//		// Change operation type to noncached in case of aligned file
//		if (!(ui64FileSize % SECTOR_SIZE))
//		{
//		    LOG_MESSAGE(DBG_DEBUG, "Reopen file in case of alignment: %s",
//		    						strSrcFile.toUtf8().data());
//
//			// Remove reference
//			pSrcFile->Delete();
//			// Reopen
//			pSrcFile = ICommonFile::CreateSpecFile(AF_INVALID_ID);
//			pSrcFile->Open(strSrcFile, FA_READ, FSHR_READ,
//						   FO_EXISTING, FF_NO_BUFFERING);
//			if (!pSrcFile->IsValid())
//			{
//				WRITE_TRACE(DBG_FATAL, "Couldn't reopen source file '%s', error %u",
//					QSTR2UTF8(strSrcFile), pSrcFile->GetError());
//				throw (PRL_ERR_OPERATION_FAILED);
//			}
//		} else
//		    LOG_MESSAGE(DBG_DEBUG, "File should not be NONcached in case of alignment: %s",
//		    						strSrcFile.toUtf8().data());
//
//
//		// position after open is undefined
//		if (pSrcFile->Seek(0, FS_BEGIN) != 0) {
//			WRITE_TRACE(DBG_FATAL,
//				"Couldn't seek to the start of src-file '%s', error %u",
//				QSTR2UTF8(strSrcFile), pSrcFile->GetError());
//			throw (PRL_ERR_OPERATION_FAILED);
//		}
//
//		// calculate buffer size
//		ui64BufSize = ui64FileSize/20;
//		// align to sector
//		ui64BufSize = ui64BufSize + SECTOR_SIZE - (ui64BufSize % SECTOR_SIZE);
//		// to optimal copying & economy memory
//		const quint64 ui64MaxBufSize = 16*1024*1024;
//		if (ui64BufSize > ui64MaxBufSize)
//			ui64BufSize = ui64MaxBufSize;
//
//		if ( !pDestOwner )
//		{
//			WRITE_TRACE(DBG_FATAL, "invalid owner" );
//			throw PRL_ERR_OPERATION_FAILED;
//		}
//
//		// open destination file
//		pDstFile = ICommonFile::CreateSpecFile(AF_INVALID_ID);
//
//		// Check is we created file abstraction correctly
//		if (!pDstFile)
//			throw PRL_ERR_OUT_OF_MEMORY;
//
//		pDstFile->Open(strDestFile, FA_WRITE, FSHR_READ, FO_CREATE, 0);
//		if(!pDstFile->IsValid())
//		{
//			WRITE_TRACE(DBG_FATAL, "Couldn't to open destination file '%s' due error %u",
//				QSTR2UTF8(strDestFile), pDstFile->GetError());
//			throw PRL_ERR_OPERATION_FAILED;
//		}
//
//		// position after open is undefined
//		if (pDstFile->Seek(0, FS_BEGIN) != 0) {
//			WRITE_TRACE(DBG_FATAL,
//				"Couldn't seek to the start of dst-file '%s', error %u",
//				QSTR2UTF8(strDestFile), pDstFile->GetError());
//			throw (PRL_ERR_OPERATION_FAILED);
//		}
//
//		if ( ! CDspAccessManager::setOwner ( strDestFile, pDestOwner, false ) )
//				throw PRL_ERR_CANT_CHANGE_OWNER_OF_FILE;
//
//
//		quint32	ui32ReadSize = 0;
//		quint64 ui64TotalReadSize = 0;
//
//		// Noncached file operates only with "block" aligned offsets in memory
//		szData = (char *)prl_valloc(ui64BufSize);
//
//		if (!szData)
//			throw PRL_ERR_CANT_ALOCA_MEM_FILE_COPY;
//
//		NotifyCopyEvent(lpcTaskHelper, PET_VM_INF_START_FILE_COPYING, devType, iDevNum);
//
//		// main copying cycle
//		PRL_UINT64 timeStampOfLastEvent = PrlGetTickCount64();
//		int nLastPercent = 0;
//		while( ui64TotalReadSize != ui64FileSize )
//		{
//			if (!pSrcFile->Read(szData, ui64BufSize, &ui32ReadSize))
//				throw PRL_ERR_FILE_READ_ERROR;
//
//			if (lpcTaskHelper->operationIsCancelled())
//				throw lpcTaskHelper->getCancelResult();
//
//			quint32 ui32WriteSize = 0;
//			pDstFile->Write(szData, ui32ReadSize, &ui32WriteSize);
//			// Check that all data was written
//			if (ui32ReadSize != ui32WriteSize)
//			{
//				quint64 uiFreeDiskSpace = 0;
//				GetDiskAvailableSpace( GetFileRoot(strDestFile),&uiFreeDiskSpace );
//
//				if (uiFreeDiskSpace <= ui32ReadSize)
//					throw PRL_ERR_FILE_DISK_SPACE_ERROR;
//				else
//					throw PRL_ERR_FILE_WRITE_ERROR;
//
//			}
//
//			ui64TotalReadSize += ui32ReadSize;
//			int nCurrentPercent = ( uiTotalCopySize == 0 )
//				?(100 * ui64TotalReadSize) / ui64FileSize
//				:(100 * ( uiTotalCompleteSize + ui64TotalReadSize ) ) / uiTotalCopySize;
//
//			// safe of event flood
//			if ( nCurrentPercent == nLastPercent
//				|| PrlGetTickCount64() <= timeStampOfLastEvent + 1 * PrlGetTicksPerSecond() )
//				continue;
//
//			// Create event for client
//			CVmEvent event(	PET_JOB_FILE_COPY_PROGRESS_CHANGED,
//							Uuid().toString(),
//							PIE_DISPATCHER
//							);
//
//			/**
//			 * Add event parameters
//			 */
//
//			event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
//				QString::number(nCurrentPercent),
//				EVT_PARAM_PROGRESS_CHANGED));
//
//			event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
//				QString::number(devType),
//				EVT_PARAM_DEVICE_TYPE));
//
//			event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
//				QString::number(iDevNum),
//				EVT_PARAM_DEVICE_INDEX));
//
//			SmartPtr<IOPackage> p =
//				DispatcherPackage::createInstance( PVE::DspVmEvent, event,
//												  lpcTaskHelper->getRequestPackage());
//
//			lpcTaskHelper->getClient()->sendPackage( p );
//
//			timeStampOfLastEvent = PrlGetTickCount64();
//			nLastPercent = nCurrentPercent;
//		}
//		NotifyCopyEvent(lpcTaskHelper, PET_VM_INF_END_FILE_COPYING, devType, iDevNum);
//
//		retCode=PRL_ERR_SUCCESS;
//	}
//	catch (PRL_RESULT ret)
//	{
//		WRITE_TRACE(DBG_FATAL,
//			"CopyFileWithNotifications [from: %s] [to: %s] failed by error [%d][ %s ]"
//			,QSTR2UTF8( strSrcFile )
//			,QSTR2UTF8( strDestFile )
//			,ret
//			, PRL_RESULT_TO_STRING( ret )
//			);
//
//		//To unblock file
//		if (pDstFile)
//		{
//			pDstFile->Delete();
//			pDstFile = NULL;
//		}
//
//		if ( ! QFile::remove( strDestFile ) )
//			WRITE_TRACE(DBG_FATAL, "QFile::remove() failed" );
//		retCode=ret;
//	}
//
//	if (szData)
//	{
//		prl_vfree(szData);
//	}
//
//	if (pSrcFile)
//	{
//		pSrcFile->Delete();
//		pSrcFile = NULL;
//	}
//
//	if (pDstFile)
//	{
//		pDstFile->Delete();
//		pDstFile = NULL;
//	}

	return retCode;
}


/**
 * @brief
 *		 copy entry
 *
 * @params
 *		 sourceDir [in]			- path of entry
 *		 targetDir [in]	- path to target dir
 *
 * @return
 *		 TRUE - rename of move was successfull
 *		 FALSE - otherwise
 */

PRL_RESULT CFileHelperDepPart::CopyDirectoryWithNotifications(const QString& strSourceDir,
																const QString& strTargetDir,
																CAuthHelper *pAuthHelper,
																CDspTaskHelper * lpcTaskHelper,
																PRL_DEVICE_TYPE devType,
																int iDevNum,
																bool bTargetDirNotParent
																)
{
	if ( pAuthHelper == 0 )
		return PRL_ERR_INVALID_ARG;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( pAuthHelper );

	//////////////////////////////////////////////////////////////////////////
	// 1. check incoming values
	QFileInfo infoSrc( strSourceDir );
	QFileInfo infoTgt( strTargetDir );

	if ( !infoSrc.isDir() || !infoSrc.isAbsolute() )
	{
		WRITE_TRACE(DBG_FATAL, "Error: source path [%s] is not absolute or is not dir.", QSTR2UTF8( strSourceDir ) );
		return PRL_ERR_INVALID_ARG;
	}

	if ( ! bTargetDirNotParent && (!infoTgt.isDir() || !infoTgt.isAbsolute()) )
	{
		WRITE_TRACE(DBG_FATAL, "Error: target path [%s] is not absolute or is not dir.", QSTR2UTF8( strTargetDir ) );
		return PRL_ERR_INVALID_ARG;
	}

	quint64 uiTotalCopySize = 0;
	quint64 uiTotalCompleteSize = 0;

	PRL_RESULT ret = CSimpleFileHelper::GetDirSize( strSourceDir, &uiTotalCopySize );
	if ( ! PRL_SUCCEEDED( ret ) )
	{
		WRITE_TRACE(DBG_FATAL, "Error: Can't get size of dir[%s] by error [%d][%s]"
			, QSTR2UTF8(strSourceDir)
			, ret
			, PRL_RESULT_TO_STRING( ret )
			);

		return ret;
	}

	NotifyCopyEvent(lpcTaskHelper, PET_VM_INF_START_BUNCH_COPYING, devType, iDevNum);
	ret = CopyDirectoryWithNotifications( strSourceDir
		,strTargetDir
		,pAuthHelper
		,lpcTaskHelper
		,devType
		,iDevNum
		,uiTotalCopySize
		,uiTotalCompleteSize
		,bTargetDirNotParent
		);
	if (ret == PRL_ERR_SUCCESS)
		NotifyCopyEvent(lpcTaskHelper, PET_VM_INF_END_BUNCH_COPYING, devType, iDevNum);

	return ret;
}

PRL_RESULT CFileHelperDepPart::CopyDirectoryWithNotifications(const QString& strSourceDir,
																 const QString& strTargetDir,
																 CAuthHelper *pAuthHelper,
																 CDspTaskHelper * lpcTaskHelper,
																 PRL_DEVICE_TYPE devType,
																 int iDevNum,
																 quint64 uiTotalCopySize,
																 quint64& uiTotalCompleteSize,
																 bool bTargetDirNotParent
																 )
{

	//////////////////////////////////////////////////////////////////////////
	// 1. create directory
	QString strNewDir;
	if ( bTargetDirNotParent )
	{
		strNewDir = strTargetDir;
	}
	else
	{
		strNewDir = QString("%1/%2").arg( strTargetDir )
			.arg( QFileInfo( strSourceDir ).fileName() );
	}

	try
	{
		if ( ! CreateDirectoryPath( strNewDir, pAuthHelper ) )
			throw PRL_ERR_MAKE_DIRECTORY;
			//throw QString("Can't create directory [%1]").arg( strNewDir );

		QList< QFileInfo > dirContent =  QDir( strSourceDir ).entryInfoList(
			QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
			QDir::DirsFirst
			);
		QListIterator< QFileInfo > it( dirContent );
		while( it.hasNext() )
		{
			const QFileInfo&	fi = it.next();

			if ( !FileCanRead( fi.filePath(), pAuthHelper ) )
				throw PRL_ERR_ACCESS_DENIED;
				//throw QString("Access denied to file [%1]").arg( fi.filePath() );

			PRL_RESULT ret = PRL_ERR_UNEXPECTED;
			if ( fi.isDir() )
			{
				// #125021, #124821 to prevent infinity recursion by QT bug in QDir::entryInfoList()
				if( QFileInfo(strSourceDir) != fi )
					ret = CopyDirectoryWithNotifications( fi.filePath(), strNewDir, pAuthHelper
							,lpcTaskHelper, devType, iDevNum
							,uiTotalCopySize, uiTotalCompleteSize);
			}
			else
			{
				QString strNewFile = QString("%1/%2").arg( strNewDir ).arg( fi.fileName() );

				ret = CopyFileWithNotifications( fi.filePath(), strNewFile, pAuthHelper
					,lpcTaskHelper, devType, iDevNum
					,uiTotalCopySize, uiTotalCompleteSize);

				if ( PRL_SUCCEEDED( ret ) )
					uiTotalCompleteSize += fi.size();
			}

			if ( ! PRL_SUCCEEDED( ret ) )
				throw ret;
				//throw QString("Copy failed to file [%s]. error [%#x]").arg( fi.filePath() ).arg( ret );
		}//while
	}
	catch (PRL_RESULT err/*QString err*/)
	{
		/*WRITE_TRACE(DBG_FATAL, "CopyDirectoryWithNotifications: Error catched: %s"
			, QSTR2UTF8( err )
			);*/

		CFileHelper::ClearAndDeleteDir( strNewDir );
		return err;
		//return PRL_ERR_FAILURE;
	}

	return PRL_ERR_SUCCESS;
}
