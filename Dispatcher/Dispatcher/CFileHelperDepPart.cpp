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

#include <prlcommon/Interfaces/ParallelsQt.h>

#include "CDspTaskHelper.h"
#include "CDspAccessManager.h"
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameter.h>
#include "CDspUserHelper.h"
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>

#include "Tasks/Task_CloneVm.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlTime.h>
//#include "Libraries/AbstractFile/CommonFile.h"  // AbstractFile commented out by request from CP team
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/CSimpleFileHelper.h>

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

namespace
{

///////////////////////////////////////////////////////////////////////////////
// struct CopyProgress

struct CopyProgress: QFile
{
	CopyProgress(const QString& filename_, const QString& uuid_, CDspTaskHelper* taskHelper_,
			PRL_DEVICE_TYPE devType_, int devNum_)
		: QFile(filename_),
			m_uuid(uuid_),
			m_total(0),
			m_read(0),
			m_currentPercent(0),
			m_taskHelper(taskHelper_),
			m_devType(devType_),
			m_devNum(devNum_)
	{
		m_total = QFile::size();
	}

protected:
	virtual qint64 readData(char* data, qint64 maxSize_)
	{
		qint64 r = QFile::readData(data, maxSize_);
		handleWrittenBytes(r);
		return r;
	}

private:
	void handleWrittenBytes(qint64 bytes_)
	{
		m_read += bytes_;
		quint32 c(((double)m_read)/((double)m_total) * 100.0 + 0.5);
		if (m_currentPercent == c || c == 100)
			return;

		m_currentPercent = c;
		notifyCurrentProgress();
	}

	void notifyCurrentProgress()
	{
		CVmEvent event(PET_JOB_FILE_COPY_PROGRESS_CHANGED, m_uuid, PIE_DISPATCHER);

		event.addEventParameter(new CVmEventParameter(PVE::UnsignedInt,
			QString::number(m_currentPercent),
			EVT_PARAM_PROGRESS_CHANGED));
		event.addEventParameter(new CVmEventParameter(PVE::UnsignedInt,
			QString::number(m_devType),
			EVT_PARAM_DEVICE_TYPE));
		event.addEventParameter(new CVmEventParameter(PVE::UnsignedInt,
			QString::number(m_devNum),
			EVT_PARAM_DEVICE_INDEX));

		SmartPtr<IOPackage> p =
			DispatcherPackage::createInstance(PVE::DspVmEvent, event,
											  m_taskHelper->getRequestPackage());

		m_taskHelper->getClient()->sendPackage(p);
	}

private:
	QString m_uuid;
	quint64 m_total;
	quint64 m_read;
	quint32 m_currentPercent;
	CDspTaskHelper* m_taskHelper;
	PRL_DEVICE_TYPE m_devType;
	int m_devNum;
};

} // anonymous namespace


PRL_RESULT CFileHelperDepPart::CopyFileWithNotifications(const QString & source_,
																  const QString & dest_,
																  CAuthHelper* owner_,
																  CDspTaskHelper* taskHelper_,
																  PRL_DEVICE_TYPE devType_,
																  int devNum_
																  )
{
	NotifyCopyEvent(taskHelper_, PET_VM_INF_START_BUNCH_COPYING, devType_, devNum_);

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate(owner_);

	NotifyCopyEvent(taskHelper_, PET_VM_INF_START_FILE_COPYING, devType_, devNum_);

	CopyProgress p(source_, Uuid().toString(), taskHelper_, devType_, devNum_);
	if (!p.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
		return PRL_ERR_FILE_NOT_FOUND;
	if (!p.copy(dest_))
		return PRL_ERR_OPERATION_FAILED;
	if (!CDspAccessManager::setOwner(source_, owner_, false))
		return PRL_ERR_CANT_CHANGE_OWNER_OF_FILE;

	NotifyCopyEvent(taskHelper_, PET_VM_INF_END_FILE_COPYING, devType_, devNum_);
	NotifyCopyEvent(taskHelper_, PET_VM_INF_END_BUNCH_COPYING, devType_, devNum_);
	return PRL_ERR_SUCCESS;;
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
					,lpcTaskHelper, devType, iDevNum);

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
