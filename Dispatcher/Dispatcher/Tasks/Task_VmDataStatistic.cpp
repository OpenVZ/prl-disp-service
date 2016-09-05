///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VmDataStatistic.cpp
///
/// Dispatcher task for collecting VM data staistic.
///
/// @author myakhin@
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

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include <prlcommon/HostUtils/HostUtils.h>
//#include "VI/Sources/ImageTool/resizer/StringTable.h"
#include "Task_VmDataStatistic.h"
#include "CDspService.h"

using namespace Parallels;


Task_VmDataStatistic::Task_VmDataStatistic( const SmartPtr<CDspClient>& pClient,
											const SmartPtr<IOPackage>& p,
											bool bNeedCleanUpInfoOnly)
: CDspTaskHelper(pClient, p),
  m_vmStatistic(new CSystemStatistics),
  m_pProcess(0),
  m_bNeedCleanUpInfoOnly(bNeedCleanUpInfoOnly)
{
}

Task_VmDataStatistic::~Task_VmDataStatistic()
{
	if ( m_pProcess )
	{
		delete m_pProcess;
		m_pProcess = 0;
	}
}

QString Task_VmDataStatistic::getVmUuid()
{
	return m_qsVmUuid;
}

PRL_RESULT Task_VmDataStatistic::prepareTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
		if ( ! pCmd->IsValid() )
			throw PRL_ERR_FAILURE;

		m_qsVmUuid = pCmd->GetVmUuid();

		m_fiVmHomePath = QFileInfo( CDspService::instance()->getVmDirManager().getVmHomeByUuid(getVmIdent()) )
							.path();
		if ( ! m_fiVmHomePath.exists() )
		{
			getLastError()->addEventParameter(
				new CVmEventParameter(PVE::String, m_fiVmHomePath.canonicalFilePath(), EVT_PARAM_MESSAGE_PARAM_0));
			throw PRL_ERR_DIRECTORY_DOES_NOT_EXIST;
		}

		m_pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(getVmIdent(), ret);
		if ( ! m_pVmConfig && PRL_SUCCEEDED(ret) )
		{
			PRL_ASSERT(0);
			ret = PRL_ERR_FAILURE;
		}
		if ( PRL_FAILED(ret) )
			throw ret;

		QDir pwd = QDir( (QCoreApplication::instance())->applicationDirPath() );
		m_qsDTName = ParallelsDirs::getDiskToolPath( pwd );
		if ( ! QFile::exists(m_qsDTName) )
		{
			WRITE_TRACE(DBG_FATAL, "VM data statistic: configuration tool '%s' not found",
									QSTR2UTF8(m_qsDTName) );
			throw PRL_ERR_DISK_TOOL_NOT_FOUND;
		}

		m_pProcess = new QProcess;

		// Set logging level (default WARNING to supress spam) #PDFM-33921
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		bool bVerbose = CDspService::instance()->getDispConfigGuard()
							.getDispCommonPrefs()->getDebug()->isVerboseLogEnabled();
		env.insert("PRL_LOG_LEVEL", QString::number(bVerbose ? DBG_DEBUG : DBG_WARNING));
		m_pProcess->setProcessEnvironment(env);
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while VM data statistic with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	getLastError()->setEventCode( ret );

	return ret;
}

PRL_RESULT Task_VmDataStatistic::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if ( PRL_FAILED( getLastErrorCode() ) )
			throw getLastErrorCode();

		if ( ! m_bNeedCleanUpInfoOnly )
		{
			ret = hostDiskSpaceUsage();
			if (PRL_FAILED(ret))
				throw ret;
		}
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while VM data statistic with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );
	}

	getLastError()->setEventCode( ret );

	return ret;
}

void Task_VmDataStatistic::finalizeTask()
{
	if ( operationIsCancelled() )
	{
		m_pProcess->waitForFinished(GET_DISK_INFO_TIMEWOUT_SEC * 1000);

		getLastError()->setEventCode( PRL_ERR_OPERATION_WAS_CANCELED );
	}

	if ( PRL_FAILED( getLastErrorCode() ) )
	{
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
	else
	{
		CProtoCommandPtr pResponse
			= CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS);

		CProtoCommandDspWsResponse* pDspWsResponseCmd
			= CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		pDspWsResponseCmd->SetSystemStatistics( m_vmStatistic->toString() );

		getClient()->sendResponse(pResponse, getRequestPackage());
	}
}

void Task_VmDataStatistic::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	CancelOperationSupport::cancelOperation(pUserSession, p);

	if (m_pProcess)
#ifdef _WIN_
		// FIXME: Do correct terminate #PWE-6589
		m_pProcess->kill();
#else
		m_pProcess->terminate();
#endif
}

CVmDataSegment* Task_VmDataStatistic::addSegment(PRL_DATA_STATISTIC_SEGMENTS nSegment)
{
	CVmDataSegment* pSegment = 0;
	foreach(pSegment, m_vmStatistic->m_VmDataStatistic.getVmDataSegments()->m_lstVmDataSegments)
	{
		if (pSegment->getType() == nSegment)
			return pSegment;
	}

	pSegment = new CVmDataSegment;
	pSegment->setType(nSegment);
	m_vmStatistic->m_VmDataStatistic.getVmDataSegments()->m_lstVmDataSegments
		+= pSegment;

	return pSegment;
}

CVmDataSegment* Task_VmDataStatistic::getSegment(PRL_DATA_STATISTIC_SEGMENTS nSegment) const
{
	foreach(CVmDataSegment* pSegment, m_vmStatistic->m_VmDataStatistic.getVmDataSegments()->m_lstVmDataSegments)
	{
		if (pSegment->getType() == nSegment)
			return pSegment;
	}
	// Unexpected
	PRL_ASSERT(0);
	return 0;
}

PRL_RESULT Task_VmDataStatistic::hostDiskSpaceUsage()
{
	// NOTE: It's important to keep order disk
	//       space usage calculation because
	//       the previous results are taken acount
	//       of the next calculations.

	PRL_RESULT ret = fullDiskSpaceUsage();
	if (PRL_FAILED(ret))
		return ret;

	ret = vmDataDiskSpaceUsage();
	if (PRL_FAILED(ret))
		return ret;

	ret = snapshotsDiskSpaceUsage();
	if (PRL_FAILED(ret))
		return ret;

	ret = miscellaneousDiskSpaceUsage();
	if (PRL_FAILED(ret))
		return ret;

	ret = reclaimDiskSpaceUsage();
	if (PRL_FAILED(ret))
		return ret;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VmDataStatistic::fullDiskSpaceUsage()
{
	quint64 nFullSize = 0;

// VM bundle size

	PRL_RESULT ret = CSimpleFileHelper::GetDirSize(m_fiVmHomePath.canonicalFilePath(), &nFullSize);
	if (PRL_FAILED(ret))
		return ret;

// + out VM bundle data size

	foreach(CVmHardDisk* pHdd, m_pVmConfig->getVmHardwareList()->m_lstHardDisks)
	{
		if ( ! (pHdd->getEmulatedType() == (PVE::HardDiskEmulatedType)PDT_USE_IMAGE_FILE
				|| pHdd->getEmulatedType() == (PVE::HardDiskEmulatedType)PDT_USE_OUTPUT_FILE) )
			continue;

		QFileInfo fiDevData(pHdd->getSystemName());
		if ( ! fiDevData.exists() )
			// Skip not an existing file
			// (deleted or placed on removable device)
			continue;
		if ( fiDevData.canonicalFilePath().startsWith(m_fiVmHomePath.canonicalFilePath()) )
			// Inside bundle VM
			continue;

		quint64 nDataSize = 0;
		if (fiDevData.isDir())
			CSimpleFileHelper::GetDirSize(fiDevData.canonicalFilePath(), &nDataSize);
		else
			nDataSize = fiDevData.size();

		nFullSize += nDataSize;
	}

	addSegment(PDSS_VM_FULL_SPACE)->setCapacity(nFullSize);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VmDataStatistic::vmDataDiskSpaceUsage()
{
	quint64 nVmDataSize = 0;

	foreach(CVmHardDisk* pHardDisk, m_pVmConfig->getVmHardwareList()->m_lstHardDisks)
	{
		if (pHardDisk->getEmulatedType() != (PVE::HardDiskEmulatedType)PDT_USE_IMAGE_FILE)
			continue;

		QFileInfo fiHardDisk(pHardDisk->getSystemName());
		if ( ! fiHardDisk.exists() )
			continue;

// Whole hard disk bundle size

		quint64 nDataSize = 0;
		CSimpleFileHelper::GetDirSize(fiHardDisk.canonicalFilePath(), &nDataSize);
		nVmDataSize += nDataSize;
	}

// Hard disk bundle size

	addSegment(PDSS_VM_DISK_DATA_SPACE)->setCapacity(nVmDataSize);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VmDataStatistic::snapshotsDiskSpaceUsage()
{
	quint64 nSnapshotsSize = 0;

	QFileInfo fiSnapshots( m_fiVmHomePath.canonicalFilePath() + "/"VM_GENERATED_WINDOWS_SNAPSHOTS_DIR );
	if (fiSnapshots.exists())
		CSimpleFileHelper::GetDirSize(fiSnapshots.canonicalFilePath(), &nSnapshotsSize);

	addSegment(PDSS_VM_SNAPSHOTS_SPACE)->setCapacity(nSnapshotsSize);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_VmDataStatistic::miscellaneousDiskSpaceUsage()
{
	quint64 nFullSize = getSegment(PDSS_VM_FULL_SPACE)->getCapacity();
	quint64 nVmDataSize = getSegment(PDSS_VM_DISK_DATA_SPACE)->getCapacity();
	quint64 nSnapshotsSize = getSegment(PDSS_VM_SNAPSHOTS_SPACE)->getCapacity();
	PRL_ASSERT( nFullSize >= nVmDataSize + nSnapshotsSize );

	quint64 nMiscSize = nFullSize - nVmDataSize - nSnapshotsSize;

	addSegment(PDSS_VM_MISCELLANEOUS_SPACE)->setCapacity(nMiscSize);

	return PRL_ERR_SUCCESS;
}

// VirtualDisk commented out by request from CP team
//static void enter_password(QProcess* pProcess)
//{
//	if ( pProcess )
//	{
//		QString qsPassword = pProcess->property("password").toString();
//		if ( ! qsPassword.isEmpty() )
//			pProcess->write(qsPassword.toUtf8());
//	}
//}

QStringList Task_VmDataStatistic::getReclaimFilesList(bool bWithLostSnapshotFiles)
{
	QStringList lstReclaimFiles;

	// Unused unattended files

	bool bUnattendedFddFound = false;
	foreach(CVmFloppyDisk* pFloppyDisk, m_pVmConfig->getVmHardwareList()->m_lstFloppyDisks)
	{
		if (pFloppyDisk->getEmulatedType() != (PVE::FloppyEmulatedType)PDT_USE_IMAGE_FILE)
			continue;

		QFileInfo fi = pFloppyDisk->getSystemName();
		if ( fi.fileName() == UNATTENDED_FDD )
		{
			bUnattendedFddFound = true;
			break;
		}
	}
	if ( ! bUnattendedFddFound )
		lstReclaimFiles << UNATTENDED_FDD;

	bool bUnattendedIsoFound = false;
	foreach(CVmOpticalDisk* pOpticDisk, m_pVmConfig->getVmHardwareList()->m_lstOpticalDisks)
	{
		if (pOpticDisk->getEmulatedType() != (PVE::CdromEmulatedType)PDT_USE_IMAGE_FILE)
			continue;

		QFileInfo fi = pOpticDisk->getSystemName();
		if ( fi.fileName() == UNATTENDED_ISO )
		{
			bUnattendedIsoFound = true;
			break;
		}
	}
	if ( ! bUnattendedIsoFound )
		lstReclaimFiles << UNATTENDED_ISO;

	// Dumps
	QFileInfoList filDumps = QDir( m_fiVmHomePath.canonicalFilePath() )
		.entryInfoList(QStringList("*.dmp"), QDir::Files | QDir::NoSymLinks | QDir::Hidden);
	foreach(QFileInfo fiDump, filDumps)
	{
		lstReclaimFiles << fiDump.fileName();
	}

	if ( CDspVm::getVmState(getVmIdent()) == VMS_STOPPED )
	{
		lstReclaimFiles
			<< PRL_VM_SUSPENDED_SCREEN_FILE_NAME;

		QFileInfoList filMems = QDir( m_fiVmHomePath.canonicalFilePath() )
			.entryInfoList(QStringList("{*}.mem*"), QDir::Files | QDir::NoSymLinks | QDir::Hidden);
		foreach(QFileInfo fiMem, filMems)
		{
			lstReclaimFiles << fiMem.fileName();
		}
	}

	QStringList lstReclaimFilePaths;
	foreach(QString qsFileName, lstReclaimFiles)
	{
		QFileInfo fiReclaim = m_fiVmHomePath.canonicalFilePath() +  "/" + qsFileName;
		if ( ! fiReclaim.exists() )
			continue;

		lstReclaimFilePaths += fiReclaim.canonicalFilePath();
	}

	// Lost snapshot files

	if ( bWithLostSnapshotFiles )
	{
		lstReclaimFilePaths += getLostSnapshotFiles();
	}

	return lstReclaimFilePaths;
}

QStringList Task_VmDataStatistic::getLostSnapshotFiles()
{
	QStringList lstReclaimFiles;

	// Search all snapshot files

	QFileInfoList filSnapshots
		= QDir( m_fiVmHomePath.canonicalFilePath() + "/"VM_GENERATED_WINDOWS_SNAPSHOTS_DIR )
			.entryInfoList(QStringList("{*}.*"), QDir::Files | QDir::NoSymLinks | QDir::Hidden);
	foreach(QFileInfo fiSnapshot, filSnapshots)
	{
		lstReclaimFiles << fiSnapshot.fileName();
	}
	if ( lstReclaimFiles.isEmpty() )
		return QStringList();

	// Get snapshot tree

	CSavedStateStore ssTree;
	if ( ! CDspService::instance()->getVmSnapshotStoreHelper()
		.getSnapshotsTree( MakeVmIdent(getVmUuid(), getClient()->getVmDirectoryUuid()), &ssTree )
		)
		return QStringList();

	// Check snapshot in tree

	foreach(QString qsFileName, lstReclaimFiles)
	{
		QString qsUuid = qsFileName.split(".").first();
		if ( ! ssTree.GetSavedStateTree()->FindByUuid(qsUuid) )
			continue;
		lstReclaimFiles.removeAll(qsFileName);
	}

	QStringList lstReclaimFilePaths;
	foreach(QString qsFileName, lstReclaimFiles)
	{
		QFileInfo fiReclaim = m_fiVmHomePath.canonicalFilePath()
								+  "/"VM_GENERATED_WINDOWS_SNAPSHOTS_DIR"/" + qsFileName;
		if ( ! fiReclaim.exists() )
			continue;

		lstReclaimFilePaths += fiReclaim.canonicalFilePath();
	}

	return lstReclaimFilePaths;
}

PRL_RESULT Task_VmDataStatistic::reclaimDiskSpaceUsage()
{
// VirtualDisk commented out by request from CP team
//// Reclaim files size
//
//	quint64 nReclaimFilesSize = 0;
//	foreach(QString qsFilePath, getReclaimFilesList(false))
//	{
//		nReclaimFilesSize += QFileInfo(qsFilePath).size();
//	}
//
//	// Correct miscellaneous capacity
//	CVmDataSegment* pSegment = getSegment(PDSS_VM_MISCELLANEOUS_SPACE);
//	quint64 nMiscSize = pSegment->getCapacity();
//	PRL_ASSERT ( nMiscSize >= nReclaimFilesSize );
//	nMiscSize -= nReclaimFilesSize;
//	pSegment->setCapacity(nMiscSize);
//
//	quint64 nLostSnapshotFilesSize = 0;
//	foreach(QString qsFilePath, getLostSnapshotFiles())
//	{
//		nLostSnapshotFilesSize += QFileInfo(qsFilePath).size();
//	}
//	nReclaimFilesSize += nLostSnapshotFilesSize;
//
//	// Correct snapshots capacity
//	pSegment = getSegment(PDSS_VM_SNAPSHOTS_SPACE);
//	quint64 nSnapshotsSize = pSegment->getCapacity();
//	PRL_ASSERT ( nSnapshotsSize >= nLostSnapshotFilesSize );
//	nSnapshotsSize -= nLostSnapshotFilesSize;
//	pSegment->setCapacity(nSnapshotsSize);
//
//// Reclaim disk(s) size
//
//	quint64 nReclaimDisksSize = 0;
//
//	foreach(CVmHardDisk* pHardDisk, m_pVmConfig->getVmHardwareList()->m_lstHardDisks)
//	{
//		if ( operationIsCancelled() )
//			return PRL_ERR_OPERATION_WAS_CANCELED;
//
//		if (pHardDisk->getEmulatedType() != (PVE::HardDiskEmulatedType)PDT_USE_IMAGE_FILE)
//			continue;
//
//		QFileInfo fiHardDisk(pHardDisk->getSystemName());
//		if ( ! fiHardDisk.exists() )
//			continue;
//
//		QString qsCmd = QString("\"%1\" compact --hdd \"%2\" --info")
//						.arg(m_qsDTName,
//						QFileInfo(pHardDisk->getSystemName()).isDir()
//						? pHardDisk->getSystemName()
//						: QFileInfo(pHardDisk->getSystemName()).canonicalPath());
//
//		QString qsOutput;
//		if ( ! HostUtils::RunCmdLineUtility(
//				qsCmd, qsOutput, GET_DISK_INFO_TIMEWOUT_SEC * 1000, m_pProcess, enter_password)
//			)
//			continue;
//
//		// Output example:
//		//
//		// Operation progress 100 %
//		// Disk information:
//		//              Block size:                   512
//		//              Total blocks:              262144
//		//              Allocated blocks:           26719
//		//              Used blocks:                26719
//		//              Operation supported:          YES
//
//		quint64 nSectorSize = SECTOR_SIZE;
//		quint64 nBlockSize = 0;
//		quint64 nAllocatedBlocks = 0;
//		quint64 nUsedBlocks = 0;
//		bool	bCompactable = false;
//
//		QStringList lstResLines = qsOutput.split("\n");
//		foreach(QString qsRes, lstResLines)
//		{
//			if (qsRes.contains(IDS_DISK_INFO__SUPPORTED))
//			{
//				qsRes.remove(IDS_DISK_INFO__SUPPORTED);
//				if (qsRes.contains(IDS_DISK_INFO__YES))
//					bCompactable = true;
//				else
//				{
//					bCompactable = false;
//					break;
//				}
//			}
//			if (qsRes.contains(IDS_DISK_INFO__BLOCK_SIZE))
//				nBlockSize = qsRes.remove(IDS_DISK_INFO__BLOCK_SIZE).toULongLong();
//			if (qsRes.contains(IDS_DISK_INFO__BLOCKS_ALLOCATED))
//				nAllocatedBlocks = qsRes.remove(IDS_DISK_INFO__BLOCKS_ALLOCATED).toULongLong();
//			if (qsRes.contains(IDS_DISK_INFO__BLOCKS_USED))
//				nUsedBlocks = qsRes.remove(IDS_DISK_INFO__BLOCKS_USED).toULongLong();
//		}
//
//		if ( ! bCompactable )
//			continue;
//
//		nReclaimDisksSize += (nAllocatedBlocks - nUsedBlocks) * nBlockSize * nSectorSize;
//	}
//
//	// Correct VM data capacity
//	pSegment = getSegment(PDSS_VM_DISK_DATA_SPACE);
//	quint64 nVmDataSize = pSegment->getCapacity();
//	PRL_ASSERT ( nVmDataSize >= nReclaimDisksSize );
//	nVmDataSize -= nReclaimDisksSize;
//	pSegment->setCapacity(nVmDataSize);
//
//// Summed reclaim size
//
//	addSegment(PDSS_VM_RECLAIM_SPACE)->setCapacity(nReclaimDisksSize + nReclaimFilesSize);
//
//	return PRL_ERR_SUCCESS;
	return PRL_ERR_FAILURE;
}
