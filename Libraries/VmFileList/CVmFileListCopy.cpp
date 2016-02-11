///////////////////////////////////////////////////////////////////////////////
///
/// @file CVmFileListCopy.cpp
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

// #define FORCE_LOGGING_LEVEL DBG_DEBUG

#include "CVmFileListCopy.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Std/PrlTime.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Std/AtomicOps.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

#define MIGRATION_PROTO_VER 0x1

#define UNDEF_PLATFORM	0
#define MAC_PLATFORM	1
#define LIN_PLATFORM	2
#define WIN_PLATFORM	3

#ifdef _WIN_
#define snprintf _snprintf
#endif

static int getPlatform()
{
#ifdef _MAC_
	return MAC_PLATFORM;
#endif
#ifdef _LIN_
	return LIN_PLATFORM;
#endif
#ifdef _WIN_
	return WIN_PLATFORM;
#endif
}

#define REQ_FIRST_VER	0
#define REQ_FIRST_PLATF	1
#define REQ_FIRST_SIZE	2

#define REP_FIRST_VER	0
#define REP_FIRST_PLATF	1

#define REQ_DIR_PATH	0
#define REQ_DIR_OWNER	1
#define REQ_DIR_GROUP	2
#define REQ_DIR_PERMS	3

#define REQ_FILE_PATH	0
#define REQ_FILE_SIZE	1
#define REQ_FILE_OWNER	2
#define REQ_FILE_GROUP	3
#define REQ_FILE_PERMS	4

#define REQ_FCHUNK_FLAGS	0
#define REQ_FCHUNK_DATA		1

#define REP_CODE	0
#define REP_EVENT	1

#define ERR_EVENT	0

enum FCHUNK_FLAGS {
	REQ_FCHUNK_FL_DATA = 0x0, /* data block contains valid data */
	REQ_FCHUNK_FL_LAST = 0x1, /* last file chunk (end of file)*/
};

namespace
{
	const UINT32 g_uiMAGIC_NUM	= 0xDEADBABA;
	const UINT32 g_uiMAGIC_IN_HANDLER	= 0xDEADAAA;
}

WaiterTillHandlerUsingObject::WaiterTillHandlerUsingObject()
:m_uiMagic( g_uiMAGIC_NUM )
{
}

WaiterTillHandlerUsingObject::~WaiterTillHandlerUsingObject()
{
	m_uiMagic=0;
}

bool WaiterTillHandlerUsingObject::lockHandler()
{
	if( AtomicCompareSwap( (UINT32*)&m_uiMagic, g_uiMAGIC_NUM, g_uiMAGIC_IN_HANDLER )
		!= g_uiMAGIC_NUM)
	{
		WRITE_TRACE( DBG_WARNING, "Unable to lock handler. Object may be destroyed." );
		return false;
	}

//	WRITE_TRACE( DBG_DEBUG, "Handler was locked." );

	return true;
}

void WaiterTillHandlerUsingObject::unlockHandler()
{
	if( AtomicCompareSwap( (UINT32*)&m_uiMagic, g_uiMAGIC_IN_HANDLER, g_uiMAGIC_NUM )
		!= g_uiMAGIC_IN_HANDLER )
	{
		PRL_ASSERT( "Wrong using object lock logic. MAGIC_NUM was changed from outside." == NULL );
	}
//	WRITE_TRACE( DBG_DEBUG, "Handler was UNlocked." );
}

bool WaiterTillHandlerUsingObject::waitUnlockAndFinalize( UINT32 timeout_msec )
{
	PRL_UINT64 nTimeOutFinished = ( 0 == timeout_msec )
		? PRL_UINT64(-1)
		:( PrlGetTickCount64() + (PRL_UINT64 )PrlGetTicksPerSecond() * timeout_msec/1000 );

	WRITE_TRACE( DBG_DEBUG, "waitUnlockAndFinalize() with timeout %u msec %s ."
		, timeout_msec, timeout_msec?"":"(infinity wait)" );
	while( g_uiMAGIC_NUM != AtomicCompareSwap( (UINT32*)&m_uiMagic, g_uiMAGIC_NUM, 0 ) )
	{
		HostUtils::Sleep( 50 );
		if( PrlGetTickCount64() > nTimeOutFinished )
		{
			WRITE_TRACE( DBG_WARNING, "timeout = %u finished.", timeout_msec );
			return false;
		}
	}
	WRITE_TRACE( DBG_DEBUG, "waitUnlockAndFinalize() finished " );
	return true;
}

//////////////////////////////////////////////////////////////////////////

CVmFileListCopySender::CVmFileListCopySender()
{

}

CVmFileListCopySender::~CVmFileListCopySender()
{
	// #439777 to protect call handler for destroying object
	m_waiter.waitUnlockAndFinalize();
}

IOSendJob::Handle CVmFileListCopySender::sendPackage(const SmartPtr<IOPackage> p)
{
	Q_UNUSED(p);
	return IOSendJob::Handle();
}

IOSendJob::Result CVmFileListCopySender::waitForSend(const IOSendJob::Handle& h, quint32 tmo)
{
	Q_UNUSED(h);
	Q_UNUSED(tmo);
	return IOSendJob::Success;
}

IOSendJob::Result CVmFileListCopySender::waitForResponse(const IOSendJob::Handle& h, quint32 tmo)
{
	Q_UNUSED(h);
	Q_UNUSED(tmo);
	return IOSendJob::Success;
}

IOSendJob::Response CVmFileListCopySender::takeResponse(IOSendJob::Handle& h)
{
	Q_UNUSED(h);
	return IOSendJob::Response();
}

void CVmFileListCopySender::urgentResponseWakeUp()
{
	return;
}

void CVmFileListCopySender::handlePackage(const SmartPtr<IOPackage> p)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (p->header.type == FileCopyError)
		m_sErrorString = UTF8_2QSTR(p->buffers[ERR_EVENT].getImpl());
}

CVmFileListCopySenderClient::CVmFileListCopySenderClient(
	const SmartPtr<IOClient> &pIoClient)
:m_pIoClient(pIoClient)
{
	bool bConnected = QObject::connect(m_pIoClient.getImpl(),
		SIGNAL(onPackageReceived(const SmartPtr<IOPackage>)),
		SLOT(handlePackage(const SmartPtr<IOPackage>)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

CVmFileListCopySenderClient::~CVmFileListCopySenderClient()
{
	QObject::disconnect(m_pIoClient.getImpl(),
		SIGNAL(onPackageReceived(const SmartPtr<IOPackage>)),
		this,
		SLOT(handlePackage(const SmartPtr<IOPackage>)));
}

IOSendJob::Handle CVmFileListCopySenderClient::sendPackage(const SmartPtr<IOPackage> p)
{
	return m_pIoClient->sendPackage(p);
}

#define SET_SEND_JOB(h)	m_mtxSendJob.lock(); m_hSendJob = (h); m_mtxSendJob.unlock();
#define CLEAR_SEND_JOB	m_mtxSendJob.lock(); m_hSendJob = IOSendJob::Handle(); m_mtxSendJob.unlock();

IOSendJob::Result CVmFileListCopySenderClient::waitForSend(const IOSendJob::Handle& h, quint32 tmo)
{
	SET_SEND_JOB(h);
	IOSendJob::Result ret = m_pIoClient->waitForSend(h, tmo);
	CLEAR_SEND_JOB;
	return ret;
}

IOSendJob::Result CVmFileListCopySenderClient::waitForResponse(const IOSendJob::Handle& h, quint32 tmo)
{
	SET_SEND_JOB(h);
	IOSendJob::Result ret = m_pIoClient->waitForResponse(h, tmo);
	CLEAR_SEND_JOB;
	return ret;
}

IOSendJob::Response CVmFileListCopySenderClient::takeResponse(IOSendJob::Handle& h)
{
	return m_pIoClient->takeResponse(h);
}

void CVmFileListCopySenderClient::urgentResponseWakeUp()
{
	m_mtxSendJob.lock();
	IOSendJob::Handle h = m_hSendJob;
	m_mtxSendJob.unlock();
	m_pIoClient->urgentResponseWakeUp(h);
	m_pIoClient->urgentSendWakeUp(h);
}

void CVmFileListCopySenderClient::handlePackage(const SmartPtr<IOPackage> p)
{
	CVmFileListCopySender::handlePackage(p);
}

CVmFileListCopySenderServer::CVmFileListCopySenderServer(
	IOServerInterface_Client &ioServer,
	IOSender::Handle hHandler)
:m_ioServer(ioServer),
m_hHandler(hHandler)
{
	bool bConnected = QObject::connect(&m_ioServer,
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

CVmFileListCopySenderServer::~CVmFileListCopySenderServer()
{
	QObject::disconnect(&m_ioServer,
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)));

	// #439777 to protect call handler for destroying object
	m_waiter.waitUnlockAndFinalize();
}

IOSendJob::Handle CVmFileListCopySenderServer::sendPackage(const SmartPtr<IOPackage> p)
{
	return m_ioServer.sendPackage(m_hHandler, p);
}

IOSendJob::Result CVmFileListCopySenderServer::waitForSend(const IOSendJob::Handle& h, quint32 tmo)
{
	SET_SEND_JOB(h);
	IOSendJob::Result ret = m_ioServer.waitForSend(h, tmo);
	CLEAR_SEND_JOB;
	return ret;
}

IOSendJob::Result CVmFileListCopySenderServer::waitForResponse(const IOSendJob::Handle& h, quint32 tmo)
{
	SET_SEND_JOB(h);
	IOSendJob::Result ret = m_ioServer.waitForResponse(h, tmo);
	CLEAR_SEND_JOB;
	return ret;
}

IOSendJob::Response CVmFileListCopySenderServer::takeResponse(IOSendJob::Handle& h)
{
	return m_ioServer.takeResponse(h);
}

void CVmFileListCopySenderServer::urgentResponseWakeUp()
{
	m_mtxSendJob.lock();
	IOSendJob::Handle h = m_hSendJob;
	m_mtxSendJob.unlock();
	m_ioServer.urgentResponseWakeUp(h);
	m_ioServer.urgentSendWakeUp(h);
}

void CVmFileListCopySenderServer::handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if( h != m_hHandler)
		return;

	CVmFileListCopySender::handlePackage(p);
}

//**************************************Implementation of file list copying base class****************************

CVmFileListCopyBase::CVmFileListCopyBase(
	CVmFileListCopySender *hSender,
	const QString &sParam,
	const QString &sWorkPath,
	CVmEvent *pEvent,
	quint32 nTimeout)
:m_hSender(hSender),
m_nTimeout(nTimeout),
m_bIsOperationWasCanceled(false)
{
	m_sParam = sParam;
	m_sWorkPath = sWorkPath;
	if (!m_sWorkPath.endsWith("/"))
		m_sWorkPath.append("/");
	m_nRemoteVersion = 0;
	m_nRemotePlatform = UNDEF_PLATFORM;
	m_nCurrentSize = 0;
	m_nProgress = 0;

	m_pSendProgressNotification = NULL;
	m_pSendCancelNotification = NULL;
	m_pSendFinishNotification = NULL;
	m_pRequest = SmartPtr<IOPackage>();
	m_pEvent = pEvent;
}

void CVmFileListCopyBase::SetRequest(const SmartPtr<IOPackage> &pRequest)
{
	m_pRequest = pRequest;
}

void CVmFileListCopyBase::SetVmDirectoryUuid(const QString &sVmDirectoryUuid)
{
	m_sVmDirectoryUuid = sVmDirectoryUuid;
}

void CVmFileListCopyBase::SetProgressNotifySender(
	void (*pSendProgressNotification)
		(const SmartPtr<IOPackage> &, const QString &, const QString &, int))
{
	m_pSendProgressNotification = pSendProgressNotification;
}

void CVmFileListCopyBase::SetCancelNotifySender(
	void (*pSendCancelNotification)
		(const SmartPtr<IOPackage> &, const QString &, const QString &))
{
	m_pSendCancelNotification = pSendCancelNotification;
}

void CVmFileListCopyBase::SetFinishNotifySender(
	void (*pSendFinishNotification)
		(const SmartPtr<IOPackage> &, const QString &, const QString &))
{
	m_pSendFinishNotification = pSendFinishNotification;
}

void CVmFileListCopyBase::NotifyClientsWithProgress()
{
	int nCurrentPercent;

	if (m_pSendProgressNotification == NULL)
		return;

  	if (m_nTotalSize == 0)
		return;

	nCurrentPercent = int((m_nCurrentSize*100)/m_nTotalSize);
	if (m_nProgress == nCurrentPercent)
		return;

	/* Notify clients that migration progress changed */
	m_nProgress = nCurrentPercent;
	WRITE_TRACE(DBG_DEBUG, "Notifying clients with files copying progress %u", m_nProgress);
	m_pSendProgressNotification(m_pRequest, m_sVmDirectoryUuid, m_sParam, m_nProgress);
}

void CVmFileListCopyBase::NotifyFileCopyWasCanceled()
{
	//Notify clients about VM migration cancelled event
	if (m_pSendCancelNotification == NULL)
		return;
	m_pSendCancelNotification(m_pRequest, m_sVmDirectoryUuid, m_sParam);
}

void CVmFileListCopyBase::NotifyFileCopyWasFinished()
{
	//Notify clients about VM migration finished event
	if (m_pSendFinishNotification == NULL)
		return;
	m_pSendFinishNotification(m_pRequest, m_sVmDirectoryUuid, m_sParam);
}

//**************************************Implementation of file list copying on source side****************************

CVmFileListCopySource::CVmFileListCopySource(
	CVmFileListCopySender *hSender,
	const QString &sVmUuid,
	const QString &sWorkPath,
	quint64 nTotalSize,
	CVmEvent *pEvent,
	quint32 nTimeout)
:
CVmFileListCopyBase(hSender, sVmUuid, sWorkPath, pEvent, nTimeout)
{
	m_sWorkPath = sWorkPath;
	m_nTotalSize = nTotalSize;

	m_pCopyObject = SmartPtr<CVmFileListCopyObject>(new CVmFileListCopyFile());
}

PRL_RESULT CVmFileListCopySource::SetCopyObject(const SmartPtr<CVmFileListCopyObject> &pCopyObject)
{
	if (pCopyObject)
		m_pCopyObject = pCopyObject;
	else
		m_pCopyObject = SmartPtr<CVmFileListCopyObject>(new CVmFileListCopyFile());
	if (!m_pCopyObject.isValid()) {
		WRITE_TRACE(DBG_FATAL, "CVmFileListCopySource::SetCopyObject() failed.");
		return PRL_ERR_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CVmFileListCopySource::SendReqAndWaitReply(
		const SmartPtr<IOPackage> &package,
		SmartPtr<IOPackage> &reply)
{
	IOSendJob::Handle job;
	IOSendJob::Response resp;
	IOSendJob::Result res;

	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if (m_hSender->m_sErrorString.size())
		return processTargetError(m_hSender->m_sErrorString);

	job = m_hSender->sendPackage(package);
	if (m_hSender->waitForSend(job, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return (PRL_ERR_OPERATION_FAILED);
	}
	res = m_hSender->waitForResponse(job, m_nTimeout);
	if (res == IOSendJob::Timeout) {
		WRITE_TRACE(DBG_FATAL, "Timeout %d expired", m_nTimeout/1000);
		return (PRL_ERR_OPERATION_FAILED);
	} else if (res != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Responce receiving failure");
		return (PRL_ERR_OPERATION_FAILED);
	}
	resp = m_hSender->takeResponse(job);
	if (resp.responseResult != IOSendJob::Success)
		return (PRL_ERR_OPERATION_FAILED);

	reply = resp.responsePackages[0];
	if (!reply.isValid())
		return (PRL_ERR_OPERATION_FAILED);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CVmFileListCopySource::Copy(
		const QList<QPair<QFileInfo, QString> > &dirList,
		const QList<QPair<QFileInfo, QString> > &fileList)
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	int i;

	if (PRL_FAILED(ret = SendFirstRequest()))
		return ret;

	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	for (i = 0; i < dirList.size(); ++i)
	{
		if (PRL_FAILED(ret = SendDirRequest(dirList.at(i))))
			return ret;
		if (m_bIsOperationWasCanceled)
			return PRL_ERR_OPERATION_WAS_CANCELED;
	}

	for (i = 0; i < fileList.size(); ++i)
	{
		if (PRL_FAILED(ret = SendFileRequest(fileList.at(i))))
			return ret;
		if (m_bIsOperationWasCanceled)
			return PRL_ERR_OPERATION_WAS_CANCELED;
	}

	if (PRL_FAILED(ret = SendFinishRequest()))
		return ret;

	return PRL_ERR_SUCCESS;
}

void CVmFileListCopySource::cancelOperation()
{
	m_hSender->urgentResponseWakeUp();
	m_bIsOperationWasCanceled = true;
}

/* send start request for remote dispatcher */
PRL_RESULT CVmFileListCopySource::SendFirstRequest()
{
	PRL_RESULT ret;
	SmartPtr<IOPackage> p;
	SmartPtr<IOPackage> reply;
	char buf[BUFSIZ];

	p = IOPackage::createInstance(FileCopyFirstRequest, 3);
	/* send migration protocol version, total migration size and
	   source platform in first request for target vm app */
	snprintf(buf, sizeof(buf), "%d", MIGRATION_PROTO_VER);
	p->fillBuffer(REQ_FIRST_VER, IOPackage::RawEncoding, buf, strlen(buf)+1);
	snprintf(buf, sizeof(buf), "%d", getPlatform());
	p->fillBuffer(REQ_FIRST_PLATF, IOPackage::RawEncoding, buf, strlen(buf)+1);
	snprintf(buf, sizeof(buf), "%lld", m_nTotalSize);
	p->fillBuffer(REQ_FIRST_SIZE, IOPackage::RawEncoding, buf, strlen(buf)+1);

	WRITE_TRACE(DBG_DEBUG, "< FileCopy first request: version: %d, platform: %d, size: %lld",
		MIGRATION_PROTO_VER, getPlatform(), m_nTotalSize);

	if ((ret = SendReqAndWaitReply(p, reply)) != PRL_ERR_SUCCESS)
		return ret;

	if (reply->header.type == FileCopyFirstReply) {
		if (sscanf(p->buffers[REP_FIRST_VER].getImpl(), "%d", &m_nRemoteVersion) != 1) {
			WRITE_TRACE(DBG_FATAL, "Bad remote version: [%s]", p->buffers[REP_FIRST_VER].getImpl());
			return PRL_ERR_INVALID_PARAM;
		}

		if (sscanf(p->buffers[REP_FIRST_PLATF].getImpl(), "%d", &m_nRemotePlatform) != 1) {
			WRITE_TRACE(DBG_FATAL, "Bad remote platform: [%s]", p->buffers[REP_FIRST_PLATF].getImpl());
			return PRL_ERR_INVALID_PARAM;
		}

		WRITE_TRACE(DBG_FATAL, "> FileCopy first reply: version: %d, platform %d",
			m_nRemoteVersion, m_nRemotePlatform);

	} else if (reply->header.type == FileCopyError) {
		QString sError = UTF8_2QSTR(p->buffers[ERR_EVENT].getImpl());
		return processTargetError(sError);
	} else {
		return (PRL_ERR_OPERATION_FAILED);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CVmFileListCopySource::SendDirRequest(const QPair<QFileInfo, QString> &dPair)
{
	SmartPtr<IOPackage> p;
	QByteArray data;
	char buf[BUFSIZ];
	QFileInfo di = dPair.first;
	QString rpath = dPair.second;

	p = IOPackage::createInstance(FileCopyDirCmd, 4);

	data = rpath.toUtf8();
	p->fillBuffer(REQ_DIR_PATH, IOPackage::RawEncoding, data, data.size()+1);
	data = di.owner().toUtf8();
	p->fillBuffer(REQ_DIR_OWNER, IOPackage::RawEncoding, data, data.size()+1);
	data = di.group().toUtf8();
	p->fillBuffer(REQ_DIR_GROUP, IOPackage::RawEncoding, data, data.size()+1);
	snprintf(buf, sizeof(buf), "%x", int(di.permissions()));
	p->fillBuffer(REQ_DIR_PERMS, IOPackage::RawEncoding, buf, strlen(buf)+1);

	WRITE_TRACE(DBG_DEBUG, "< FileCopy DirRequest: %x\t%s.%s\t%s",
			int(di.permissions()),
			QSTR2UTF8(di.owner()),
			QSTR2UTF8(di.group()),
			QSTR2UTF8(rpath));

	return (SendReqWithAck(p));
}

PRL_RESULT CVmFileListCopySource::SendFileRequest(const QPair<QFileInfo, QString> &fPair)
{
	PRL_RESULT ret;
	SmartPtr<IOPackage> p = IOPackage::createInstance(FileCopyFileCmd, 5);
	QByteArray data;
	char buf[BUFSIZ];
	QFileInfo fi = fPair.first;
	QString rpath = fPair.second;

	data = rpath.toUtf8();
	p->fillBuffer(REQ_FILE_PATH, IOPackage::RawEncoding, data.data(), data.size()+1);
	snprintf(buf, sizeof(buf), "%lld", fi.size());
	p->fillBuffer(REQ_FILE_SIZE, IOPackage::RawEncoding, buf, strlen(buf)+1);
	data = fi.owner().toUtf8();
	p->fillBuffer(REQ_FILE_OWNER, IOPackage::RawEncoding, data.data(), data.size()+1);
	data = fi.group().toUtf8();
	p->fillBuffer(REQ_FILE_GROUP, IOPackage::RawEncoding, data.data(), data.size()+1);
	snprintf(buf, sizeof(buf), "%x", int(fi.permissions()));
	p->fillBuffer(REQ_FILE_PERMS, IOPackage::RawEncoding, buf, strlen(buf)+1);

	WRITE_TRACE(DBG_DEBUG, "< FileCopy File Request: %x\t%s.%s\t%lld\t%s",
			int(fi.permissions()),
			fi.owner().toUtf8().constData(),
			fi.group().toUtf8().constData(),
			fi.size(),
			rpath.toUtf8().constData());

	ret = SendReqWithAck(p);
	if (PRL_FAILED(ret))
		return (ret);

	ret = SendFileBody(fi.absoluteFilePath());
	if (PRL_FAILED(ret))
		return (ret);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CVmFileListCopySource::SendFileBody(const QString & path)
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> p = IOPackage::createInstance(FileCopyFileChunkCmd, 2);
	quint64 flags = REQ_FCHUNK_FL_DATA;
	qint64 size;
	int done = 0;
	char buf[BUFSIZ];
	IOSendJob::Handle job;

	m_pCopyObject->setName(path);
	if (!m_pCopyObject->open(QIODevice::ReadOnly)) {
		WRITE_TRACE(DBG_FATAL, "Can't open file \"%s\"", QSTR2UTF8(path));
		return (PRL_ERR_FAILURE);
	}

	/* will send file body without ack waiting, but will listen incoming messages via
	   QObject::connect() in m_hSender for onPackageReceived signal and will process FileCopyError packages */
	while (!done) {
		if (m_bIsOperationWasCanceled) {
			ret = PRL_ERR_OPERATION_WAS_CANCELED;
			break;
		}

		if (m_hSender->m_sErrorString.size()) {
			ret = processTargetError(m_hSender->m_sErrorString);
			break;
		}
		if ((size = m_pCopyObject->getBuffer()) == -1) {
			WRITE_TRACE(DBG_FATAL, "file \"%s\" read error", QSTR2UTF8(path));
			ret = PRL_ERR_FILE_READ_ERROR;
			break;
		}
		if (m_pCopyObject->atEnd()) {
			flags |= REQ_FCHUNK_FL_LAST;
			done = 1;
		}
		snprintf(buf, sizeof(buf), "%llu", flags);
		p->fillBuffer(REQ_FCHUNK_FLAGS, IOPackage::RawEncoding, buf, strlen(buf)+1);
		p->setBuffer(REQ_FCHUNK_DATA, IOPackage::RawEncoding, m_pCopyObject->m_pBuffer, size);

		job = m_hSender->sendPackage(p);
		if (m_hSender->waitForSend(job, m_nTimeout) != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Package sending failure");
			ret = PRL_ERR_OPERATION_FAILED;
			break;
		}
		m_pCopyObject->freeBuffer();
		m_nCurrentSize += size;
		NotifyClientsWithProgress();
	}
	m_pCopyObject->close();

	return ret;
}

PRL_RESULT CVmFileListCopySource::SendFinishRequest()
{
	SmartPtr<IOPackage> p = IOPackage::createInstance(FileCopyFinishCmd, 0);
	LOG_MESSAGE(DBG_DEBUG, "< Finish Request");
	return (SendReqWithAck(p));
}

PRL_RESULT CVmFileListCopySource::processTargetError(QString sErrorString)
{
	PRL_RESULT nRetCode = PRL_ERR_OPERATION_FAILED;
	CVmEvent cEvent;

	if (cEvent.fromString(sErrorString)) {
		WRITE_TRACE(DBG_FATAL, "Can't parse error package from target");
	} else {
		nRetCode = cEvent.getEventCode();
		WRITE_TRACE(DBG_FATAL, "Error package was received from target: 0x%.8X '%s'",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
	}
	if (m_pEvent)
		m_pEvent->fromString(sErrorString);
	return nRetCode;
}

PRL_RESULT CVmFileListCopySource::SendReqWithAck(SmartPtr<IOPackage> p)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> ack;

	if ((nRetCode = SendReqAndWaitReply(p, ack)) != PRL_ERR_SUCCESS)
		return nRetCode;

	if (ack->header.type == FileCopyError) {
		QString sError = UTF8_2QSTR(ack->buffers[ERR_EVENT].getImpl());
		nRetCode = processTargetError(sError);
	} else if (ack->header.type != FileCopyReply) {
		WRITE_TRACE(DBG_FATAL, "Invalid package type : %d", ack->header.type);
		nRetCode = PRL_ERR_OPERATION_FAILED;
	}
	return nRetCode;
}

//******************************************Implementation of file list copying on target side*************************

CVmFileListCopyTarget::CVmFileListCopyTarget(
	CVmFileListCopySender *hSender,
	const QString &sVmUuid,
	const QString &sWorkPath,
	CVmEvent *pEvent,
	quint32 nTimeout,
	bool bOverwriteMode)
:
CVmFileListCopyBase(hSender, sVmUuid, sWorkPath, pEvent, nTimeout),
m_bOverwriteMode(bOverwriteMode)
{
	m_Event.setEventCode(PRL_ERR_SUCCESS);
}

CVmFileListCopyTarget::~CVmFileListCopyTarget()
{
	cancelOperation();
}

#define EVENT_ERR_FILECOPY_PROTOCOL(msg, field, value) \
	{ \
		m_Event.setEventCode(PRL_ERR_FILECOPY_PROTOCOL); \
		m_Event.addEventParameter(new CVmEventParameter( \
			PVE::String, \
			(msg), \
			EVT_PARAM_MESSAGE_PARAM_0)); \
		m_Event.addEventParameter(new CVmEventParameter( \
			PVE::Integer, \
			(field), \
			EVT_PARAM_MESSAGE_PARAM_1)); \
		m_Event.addEventParameter(new CVmEventParameter( \
			PVE::String, \
			(value), \
			EVT_PARAM_MESSAGE_PARAM_2)); \
	}




PRL_RESULT CVmFileListCopyTarget::handlePackage(const SmartPtr<IOPackage> p, bool *bFinish)
{
	bool bExit = false;
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	WRITE_TRACE(DBG_DEBUG, "package type %u", p->header.type);

	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if ((p->header.type <= FileCopyRangeStart) || (p->header.type >= FileCopyRangeEnd)) {
		if (bFinish)
			*bFinish = bExit;
		return PRL_ERR_SUCCESS;
	}

	if (PRL_FAILED(m_Event.getEventCode())) {
		/* Error already occurred before. To repeat error reply */
		bExit = true;
		SendError(p);
		NotifyFileCopyWasCanceled();
		if (bFinish)
			*bFinish = bExit;
		return m_Event.getEventCode();
	}
	switch (p->header.type)
	{
		case FileCopyFirstRequest:
			if (PRL_FAILED(nRetCode = RecvFirstRequest(p)))
				bExit = true;
			break;
		case FileCopyDirCmd:
			if (PRL_FAILED(nRetCode = RecvDirRequest(p)))
				bExit = true;
			break;
		case FileCopyFileCmd:
			if (PRL_FAILED(nRetCode = RecvFileRequest(p)))
				bExit = true;
			break;
		case FileCopyFileChunkCmd:
			if (PRL_FAILED(nRetCode = RecvFileChunk(p)))
				bExit = true;
			break;
		case FileCopyFinishCmd:
			nRetCode = RecvFinishRequest(p);
			bExit = true;
			break;
		case FileCopyCancelCmd:
			nRetCode = RecvCancelRequest(p);
			bExit = true;
			break;
	}
	if (bFinish)
		*bFinish = bExit;
	return nRetCode;
}

void CVmFileListCopyTarget::cancelOperation()
{
	QMutexLocker locker(&m_mtxFile);
	if (file.isValid() && file->isOpen())
		file->close();
	m_bIsOperationWasCanceled = true;
}

/* send start request for remote dispatcher */
PRL_RESULT CVmFileListCopyTarget::RecvFirstRequest(const SmartPtr<IOPackage> &p)
{
	SmartPtr<IOPackage> reply;
	char buf[BUFSIZ];

	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	/* recv migration protocol version, total migration size and
	   source platform in first request */
	if (sscanf(p->buffers[REQ_FIRST_VER].getImpl(), "%d", &m_nRemoteVersion) != 1) {
		WRITE_TRACE(DBG_FATAL, "Bad remote version: [%s]", p->buffers[REQ_FIRST_VER].getImpl());

		EVENT_ERR_FILECOPY_PROTOCOL(
			"FileCopyFirstRequest",
			QString("%1").arg(REQ_FIRST_VER),
			p->buffers[REQ_FIRST_VER].getImpl());
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_PROTOCOL;
	}

	if (sscanf(p->buffers[REQ_FIRST_PLATF].getImpl(), "%d", &m_nRemotePlatform) != 1) {
		WRITE_TRACE(DBG_FATAL, "Bad remote platform: [%s]", p->buffers[REQ_FIRST_PLATF].getImpl());

		EVENT_ERR_FILECOPY_PROTOCOL(
			"FileCopyFirstRequest",
			QString("%1").arg(REQ_FIRST_PLATF),
			p->buffers[REQ_FIRST_PLATF].getImpl());
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_PROTOCOL;
	}

	if (sscanf(p->buffers[REQ_FIRST_SIZE].getImpl(), "%lld", &m_nTotalSize) != 1) {
		WRITE_TRACE(DBG_FATAL, "Bad total size: [%s]", p->buffers[REQ_FIRST_SIZE].getImpl());

		EVENT_ERR_FILECOPY_PROTOCOL(
			"FileCopyFirstRequest",
			QString("%1").arg(REQ_FIRST_SIZE),
			p->buffers[REQ_FIRST_SIZE].getImpl());
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_PROTOCOL;
	}

	WRITE_TRACE(DBG_DEBUG, "> FileCopy first request: version: %d, platform: %d, size: %lld",
		m_nRemoteVersion, m_nRemotePlatform, m_nTotalSize);

	reply = IOPackage::createInstance(FileCopyFirstReply, 2, p);

	/* send reply with migration protocol version and source platform */
	snprintf(buf, sizeof(buf), "%d", MIGRATION_PROTO_VER);
	reply->fillBuffer(REP_FIRST_VER, IOPackage::RawEncoding, buf, strlen(buf)+1);

	snprintf(buf, sizeof(buf), "%d", getPlatform());
	reply->fillBuffer(REP_FIRST_PLATF, IOPackage::RawEncoding, buf, strlen(buf)+1);

	WRITE_TRACE(DBG_FATAL, "< FileCopy first reply: version: %d, platform: %d",
		MIGRATION_PROTO_VER, getPlatform());

	m_hSender->sendPackage(reply);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CVmFileListCopyTarget::RecvDirRequest(const SmartPtr<IOPackage> &p)
{
	QString owner;
	QString group;
	QString path;
	int perms;
	SmartPtr<QDir> dir = SmartPtr<QDir>(new QDir());

	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	path = UTF8_2QSTR(p->buffers[REQ_DIR_PATH].getImpl());
	if (QFileInfo(path).isRelative())
		path.insert(0, m_sWorkPath);
	owner = UTF8_2QSTR(p->buffers[REQ_DIR_OWNER].getImpl());
	group = UTF8_2QSTR(p->buffers[REQ_DIR_GROUP].getImpl());
	if (sscanf(p->buffers[REQ_DIR_PERMS].getImpl(), "%x", &perms) != 1) {
		WRITE_TRACE(DBG_FATAL, "Bad directory permissions: [%s]",\
			p->buffers[REQ_DIR_PERMS].getImpl());

		EVENT_ERR_FILECOPY_PROTOCOL(
			"FileCopyDirRequest",
			QString("%1").arg(REQ_DIR_PERMS),
			p->buffers[REQ_DIR_PERMS].getImpl());
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_PROTOCOL;
	}

	WRITE_TRACE(DBG_DEBUG, "> FileCopy DirRequest: %x\t%s.%s\t%s",
			perms,
			QSTR2UTF8(owner),
			QSTR2UTF8(group),
			QSTR2UTF8(path));

	dir->setPath(path);

	if (!dir->mkpath(path)) {
		WRITE_TRACE(DBG_FATAL, "Can't create directory: [%s]", QSTR2UTF8(path));

		m_Event.setEventCode(PRL_ERR_FILECOPY_CANT_CREATE_DIR);
		m_Event.addEventParameter(new CVmEventParameter(
			PVE::String,
			path,
			EVT_PARAM_MESSAGE_PARAM_0));
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_CANT_CREATE_DIR;
	}

	QMutexLocker locker(&m_mtxFile);

	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	file = SmartPtr<QFile>(new QFile());
	file->setFileName(path.append("/."));
	if (!file->setPermissions((QFile::Permissions)perms)) {
		WRITE_TRACE(DBG_FATAL, "Can't set permissions [%x] for directory: [%s]", perms, QSTR2UTF8(path));
	}

	return SendAck(p);
}

PRL_RESULT CVmFileListCopyTarget::RecvFileRequest(const SmartPtr<IOPackage> &p)
{
	QString owner;
	QString group;
	QString path;
	int perms;

	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	path = UTF8_2QSTR(p->buffers[REQ_FILE_PATH].getImpl());
	/*
	   If we got absolute path, do not modify it.
	   As sample, memory dump file for nfs-based Vm saved out of Vm home directory (at /tmp/ now).
	   #472306, #472919
	*/
	if (QFileInfo(path).isRelative())
		path.insert(0, m_sWorkPath);
	owner = UTF8_2QSTR(p->buffers[REQ_FILE_OWNER].getImpl());
	group = UTF8_2QSTR(p->buffers[REQ_FILE_GROUP].getImpl());

	if (sscanf(p->buffers[REQ_FILE_SIZE].getImpl(), "%lld", &fileSize) != 1) {
		WRITE_TRACE(DBG_FATAL, "Bad file size: [%s]", p->buffers[REQ_FILE_SIZE].getImpl());

		EVENT_ERR_FILECOPY_PROTOCOL(
			"FileCopyFileRequest",
			QString("%1").arg(REQ_FILE_SIZE),
			p->buffers[REQ_FILE_SIZE].getImpl());
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_PROTOCOL;
	}

	if (sscanf(p->buffers[REQ_FILE_PERMS].getImpl(), "%x", &perms) != 1) {
		WRITE_TRACE(DBG_FATAL, "Bad file permissions: [%s]", p->buffers[REQ_FILE_PERMS].getImpl());

		EVENT_ERR_FILECOPY_PROTOCOL(
			"FileCopyFileRequest",
			QString("%1").arg(REQ_FILE_PERMS),
			p->buffers[REQ_FILE_PERMS].getImpl());
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_PROTOCOL;
	}

	WRITE_TRACE(DBG_DEBUG, "> FileCopy FileRequest: %x\t%s.%s\t%lld\t%s",
			perms,
			QSTR2UTF8(owner),
			QSTR2UTF8(group),
			fileSize,
			QSTR2UTF8(path));

	QMutexLocker locker(&m_mtxFile);

	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if (file.getImpl() && file->isOpen()) {
		WRITE_TRACE(DBG_FATAL, "New file header received, but old file is still transferring");

		m_Event.setEventCode(PRL_ERR_FILECOPY_INTERNAL);
		m_Event.addEventParameter(new CVmEventParameter(
			PVE::String,
			"New file header received, but old file is still transferring",
			EVT_PARAM_MESSAGE_PARAM_0));
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_INTERNAL;
	}
	file = SmartPtr<QFile>(new QFile());
	file->setFileName(path);
	if (!m_bOverwriteMode && file->exists()) {
		WRITE_TRACE(DBG_FATAL, "File already exists: [%s]", QSTR2UTF8(path));

		m_Event.setEventCode(PRL_ERR_FILECOPY_FILE_EXIST);
		m_Event.addEventParameter(new CVmEventParameter(
			PVE::String,
			path,
			EVT_PARAM_MESSAGE_PARAM_0));
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_FILE_EXIST;
	}

	if (!file->open(m_bOverwriteMode ?
			QIODevice::WriteOnly|QIODevice::Truncate :
			QIODevice::WriteOnly)) {
		WRITE_TRACE(DBG_FATAL, "Can't open file: [%s]", QSTR2UTF8(path));

		m_Event.setEventCode(PRL_ERR_FILECOPY_CANT_OPEN_FILE);
		m_Event.addEventParameter(new CVmEventParameter(
			PVE::String,
			path,
			EVT_PARAM_MESSAGE_PARAM_0));
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_CANT_OPEN_FILE;
	}
	if (!file->setPermissions((QFile::Permissions)perms)) {
		WRITE_TRACE(DBG_FATAL, "Can't set permissions [%x] for file: [%s]", perms, QSTR2UTF8(path));
	}

	return SendAck(p);
}

PRL_RESULT CVmFileListCopyTarget::RecvFileChunk(const SmartPtr<IOPackage> &p)
{
	QMutexLocker locker(&m_mtxFile);

	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if (!file->isOpen()) {
		WRITE_TRACE(DBG_FATAL, "File chunk without file open");

		m_Event.setEventCode(PRL_ERR_FILECOPY_INTERNAL);
		m_Event.addEventParameter(new CVmEventParameter(
			PVE::String,
			"File chunk without file open",
			EVT_PARAM_MESSAGE_PARAM_0));
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_INTERNAL;
	}

	IOPackage::EncodingType enc;
	SmartPtr<char> buff;
	quint32 size;
	quint64 flags;

	if (sscanf(p->buffers[REQ_FCHUNK_FLAGS].getImpl(), "%llu", &flags) != 1) {
		WRITE_TRACE(DBG_FATAL, "Bad flags: [%s]", p->buffers[REQ_FCHUNK_FLAGS].getImpl());

		EVENT_ERR_FILECOPY_PROTOCOL(
			"FileCopyFileChunk",
			QString("%1").arg(REQ_FCHUNK_FLAGS),
			p->buffers[REQ_FCHUNK_FLAGS].getImpl());
		SendError(p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_PROTOCOL;
	}
	p->getBuffer(REQ_FCHUNK_DATA, enc, buff, size);

	if (file->write(buff.getImpl(), size) == -1) {
		file->close();
		WRITE_TRACE(DBG_FATAL, "Write error");

		m_Event.setEventCode(PRL_ERR_FILECOPY_CANT_WRITE);
		SendError( p);

		NotifyFileCopyWasCanceled();
		return PRL_ERR_FILECOPY_CANT_WRITE;
	}
	m_nCurrentSize += size;

	if (flags & REQ_FCHUNK_FL_LAST)
		file->close();

	NotifyClientsWithProgress();

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CVmFileListCopyTarget::RecvFinishRequest(const SmartPtr<IOPackage> &p)
{
	if (m_bIsOperationWasCanceled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	LOG_MESSAGE(DBG_DEBUG, "> Finish Request");
	NotifyFileCopyWasFinished();
	/* will check error - upper level can set error before
	   (https://jira.sw.ru/browse/PSBM-10300) */
	if (PRL_FAILED(m_Event.getEventCode())) {
		SendError(p);
		return m_Event.getEventCode();
	} else {
		return SendAck(p);
	}
}

PRL_RESULT CVmFileListCopyTarget::RecvCancelRequest(const SmartPtr<IOPackage> &p)
{
	CDispToDispCommandPtr pResponseCmd =
		CDispToDispProtoSerializer::CreateDispToDispResponseCommand(PRL_ERR_SUCCESS, p);
	SmartPtr<IOPackage> pResponsePkg = DispatcherPackage::createInstance(pResponseCmd, p);
	m_hSender->sendPackage(pResponsePkg);
	NotifyFileCopyWasCanceled();
	cancelOperation();
	return PRL_ERR_OPERATION_WAS_CANCELED;
}

/* send error event to source vm app */
void CVmFileListCopyTarget::SendError(const SmartPtr<IOPackage> &p)
{
	SmartPtr<IOPackage> reply;
	IOSendJob::Handle job;
	QString msg = m_Event.toString();
	QByteArray data = msg.toUtf8();

	/* send event body */
	reply = IOPackage::createInstance(FileCopyError, 1, p);
	reply->fillBuffer(ERR_EVENT, IOPackage::RawEncoding, data, data.size()+1);

	job = m_hSender->sendPackage(reply);
	WRITE_TRACE(DBG_DEBUG, "< FileCopy error, code %d", m_Event.getEventCode());

	if (m_pEvent)
		m_pEvent->fromString(msg);
}

void CVmFileListCopyTarget::SetError(const CVmEvent &event)
{
	m_Event.fromString(event.toString());
}

/* send acknowledgment to source vm app */
PRL_RESULT CVmFileListCopyTarget::SendAck(const SmartPtr<IOPackage> &p)
{
	SmartPtr<IOPackage> reply;
	char buf[BUFSIZ];
	IOSendJob::Handle job;

	/* send retcode only */
	reply = IOPackage::createInstance(FileCopyReply, 1, p);
	snprintf(buf, sizeof(buf), "%d", PRL_ERR_SUCCESS);
	reply->fillBuffer(REP_CODE, IOPackage::RawEncoding, buf, strlen(buf)+1);

	WRITE_TRACE(DBG_DEBUG, "< FileCopy reply");

	job = m_hSender->sendPackage(reply);
	if (m_hSender->waitForSend(job, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}


