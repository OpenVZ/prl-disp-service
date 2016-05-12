///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspProblemReportHelper.h
///
/// helper functions for problem report dispatcher side collection
///
/// @author artemr@
///
/// Copyright (c) 2009-2015 Parallels IP Holdings GmbH
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

#include "CDspProblemReportHelper.h"

#include <QFile>
#include <QByteArray>
#include <QDataStream>
#include <QBuffer>
#include <QMap>
#include <QtAlgorithms>

#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlsdk/PrlEventsValues.h>
#include <prlsdk/PrlEnums.h>
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "Build/Current.ver"

#include "CDspService.h"
#include "CDspVmInfoDatabase.h"
#include "CDspShellHelper.h"

#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/Messaging/CVmBinaryEventParameter.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/GuestOsInformation/CVmGuestOsInformation.h>

#include "Libraries/ProblemReportUtils/CPackedProblemReport.h"
#include "Libraries/ProblemReportUtils/CInstalledSoftwareCollector.h"
#include "Libraries/ProblemReportUtils/CProblemReportUtils.h"
#include "Libraries/ProblemReportUtils/CProblemReportPostWrap.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/CSimpleFileHelper.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirsDefs.h>

#ifdef _WIN_
#include <prlcommon/PrlCommonUtilsBase/countof.h>
#include "Libraries/WinDbgUtils/EventLog.h"
#include "Libraries/WinDbgUtils/MiniDump.h"
#include <list>
#include <string>
#include <algorithm>
#endif


#include "CDspVzHelper.h"
#include "CDspLibvirtExec.h"

#ifndef EXTERNALLY_AVAILABLE_BUILD

// uncomment this define to use report profiler
//#define _PRL_USE_PROBLEM_REPORT_PROFILER_

#endif //EXTERNALLY_AVAILABLE_BUILD

#ifdef _PRL_USE_PROBLEM_REPORT_PROFILER_

	static QTime g_Time;

	#define WRITE_REPORT_PROFILER_STRING( szData ) \
	{ \
		int iSpentMsecs = g_Time.msecsTo( QTime::currentTime() );\
		g_Time = QTime::currentTime();\
		WRITE_TRACE( DBG_FATAL, "PROB_REPORT_PROFILER %s : spentTime = %i",\
		szData, iSpentMsecs );\
	}\

	#define INIT_PROBLEM_REPORT_PROFILER_TIME \
	{\
		g_Time = QTime::currentTime();\
		WRITE_TRACE( DBG_FATAL, "PROB_REPORT_PROFILER ProfileStarted Time = %s",\
		QSTR2UTF8( QTime::currentTime().toString( "hh:mm:ss.zzz" ) ) );\
	}\

#else

	#define WRITE_REPORT_PROFILER_STRING( szData ) void(szData);
	#define INIT_PROBLEM_REPORT_PROFILER_TIME

#endif //_PRL_USE_PROBLEM_REPORT_PROFILER_

#define CRASH_DUMPS_MAX_COUNT				10
#define STATISTICS_DATA_MAX_SIZE			10*1024*1024
#define GUEST_CRASH_DUMP_DATA_MAX_SIZE		10*1024*1024
#define GUEST_CRASH_DUMPS_MAX_COUNT			5
#define GUEST_CRASH_DUMPS_MAX_AGE_IN_DAYS	7
#define HOST_PROCS_MINIDUMPS_MAX_COUNT		9
#define HOST_PROC_MINIDUMP_CREATE_CMD_TIMEOUT	1 * 60 * 1000
#define REPORT_COMMAND_TIMEOUT			3 * 60 * 1000

///////////////////////////////////////////////////////////////////////////////
// struct Trace

struct Trace
{
	explicit Trace(const QString& uuid_):
	m_guest(Libvirt::Kit.vms().at(uuid_).getGuest())
	{
	}

	void collect(unsigned timeout_ = 1000)
	{
		m_guest.traceEvents(true);
		HostUtils::Sleep(timeout_);
		m_guest.traceEvents(false);
	}

private:
	Libvirt::Instrument::Agent::Vm::Guest m_guest;
};

class CVzConfig
{
public:
	CVzConfig(const QString& path) :
		m_qsPath(path) {};
	~CVzConfig() {};

	/* reads config and loads data */
	int load();

	/* returns stored data */
	QString getData() const {
		return m_qsData;
	}

	/* set stored data */
	void setData(const QString &data) {
		m_qsData = data;
	}

private:
	bool ShouldSkip(const QString &line);
	QString	m_qsPath;
	QString	m_qsData;
};

bool CVzConfig::ShouldSkip(const QString &cline)
{
	QString line(cline.trimmed());
	if (line.isEmpty() || line.startsWith('#'))
		return true;
        QStringList list = line.split(QChar('='));
        QString key = list.at(0).trimmed();
	QStringList qlExceptions;
	qlExceptions << QString("IP_ADDRESS");
	qlExceptions << QString("EXT_IP_ADDRESS");
	qlExceptions << QString("HOSTNAME");
	qlExceptions << QString("NAMESERVER");
	qlExceptions << QString("SEARCHDOMAIN");
	foreach(QString str, qlExceptions)
	{
		if (key == str)
			return true;
	}
	return false;
}

int CVzConfig::load()
{
	QFile file(m_qsPath);
	if (!file.open(QIODevice::ReadOnly))
	         return 1;

	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		if (!ShouldSkip(line)) {
			m_qsData.append(line);
			m_qsData.append("\n");
		}
	}
	return 0;
}

/** Create package with problem report event*/
SmartPtr<IOPackage> CDspProblemReportHelper::createProblemReportEventPackage(
												CPackedProblemReport & cReport,
												CProblemReportUtils::ReportVersion version,
												const SmartPtr<IOPackage>& parent,
												bool bDublicateHeader)
{
#define RETURN_ERROR_PACKAGE \
		if ( parent )\
		{\
			CProtoCommandPtr pResponse =\
				CProtoSerializer::CreateDspWsResponseCommand( parent, PRL_ERR_OPERATION_FAILED );\
			SmartPtr<IOPackage> response =\
				DispatcherPackage::createInstance( PVE::DspWsResponse, pResponse, parent, false, bDublicateHeader );\
			return response;\
		}\
		else\
			return SmartPtr<IOPackage>(0);

	PVE::IDispatcherCommands cmd = ( !parent.getImpl() ? PVE::DspVmBinaryEvent : PVE::DspWsBinaryResponse );

	CVmBinaryEventParameter * pBinaryEventParam = NULL;
	pBinaryEventParam =	new CVmBinaryEventParameter(EVT_PARAM_VM_PROBLEM_REPORT);

	if( PRL_FAILED( cReport.packReport() ) )
	{
		WRITE_TRACE(DBG_FATAL, "Fatal error - couldn't pack problem report");
		RETURN_ERROR_PACKAGE
	}

	QFile packedFile( cReport.getArchivePath() );
	if( !packedFile.open( QIODevice::ReadOnly ) )
	{
		WRITE_TRACE(DBG_FATAL, "Fatal error - couldn't open packed problem report file");
		RETURN_ERROR_PACKAGE
	}

	QByteArray data = packedFile.readAll();
	SmartPtr<QDataStream> pStream = pBinaryEventParam->getBinaryDataStream();
	pStream->writeRawData( data.data(), data.size() );


	CVmConfiguration vmConfig;
	QString strVmUuid;
	if( PRL_SUCCEEDED( vmConfig.fromString( cReport.getVmConfigFromArchive() ) ) )
		strVmUuid = vmConfig.getVmIdentification()->getVmUuid();

	SmartPtr<CVmEvent> pProblemReportEvent(
		new CVmEvent(PET_DSP_EVT_VM_PROBLEM_REPORT_CREATED,
		strVmUuid, PIE_VIRTUAL_MACHINE) );

	if ( pBinaryEventParam )
		pProblemReportEvent->addEventParameter( pBinaryEventParam );

	CVmEventParameter * pEventParam =
		new CVmEventParameter( PVE::String, cReport.getTempDirPath(), EVT_PARAM_VM_PROBLEM_REPORT_DIR_PATH);

	pProblemReportEvent->addEventParameter( pEventParam );

	pEventParam =
		new CVmEventParameter( PVE::Integer, QString::number(version) , EVT_PARAM_VM_PROBLEM_REPORT_VERSION );

	pProblemReportEvent->addEventParameter( pEventParam );

	// for problem report always success !
	pProblemReportEvent->setEventCode( PRL_ERR_SUCCESS );
	QByteArray _byte_array;
	QBuffer _buffer(&_byte_array);
	bool bRes = _buffer.open(QIODevice::ReadWrite);
	PRL_ASSERT(bRes);
	if (!bRes)
	{
		WRITE_TRACE(DBG_FATAL, "Fatal error - couldn't to open binary data buffer for read/write");
		RETURN_ERROR_PACKAGE
	}
	QDataStream _data_stream(&_buffer);
	_data_stream.setVersion(QDataStream::Qt_4_0);
	if ( PVE::DspWsBinaryResponse == cmd )
	{
		CProtoCommandPtr pResponseCmd = CProtoSerializer::CreateDspWsResponseCommand(PVE::DspCmdVmGetPackedProblemReport, PRL_ERR_SUCCESS);
		pResponseCmd->GetCommand()->Serialize(_data_stream);
	}
	pProblemReportEvent->Serialize(_data_stream);
	_buffer.reset();

	return DispatcherPackage::createInstance(cmd,
		_data_stream,
		_byte_array.size(),
		parent,
		bDublicateHeader);
}

SmartPtr<IOPackage> CDspProblemReportHelper::createProblemReportOldFormatEventPackage(
	CProblemReport & cReport,
	const SmartPtr<IOPackage>& parent,
	bool bDublicateHeader)
{
	CVmBinaryEventParameter *pBinaryEventParam =
		new CVmBinaryEventParameter(EVT_PARAM_VM_PROBLEM_REPORT);
	QString sProblemReport = cReport.toString(true);
	CPrlStringDataSerializer(sProblemReport).Serialize(*pBinaryEventParam->getBinaryDataStream().getImpl());

	CVmConfiguration vmConfig;
	vmConfig.fromString(cReport.getVmConfig());

	SmartPtr<CVmEvent> pProblemReportEvent(
		new CVmEvent(PET_DSP_EVT_VM_PROBLEM_REPORT_CREATED,
		vmConfig.getVmIdentification()->getVmUuid(), PIE_VIRTUAL_MACHINE) );
	pProblemReportEvent->addEventParameter(pBinaryEventParam);
	// for problem report always success !
	pProblemReportEvent->setEventCode( PRL_ERR_SUCCESS );
	QByteArray _byte_array;
	QBuffer _buffer(&_byte_array);
	bool bRes = _buffer.open(QIODevice::ReadWrite);
	PRL_ASSERT(bRes);
	if (!bRes)
	{
		WRITE_TRACE(DBG_FATAL, "Fatal error - couldn't to open binary data buffer for read/write");
		return SmartPtr<IOPackage>();
	}
	QDataStream _data_stream(&_buffer);
	_data_stream.setVersion(QDataStream::Qt_4_0);
	pProblemReportEvent->Serialize(_data_stream);
	_buffer.reset();

	PVE::IDispatcherCommands	cmd;
	if ((cReport.getReportType() == PRT_AUTOMATIC_VM_GENERATED_REPORT) ||
		(cReport.getReportType() == PRT_AUTOMATIC_DISPATCHER_GENERATED_REPORT) ||
		(cReport.getReportType() == PRT_AUTOMATIC_DETECTED_REPORT) ||
		(cReport.getReportType() == PRT_AUTOMATIC_GUEST_GENERATED_REPORT)
		)
		cmd = PVE::DspVmBinaryEvent;
	else
		cmd = PVE::DspVmBinaryResponse;

	return DispatcherPackage::createInstance(cmd,
		_data_stream,
		_byte_array.size(),
		parent,
		bDublicateHeader);
}

#ifdef _LIN_
// calc amount of run Containers
static unsigned int calcRunContainers()
{
	char buf[80];
	unsigned int num = 0;
	int res;
	int dummy1, dummy2, ctid;
	FILE *file;

	file = fopen("/proc/vz/veinfo", "r");
	if (!file)
		return 0;
	while (fgets(buf, sizeof(buf), file))
	{
		res = sscanf(buf,"%d %d %d", &ctid,
			&dummy1, &dummy2);
		if (res != 3 || ctid == 0 || ctid == 1)
			continue;
		num++;
	}

	fclose(file);
	return num;
}

#endif

static bool isValidFileSignature( const QString& path )
{
	(void)path;
	return false;
}

static bool isValidDispatcherSignature()
{
	return isValidFileSignature( QCoreApplication::applicationFilePath() );
}

void CDspProblemReportHelper::FillCommonReportData( CProblemReport & cReport, bool /*bHidePrivateData*/ )
{

	//////////////////////////////////////////////////////////////////////////
	// add server product version!
	CVmConfiguration cfg;
	cfg.makeAppVersion();
	cReport.setServerVersion(cfg.getAppVersion());
	cReport.setServerRevision( VER_FEATURES_STR );
	cReport.setProductName( VER_SHORTPRODUCTNAME_STR );

#ifdef _LIN_
	QFile f("/etc/parallels-release");

	if(f.open(QIODevice::ReadOnly))
	{
		cReport.setIsoVersion( CProblemReportUtils::extractIsoVersion(f.readLine()) );
	}
	else
	{
		WRITE_TRACE(DBG_FATAL,
			"Failed to open file '/etc/parallels-release': %s", QSTR2UTF8(f.errorString()));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// add computer model
	cReport.setComputerModel( CProblemReportUtils::generateComputerModel() );

	cReport.getSignInfo()->setDispatcherSigned( isValidDispatcherSignature() );


	//////////////////////////////////////////////////////////////////////////
	// set host info data
	{
		// in brackets to prevent deadlock with locked hwinfo
		CDspService::instance()->getHostInfo()->updateMemSettings();
	}
	CHostHardwareInfo hostHwInfo(CDspService::instance()->getHostInfo()->data());

	cReport.setHostInfo( hostHwInfo.toString() );


        //////////////////////////////////////////////////////////////////////////
        // set network config data
        cReport.setNetConfig( CDspService::instance()->getNetworkConfig()->toString() );

#ifdef _LIN_
        if (CVzHelper::is_vz_running())
        {
            ContainersInfo *pCTInfo = new ContainersInfo;

            // set vz global config
            CVzConfig VzConfig(QString("/etc/vz/vz.conf"));
            VzConfig.load();
            pCTInfo->setVzConfig(VzConfig.getData());

            // set Vz networking config
            CVzConfig VzNetConfig(QString("/etc/vz/vznet.conf"));
            VzNetConfig.load();
            pCTInfo->setVzNetConfig(VzNetConfig.getData());

            // set amount of run Containers
            pCTInfo->setContainersRunning(calcRunContainers());
            cReport.setContainersInfo(pCTInfo);

            // gather vz report
            QString sReportFile;
            QString sVzReportCollector("/usr/libexec/vzreport/vzreport_collector -p");
	    QStringList lstEnv;
	    lstEnv += QString("%1=%2").arg("VZREPORT_COLLECTOR_TIMEOUT").arg(REPORT_COMMAND_TIMEOUT/1000); // in seconds
	    QProcess proc;
	    proc.setEnvironment(lstEnv);
	    bool result = HostUtils::RunCmdLineUtility(sVzReportCollector, sReportFile, REPORT_COMMAND_TIMEOUT, &proc);
            if( !sReportFile.isEmpty() )
            {
                // sample of the first line of the output:
                // "Collecting report in /tmp/vzreport.manual.176961/vzreport.tgz\n"
                QString path = sReportFile.split("\n").first().split(" ").last();
                QFileInfo fVzReport(path);

                if( result )
                {
                    cReport.appendSystemLog(fVzReport.filePath(), fVzReport.fileName());
                    // report data copied to parallels report - cleanup original data
                    QFile::remove(fVzReport.filePath());
                }

                QDir tmpDir(fVzReport.dir());
                if (tmpDir.dirName().contains("vzreport.manual."))
                {
                    // cleanup temporary dir also
                    CFileHelper::ClearAndDeleteDir(tmpDir.path());
                }
            }

        }

        QFileInfo updateInfo( "/var/log/anaconda_report" );
        if( updateInfo.exists() )
            cReport.appendSystemLog( updateInfo.filePath(), updateInfo.fileName() );

        QFileInfo shamanInfo( "/var/log/shaman.log" );
        if( shamanInfo.exists() )
            cReport.appendSystemLog( shamanInfo.filePath(), shamanInfo.fileName() );
#endif
	//////////////////////////////////////////////////////////////////////////
	// set host statistic data
	cReport.setHostStatistic( CDspStatisticsGuard::GetStatisticsStringRepresentation() );

	//////////////////////////////////////////////////////////////////////////
	// set installed software list
	cReport.setInstalledSoftware( CInstalledSoftwareCollector::getInstalledSoftwareListSync() );
}

void CDspProblemReportHelper::FillTimeZoneData( CProblemReport & cReport )
{
	//////////////////////////////////////////////////////////////////////////
	// Set time zone (in seconds)
	QDateTime dtCurrent = QDateTime::currentDateTime();
	QDateTime dtUTC = dtCurrent.toUTC();
	dtUTC.setTimeSpec(Qt::LocalTime);

	int nTimeZone = dtUTC.secsTo( dtCurrent );
	cReport.setTimeZone(nTimeZone);

	//////////////////////////////////////////////////////////////////////////
	// Set time zone (in string presentation)
	QString sTimezone;
	QString sAbbr;
#if defined(_WIN_)
	{
		__time64_t ltime;
		struct tm today;
		_time64( &ltime );
		if (0 == _localtime64_s( &today, &ltime ))
		{
			char stz[100];
			strftime(stz, sizeof(stz), "%Z", &today);
			sTimezone.append(stz);
			sAbbr = QString("isdst=%1").arg(today.tm_isdst);
		}
	}
#elif defined(_LIN_)
	{
		time_t ltime;
		struct tm today;
		time( &ltime );
		localtime_r( &ltime, &today );
		char stz[100];
		strftime(stz, sizeof(stz), "%Z", &today);
		sTimezone.append(stz);
		sAbbr = QString("isdst=%1").arg(today.tm_isdst);
	}
#else
#error "Unknow build platform!"
#endif
	cReport.setTimeZoneStr(QString("%1, %2").arg(sAbbr).arg(sTimezone));
}

/**
* wait wile data will be saved to strMonitoringFile.
* we are watches file size.
* if file size not modified at least internal timeout time - then waiting complete
**/
void CDspProblemReportHelper::waitWhileKernelDataWillBeSaved( const QString & strMonitoringFile )
{
	// wait while data will be save to messages file
	const int iWaitTimeMs = 500;
	QFile file( strMonitoringFile );
	qint64 iSize = file.size();
	// to prevent possible deadlock - set max 60 attempts - 30 sec by den request
	int iAttempts = 0;
	HostUtils::Sleep( iWaitTimeMs );
	while( 1 )
	{
		HostUtils::Sleep( iWaitTimeMs );
		qint64 iActualSize = file.size();
		if ( ( iSize == iActualSize ) || ( iAttempts >= 60 ) )
			break;
		iSize = iActualSize;
		iAttempts++;
	}
}

static void clearInternalDataFromChilds( CSavedStateTree *pTree )
{
	QList<CSavedStateTree*> * childs = pTree->GetChilds();
	for( int i = 0 ; i < childs->size() ; i++ )
	{
		CSavedStateTree * pChild = childs->at( i );
		if( !pChild )
			continue;

		pChild->SetScreenShot( "" ); // DROP huge files as base64
		pChild->SetDescription( QString("PRL_LEN=%1").arg( pChild->GetDescription().size() )
			); // DROP CDATA sections
		if( pChild->GetChildCount() != 0 )
			clearInternalDataFromChilds( pChild );
	}
}

static void addDiskDescriptors( CRepAdvancedVmInfo *const pAdvancedVmInfo,
								const CVmConfiguration & vmConfig )
{
	foreach(CVmHardDisk* pHardDisk, vmConfig.getVmHardwareList()->m_lstHardDisks)
	{
		CRepHdd* pHdd = new CRepHdd();

		pHdd->setIndex(QString::number(pHardDisk->getIndex()));
		QString qsDiskDescriptor = QFileInfo( pHardDisk->getSystemName() ).filePath()
			+ "/"DISK_DESCRIPTOR_XML;
		QFile file(qsDiskDescriptor);
		if (file.open(QIODevice::ReadOnly))
		{
			pHdd->setDiskInfo(UTF8_2QSTR(file.readAll()));
			file.close();
		}

		pAdvancedVmInfo->m_lstHdds += pHdd;
	}
}

static void addSnapshots( CRepAdvancedVmInfo *const pAdvancedVmInfo, const QString & strVmHome )
{
	// set snapshot xml
	QFileInfo fileInfo( strVmHome );
	QString sSnapshotsTreePath =
		fileInfo.dir().absolutePath() + "/" + VM_GENERATED_SNAPSHOTS_CONFIG_FILE;

	CSavedStateStore ssTree;
	if (! CDspService::instance()->getVmSnapshotStoreHelper().getSnapshotsTree( strVmHome, &ssTree))
		return;

	// remove pictures from xml
	CSavedStateTree *pTree = ssTree.GetSavedStateTree();
	if( !pTree )
		return;

	clearInternalDataFromChilds( pTree );

	QString strTempFilePath = QDir::tempPath();
	strTempFilePath += "/";
	strTempFilePath += QUuid::createUuid().toString();
	ssTree.Save( strTempFilePath );
	QFile fileToLoad( strTempFilePath );
	if( !fileToLoad.open( QIODevice::ReadOnly ) )
		return;

	pAdvancedVmInfo->setSnapshots( UTF8_2QSTR( fileToLoad.readAll() ) );
	fileToLoad.close();
	QFile::remove(strTempFilePath );
}

static void addVmAdvancedInfoToReport( CProblemReport & cReport,
									const CVmConfiguration & vmConfig )
{
	CRepAdvancedVmInfo* pAdvancedVmInfo = new CRepAdvancedVmInfo();

	// set disk descriptors xml
	addDiskDescriptors(pAdvancedVmInfo, vmConfig);

	// set snapshots xml
	addSnapshots( pAdvancedVmInfo, vmConfig.getVmIdentification()->getHomePath() );
	cReport.setAdvancedVmInfo(pAdvancedVmInfo);

}

static void addSysrqTriggerToReport( CProblemReport & cReport )
{
#ifdef _LIN_

	if( QFile::exists( "/proc/sysrq-trigger" ) )
	{
		// collect data to /var/log/messages again and save it log!
		QFile file( "/proc/sysrq-trigger" );
		// three iteration
		for( int i = 0 ; i < 3 ; i++ )
			if ( file.open( QIODevice::WriteOnly ) )
			{
				file.write( "w", sizeof( char ) );
				file.close();
			}

		QString strTmp = "/var/log/messages";
		CDspProblemReportHelper::waitWhileKernelDataWillBeSaved( strTmp );

		cReport.appendSystemLog( strTmp, QFileInfo(strTmp).fileName() );
	}

#else
	Q_UNUSED( cReport )
#endif // _LIN_

}

static void addCrashDumpsToReport( CProblemReport & cReport,
								  const ParallelsDirs::UserInfo* pUserInfo,
								  unsigned maxDumpsCount)
{
	unsigned dumpsAdded = 0;

	{
		QStringList crashDirList;

		(void)pUserInfo;
		crashDirList << ParallelsDirs::getCrashDumpsPath();
		QStringList strLogs = CDspVmDirHelper::getListOfLastCrashedLogs( crashDirList );

		for (int i = 0 ; i < strLogs.size() ; i++)
		{
			if (dumpsAdded >= maxDumpsCount)
				break;

			QString dumpPath = strLogs[i];

			// Read dump
			QFile dump( dumpPath );
			bool o = dump.open( QFile::ReadOnly );
			if ( ! o )
			{
				WRITE_TRACE( DBG_DEBUG, "Error: can't open crash dump '%s' for reading!",
					QSTR2UTF8(dumpPath));
				continue;
			}
			QByteArray dumpBa = dump.readAll();
			dump.close();

			CRepCrashDump* crashDump = new CRepCrashDump;
			crashDump->setDump( dumpBa.toBase64() );
			crashDump->setPath( dumpPath );
			crashDump->setApplicationPid( QString("%1").arg(-1) );
			crashDump->setCreationDateTime( QFileInfo( dumpPath ).lastModified()
				.toString(XML_DATETIME_FORMAT_LONG) );

			cReport.appendCrashDump( crashDump );

			++dumpsAdded;
		}

	}
}

bool sortByLogTimeModificationRoutine( const QPair < QString, QString >& s1,
										const QPair < QString, QString >& s2 )
{
	QFileInfo info1( s1.first + QString("/") + PRL_COMMON_LOGFILENAME );
	QFileInfo info2( s2.first + QString("/") + PRL_COMMON_LOGFILENAME );

	QDateTime dateTime1;
	if( info1.exists() )
		dateTime1 = info1.lastModified();

	QDateTime dateTime2;
	if( info2.exists() )
		dateTime2 = info2.lastModified();

	return dateTime1 > dateTime2;
}

static QString getVmLogPath( const QString & strVmHome )
{
	return strVmHome + QString("/") + PRL_COMMON_LOGFILENAME;
}

static QString getVmReconfigLogPath( const QString & strVmHome )
{
	return strVmHome + QString("/") + PRL_RECONFIGURATION_LOGFILENAME;
}

static void addFreshRegisteredVmLogsAndConfigs( CProblemReport & cReport,
									 const QString & strVmDirUuid,
									 const QString & strVmUuidToExclude )
{
	// copy from locked pointer data
	QList< QPair < QString, QString > > lstAllVmsData;
	{
		CDspLockedPointer<CVmDirectory> pCommonVmDir =
			CDspService::instance()->getVmDirManager().getVmDirectory( strVmDirUuid );

		if( !pCommonVmDir )
			return;

		foreach( CVmDirectoryItem * pItem, pCommonVmDir->m_lstVmDirectoryItems )
		{
			if( !pItem )
				continue;

			QString strVmHomeDir = CFileHelper::GetFileRoot( pItem->getVmHome() );
			if( !QFile::exists( getVmLogPath( strVmHomeDir ) ) || strVmUuidToExclude == pItem->getVmUuid() )
				continue;

			lstAllVmsData << QPair < QString, QString >( strVmHomeDir, pItem->getVmUuid() );
		}
	}

	qSort( lstAllVmsData.begin(), lstAllVmsData.end(), sortByLogTimeModificationRoutine );

	// get logs from 3 fresh vms
	int iSize = ( lstAllVmsData.size() < 3 ) ? lstAllVmsData.size() : 3;
	for ( int i = 0 ; i < iSize ; i++ )
	{
		QPair < QString, QString > item = lstAllVmsData.at( i );
		// save vm config
		cReport.appendSystemLog( item.first + QString("/") + VMDIR_DEFAULT_VM_CONFIG_FILE,
								QString("vm-%1-config.pvs.log").arg( item.second ) );

		// save vm log
		cReport.appendTemplateSystemLog( getVmLogPath( item.first ),
										 QString("vm-%1.log").arg( item.second ),
										 2);
	}
}


void CDspProblemReportHelper::FillVmProblemReportData
	(CPackedProblemReport& cReport, CVmConfiguration &vmConfig, const QString & strDirUuid)
{
	QString strVmUuid = vmConfig.getVmIdentification()->getVmUuid();
	QString strVmHome = CDspVmDirManager::getVmHomeByUuid( MakeVmIdent( strVmUuid, strDirUuid ) );
	vmConfig.getVmIdentification()->setHomePath( strVmHome );

	PRL_ASSERT( !strVmHome.isEmpty() );
	QString strVmDir = CFileHelper::GetFileRoot( strVmHome );

	QString strParallelsLogPath = getVmLogPath( strVmDir );
	WRITE_REPORT_PROFILER_STRING( "addVmLog" );
	cReport.appendTemplateSystemLog( strParallelsLogPath, "vm.log", 2 );

	QString strReconfigLogPath = getVmReconfigLogPath( strVmDir );
	WRITE_REPORT_PROFILER_STRING( "addVmReconfigLog" );
	cReport.appendSystemLog( strReconfigLogPath, "vm_reconfiguration.log" );

	WRITE_REPORT_PROFILER_STRING( "addVmSerialPortLogs" );
	foreach( CVmSerialPort * pVmSerialPort, vmConfig.getVmHardwareList()->m_lstSerialPorts )
	{
		if ( pVmSerialPort->getEmulatedType() == PVE::SerialOutputFile )
		{
			cReport.appendSystemLog( pVmSerialPort->getSystemName(),
					QString( "vm-serial%1.log" ).arg( pVmSerialPort->getIndex() ) );
		}
	}

	WRITE_REPORT_PROFILER_STRING( "addAdvancedVmInfo" );
	addVmAdvancedInfoToReport( cReport, vmConfig );
	cReport.setVmConfig( vmConfig.toString() );

	// Append domain description
	Libvirt::Instrument::Agent::Vm::Unit u = Libvirt::Kit.vms().at(strVmUuid);
	QString strDomainDesc;
	u.getConfig(strDomainDesc);
	cReport.setVmDomain(strDomainDesc);

	bool isRunning = CDspVm::getVmState(strVmUuid, strDirUuid) == VMS_RUNNING;

	QString tmpFileName = QDir::temp()
		.absoluteFilePath("vz-%1-%2").arg(strVmUuid)
		.arg(QDateTime::currentMSecsSinceEpoch());

	QString screenImage = tmpFileName + ".pnm";
	// QEMU renerates screenshot in PNM format
	u.getGuest().dumpScreen(screenImage);

	// Convert to PNG and attach
	QProcess convertImage;
	convertImage.setStandardOutputFile(screenImage + ".png");
	QString output;
	if (HostUtils::RunCmdLineUtility("pnmtopng " + screenImage, output, 10000, &convertImage))
		cReport.appendScreenshot(screenImage + ".png", "screenshot.png");

	QFile::remove(screenImage);
	QFile::remove(screenImage + ".png");

	QString stateFile = tmpFileName + ".state";
	Prl::Expected<Libvirt::Instrument::Agent::Vm::Command::Future, Error::Simple> e = 
		u.getGuest().dumpState(stateFile);

	if (e.isSucceed())
		e.value().wait(10000);

	// It doesn't matter if wait failed or migration failed, we need to try unpause VM.
	VIRTUAL_MACHINE_STATE currentState;
	if ((isRunning && u.getState(currentState).isFailed()) ||
		(currentState == VMS_PAUSED && u.unpause().isFailed()))
	{
		WRITE_TRACE(DBG_FATAL, "Problem report got error. VM %s may be paused", qPrintable(strVmUuid));
	}

	// Try to add file even after error
	cReport.appendSystemLog(stateFile, "qemu-statefile.gz");
	QFile::remove(stateFile);

	const QDateTime minDumpTime = QDateTime::currentDateTime().addDays(-GUEST_CRASH_DUMPS_MAX_AGE_IN_DAYS);
	WRITE_REPORT_PROFILER_STRING( "addGuestCrashDumps" );
	CDspProblemReportHelper::AddGuestCrashDumps( cReport, strVmDir,
												 &minDumpTime,
												 GUEST_CRASH_DUMPS_MAX_COUNT );

	//////////////////////////////////////////////////////////////////////////
	// Add Guest Os Information
	//
	CVmGuestOsInformation vmGuestOsInfo;
	const QString sPath = ParallelsDirs::getVmInfoPath( CFileHelper::GetFileRoot( strVmHome ) );
	SmartPtr<CVmInfo> pVmInfo = CDspVmInfoDatabase::readVmInfo( sPath );
	if ( pVmInfo && pVmInfo->getGuestOsInformation() )
		vmGuestOsInfo = *(pVmInfo->getGuestOsInformation());

	CVmCommonOptions *pCommonOptions = vmConfig.getVmSettings()->getVmCommonOptions();
	vmGuestOsInfo.setConfOsType ( PVS_GUEST_TO_STRING ( pCommonOptions->getOsType() ) );
	vmGuestOsInfo.setConfOsVersion ( PVS_GUEST_TO_STRING ( pCommonOptions->getOsVersion() ) );

	if ( cReport.getGuestOs().isEmpty() )
		vmGuestOsInfo.setStarted( false );
	else
		vmGuestOsInfo.setStarted( true );

	cReport.setGuestOs( vmGuestOsInfo.toString() );
}

void CDspProblemReportHelper::FillCtProblemReportData()
{

}

namespace
{
void addSystemLog(CProblemReport& report, const QFileInfo& info)
{
	if (info.exists())
		report.appendSystemLog(info.filePath(), info.fileName());
}
}

/**
* fill data to problem report object .
*
**/

void CDspProblemReportHelper::FillProblemReportData
	(CPackedProblemReport& cReport, const SmartPtr<CDspClient>& pUser, const QString& strDirUuid)
{
	INIT_PROBLEM_REPORT_PROFILER_TIME

	CAuthHelperImpersonateWrapperPtr pImpersonate;
	ParallelsDirs::UserInfo info, *pUserInfo = 0;

	const bool bVerboseLog = CDspService::instance()->getDispConfigGuard().
		getDispCommonPrefs()->getDebug()->isVerboseLogEnabled();

	if ( pUser.isValid() )
	{
		pUserInfo = &(info = pUser->getAuthHelper().getParallelsDirUserInfo());
		pImpersonate = CAuthHelperImpersonateWrapper::create( &pUser->getAuthHelper() );
	}

	FillCommonReportData( cReport, false );
	FillTimeZoneData( cReport );

	WRITE_REPORT_PROFILER_STRING( "GetMoreHostInfo" );
	cReport.setMoreHostInfo( HostUtils::GetMoreHostInfo() );

	WRITE_REPORT_PROFILER_STRING( "GetAllProcesses" );
	cReport.setAllProcesses( HostUtils::GetAllProcesses(bVerboseLog) );

	//////////////////////////////////////////////////////////////////////////
	// set dispatcher config data
	{
		CDispatcherConfig tempCfg(CDspService::instance()->getDispConfigGuard().getDispConfig().getPtr());
		tempCfg.getDispatcherSettings()->getCommonPreferences()->getWorkspacePreferences()
			->setHeadlessModeEnabled( CDspShellHelper::isHeadlessModeEnabled() );
		cReport.setAppConfig(tempCfg.toString());
	}

        //////////////////////////////////////////////////////////////////////////
        // set vm directory data
        cReport.setVmDirectory(
            CDspService::instance()->getVmDirManager().getVmDirCatalogue()->toString() ) ;
    /**
    * add common log files
    */
    WRITE_REPORT_PROFILER_STRING( "addCommonLogFiles" );
	CProblemReportUtils::addCommonLogFiles( cReport );

        // add current virtual machine log to problem report
        CVmConfiguration vmConfig;
        CPackedProblemReport * pPackRep = dynamic_cast<CPackedProblemReport*>(&cReport);
        QString strVmCfg;
        if ( pPackRep )
            strVmCfg = pPackRep->getVmConfigFromArchive();
        else
            strVmCfg = cReport.getVmConfig();

        bool bHasValidVmCfg = PRL_SUCCEEDED( vmConfig.fromString(strVmCfg) );
        PRL_VM_TYPE nType = PVT_CT;
        if (bHasValidVmCfg)
            nType = vmConfig.getVmType();

        QString strVmUuid = vmConfig.getVmIdentification()->getVmUuid();

        bool bVmRunning =
            ( CDspVm::getVmState( strVmUuid , strDirUuid ) == VMS_RUNNING );

	QFuture<void> trace;
	if (bVmRunning && nType == PVT_VM)
	{
		trace = QtConcurrent::run(Trace(strVmUuid), &Trace::collect, 15000);
	}

        if ( bHasValidVmCfg ) {
            if ( nType == PVT_VM )
                FillVmProblemReportData( cReport, vmConfig, strDirUuid );
            else
                FillCtProblemReportData();
        }

        if( !bHasValidVmCfg || !bVmRunning )
            addFreshRegisteredVmLogsAndConfigs( cReport, strDirUuid, strVmUuid );

	WRITE_REPORT_PROFILER_STRING( "addSysrqTriggerToReport" );
	addSysrqTriggerToReport( cReport );

	WRITE_REPORT_PROFILER_STRING( "addCrashDumpsToReport" );
	addCrashDumpsToReport( cReport, pUserInfo, CRASH_DUMPS_MAX_COUNT );


	WRITE_REPORT_PROFILER_STRING( "GetLoadedDrivers" );
	cReport.setLoadedDrivers( HostUtils::GetLoadedDrivers() );

	WRITE_REPORT_PROFILER_STRING( "GetMountInfo" );
	cReport.setMountInfo( HostUtils::GetMountInfo() );

#ifdef _LIN_
	addSystemLog(cReport, QFileInfo("/var/log/shaman.log"));
	QFileInfoList a = CDspHaClusterHelper::getReport();
	foreach (QFileInfo f, a)
	{
		cReport.appendSystemLog(f.filePath(), f.fileName());
		QFile::remove(f.filePath());
	}
	addSystemLog(cReport, QFileInfo("/var/log/cpufeatures.log"));
	addSystemLog(cReport, QFileInfo("/var/log/ploop.log"));
	addSystemLog(cReport, QFileInfo("/var/log/pcompact.log"));
	if (nType == PVT_VM)
	{
		if (bVmRunning)
			trace.waitForFinished();

		cReport.appendSystemLog(
			QString("/var/log/libvirt/qemu/%1.log")
				.arg(vmConfig.getVmIdentification()->getVmName()),
			QString("libvirt-%1.log")
				.arg(vmConfig.getVmIdentification()->getVmName()));
	}
	addSystemLog(cReport, QFileInfo("/var/log/libvirt/libvirtd.log"));
	addSystemLog(cReport, QFileInfo("/var/log/vcmmd.log"));
#endif
	WRITE_REPORT_PROFILER_STRING( "EndOfFillProblemReport" );
}

void CDspProblemReportHelper::FormProblemReportDataForDisconnect(CProblemReport & cReport,
														 SmartPtr<CVmConfiguration> pVmConfig,
														 PRL_PROBLEM_REPORT_TYPE type)
{
	if( pVmConfig )
		cReport.setVmConfig(pVmConfig.getImpl()->toString());

	cReport.setReportType(type);
}

bool CDspProblemReportHelper::getProblemReport(SmartPtr<CDspClient> pUser,
    const SmartPtr<IOPackage> &p, bool bSendByTimeout)
{
    CProblemReportUtils::ReportVersion iVersion;
    switch( p->header.type )
    {
    case PVE::DspCmdVmGetPackedProblemReport:
        iVersion = CProblemReportUtils::packVersion;
        break;
    case PVE::DspCmdVmGetProblemReport:
    case PVE::DspCmdSendProblemReport:
        pUser->sendSimpleResponse(p, PRL_ERR_VM_GET_PROBLEM_REPORT_FAILED);
        WRITE_TRACE(DBG_FATAL, "Requested problem report type is not supported");
        return false;
    default:
        iVersion = CProblemReportUtils::endVersion;
        break;
    }

    SmartPtr<CPackedProblemReport> pReport = getProblemReportObj(pUser, p, bSendByTimeout);
    if (0 == pReport.getImpl())
        return false;

    SmartPtr<IOPackage> newPackage;
    if ( ! CDspProblemReportHelper::isOldProblemReportClient(pUser, p) )
    {
        newPackage =
            CDspProblemReportHelper::createProblemReportEventPackage( *pReport.getImpl(), iVersion, p );
    }
    else
    {
        SmartPtr<CProblemReport> pOldReport = pReport->convertToProblemReportOldFormat();
        newPackage =
            CDspProblemReportHelper::createProblemReportOldFormatEventPackage( *pOldReport.getImpl(), p );
    }
    pUser->sendPackage(newPackage);
    WRITE_TRACE(DBG_FATAL, " problem report %s was posted to client %s",
        bSendByTimeout?"by timeout":"",
        QSTR2UTF8(pUser->getUserName()) );
    return true;
}

SmartPtr<CPackedProblemReport> CDspProblemReportHelper::getProblemReportObj(
                                        SmartPtr<CDspClient> pUser,
										const SmartPtr<IOPackage>&p,
										bool bSendByTimeout)
{
	PRL_VM_TYPE nType = PVT_VM;

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand( p );
	if ( ! cmd->IsValid() ) {
		// Send error
		pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
        return SmartPtr<CPackedProblemReport>();
	}

	//create "Problem Report" directory and fix permissions
    CProblemReportUtils::GetProblemReportPath( ParallelsDirs::getCommonDefaultVmCatalogue() );

	QString sVmUuid = cmd->GetVmUuid();
	CDspService::instance()->getVmDirManager().getVmTypeByUuid(sVmUuid, nType);
	WRITE_TRACE(DBG_FATAL, "Creating report for vm %s", qPrintable(sVmUuid));

        CPackedProblemReport * pTmpReport = NULL;
        CPackedProblemReport::createInstance( CPackedProblemReport::DispSide, &pTmpReport );
        SmartPtr<CPackedProblemReport> pReport( pTmpReport );
        if ( !pReport )
		{
			WRITE_TRACE(DBG_FATAL, "cannot create report instance!");
			PRL_ASSERT(false);
			pUser->sendSimpleResponse( p, PRL_ERR_FAILURE );
            return SmartPtr<CPackedProblemReport>();
		}

        if (p->header.type == PVE::DspCmdSendProblemReport)
        {
            // The incoming report being received with the command DspCmdSendProblemReport
            // has the old xml-based format.

            // TODO XXX FIXME: it is required to add other info to the report such as logs
            // JIRA: #PM-6888
            SmartPtr<CProblemReport> xmlPR(new CProblemReport);
			xmlPR->fromString(CProtoSerializer::CastToProtoCommand<CProtoSendProblemReport>(cmd)->GetReportData());

			// Save report reason
			pReport->setReportReason(xmlPR->getReportReason());

            // Save ClientInfo
            if (ClientInfo *const ci = xmlPR->getClientInfo())
                pReport->setClientInfo(new ClientInfo(ci));
        }

		PRL_RESULT res = PRL_ERR_SUCCESS;
		SmartPtr<CVmConfiguration> pVmConfig;
		if (nType == PVT_VM) {
			pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(
				pUser,
				sVmUuid,
				res);
		}
#ifdef _CT_
		else {
			pVmConfig = CDspService::instance()->getVzHelper()->getCtConfig(pUser, sVmUuid);
		}
#endif

		if( pVmConfig )
			CDspProblemReportHelper::FormProblemReportDataForDisconnect( *pReport.getImpl(), pVmConfig );
		else
		{
			WRITE_TRACE(DBG_FATAL, "Unable to get vm config by uuid %s, error %s"
				, QSTR2UTF8( sVmUuid )
				, PRL_RESULT_TO_STRING( res ) );
		}

/*		FIXME enable back when new perfCounters platform will be ready
		QString qsPerfCountersInfo = CDspService::instance()
			->getRequestToVmHandler().getPerfCountersInfo(Uuid::toString(p->header.uuid));

		pReport->setPerformanceCounters( qsPerfCountersInfo ); */

		CDspProblemReportHelper::FillProblemReportData( *pReport.getImpl(), pUser, pUser->getVmDirectoryUuid() );
		// set report type
		if(bSendByTimeout)
		{
			pReport->setReportType(PRT_USER_DEFINED_ON_NOT_RESPONDING_VM_REPORT);
		}
		else if (sVmUuid.isEmpty())
		{
			pReport->setReportType(PRT_USER_DEFINED_ON_CONNECTED_SERVER);
		}
		else if (nType == PVT_VM)
		{
			switch(CDspVm::getVmState(sVmUuid, pUser->getVmDirectoryUuid()))
			{
			case VMS_RUNNING:
			case VMS_PAUSED:
				pReport->setReportType(PRT_USER_DEFINED_ON_RUNNING_VM_REPORT);
				break;
			default:
				pReport->setReportType(PRT_USER_DEFINED_ON_STOPPED_VM_REPORT);
			}
		}
		else
		{
			pReport->setReportType(PRT_USER_DEFINED_ON_CONTAINER_REPORT);
		}

		pReport->saveMainXml();
        return pReport;
}

bool CDspProblemReportHelper::isOldProblemReportClient(SmartPtr<CDspClient> pClient, const SmartPtr<IOPackage>& p)
{
	IOCommunication::ProtocolVersion _proto_version;
	memset(&_proto_version, 0, sizeof(IOCommunication::ProtocolVersion));

	if ( pClient
		 && CDspService::instance()->getIOServer().clientProtocolVersion(pClient->getClientHandle(), _proto_version)
		 && ! IOPROTOCOL_BINARY_RESPONSE_SUPPORT(_proto_version)
		)
	{
		return true;
	}

	if ( p && p->header.type == PVE::DspCmdVmGetProblemReport )
		return true;

	return false;
}

void CDspProblemReportHelper::getOldAndNewProblemReportClients( QString vmDirUuid,
																QString vmUuid,
																QList< SmartPtr<CDspClient> >& lstOldClients,
																QList< SmartPtr<CDspClient> >& lstNewClients,
																const SmartPtr<IOPackage>& p)
{
	QList< SmartPtr<CDspClient> > lstAllClients =
		CDspService::instance()->getClientManager().getSessionListByVm( vmDirUuid, vmUuid ).values();

	lstOldClients.clear();
	lstNewClients.clear();

	foreach(SmartPtr<CDspClient> pClient, lstAllClients)
	{
		if (isOldProblemReportClient(pClient, p))
			lstOldClients += pClient;
		else
			lstNewClients += pClient;
	}
}

void CDspProblemReportHelper::AddVmMemoryDump(CProblemReport & cReport,
											  const SmartPtr<CDspClient>& pUser,
											  const QString &sVmHome )
{
	if ( sVmHome.isEmpty() )
		return;

	CAuthHelperImpersonateWrapperPtr pImpersonate;
	if ( pUser.isValid() )
		pImpersonate = CAuthHelperImpersonateWrapper::create( &pUser->getAuthHelper() );

	QDir _vm_home_dir( sVmHome );

	QFileInfo fiLastDump;
	QFileInfoList _entries = _vm_home_dir.entryInfoList( QStringList() << "*.dmp",
		QDir::Files | QDir::NoSymLinks | QDir::Hidden, QDir::Time );
	foreach( const QFileInfo &_dump, _entries )
	{
		if ( "memory.dmp" == _dump.fileName() || "memory.elf.dmp" == _dump.fileName()
			|| "monitor.dmp" == _dump.fileName()//Skip OS memory dump
			|| _dump.fileName().startsWith("memory_") )
			continue;

		fiLastDump = _dump;
		break;
	}

	_entries = _vm_home_dir.entryInfoList( QStringList() << "memory_?*.dmp",
		QDir::Files | QDir::NoSymLinks | QDir::Hidden );
	_entries += fiLastDump;

	foreach( const QFileInfo &_dump, _entries )
	{
		CRepMemoryDump* pMemoryDump = new CRepMemoryDump;
		pMemoryDump->setPath( _dump.absoluteFilePath() );//For new packed scheme absolute path to file
		cReport.appendMemoryDump( pMemoryDump );
	}
}

void CDspProblemReportHelper::AddGuestCrashDumps( CProblemReport & cReport, const QString &sVmHome,
												  const QDateTime *const minDumpTime,
												  const unsigned maxDumpsCount )
{
	if ( sVmHome.isEmpty() )
		return;

	const QString dumpsDirPath = ParallelsDirs::getVmGuestCrashDumpsDir( sVmHome );
	WRITE_TRACE( DBG_DEBUG, "Searching guest crash dumps in \"%s\"",
				 QSTR2UTF8( dumpsDirPath ) );
	const QDir::Filters dumpsFilter = QDir::Files | QDir::NoSymLinks | QDir::Hidden;
	QStringList dumpsNameFilter;
	dumpsNameFilter.append( "*.dmp" );
	dumpsNameFilter.append( "*.crash" );
	dumpsNameFilter.append( "*.dump" );

	QDir dumpsDir( dumpsDirPath );
	QFileInfoList dumpsList = dumpsDir.entryInfoList( dumpsNameFilter, dumpsFilter, QDir::Time );

	unsigned dumpsAdded = 0;
	const int dumpsFound = dumpsList.size();
	WRITE_TRACE( DBG_DEBUG, "Total %i guest crash dump(s) found",
				 (int)dumpsFound );
	for ( int i = 0; dumpsFound > i; ++i )
	{
		const QFileInfo &dump = dumpsList.at(i);
		const QString dumpPath = dump.absoluteFilePath();
		const QDateTime dumpModTime = dump.lastModified();
		const QString dumpModTimeStr = dumpModTime.toString(XML_DATETIME_FORMAT_LONG);

		if ( 0 != minDumpTime && dumpModTime < *minDumpTime )
		{
			const QString minDumpTimeStr = minDumpTime->toString(XML_DATETIME_FORMAT_LONG);
			WRITE_TRACE( DBG_DEBUG, "Guest crash dump #%i \"%s\" was created at \"%s\" "
						 "and older than \"%s\", will be ignored with other %i dump(s)",
						 (int)i, QSTR2UTF8(dumpPath),
						 QSTR2UTF8(dumpModTimeStr), QSTR2UTF8(minDumpTimeStr),
						 (int)(dumpsFound - i - 1));
			break;
		}

		WRITE_TRACE( DBG_DEBUG, "Adding guest crash dump #%i \"%s\" to report",
					 (int)i, QSTR2UTF8( dumpPath ) );

		QByteArray dumpData;
		if ( !CSimpleFileHelper::ReadfromFile( dumpPath, GUEST_CRASH_DUMP_DATA_MAX_SIZE,
											   dumpData, false, false ) )
		{
			WRITE_TRACE( DBG_WARNING, "Failed to read guest crash dump \"%s\"",
						 QSTR2UTF8(dumpPath));
			continue;
		}

		CRepCrashDump *pCrashDump = new CRepCrashDump;
		pCrashDump->setDump( dumpData.toBase64() );
		pCrashDump->setPath( dumpPath );
		pCrashDump->setApplicationPid( QString( "%1" ).arg( 0 ) );
		pCrashDump->setCreationDateTime( dumpModTimeStr );
		cReport.appendCrashDump( pCrashDump );

		++dumpsAdded;
		if ( maxDumpsCount == dumpsAdded )
			break;
	}
}

void CDspProblemReportHelper::AddProcsMiniDumps(CProblemReport & cReport,
												const QStringList & procsNames,
												const unsigned maxDumpsCount)
{
	Q_UNUSED(cReport);

	if (procsNames.isEmpty() || !maxDumpsCount)
		return;

#ifdef _WIN_

	QString tmpPath = ParallelsDirs::getSystemTempDir();
	PRL_ASSERT(!tmpPath.isEmpty());
	PRL_ASSERT(QDir(tmpPath).exists());

	if (tmpPath.isEmpty())
		return;

	tmpPath += "\\" + Uuid::createUuid().toString();
	if (!QDir().mkdir(tmpPath))
		return;

	unsigned dumpsCount = 0;
	Q_FOREACH(const QString& procName, procsNames)
	{
		QStringList pids = HostUtils::GetProcessPidsByName(procName);
		Q_FOREACH(const QString& pid, pids)
		{
			if (dumpsCount >= maxDumpsCount)
				break;

			DWORD dwPid = (DWORD)pid.toULong();
			QString procDumpTool = ParallelsDirs::getProcDumpToolPath(WinDbgUtils::Is64BitProc(dwPid));
			QString dumpFilepath = QString("%1\\%2_%3.dmp").arg(tmpPath).arg(procName).arg(pid);

			if (!QFile::exists(procDumpTool))
			{
				if (!WinDbgUtils::GenerateMiniDump(dwPid, (LPCWSTR)QSTR2UTF16(dumpFilepath)))
					continue;
			}
			else
			{
				// prl_procdump <pid> <dumppath>
				QString sOut;
				QString cmd = QString("\"%1\" %2 \"%3\"").arg(procDumpTool).arg(pid).arg(dumpFilepath);
				if (!HostUtils::RunCmdLineUtility(cmd, sOut, HOST_PROC_MINIDUMP_CREATE_CMD_TIMEOUT))
					continue;
			}

			CRepMemoryDump* pMemoryDump = new CRepMemoryDump;
			pMemoryDump->setPath(dumpFilepath);
			cReport.appendMemoryDump(pMemoryDump);
			QFile::remove(dumpFilepath);

			++dumpsCount;
		} // pids
	} // procsnames

	QDir().rmdir(tmpPath);

#endif //_ WIN_
}
