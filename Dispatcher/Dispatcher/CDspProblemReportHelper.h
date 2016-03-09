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

#ifndef __CDsp_ProblemReportHelper_H_
#define __CDsp_ProblemReportHelper_H_

#include "Libraries/ProblemReportUtils/CProblemReportUtils.h"
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/IOService/IOCommunication/IOServer.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include "CDspClient.h"
#include "CDspVm.h"

class CProblemReport;

class CDspProblemReportHelper
{
public:

	/** Create package with problem report event*/
	static SmartPtr<IOPackage> createProblemReportEventPackage( CPackedProblemReport & cReport,
			CProblemReportUtils::ReportVersion version = CProblemReportUtils::packVersion,
			const SmartPtr<IOPackage>& parent = SmartPtr<IOService::IOPackage>(),
			bool bDublicateHeader = false );

	/** Create package with problem report old format event*/
	static SmartPtr<IOPackage> createProblemReportOldFormatEventPackage(CProblemReport & cReport,
		const SmartPtr<IOPackage>& parent = SmartPtr<IOService::IOPackage>(),
		bool bDublicateHeader = false);

	/**
	* Fill Common Reports data ( common for StatisticReport and ProblemReport )
	*
	**/
	static void FillCommonReportData( CProblemReport & cReport, bool bHidePrivateData = false );

	/**
	* Fill TimeZone data ( common for StatisticReport and ProblemReport )
	*
	**/
	static void FillTimeZoneData( CProblemReport & cReport );

	/**
	* fill data to problem report object .
	* note this function uses locked pointers
	* do not call it in lock space
	**/
	static void FillProblemReportData
		(CPackedProblemReport& cReport,	const SmartPtr<CDspClient>& pUser, const QString& strDirUuid);

	/**
	* fill data to problem report object if generated on disconnect vm.
	*
	**/
	static void FormProblemReportDataForDisconnect(CProblemReport & cReport,
			SmartPtr<CVmConfiguration> pVmConfig,
			PRL_PROBLEM_REPORT_TYPE type = PRT_AUTOMATIC_DISPATCHER_GENERATED_REPORT);


	/**
	* wait wile data will be saved to strMonitoringFile.
	* we are watches last change date modified.
	* if date not modified at least internal timeout time-then waiting complete
	**/
	static void waitWhileKernelDataWillBeSaved( const QString & strMonitoringFile );

    // collect and send problem report for vm
	static bool getProblemReport (SmartPtr<CDspClient> pUser,
			const SmartPtr<IOPackage>&p,
			bool bSendByTimeout = false );

    // This command composes CProblemReport at a disp side
    // In case of error it returns SmartPtr<..>(). Answers for the callers are also
    // included to this function in case of errors.
    static SmartPtr<CPackedProblemReport> getProblemReportObj (SmartPtr<CDspClient> pUser,
            const SmartPtr<IOPackage>&p,
            bool bSendByTimeout = false );

	/**
	 * Check client protocol <= 6.1 version to form problem report old format
	 */
	static bool isOldProblemReportClient(SmartPtr<CDspClient> pClient,
										 const SmartPtr<IOPackage>& p = SmartPtr<IOService::IOPackage>());

	/**
	 * Get old and new clients to send them old and new problem report
	 */
	static void getOldAndNewProblemReportClients(QString vmDirUuid,
												 QString vmUuid,
												 QList< SmartPtr<CDspClient> >& lstOldClients,
												 QList< SmartPtr<CDspClient> >& lstNewClients,
												 const SmartPtr<IOPackage>& p = SmartPtr<IOService::IOPackage>());
	/**
	 * Adds last monitor memory dump to report from specified VM home dir
	 * @param problem report object reference
	 * @param path to VM home dir
	 */
	static void AddVmMemoryDump( CProblemReport & cReport, const SmartPtr<CDspClient>& pUser, const QString &sVmHome );

	/**
	 * Adds last guest crash dumps to report from specified VM home dir
	 * @param problem report object reference
	 * @param path to VM home dir
	 */
	static void AddGuestCrashDumps( CProblemReport & cReport, const QString &sVmHome,
								    const QDateTime *const minDumpTime,
								    const unsigned maxDumpsCount );

	/**
	* Creates and adds Windows processes mini dumps (as mem dump) into problem report.
	* @param cReport problem report object reference
	* @param procsNames list of process names to create dumps
	* @param maxDumpsCount maximum dump count
	*/
	static void AddProcsMiniDumps(
		CProblemReport & cReport,
		const QStringList & procsNames,
		const unsigned maxDumpsCount);

private:

	static void fillVersionMatrix( CRepVersionMatrix* pVersionMatrix );

	static void FillVmProblemReportData
		(CPackedProblemReport& cReport, CVmConfiguration& vmConfig, const QString& strDirUuid);

	static void FillCtProblemReportData();
};


#endif //__CDsp_ProblemReportHelper_H_
