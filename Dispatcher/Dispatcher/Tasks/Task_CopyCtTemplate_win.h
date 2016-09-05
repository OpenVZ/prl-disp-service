///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CopyCtTemplate_vzwin.h
///
/// Dispatcher task for VZWIN CT template migration
///
/// @author yur@
///
/// Copyright (c) 2011-2015 Parallels IP Holdings GmbH
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

#ifndef __Task_CopyCtTemplate_win_H_
#define __Task_CopyCtTemplate_win_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "Task_DispToDispConnHelper.h"
#include "CDspClient.h"
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "CDspDispConnection.h"
#include "Libraries/VmFileList/CVmFileListCopy.h"

class Task_CopyCtTemplateSource : public CDspTaskHelper, public Task_DispToDispConnHelper
{
	Q_OBJECT

private:
	quint32 m_nVersion;
	QString m_sTmplName;
	QString m_sOsTmplName;
	QString m_sServerHostname;
	quint32 m_nServerPort;
	QString m_sServerSessionUuid;
	quint32 m_nFlags;
	quint32 m_nReservedFlags;

public:
	Task_CopyCtTemplateSource(
		const SmartPtr<CDspClient> &,
		const CProtoCommandPtr,
		const SmartPtr<IOPackage> &);

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual bool isCancelled() { return operationIsCancelled(); }
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p);

private:
	/* smart pointer to object for data sending. pVmCopySource will use it */
	SmartPtr<CVmFileListCopySender> m_pSender;
	/** Pointer to the VM files copying maintainer */
	SmartPtr<CVmFileListCopySource> m_pCopySource;

	IOSendJob::Handle m_hCheckReqJob;
};

class Task_CopyCtTemplateTarget :  public CDspTaskHelper, public Task_DispToDispConnHelper
{
	Q_OBJECT

public:
	Task_CopyCtTemplateTarget(
		const SmartPtr<CDspDispConnection> &,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p);

	/* send package with migration data to target dispatcher */
private:
	SmartPtr<CDspDispConnection> m_pDispConnection;
	IOSender::Handle m_hConnHandle;

	SmartPtr<CVmFileListCopySender> m_pSender;
	SmartPtr<CVmFileListCopyTarget> m_pTarget;

	quint32 m_nVersion;
	QString m_sTmplName;
	QString m_sOsTmplName;
	quint32 m_nFlags;
	quint32 m_nReservedFlags;

private slots:
	void handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
	void clientDisconnected(IOSender::Handle h);
};

#endif //__Task_CopyCtTemplate_win_H_
