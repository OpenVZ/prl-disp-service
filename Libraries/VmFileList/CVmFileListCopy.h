///////////////////////////////////////////////////////////////////////////////
///
/// @file CVmFileListCopy.h
///
/// VM files copying procedure
///
/// @author krasnov
/// @owner sergeym
///
/// Copyright (c) 2008-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef CVmFileListCopy_H
#define CVmFileListCopy_H

#include <QPair>
#include <QList>

#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/IOService/IOCommunication/IOClient.h>
#include <prlcommon/IOService/IOCommunication/IOServer.h>
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"

using namespace IOService;
using namespace Parallels;

class WaiterTillHandlerUsingObject
{
public:
	WaiterTillHandlerUsingObject();
	~WaiterTillHandlerUsingObject();

	bool lockHandler();
	void unlockHandler();
	bool waitUnlockAndFinalize( UINT32 timeout_msec = 0 );

public:
	// helper to autounlock
	class AutoUnlock
	{
	public:
		AutoUnlock( WaiterTillHandlerUsingObject& waiter ):m_waiter(waiter)
			{ m_isLocked = m_waiter.lockHandler(); }
		~AutoUnlock()
			{ if( m_isLocked ) m_waiter.unlockHandler(); }

		bool isLocked() { return m_isLocked; }

	private:
		bool m_isLocked;
		WaiterTillHandlerUsingObject& m_waiter;
	};

private:
	UINT32	m_uiMagic;
};

class CVmFileListCopySender : public QObject
{
	Q_OBJECT
public:
	CVmFileListCopySender();
	virtual ~CVmFileListCopySender();
	virtual IOSendJob::Handle sendPackage(const SmartPtr<IOPackage> p);
	virtual IOSendJob::Result waitForSend(const IOSendJob::Handle& h, quint32 tmo = UINT_MAX );
	virtual IOSendJob::Result waitForResponse(const IOSendJob::Handle& h, quint32 tmo = UINT_MAX );
	virtual IOSendJob::Response takeResponse(IOSendJob::Handle& h);
	virtual void urgentResponseWakeUp();
	void handlePackage(const SmartPtr<IOPackage> p);
public:
	QString m_sErrorString;
	IOSendJob::Handle m_hSendJob;
	QMutex m_mtxSendJob;

private:
	WaiterTillHandlerUsingObject m_waiter;
};

class CVmFileListCopySenderClient : public CVmFileListCopySender
{
	Q_OBJECT
public:
	CVmFileListCopySenderClient(const SmartPtr<IOClient> &pIoClient);
	virtual ~CVmFileListCopySenderClient();

	virtual IOSendJob::Handle sendPackage(const SmartPtr<IOPackage> p);
	virtual IOSendJob::Result waitForSend(const IOSendJob::Handle& h, quint32 tmo);
	virtual IOSendJob::Result waitForResponse(const IOSendJob::Handle& h, quint32 tmo);
	virtual IOSendJob::Response takeResponse(IOSendJob::Handle& h);
	virtual void urgentResponseWakeUp();
private:
	SmartPtr<IOClient> m_pIoClient;
private slots:
	void handlePackage(const SmartPtr<IOPackage> p);
};

class CVmFileListCopySenderServer : public CVmFileListCopySender
{
	Q_OBJECT
public:
	CVmFileListCopySenderServer(
		IOServerInterface_Client &ioServer,
		IOSender::Handle hHandler);
	virtual ~CVmFileListCopySenderServer();

	virtual IOSendJob::Handle sendPackage(const SmartPtr<IOPackage> p);
	virtual IOSendJob::Result waitForSend(const IOSendJob::Handle& h, quint32 tmo);
	virtual IOSendJob::Result waitForResponse(const IOSendJob::Handle& h, quint32 tmo);
	virtual IOSendJob::Response takeResponse(IOSendJob::Handle& h);
	virtual void urgentResponseWakeUp();
private:
	IOServerInterface_Client &m_ioServer;
	IOSender::Handle m_hHandler;
private slots:
	void handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p);

private:
	WaiterTillHandlerUsingObject m_waiter;
};

/**
 * VM files copying procedure
 */
class CVmFileListCopyBase
{
public:
	CVmFileListCopyBase(
		CVmFileListCopySender *hSender,
		const QString &sVmUuid,
		const QString &sWorkPath,
		CVmEvent *event,
		quint32 nTimeout);

	virtual ~CVmFileListCopyBase() {}
	void SetRequest(const SmartPtr<IOPackage> &);
	void SetVmDirectoryUuid(const QString &);
	void SetProgressNotifySender(
		void (*pSendProgressNotification)
			(const SmartPtr<IOPackage> &, const QString &, const QString &, int));
	void SetCancelNotifySender(
		void (*pSendCancelNotification)
			(const SmartPtr<IOPackage> &, const QString &, const QString &));
	void SetFinishNotifySender(
		void (*pSendFinishNotification)
			(const SmartPtr<IOPackage> &, const QString &, const QString &));
	void NotifyClientsWithProgress();
	void NotifyFileCopyWasFinished();
	void NotifyFileCopyWasCanceled();

	quint64 getCurrentSize() { return m_nCurrentSize; }
	void setCurrentSize(quint64 nCurrentSize) { m_nCurrentSize += nCurrentSize; }
	int getProgress() { return  m_nProgress; }
	void setProgress(int nProgress) { m_nProgress = nProgress; }
protected:
	QString m_sParam;
	quint64 m_nTotalSize;
	quint64 m_nCurrentSize;
	int m_nProgress;
	QString m_sWorkPath;
	int m_nRemoteVersion;
	int m_nRemotePlatform;
	void (*m_pSendProgressNotification)
		(const SmartPtr<IOPackage> &, const QString &, const QString &, int);
	void (*m_pSendCancelNotification)
		(const SmartPtr<IOPackage> &, const QString &, const QString &);
	void (*m_pSendFinishNotification)
		(const SmartPtr<IOPackage> &, const QString &, const QString &);
	CVmFileListCopySender *m_hSender;
	/* directory uuid */
	QString m_sVmDirectoryUuid;
	/* request from client, will use for notifications of client */
	SmartPtr<IOPackage> m_pRequest;
	CVmEvent *m_pEvent;

	quint32 m_nTimeout;
	bool m_bIsOperationWasCanceled;
};

/*
   This is wrapper of QFile object, was added as base class for raw of source guest memory to remote memory swap file
*/
class CVmFileListCopyObject
{
public:
	SmartPtr<char> m_pBuffer;
public:
	virtual ~CVmFileListCopyObject(){}
	virtual bool open(QFile::OpenMode mode) { Q_UNUSED(mode); return true; }
	virtual void close() { return; }
	virtual bool atEnd() { return true; }
	virtual void setName(const QString &sName) { Q_UNUSED(sName); }
	virtual qint64 getBuffer() { return 0; }
	virtual void freeBuffer() { return; }
};

/*
   Class for plain file to plain file copy
*/
class CVmFileListCopyFile : public CVmFileListCopyObject
{
private:
	QFile m_cFile;
	quint64 m_nBufSize;
public:
	CVmFileListCopyFile()
	{
		m_nBufSize = 1024*1024;
		m_pBuffer = SmartPtr<char>(new char[m_nBufSize], SmartPtrPolicy::ArrayStorage);
	}
	virtual bool open(QFile::OpenMode mode) { return m_cFile.open(mode); }
	virtual void close() { m_cFile.close(); }
	virtual bool atEnd() { return m_cFile.atEnd(); }
	virtual void setName(const QString &sName) { m_cFile.setFileName(sName); }
	virtual qint64 getBuffer() { return m_cFile.read(m_pBuffer.getImpl(), m_nBufSize); }
};

/**
 * VM migration files copying procedure source side
 */
class CVmFileListCopySource : public CVmFileListCopyBase
{
public:
	CVmFileListCopySource(
		CVmFileListCopySender *hSender,
		const QString &sVmUuid,
		const QString &sWorkPath,
		quint64 nTotalSize,
		CVmEvent *event,
		quint32 nTimeout);
	~CVmFileListCopySource(){}

	PRL_RESULT SendReqAndWaitReply(const SmartPtr<IOPackage> &package, SmartPtr<IOPackage> &reply);
	PRL_RESULT Copy(
		const QList<QPair<QFileInfo, QString> > &dirList,
		const QList<QPair<QFileInfo, QString> > &fileList);
	PRL_RESULT SendFirstRequest();
	PRL_RESULT SendDirRequest(const QPair<QFileInfo, QString> &dPair);
	PRL_RESULT SendFileRequest(const QPair<QFileInfo, QString> &fPair);
	PRL_RESULT SendFinishRequest();
	PRL_RESULT SetCopyObject(const SmartPtr<CVmFileListCopyObject> &pCopyObject);

	void cancelOperation();

private:
	PRL_RESULT SendReqWithAck(SmartPtr<IOPackage> pPkg);
	PRL_RESULT SendFileBody(const QString & path);
	PRL_RESULT processTargetError(QString sErrorString);

private:
	SmartPtr<CVmFileListCopyObject> m_pCopyObject;

};

/**
 * VM file copying procedure target side
 */
class CVmFileListCopyTarget : public CVmFileListCopyBase
{
public:
	CVmFileListCopyTarget(
		CVmFileListCopySender *hSender,
		const QString &sVmUuid,
		const QString &sWorkPath,
		CVmEvent *event,
		quint32 nTimeout,
		bool bOverwriteMode = false);
	~CVmFileListCopyTarget();

	PRL_RESULT handlePackage(const SmartPtr<IOPackage> p, bool *bExit = NULL);

	PRL_RESULT RecvCancelRequest(const SmartPtr<IOPackage> &p);

	quint64 getSize(){ return m_nCurrentSize; }

	void SetError(const CVmEvent &event);

	void cancelOperation();

private:
	PRL_RESULT RecvFirstRequest(const SmartPtr<IOPackage> &p);
	PRL_RESULT RecvDirRequest(const SmartPtr<IOPackage> &p);
	PRL_RESULT RecvFileRequest(const SmartPtr<IOPackage> &p);
	PRL_RESULT RecvFileChunk(const SmartPtr<IOPackage> &p);
	PRL_RESULT RecvFinishRequest(const SmartPtr<IOPackage> &p);

	PRL_RESULT SendAck(const SmartPtr<IOPackage> &p);
	/* send error event to source vm app */
	void SendError(const SmartPtr<IOPackage> &p);

private:
	/* current transfer file*/
	SmartPtr<QFile> file;
	quint64 fileSize;
	CVmEvent m_Event;
	bool m_bOverwriteMode;
	QMutex m_mtxFile;
};
#endif //CVmFileListCopy_H

