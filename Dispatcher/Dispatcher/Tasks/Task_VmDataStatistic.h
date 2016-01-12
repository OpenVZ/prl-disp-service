///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_VmDataStatistic.h
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

#ifndef __Task_VmDataStatistic_H__
#define __Task_VmDataStatistic_H__

#include <prlxmlmodel/HostHardwareInfo/CSystemStatistics.h>
#include "CDspTaskHelper.h"

class Task_VmDataStatistic : public CDspTaskHelper
{
	Q_OBJECT
public:

	Task_VmDataStatistic(const SmartPtr<CDspClient>& pClient,
						 const SmartPtr<IOPackage>& p,
						 bool bNeedCleanUpInfoOnly = false);
	virtual ~Task_VmDataStatistic();

	virtual void cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p);

	QStringList getReclaimFilesList(bool bWithLostSnapshotFiles = true);

private:

	virtual QString getVmUuid();

	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

	CVmDataSegment* addSegment(PRL_DATA_STATISTIC_SEGMENTS nSegment);
	CVmDataSegment* getSegment(PRL_DATA_STATISTIC_SEGMENTS nSegment) const;

	PRL_RESULT hostDiskSpaceUsage();
	PRL_RESULT fullDiskSpaceUsage();
	PRL_RESULT vmDataDiskSpaceUsage();
	PRL_RESULT snapshotsDiskSpaceUsage();
	PRL_RESULT miscellaneousDiskSpaceUsage();
	PRL_RESULT reclaimDiskSpaceUsage();
	QStringList getLostSnapshotFiles();

	QString						m_qsVmUuid;
	QFileInfo					m_fiVmHomePath;
	SmartPtr<CVmConfiguration>	m_pVmConfig;

	SmartPtr<CSystemStatistics>	m_vmStatistic;

	QString		m_qsDTName;
	QProcess*	m_pProcess;
	bool		m_bNeedCleanUpInfoOnly;
};

#endif	// __Task_VmDataStatistic_H__
