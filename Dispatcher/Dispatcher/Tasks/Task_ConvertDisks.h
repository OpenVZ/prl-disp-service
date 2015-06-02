///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_ConvertDisks.h
///
/// Dispatcher task for doing conversion VM image disks.
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

#ifndef __Task_ConvertDisks_H__
#define __Task_ConvertDisks_H__

#include "CDspTaskHelper.h"


class CVmConfiguration;


class Task_ConvertDisks : public CDspTaskHelper
{
	Q_OBJECT
public:

	Task_ConvertDisks(const SmartPtr<CDspClient>& pClient,
					  const SmartPtr<IOPackage>& p,
					  const QString& qsVmUuid,
					  PRL_UINT32 nDiskMask,
					  PRL_UINT32 nFlags);
	virtual ~Task_ConvertDisks();

private slots:

	void onReadyReadStandardError();
	void onReadyReadStandardOutput();
	void onFinished( int nExitCode, QProcess::ExitStatus nExitStatus );

private:

	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual void cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p);

	void cancelAndWait();
	bool isTaskShutdown() { return (m_nFlags & PCVD_CANCEL); }
	PRL_RESULT convertDisks();
	void deleteSnapshotsData();
	void sendCoversionProgressParams(const QString& qsOutput);
	void sendProgressEvent(const CVmEvent& evt);
	void lockConvertDisks();
	void unlockConvertDisks();

	CVmIdent	m_vmIdent;
	PRL_UINT32	m_nDiskMask;
	PRL_UINT32	m_nFlags;

	SmartPtr<CVmConfiguration> m_pVmConfig;
	bool		m_bVmConfigWasChanged;
	bool		m_flgLockRegistered;
	QString		m_qsDTName;
	QProcess*	m_pProcess;
	unsigned int m_nCurHddIndex;
	int			m_nCurHddItemId;

	static QMutex						s_lockConvertDisks;
	static QMap<CVmIdent , QString >	s_mapVmIdTaskId;
};


#endif	// __Task_ConvertDisks_H__
