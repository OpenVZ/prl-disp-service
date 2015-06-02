/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */


#include <QCoreApplication>
#include <QHash>
#include <QMutex>
#include <QByteArray>
#include <QBuffer>
#include <QDataStream>
#include <QtTest>

#include "IOClient.h"
#include "IORoutingTableHelper.h"
#include "IOServer.h"
#include "Libraries/Logging/Logging.h"
#include "Libraries/PrlUuid/Uuid.h"

#ifdef _WIN_
 #include <windows.h>
 #define sleepMsecs Sleep
#else
 #include <unistd.h>
 #define sleepMsecs(msecs) usleep(msecs * 1000)
#endif

using namespace IOService;

/*****************************************************************************/

#define MY_INT_QVERIFY(val, cond) if ( ! (cond ) ) { \
        WRITE_TRACE(DBG_FATAL, "QVERIFY: condition is false for value '" TO_STR(val) \
                    "'== %d", (val)); \
        QVERIFY( (cond) );           \
    }

/*****************************************************************************/

// Declared in 'ProcessStarter.cpp'
int createProcess ( const QString& program, const QStringList& arguments,
                    IOCommunication::SocketHandle sock = IOCommunication::SocketHandle() );

/*****************************************************************************/

static int qrandBetween ( int from, int to )
{
    struct Helper { Helper () { ::qsrand( IOService::msecsFromEpoch() ); }};
    static Helper srandHelper;

    double fromVal = from;
    double toVal = to;
    return from + (int) (toVal * (::qrand() / (RAND_MAX + fromVal)));
}

static IOPackage::EncodingType randEncoding ()
{
    return static_cast<IOPackage::EncodingType>(
                   qrandBetween( 0, IOPackage::EncodingFinalBound - 1 ));
}

/*****************************************************************************/

static const quint32 RemotePortNumber = 5555;
static const quint32 ProxyPortNumber = 64666;
static const QString TimeFormat( "hh:mm:ss.zzz" );
static const quint32 ClientsNumber = 10;
static const quint32 BuffersNumber = 10;
static const quint32 MaxSleep = 120000; //2 min
static const quint32 MinSleep = 200;   //0.2 sec
static const quint32 MaxBytesToSend = 1 * 1024 * 1024; //1 mb
static const quint32 ThreadsNumber = 10;
static const quint32 WaitTimeout = MaxSleep;
static const quint32 WaitIterations = WaitTimeout / MinSleep;

/*****************************************************************************/

class Receiver : public QObject
{
Q_OBJECT
public slots:
    void onDetachedClientReceivedToClient (
                                    IOClientInterface*,
                                    const SmartPtr<IOPackage>,
                                    const IOCommunication::DetachedClient );
    void onClientConnectedToServer ( IOSender::Handle );
    void onClientAttachedToServer ( IOSender::Handle,
                                    const SmartPtr<IOPackage> );

    void onPackageReceivedToClient ( IOClientInterface*,
                                     const SmartPtr<IOPackage> );

public:
    IOCommunication::DetachedClient getDetachedClient ();
    IOSender::Handle waitForClientConnected ();
    IOSender::Handle waitForClientAttached ( SmartPtr<IOPackage>& additional );
    SmartPtr<IOPackage> waitForPackage ( IOPackage::Type );

private:
    IOCommunication::DetachedClient m_detachedClient;
    IOSender::Handle m_newClientConnHandle;
    IOSender::Handle m_newClientAttachedHandle;
    SmartPtr<IOPackage> m_newClientAdditionalPkg;
    QList< SmartPtr<IOPackage> > m_pkgs;
    QMutex m_mutex;
};

void Receiver::onDetachedClientReceivedToClient (
                                    IOClientInterface*,
                                    const SmartPtr<IOPackage>,
                                    const IOCommunication::DetachedClient d )
{
    QMutexLocker locker( &m_mutex );
    m_detachedClient = d;
}

void Receiver::onClientConnectedToServer ( IOSender::Handle h )
{
    QMutexLocker locker( &m_mutex );
    m_newClientConnHandle = h;
}

void Receiver::onClientAttachedToServer ( IOSender::Handle h,
                                          const SmartPtr<IOPackage> addPkg )
{
    QMutexLocker locker( &m_mutex );
    m_newClientAttachedHandle = h;
    m_newClientAdditionalPkg = addPkg;
}

void Receiver::onPackageReceivedToClient ( IOClientInterface*,
                                           const SmartPtr<IOPackage> p )
{
    QMutexLocker locker( &m_mutex );
    m_pkgs.append(p);
}

IOCommunication::DetachedClient Receiver::getDetachedClient ()
{
    IOCommunication::DetachedClient d;

    for ( quint32 i = 0; i < WaitIterations; ++i ) {
        QMutexLocker locker( &m_mutex );
        if ( m_detachedClient.isValid() ) {
            d = m_detachedClient;
            break;
        }
        locker.unlock();
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }
    return d;
}

IOSender::Handle Receiver::waitForClientConnected ()
{
    IOSender::Handle h;

    for ( quint32 i = 0; i < WaitIterations; ++i ) {
        QMutexLocker locker( &m_mutex );
        if ( ! m_newClientConnHandle.isEmpty() ) {
            h = m_newClientConnHandle;
            break;
        }
        locker.unlock();
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }
    return h;
}

IOSender::Handle Receiver::waitForClientAttached (
    SmartPtr<IOPackage>& additionalPkg )
{
    IOSender::Handle h;

    for ( quint32 i = 0; i < WaitIterations; ++i ) {
        QMutexLocker locker( &m_mutex );
        if ( ! m_newClientAttachedHandle.isEmpty() ) {
            h = m_newClientAttachedHandle;
            additionalPkg = m_newClientAdditionalPkg;
            break;
        }
        locker.unlock();
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }
    return h;
}

SmartPtr<IOPackage> Receiver::waitForPackage ( IOPackage::Type t )
{
    SmartPtr<IOPackage> p;

    for ( quint32 i = 0; i < WaitIterations; ++i ) {
        QMutexLocker locker( &m_mutex );
        foreach ( SmartPtr<IOPackage> pkg, m_pkgs ) {
            if ( pkg->header.type == t ) {
                p = pkg;
                break;
            }
        }
        if ( p.isValid() )
            break;
        locker.unlock();
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }
    return p;
}

/*****************************************************************************/

class CommunicationTest : public QObject
{
    Q_OBJECT

public:
    enum PackageType {
        UnknownType = 0,
        ResponseType  = 100,
        RequestType2,
        RequestType,
        AttachDetachRequest,
        AttachDetachResponse,
        CreateProcessRequest,
        GetDetachedClientRequest,
        FirstPackage,
    };

    CommunicationTest ( IOSender::ConnectionMode );

private:
    void checkServerReceive ( const QList< SmartPtr<IOPackage> >& );
    void checkAllClientsReceive ( const SmartPtr<IOPackage>& );
    void checkExactClientReceive ( const IOSender::Handle&,
                                   const SmartPtr<IOPackage>&,
                                   bool comparePackageUuids = false );
    void receiveTimePackage ( const QTime&, const SmartPtr<IOPackage>& );

    void comparePackages ( const SmartPtr<IOPackage>&,
                           const SmartPtr<IOPackage>&,
                           bool comparePackageUuids = false );

    void initServerAndClients ();
    void cleanupServerAndClients ();

    void sendBigMessageFromServerToAllClients ();
    void sendBigMessageFromAllClientsToServer ();
    void sendShortMessageFromServerToExactClients ();

    void checkUuids ();
    void sendShortMessageAndWait_MultiThread (
                                              bool senderIsClient,
                                              bool waitSend,
                                              bool waitResponse );

    void sendBigMessageAndWaitFromThreads (
                                           bool senderIsClient,
                                           bool waitSend,
                                           bool waitResponse );

    void compareJobs ();

    void detachAndSendDetachedClient ();
    void startProcessAndSendDetachedClient ();

    void stopClientOrServerWhileWaitingForSendOrResponseResults (
                                                            bool isClient,
                                                            bool waitForSend );

    bool waitForDetachedClient ( IOSender::Handle,
                                 IOServerInterface*,
                                 IOCommunication::DetachedClient&,
                                 quint32 clientsNumber );

protected slots:
    void onPackageToServer ( IOServerInterface*,
                             IOSender::Handle,
                             const SmartPtr<IOPackage> );
    void onResponsePackageToServer ( IOSender::Handle,
                                     IOSendJob::Handle,
                                     const SmartPtr<IOPackage> );
    void onDetachClientOnServer ( IOServerInterface*,
                                  IOSender::Handle,
                                  const IOCommunication::DetachedClient );
    void onDetachedClientToServer ( IOServerInterface*,
                                    IOSender::Handle,
                                    const SmartPtr<IOPackage>,
                                    const IOCommunication::DetachedClient );

    void onPackageToClient ( IOClientInterface*, const SmartPtr<IOPackage> );
    void onResponsePackageToClient ( IOClientInterface*,
                                     IOSendJob::Handle,
                                     const SmartPtr<IOPackage> );
    void onDetachedClientToClient ( IOClientInterface*,
                                    const SmartPtr<IOPackage>,
                                    const IOCommunication::DetachedClient );

    void clientConnectedToServer ( IOSender::Handle );
    void clientDisconnectedFromServer ( IOSender::Handle );


private slots:
    // Remote
    void Remote_initServerAndClients ();

    void Remote_sendBigMessageFromServerToAllClients ();
    void Remote_sendBigMessageFromAllClientsToServer ();
    void Remote_sendShortMessageFromServerToExactClients ();

    void Remote_checkUuids ();

    void Remote_sendShortMessageFromClientToServerAndWaitForResponse_MultiThread ();
    void Remote_sendShortMessageFromClientToServerAndWaitForSendAndResponse_MultiThread ();
    void Remote_sendShortMessageFromServerToAllClientsAndWaitForResponse_MultiThread ();
    void Remote_sendShortMessageFromServerToAllClientsAndWaitForSendAndResponse_MultiThread ();

    void Remote_sendBigMessageFromClientToServerAndWaitForResponseFromThreads ();
    void Remote_sendBigMessageFromClientToServerAndWaitForSendAndResponseFromThreads ();
    void Remote_sendBigMessageFromServerToAllClientsAndWaitForResponseFromThreads ();
    void Remote_sendBigMessageFromServerToAllClientsAndWaitForSendAndResponseFromThreads ();

    void Remote_compareJobs ();

    void Remote_detachAndSendDetachedClient ();
    void Remote_startProcessAndSendDetachedClient ();

    void Remote_stopClientWhileWaitingForSendResults ();
    void Remote_stopClientWhileWaitingForResponseResults ();
    void Remote_stopServerWhileWaitingForSendResults ();
    void Remote_stopServerWhileWaitingForResponseResults ();

    void Remote_cleanupServerAndClients ();

public:
    IOSender::ConnectionMode m_connMode;
    IOSender::Handle m_proxyServerConnUuid;
    quint64 m_msecs;
    volatile quint32 m_connectedClients;
    volatile quint32 m_outProcessConnClients;

    IOServer* m_server;
    QList<IOClient*> m_clientList;
    QHash< IOClient*, QString > m_clientsUuids;

    struct DetachedClientInfo
    {
        IOServerInterface* serverImpl;
        IOCommunication::DetachedClient detachedClient;
        SmartPtr<IOPackage> requestPkg;
        IOSender::Handle outerClientHandle;
    };

    QHash< QString, DetachedClientInfo > m_detachedClients;

    QHash< IOClient*, SmartPtr<IOPackage> > m_clientsPackages;
    QHash< IOClient*, SmartPtr<IOPackage> > m_serverPackages;
    QHash< QString, IOSendJob::Handle > m_jobHash;
    QHash< QString, QString> m_outerClients;

    QMutex m_serverMutex;
    QMutex m_clientsMutex;
    QMutex m_jobMutex;
    QMutex m_processMutex;
    QWaitCondition m_jobWait;

    quint32 m_serverResponses;
    quint32 m_clientResponses;
    bool m_compareJobs;
    bool m_isOuterProcess;
};

/*****************************************************************************/

CommunicationTest::CommunicationTest ( IOSender::ConnectionMode connMode ) :
    m_connMode(connMode),
    m_proxyServerConnUuid(Uuid::createUuid().toString()),
    m_msecs(0),
    m_connectedClients(0),
    m_outProcessConnClients(0),
    m_server(0),
    m_serverMutex( QMutex::Recursive ),
    m_clientsMutex( QMutex::Recursive ),
    m_serverResponses(0),
    m_clientResponses(0),
    m_compareJobs(false),
    m_isOuterProcess(false)
{}

void CommunicationTest::onPackageToServer (
    IOServerInterface* servImpl,
    IOSender::Handle h,
    const SmartPtr<IOPackage> p )
{
    QString clientUuid;
    IOClient* client = 0;

    if ( ! m_isOuterProcess ) {
        clientUuid = Uuid::toString( p->header.senderUuid );

        QList<IOClient*> keys = m_clientsUuids.keys();
        QList<IOClient*>::Iterator it = keys.begin();

        // Found client by uuid
        for ( ; it != keys.end(); ++it ) {
            QString uuid = m_clientsUuids[*it];
            if ( uuid == clientUuid ) {
                client = *it;
                break;
            }
        }

        Q_ASSERT( client );

        m_serverMutex.lock();
        m_serverPackages[client] = p;
        m_serverMutex.unlock();
    }

    if ( p->header.type == CommunicationTest::RequestType ||
         p->header.type == CommunicationTest::RequestType2 ) {

        // Wait for all waitings
        if ( p->header.type == CommunicationTest::RequestType2 ) {

            const quint32 MaxIterations = 50;
            quint32 iterations = 0;

            IOSendJob::Handle job;
            do {

                if ( ! job.isValid() ) {
                    m_jobMutex.lock();
                    if ( m_jobHash.contains(h) )
                        job = m_jobHash[h];
                    m_jobMutex.unlock();
                }

                if ( ! job.isValid() ||
                     client->getResponseWaitingsNumber(job) !=
                     ThreadsNumber ) {

                    sleepMsecs( MinSleep );
                    continue;
                }
                else
                    break;

            } while ( iterations++ <= MaxIterations );

            QVERIFY( job.isValid() );

            QVERIFY( client->getResponseWaitingsNumber(job) ==
                     ThreadsNumber );
        }

        SmartPtr<IOPackage> response = IOPackage::createInstance(
                                              CommunicationTest::ResponseType,
                                              0, p );

        // Try to send again and again if queue is full
        quint32 sends = 0;
        const quint32 MaxSends = 20;
        do {
            QVERIFY( ++sends <= MaxSends );

            IOSendJob::Handle job = m_server->sendPackage( h, response );
            QVERIFY(job.isValid());
            IOSendJob::Result res = m_server->getSendResult( job );
            MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                                res == IOSendJob::SendPended ||
                                res == IOSendJob::SendQueueIsFull);
            if ( res == IOSendJob::SendQueueIsFull ) {
                sleepMsecs( MinSleep );
                continue;
            }
            break;
        } while (1);
    }
    else if ( p->header.type == CommunicationTest::AttachDetachRequest ) {
        if ( p->header.buffersNumber == 0 ) {
            SmartPtr<IOPackage> response = IOPackage::createInstance(
                                       CommunicationTest::AttachDetachResponse,
                                       1, p );
            // Fill response with ptr
            IOServerInterface** pp = &servImpl;
            response->fillBuffer( 0, IOPackage::RawEncoding,
                                  reinterpret_cast<const char*>(pp),
                                  sizeof(IOServerInterface*) );
            IOSendJob::Handle job = servImpl->sendPackage( h, response );
            QVERIFY(job.isValid());
            IOSendJob::Result res = servImpl->waitForSend( job );
            MY_INT_QVERIFY(res, res == IOSendJob::Success);
            res = m_server->getSendResult( job );
            MY_INT_QVERIFY(res, res == IOSendJob::Success);
        }
        else {
            Q_ASSERT(0);
        }
    }
    else if ( p->header.type == CommunicationTest::CreateProcessRequest ) {
        // Lock to prevent simultaneous process creation
        QMutexLocker processLocker( &m_processMutex );

        IOSender::Handle sender = p->buffers[0].getImpl();
        QVERIFY( h == sender );

        // Create detached client socket
        IOCommunication::SocketHandle socketH =
            servImpl->createDetachedClientSocket();
        QVERIFY( socketH.isValid() );

        qint64 pid = 0;
        IOSender::Handle newClientH;
        // Create process, pass socket handle to it and get new client
        {
            Receiver receiver;
            // Connect
            bool res = QObject::connect( servImpl,
                                         SIGNAL(onClientConnected(
                                                       IOSender::Handle)),
                                         &receiver,
                                         SLOT(onClientConnectedToServer(
                                                       IOSender::Handle)),
                                         Qt::DirectConnection );
            QVERIFY(res);
            pid = createProcess(
                            QCoreApplication::applicationFilePath(),
#ifdef _WIN_
                            QStringList() << "test",
                            socketH
#else
                            QStringList() << socketH->socketToString() << "test"
#endif
 );
            QVERIFY(pid != 0);

            newClientH = receiver.waitForClientConnected();
            QVERIFY( ! newClientH.isEmpty() );
        }

        // Create detached client with specified pid (used only on Windows)
        QMutexLocker locker( &m_serverMutex );
        DetachedClientInfo clientInfo;
        clientInfo.serverImpl = servImpl;
        clientInfo.requestPkg = p;
        clientInfo.outerClientHandle = newClientH;
        m_detachedClients[h] = clientInfo;
        m_outerClients[newClientH] = h;
        locker.unlock();

        // Unlock process mutex
        processLocker.unlock();

        // Detach. Process execution will be done at appropriate slot
        bool res = servImpl->detachClient( h, (int)pid, p );
        QVERIFY(res);
    }
    else if ( p->header.type == CommunicationTest::GetDetachedClientRequest ) {
        QMutexLocker locker( &m_serverMutex );
        QVERIFY(m_outerClients.contains(h));
        IOSender::Handle innerH = m_outerClients[ h ];
        QVERIFY(m_detachedClients.contains(innerH));
        DetachedClientInfo& clientInfo = m_detachedClients[innerH];
        QVERIFY( ! clientInfo.outerClientHandle.isEmpty() );
        QVERIFY( clientInfo.detachedClient.isValid() );

        // Save detached client
        IOCommunication::DetachedClient detachedClient =
            clientInfo.detachedClient;
        IOSender::Handle outerClientHandle =
            clientInfo.outerClientHandle;
        // Free handle
        clientInfo.detachedClient = IOCommunication::DetachedClient();

        // Unlock
        locker.unlock();

        IOSendJob::Handle job =
            servImpl->sendDetachedClient( outerClientHandle,
                                          detachedClient, p );
        QVERIFY(job.isValid());
        IOSendJob::Result res = servImpl->waitForSend(job);
        MY_INT_QVERIFY(res, res == IOSendJob::Success);
        res = servImpl->getSendResult(job);
        MY_INT_QVERIFY(res, res == IOSendJob::Success);
    }
}

void CommunicationTest::onResponsePackageToServer (
    IOSender::Handle,
    IOSendJob::Handle job,
    const SmartPtr<IOPackage> p )
{
    if ( ! m_isOuterProcess ) {
        QVERIFY( p->header.type == CommunicationTest::ResponseType );
        m_jobMutex.lock();
        ++m_serverResponses;

        if ( m_compareJobs ) {
            const QString& srvUuid = m_server->senderHandle();

            // Compare jobs
            if ( m_jobHash.contains(srvUuid) ) {
                const IOSendJob::Handle& jobH = m_jobHash[srvUuid];
                QVERIFY( m_server->getJobUuid(jobH) ==
                         m_server->getJobUuid(job) );
                m_jobWait.wakeOne();
            }
            else {
                m_jobHash[srvUuid] = job;
            }
        }

        m_jobMutex.unlock();
    }
}

void CommunicationTest::onPackageToClient ( IOClientInterface* clientInt,
                                            const SmartPtr<IOPackage> p )
{
    IOClient* client = qobject_cast<IOClient*>(clientInt);
    Q_ASSERT(client);

    if ( ! m_isOuterProcess ) {

        QTime time = QTime::currentTime();

        Q_ASSERT(p.isValid());

        // Handle time package
        if ( p->header.buffersNumber > 0 &&
             p->buffers[0].isValid() )
            if ( 0 == memcmp(p->buffers[0].getImpl(), "TIME", 4) )
                receiveTimePackage( time, p );

        m_clientsMutex.lock();
        m_clientsPackages[client] = p;
        m_clientsMutex.unlock();
    }

    if ( p->header.type == CommunicationTest::RequestType ||
         p->header.type == CommunicationTest::RequestType2 ) {

        // Wait for all waitings
        if ( p->header.type == CommunicationTest::RequestType2 ) {
            const quint32 MaxIterations = 50;
            quint32 iterations = 0;

            IOSendJob::Handle job;
            do {

                if ( ! job.isValid() ) {
                    m_jobMutex.lock();
                    if ( m_jobHash.contains(m_server->senderHandle()) )
                        job = m_jobHash[m_server->senderHandle()];
                    m_jobMutex.unlock();
                }

                if ( ! job.isValid() ||
                     m_server->getResponseWaitingsNumber(job) !=
                     ThreadsNumber ) {

                    sleepMsecs( MinSleep );
                    continue;
                }
                else
                    break;

            } while ( iterations++ <= MaxIterations );

            QVERIFY( job.isValid() );

            QVERIFY( m_server->getResponseWaitingsNumber(job) ==
                     ThreadsNumber );
        }

        SmartPtr<IOPackage> response = IOPackage::createInstance(
                                              CommunicationTest::ResponseType,
                                              0, p );

        QVERIFY( m_clientsUuids.contains(client) == true );
        QString clientUuid = m_clientsUuids[client];

        // Fill client uuid
        Uuid::dump( clientUuid, response->header.senderUuid );

        // Try to send again and again if queue is full
        quint32 sends = 0;
        const quint32 MaxSends = 20;
        do {
            QVERIFY( ++sends <= MaxSends );

            IOSendJob::Handle job = client->sendPackage( response );
            QVERIFY(job.isValid());
            IOSendJob::Result res = client->getSendResult( job );
            MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                                res == IOSendJob::SendPended ||
                                res == IOSendJob::SendQueueIsFull);
            if ( res == IOSendJob::SendQueueIsFull ) {
                sleepMsecs( MinSleep );
                continue;
            }
            break;
        } while (1);
    }
    else if ( p->header.type == CommunicationTest::AttachDetachRequest ) {
        // Lock server
        QMutexLocker locker( &m_serverMutex );
        QVERIFY(m_detachedClients.contains(client->senderHandle()));

        DetachedClientInfo clientInfo =
            m_detachedClients[ client->senderHandle() ];
        m_detachedClients.remove(client->senderHandle());

        // Unlock
        locker.unlock();

        QVERIFY( clientInfo.requestPkg.isValid() );
        // request is turning into response :)
        clientInfo.requestPkg->makeDirectResponse(p);

        IOSendJob::Handle job = client->sendPackage( clientInfo.requestPkg );
        IOSendJob::Result res = client->waitForSend( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success);
        res = client->getSendResult( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success);
    }
}

void CommunicationTest::onResponsePackageToClient (
    IOClientInterface* clientInt,
    IOSendJob::Handle job,
    const SmartPtr<IOPackage> p )
{
    IOClient* client = qobject_cast<IOClient*>(clientInt);
    Q_ASSERT(client);

    if ( p->header.type == CommunicationTest::ResponseType ) {
        QMutexLocker locker( &m_jobMutex );
        ++m_clientResponses;

        if ( m_compareJobs ) {
            const QString& cliUuid = client->senderHandle();

            // Compare jobs
            if ( m_jobHash.contains(cliUuid) ) {
                const IOSendJob::Handle& jobH = m_jobHash[cliUuid];
                QVERIFY( m_server->getJobUuid(jobH) ==
                         m_server->getJobUuid(job) );
                m_jobWait.wakeOne();
            }
            else {
                m_jobHash[cliUuid] = job;
            }
        }
    }
    else if ( p->header.type == CommunicationTest::AttachDetachResponse ) {
    }
}

void CommunicationTest::onDetachClientOnServer (
    IOServerInterface* serverImpl,
    IOSender::Handle sender,
    const IOCommunication::DetachedClient detachedClient )
{
    QMutexLocker locker( &m_serverMutex );
    if ( ! m_detachedClients.contains(sender) ) {
        DetachedClientInfo clientInfo;
        clientInfo.serverImpl = serverImpl;
        clientInfo.detachedClient = detachedClient;
        clientInfo.requestPkg = SmartPtr<IOPackage>();
        m_detachedClients[sender] = clientInfo;
    } else {
        QVERIFY( m_isOuterProcess );
        DetachedClientInfo& clientInfo = m_detachedClients[sender];
        QVERIFY( clientInfo.serverImpl == serverImpl );
        QVERIFY( ! clientInfo.detachedClient.isValid() );
        QVERIFY( clientInfo.requestPkg.isValid() );
        QVERIFY( ! clientInfo.outerClientHandle.isEmpty() );

        // Save detached
        clientInfo.detachedClient = detachedClient;

        // Unlock
        locker.unlock();

        // Send first package to start session
        SmartPtr<IOPackage> p =
            IOPackage::createInstance( CommunicationTest::FirstPackage, 0);
        IOSendJob::Handle job =
            serverImpl->sendPackage( clientInfo.outerClientHandle, p );
        IOSendJob::Result jRes = serverImpl->waitForSend( job, MaxSleep );
        jRes = serverImpl->getSendResult( job );
        MY_INT_QVERIFY(jRes, jRes == IOSendJob::Success);
    }
}

void CommunicationTest::onDetachedClientToServer (
    IOServerInterface*,
    IOSender::Handle ,
    const SmartPtr<IOPackage> ,
    const IOCommunication::DetachedClient  )
{
    qWarning("NOT IMPL");
}

void CommunicationTest::onDetachedClientToClient (
    IOClientInterface*,
    const SmartPtr<IOPackage>,
    const IOCommunication::DetachedClient )
{
    qWarning("NOT IMPL!");
}

void CommunicationTest::clientConnectedToServer (
    IOSender::Handle h )
{
    m_clientsMutex.lock();

    if ( ! m_isOuterProcess ) {
        IOClient* nowConnectedClient = m_clientList[m_connectedClients];
        IOSender::State st = nowConnectedClient->waitForConnection();
        MY_INT_QVERIFY(st, st == IOSender::Connected);
        QVERIFY( nowConnectedClient->senderHandle() == h );
        QVERIFY( nowConnectedClient->senderType() ==
                 m_server->clientSenderType(h) );
        QVERIFY( nowConnectedClient->senderType() ==
                 m_server->clientSenderType(h) );

        ++m_connectedClients;
    }
    else {
        ++m_outProcessConnClients;
    }

    m_clientsMutex.unlock();

    LOG_MESSAGE( DBG_INFO,"Client attached : %s", qPrintable(h));
}

void CommunicationTest::clientDisconnectedFromServer (
    IOSender::Handle h )
{
    m_clientsMutex.lock();

    if ( ! m_isOuterProcess )
        --m_connectedClients;
    else
        --m_outProcessConnClients;

    m_clientsMutex.unlock();
    (void)h;
    LOG_MESSAGE( DBG_INFO,"Client detached : %s", qPrintable(h));
}

void CommunicationTest::receiveTimePackage ( const QTime& currentTime,
                                             const SmartPtr<IOPackage>& p )
{
    QString timeStr( QByteArray(p->buffers[0].getImpl() + 4) );
    QTime time = QTime::fromString( timeStr, TimeFormat );
    if ( ! time.isValid() ) {
        LOG_MESSAGE( DBG_FATAL,"Wrong time format! : %s", qPrintable(timeStr) );
        return;
    }

    Q_UNUSED(currentTime);
}

void CommunicationTest::comparePackages ( const SmartPtr<IOPackage>& pkg1,
                                          const SmartPtr<IOPackage>& pkg2,
                                          bool comparePackageUuids )
{
    QVERIFY( ! Uuid::toUuid(pkg1->header.uuid).isNull() );
    QVERIFY( ! Uuid::toUuid(pkg1->header.senderUuid).isNull() );

    if ( comparePackageUuids )
        QVERIFY( 0 == memcmp(pkg1->header.uuid,
                             pkg2->header.uuid,
                             sizeof(Uuid_t)) );

    QVERIFY( 0 == memcmp(pkg1->header.parentUuid,
                         pkg2->header.parentUuid,
                         sizeof(Uuid_t)) );
    QVERIFY( 0 == memcmp(pkg1->header.senderUuid,
                         pkg2->header.senderUuid,
                         sizeof(Uuid_t)) );
    QVERIFY( 0 == memcmp(pkg1->header.receiverUuid,
                         pkg2->header.receiverUuid,
                         sizeof(Uuid_t)) );

    QCOMPARE( pkg1->header.type, pkg2->header.type );
    QCOMPARE( pkg1->header.numericId,
              pkg2->header.numericId );
    QCOMPARE( pkg1->header.buffersNumber,
              pkg2->header.buffersNumber );

    for ( uint i = 0; i < pkg2->header.buffersNumber; ++i ) {
        IOPackage::EncodingType enc1, enc2;
        quint32 size1, size2;
        SmartPtr<char> buff1, buff2;

        pkg1->getBuffer(i, enc1, buff1, size1);
        pkg2->getBuffer(i, enc2, buff2, size2);

        QCOMPARE( enc1, enc2 );
        QCOMPARE( size1, size2 );

        QVERIFY( 0 == memcmp(buff1.getImpl(),
                             buff2.getImpl(),
                             size1) );
    }
}

void CommunicationTest::checkServerReceive (
    const QList< SmartPtr<IOPackage> >& packages )
{
    QList< SmartPtr<IOPackage> > sentPackages = packages;

    bool allPackagesReceived = false;

    quint32 Iterations = MaxSleep / MinSleep;
    while ( --Iterations ) {

        QCoreApplication::processEvents();

        m_serverMutex.lock();

        QHash< IOClient*, SmartPtr<IOPackage> >
            localServerPackages = m_serverPackages;

        m_serverMutex.unlock();

        bool packagesReceived = true;

        QList<IOClient*>::Iterator it = m_clientList.begin();
        for ( ; it != m_clientList.end(); ++it ) {
            IOClient* client = *it;
            if ( localServerPackages.contains(client) ) {
                SmartPtr<IOPackage> package =
                    localServerPackages[client];

                SmartPtr<IOPackage> p;

                // Find received package in sent list
                QList< SmartPtr<IOPackage> >::Iterator pkgIt =
                    sentPackages.begin();

                while ( pkgIt != sentPackages.end() ) {
                    SmartPtr<IOPackage> sentPkg = *pkgIt;

                    if ( 0 == ::memcmp(sentPkg->buffers[0].getImpl(),
                                       package->buffers[0].getImpl(),
                                       sizeof(Uuid_t)) ) {

                        p = sentPkg;
                        pkgIt = sentPackages.erase(pkgIt);
                        break;
                    }
                    else
                        ++pkgIt;
                }

                if ( ! p.isValid() )
                    continue;

                comparePackages( package, p );
            }
            else {
                packagesReceived = false;
                break;
            }
        }

        if ( packagesReceived ) {
            allPackagesReceived = true;
            break;
        }

        sleepMsecs( MinSleep );
    }

    // Check
    QVERIFY( allPackagesReceived );
}

void CommunicationTest::checkExactClientReceive (
    const IOSender::Handle& h,
    const SmartPtr<IOPackage>& p,
    bool comparePackageUuids )
{
    quint32 Iterations = MaxSleep / MinSleep;
    while ( --Iterations ) {

        QCoreApplication::processEvents();

        sleepMsecs( MinSleep );

        m_clientsMutex.lock();

        QHash< IOClient*, SmartPtr<IOPackage> >
            localClientsPackages = m_clientsPackages;

        m_clientsMutex.unlock();

        QList<IOClient*>::Iterator it = m_clientList.begin();
        for ( ; it != m_clientList.end(); ++it ) {
            IOClient* client = *it;

            IOSender::Handle clientHandle =
                m_clientsUuids[client];


            if ( localClientsPackages.contains(client) &&
                 m_clientsUuids.contains(client) &&
                 m_clientsUuids[client] == h ) {
                SmartPtr<IOPackage> package =
                    localClientsPackages[client];

                comparePackages( package, p, comparePackageUuids );

                // Great. Wait a little, to be sure other clients will
                // not receive this package
                if ( Iterations > 3 )
                    Iterations = 3;
            }
            else if ( localClientsPackages.contains(client) &&
                      ! m_clientsUuids.contains(client) ) {
                // Other clients must not receive this package
                QVERIFY( false );
                return;
            }
        }
    }
}

void CommunicationTest::checkAllClientsReceive (
    const SmartPtr<IOPackage>& p )
{
    bool allPackagesReceived = false;

    quint32 Iterations = MaxSleep / MinSleep;
    while ( --Iterations ) {

        QCoreApplication::processEvents();

        m_clientsMutex.lock();

        QHash< IOClient*, SmartPtr<IOPackage> >
            localClientsPackages = m_clientsPackages;

        m_clientsMutex.unlock();

        bool packagesReceived = true;

        QList<IOClient*>::Iterator it = m_clientList.begin();
        for ( ; it != m_clientList.end(); ++it ) {
            IOClient* client = *it;
            if ( localClientsPackages.contains(client) ) {
                SmartPtr<IOPackage> package =
                    localClientsPackages[client];

                comparePackages( package, p );
            }
            else {
                packagesReceived = false;
                break;
            }
        }

        if ( packagesReceived ) {
            allPackagesReceived = true;
            break;
        }

        sleepMsecs( MinSleep );
    }

    // Check
    QVERIFY( allPackagesReceived );
}

bool CommunicationTest::waitForDetachedClient (
    IOSender::Handle h,
    IOServerInterface* serverInt,
    IOCommunication::DetachedClient& detachedClient,
    quint32 clientsNumber )
{
    detachedClient = IOCommunication::DetachedClient();

    for ( quint32 i = 0; i < WaitIterations; ++i ) {
        {
            // Lock
            QMutexLocker locker( &m_serverMutex );
            if ( m_detachedClients.contains(h) ) {
                if ( m_detachedClients[h].serverImpl == serverInt ) {
                    detachedClient = m_detachedClients[h].detachedClient;
                    break;
                }

                return false;
            }
        }
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }

    if ( ! detachedClient.isValid() )
        return false;

    for ( quint32 i = 0; i < WaitIterations; ++i ) {
        quint32 clientsNum = serverInt->countClients();
        if ( clientsNum == clientsNumber )
            return true;
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }

    return false;
}

/********************* COMMON ************************************************/

void CommunicationTest::initServerAndClients ()
{
    Q_ASSERT( m_connectedClients == 0 );
    Q_ASSERT( m_server == 0 );
    Q_ASSERT( m_clientList.size() == 0 );
    Q_ASSERT( m_clientsUuids.size() == 0 );
    Q_ASSERT( m_detachedClients.size() == 0 );
    Q_ASSERT( m_outerClients.size() == 0 );
    Q_ASSERT( m_clientsPackages.size() == 0 );
    Q_ASSERT( m_serverPackages.size() == 0 );
    Q_ASSERT( m_jobHash.size() == 0 );
    Q_ASSERT( m_serverResponses == 0 );
    Q_ASSERT( m_clientResponses == 0 );

    if ( m_connMode == IOSender::DirectConnectionMode )
        m_server = new IOServer(
                 IORoutingTableHelper::GetServerRoutingTable(PSL_LOW_SECURITY),
                 IOSender::Dispatcher, IOService::LoopbackAddr, RemotePortNumber );
    else
        m_server = new IOServer(
                 IORoutingTableHelper::GetServerRoutingTable(PSL_LOW_SECURITY),
                 IOSender::Dispatcher, m_proxyServerConnUuid,
                 //XXX Proxy server does not support IPv6, so omit many warning logs
                 "127.0.0.1", ProxyPortNumber );


    QObject::connect( m_server,
                     SIGNAL(onPackageReceived(IOServerInterface*,
                                              IOSender::Handle,
                                              const SmartPtr<IOPackage>)),
                     SLOT(onPackageToServer(IOServerInterface*,
                                            IOSender::Handle,
                                            const SmartPtr<IOPackage>)),
                     Qt::DirectConnection );
    QObject::connect( m_server,
                     SIGNAL(onResponsePackageReceived(IOSender::Handle,
                                                      IOSendJob::Handle,
                                              const SmartPtr<IOPackage>)),
                     SLOT(onResponsePackageToServer(IOSender::Handle,
                                                    IOSendJob::Handle,
                                            const SmartPtr<IOPackage>)),
                     Qt::DirectConnection );
    QObject::connect( m_server,
                     SIGNAL(onDetachClient(IOServerInterface*,
                                                     IOSender::Handle,
                                      const IOCommunication::DetachedClient)),
                     SLOT(onDetachClientOnServer(IOServerInterface*,
                                            IOSender::Handle,
                                      const IOCommunication::DetachedClient)),
                     Qt::DirectConnection );
    QObject::connect( m_server,
                     SIGNAL(onDetachedClientReceived(IOServerInterface*,
                                                     IOSender::Handle,
                                      const SmartPtr<IOPackage>,
                                      const IOCommunication::DetachedClient)),
                     SLOT(onDetachedClientToServer(IOServerInterface*,
                                            IOSender::Handle,
                                      const SmartPtr<IOPackage>,
                                      const IOCommunication::DetachedClient)),
                     Qt::DirectConnection );

    QObject::connect( m_server,
                      SIGNAL(onClientConnected(IOSender::Handle)),
                      SLOT(clientConnectedToServer(IOSender::Handle)),
                      Qt::DirectConnection );

    QObject::connect( m_server,
                      SIGNAL(onClientDisconnected(IOSender::Handle)),
                      SLOT(clientDisconnectedFromServer(IOSender::Handle)),
                      Qt::DirectConnection );

    IOSender::State state = m_server->listen();
    if ( m_connMode == IOSender::ProxyConnectionMode )
        state = m_server->waitForProxyConnection();

    // Should be connected
    QVERIFY( state == IOSender::Connected );

    for ( uint i = 0; i < ClientsNumber; ++i ) {

        IOSender::Type senderType = (IOSender::Type)qrandBetween(
                                         IOSender::UnknownType,
                                         IOSender::EndTypeBound );

        if ( senderType == IOSender::UnknownType )
            senderType = (IOSender::Type)((int)senderType + 1);
        else if ( senderType == IOSender::EndTypeBound )
            senderType = (IOSender::Type)((int)senderType - 1);

        IOClient* client = 0;

        if ( m_connMode == IOSender::DirectConnectionMode )
            client = new IOClient(
                IORoutingTableHelper::GetClientRoutingTable(PSL_HIGH_SECURITY),
                senderType, IOService::LoopbackAddr, RemotePortNumber );
        else
            client = new IOClient(
                IORoutingTableHelper::GetClientRoutingTable(PSL_HIGH_SECURITY),
                senderType,
                m_proxyServerConnUuid,
                 //XXX Proxy server does not support IPv6, so omit many warning logs
                "127.0.0.1", ProxyPortNumber );

        QObject::connect( client,
                     SIGNAL(onPackageReceived(IOClientInterface*,
                                              const SmartPtr<IOPackage>)),
                     SLOT(onPackageToClient(IOClientInterface*,
                                            const SmartPtr<IOPackage>)),
                     Qt::DirectConnection );

        QObject::connect( client,
                     SIGNAL(onResponsePackageReceived(IOClientInterface*,
                                                      IOSendJob::Handle,
                                              const SmartPtr<IOPackage>)),
                     SLOT(onResponsePackageToClient(IOClientInterface*,
                                                    IOSendJob::Handle,
                                            const SmartPtr<IOPackage>)),
                     Qt::DirectConnection );

        QObject::connect( client,
                     SIGNAL(onDetachedClientReceived(IOClientInterface*,
                                      const SmartPtr<IOPackage>,
                                      const IOCommunication::DetachedClient)),
                     SLOT(onDetachedClientToClient(IOClientInterface*,
                                      const SmartPtr<IOPackage>,
                                      const IOCommunication::DetachedClient)),
                     Qt::DirectConnection );

        m_clientList.append( client );
    }

    for ( uint i = 0; i < ClientsNumber; ++i ) {
        IOClient* client = m_clientList[i];
        client->connectClient();
        IOSender::State state = client->waitForConnection();
        // Should be connected
        QVERIFY( state == IOSender::Connected );
        m_clientsUuids[client] = client->senderHandle();
    }

    while ( m_connectedClients != ClientsNumber ) {
        sleepMsecs( MinSleep );
        QCoreApplication::processEvents();
    }

    m_msecs = IOService::msecsFromEpoch();
}

void CommunicationTest::cleanupServerAndClients ()
{
    for ( uint i = 0; i < ClientsNumber; ++i )
        delete m_clientList[i];
    m_clientList.clear();

    delete m_server;
    m_server = 0;

    m_connectedClients = 0;

    m_clientsUuids.clear();
    m_clientsPackages.clear();
    m_serverPackages.clear();
    m_jobHash.clear();
    m_detachedClients.clear();
    m_outerClients.clear();

    m_serverResponses = 0;
    m_clientResponses = 0;

    qWarning("Elapsed %lld msecs", IOService::msecsFromEpoch() - m_msecs);
}

void CommunicationTest::sendBigMessageFromServerToAllClients ()
{
    m_clientsMutex.lock();
    m_clientsPackages.clear();
    m_clientsMutex.unlock();

    SmartPtr<IOPackage> p = IOPackage::createInstance( UnknownType,
                                                       BuffersNumber * 2 );

    // Fill uuids
    Uuid::createUuid( p->header.parentUuid );
    Uuid::createUuid( p->header.receiverUuid );
    Uuid::createUuid( p->header.senderUuid );

    for ( quint32 i = 0; i < BuffersNumber * 2; ++i ) {
        if ( i % 2 )
            continue;

        QString uuid = Uuid::createUuid().toString();
        QByteArray data;
        while ( (quint32)data.size() < MaxBytesToSend )
            data.append(uuid);

        p->fillBuffer(i, randEncoding(), data.data(), data.size());
    }

    // Get clients handles
    QList<IOSender::Handle> handles = m_server->getClientsHandles();

    // Multiple send
    foreach ( IOSender::Handle h, handles )
        m_server->sendPackage( h, p );

    checkAllClientsReceive( p );
}

void CommunicationTest::sendBigMessageFromAllClientsToServer ()
{
    m_serverMutex.lock();
    m_serverPackages.clear();
    m_serverMutex.unlock();

    QList< SmartPtr<IOPackage> > packages;

    for ( quint32 i = 0; i < ClientsNumber; ++i ) {
        IOClient* client = m_clientList[i];
        Q_ASSERT( client );
        Q_ASSERT( m_clientsUuids.contains(client) );

        QString clientUuid = m_clientsUuids[client];
        SmartPtr<IOPackage> p =
            IOPackage::createInstance( UnknownType,
                                       BuffersNumber * 2 );

        // Fill client uuid
        Uuid::createUuid( p->header.parentUuid );
        Uuid::dump( clientUuid, p->header.senderUuid );
        Uuid::createUuid( p->header.receiverUuid );

        // Set uuid to first buffer
        Uuid_t uuid = {0};
        Uuid::createUuid( uuid );
        p->fillBuffer(0, IOPackage::RawEncoding,
                      reinterpret_cast<const char*>(uuid),
                      sizeof(Uuid_t));

        for ( quint32 i = 1; i < BuffersNumber * 2; ++i ) {
            if ( i % 2 )
                continue;

            QString uuid = Uuid::createUuid().toString();
            QByteArray data;
            while ( (quint32)data.size() < MaxBytesToSend )
                data.append(uuid);

            p->fillBuffer(i, randEncoding(), data.data(), data.size());
        }

        client->sendPackage( p );

        packages.append( p );
    }

    checkServerReceive( packages );
}

void CommunicationTest::sendShortMessageFromServerToExactClients ()
{
    m_clientsMutex.lock();
    m_clientsPackages.clear();
    m_clientsMutex.unlock();

    for ( uint j = 0; j < ClientsNumber; ++j ) {
        IOClient* client = m_clientList[j];
        Q_ASSERT( client );
        Q_ASSERT( m_clientsUuids.contains(client) );

        IOSender::Handle h = m_clientsUuids[client];

        QString timeStr = QTime::currentTime().toString(TimeFormat);
        timeStr.prepend( "TIME" );

        m_clientsMutex.lock();
        m_clientsPackages.clear();
        m_clientsMutex.unlock();

        SmartPtr<IOPackage> p =
            IOPackage::createInstance( UnknownType, 1 );

        // Fill uuids
        Uuid::createUuid( p->header.parentUuid );
        Uuid::dump( h, p->header.receiverUuid );
        Uuid::dump( h, p->header.senderUuid );

        p->fillBuffer(0, randEncoding(), timeStr.toAscii().data(),
                      timeStr.length() + 1);


        m_server->sendPackage( h, p );
        checkExactClientReceive( h, p );
    }
}

void CommunicationTest::checkUuids ()
{
    QVERIFY( m_clientList.size() > 0 );

    IOClient* client = m_clientList[0];
    Q_ASSERT( client );
    Q_ASSERT( m_clientsUuids.contains(client) );

    IOSender::Handle h = m_clientsUuids[client];

    // Fill sender and package uuids empty
    {
        m_clientsMutex.lock();
        m_clientsPackages.clear();
        m_clientsMutex.unlock();

        SmartPtr<IOPackage> p =
            IOPackage::createInstance( UnknownType, 0 );

        // Fill uuids
        Uuid::createUuid( p->header.parentUuid );
        Uuid::dump( h, p->header.receiverUuid );

        IOSendJob::Handle job = m_server->sendPackage( h, p );
        IOSendJob::Result res = m_server->getSendResult( job );
        MY_INT_QVERIFY(res, res == IOSendJob::SendPended ||
                            res == IOSendJob::Success);

        res = m_server->waitForSend( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success);

        res = m_server->getSendResult( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success);

        // Fill sender uuid for correct comparison
        Uuid::dump( m_server->senderHandle(), p->header.senderUuid );

        checkExactClientReceive( h, p );
    }

    // Rewrite sender uuid and fill package uuid empty
    {
        m_clientsMutex.lock();
        m_clientsPackages.clear();
        m_clientsMutex.unlock();

        SmartPtr<IOPackage> p =
            IOPackage::createInstance( UnknownType, 0 );

        // Fill uuids
        Uuid::createUuid( p->header.parentUuid );
        Uuid::dump( h, p->header.receiverUuid );
        Uuid::createUuid( p->header.senderUuid );

        IOSendJob::Handle job = m_server->sendPackage( h, p );
        IOSendJob::Result res = m_server->getSendResult( job );
        MY_INT_QVERIFY(res, res == IOSendJob::SendPended ||
                            res == IOSendJob::Success);

        res = m_server->waitForSend( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success);

        res = m_server->getSendResult( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success);

        checkExactClientReceive( h, p );
    }

    // Rewrite sender uuid and package uuid
    {
        m_clientsMutex.lock();
        m_clientsPackages.clear();
        m_clientsMutex.unlock();

        SmartPtr<IOPackage> p =
            IOPackage::createInstance( UnknownType, 0 );

        // Fill uuids
        Uuid::createUuid( p->header.parentUuid );
        Uuid::dump( h, p->header.receiverUuid );
        Uuid::createUuid( p->header.senderUuid );
        Uuid::createUuid( p->header.uuid );

        IOSendJob::Handle job = m_server->sendPackage( h, p );
        IOSendJob::Result res = m_server->getSendResult( job );
        MY_INT_QVERIFY(res, res == IOSendJob::SendPended ||
                            res == IOSendJob::Success);

        res = m_server->waitForSend( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success);

        res = m_server->getSendResult( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success);

        // Compare uuids too
        checkExactClientReceive( h, p, true );
    }
}

class ThreadSender : public QThread
{
private:
    CommunicationTest* m_t;
    QMutex& m_mutex;
    QWaitCondition& m_wait;
    volatile quint32& m_countThreads;
    bool m_senderIsClient;
    bool m_waitForSend;
    bool m_waitForResponse;

public:
    ThreadSender ( CommunicationTest* t,
                   QMutex& mutex,
                   QWaitCondition& wait,
                   volatile quint32& countThreads,
                   bool senderIsClient,
                   bool waitForSend,
                   bool waitForResponse ) :
        m_t(t),
        m_mutex(mutex),
        m_wait(wait),
        m_countThreads(countThreads),
        m_senderIsClient(senderIsClient),
        m_waitForSend(waitForSend),
        m_waitForResponse(waitForResponse)

    {}

    void run ()
    {
        m_mutex.lock();

        // Get first client
        QVERIFY( m_t->m_clientList.size() > 0 );
        IOClient* client = m_t->m_clientList[0];
        Q_ASSERT( client );
        Q_ASSERT( m_t->m_clientsUuids.contains(client) );
        IOSender::Handle h = m_t->m_clientsUuids[client];

        // Create request package
        SmartPtr<IOPackage> p =
            IOPackage::createInstance( CommunicationTest::RequestType, 0 );

        // Fill sender uuid if sender is client
        if ( m_senderIsClient )
            Uuid::dump( h, p->header.senderUuid );

        ++m_countThreads;
        m_wait.wait( &m_mutex );
        m_mutex.unlock();

        // Start sending

        IOSendJob::Handle job;
        if ( m_senderIsClient )
            job = client->sendPackage( p );
        else
            job = m_t->m_server->sendPackage( h, p );

        // If wait for send
        if ( m_waitForSend ) {
            IOSendJob::Result res;
            if ( m_senderIsClient ) {
                res = client->waitForSend( job );
                MY_INT_QVERIFY(res, res == IOSendJob::Success);
                res = client->getSendResult( job );
                MY_INT_QVERIFY(res, res == IOSendJob::Success);
            }
            else {
                res = m_t->m_server->waitForSend( job );
                MY_INT_QVERIFY(res, res == IOSendJob::Success);
                res = m_t->m_server->getSendResult( job );
                MY_INT_QVERIFY(res, res == IOSendJob::Success );
            }
        }

        // If wait for response
        if ( m_waitForResponse ) {
            IOSendJob::Result res;
            if ( m_senderIsClient ) {
                res = client->waitForResponse( job, MaxSleep );
            }
            else {
                res = m_t->m_server->waitForResponse( job, MaxSleep );
            }

            MY_INT_QVERIFY(res, res == IOSendJob::Success);

            IOSendJob::Response resp;
            if ( m_senderIsClient )
                resp = client->takeResponse( job );
            else
                resp = m_t->m_server->takeResponse( job );

            QVERIFY(resp.responseResult == IOSendJob::Success);
            QVERIFY(resp.responsePackages.size()  == 1);
            QVERIFY(resp.responsePackages[0]->header.type ==
                    CommunicationTest::ResponseType);
        }

        // Send to all the clients
        if ( ! m_senderIsClient ) {
            // Get clients handles
            QList<IOSender::Handle> handles =
                m_t->m_server->getClientsHandles();

            // Multiple send
            QList<IOSendJob::Handle> jobs;
            foreach ( IOSender::Handle h, handles )
                jobs.append( m_t->m_server->sendPackage(h, p) );

            // If wait for send
            if ( m_waitForSend ) {
                foreach ( IOSendJob::Handle job, jobs ) {
                    IOSendJob::Result res =
                        m_t->m_server->waitForSend( job );
                    MY_INT_QVERIFY(res, res == IOSendJob::Success);
                }
            }

            // If wait for response
            if ( m_waitForResponse ) {
                foreach ( IOSendJob::Handle job, jobs ) {
                    IOSendJob::Result res =
                        m_t->m_server->waitForResponse( job, MaxSleep );

                    MY_INT_QVERIFY(res, res == IOSendJob::Success);
                }

                foreach ( IOSendJob::Handle job, jobs ) {
                    IOSendJob::Response resp =
                        m_t->m_server->takeResponse( job );

                    QVERIFY(resp.responseResult == IOSendJob::Success);
                    QVERIFY(resp.responsePackages.size() == 1);
                    QVERIFY(resp.responsePackages[0]->header.type ==
                            CommunicationTest::ResponseType);
                }
            }
        }

        // Finish
        m_mutex.lock();
        --m_countThreads;
        m_mutex.unlock();
    }
};

void CommunicationTest::sendShortMessageAndWait_MultiThread (
    bool senderIsClient,
    bool waitSend,
    bool waitResponse )
{
    m_clientsMutex.lock();
    m_clientsPackages.clear();
    m_clientsMutex.unlock();

    m_serverMutex.lock();
    m_serverPackages.clear();
    m_serverMutex.unlock();

    QMutex mutex;
    QWaitCondition wait;
    volatile quint32 startedThreads = 0;

    m_serverResponses = 0;
    m_clientResponses = 0;

    QList< SmartPtr<ThreadSender> > threadPool;
    for ( quint32 i = 0; i < ThreadsNumber; ++i ) {
        SmartPtr<ThreadSender> thread(
            new ThreadSender( this,
                              mutex, wait, startedThreads,
                              senderIsClient,
                              waitSend,
                              waitResponse ) );

        thread->start();
        threadPool.append( thread );
    }

    // Ready
    while ( startedThreads != ThreadsNumber ) {
        sleepMsecs( MinSleep );
        QCoreApplication::processEvents();
    }

    QVERIFY( startedThreads == ThreadsNumber );

    // Steady
    sleepMsecs( 2000 );

    // Go
    wait.wakeAll();

    // Wait for finish
    while ( startedThreads != 0 ) {
        sleepMsecs( MinSleep );
        QCoreApplication::processEvents();
    }

    // Wait for finish threads
    for ( quint32 i = 0; i < ThreadsNumber; ++i ) {
        threadPool[i]->wait();
    }

    // Clean all threads
    threadPool.clear();
}

class ThreadWaiter : public QThread {
public:
    CommunicationTest* m_t;
    QMutex& m_mutex;
    QWaitCondition& m_wait;
    volatile quint32& m_countThreads;
    volatile bool& m_responseHasBeenTaken;
    bool m_senderIsClient;
    bool m_waitForSend;
    bool m_waitForResponse;
    IOClient* m_client;
    IOSendJob::Handle& m_job;

public:
    ThreadWaiter ( CommunicationTest* t,
                   QMutex& mutex,
                   QWaitCondition& wait,
                   volatile quint32& countThreads,
                   volatile bool& responseHasBeenTaken ,
                   bool senderIsClient,
                   bool waitForSend,
                   bool waitForResponse,
                   IOClient* client,
                   IOSendJob::Handle& job ) :
        m_t(t),
        m_mutex(mutex),
        m_wait(wait),
        m_countThreads(countThreads),
        m_responseHasBeenTaken(responseHasBeenTaken),
        m_senderIsClient(senderIsClient),
        m_waitForSend(waitForSend),
        m_waitForResponse(waitForResponse),
        m_client(client),
        m_job(job)
    {}

    void run ()
    {
        m_mutex.lock();
        ++m_countThreads;
        m_wait.wait( &m_mutex );
        m_mutex.unlock();

        // If wait for send
        if ( m_waitForSend ) {
            IOSendJob::Result res;
            if ( m_senderIsClient ) {
                res = m_client->waitForSend( m_job );
                MY_INT_QVERIFY(res, res == IOSendJob::Success);
                res = m_client->getSendResult( m_job );
                MY_INT_QVERIFY(res, res == IOSendJob::Success);
            }
            else {
                res = m_t->m_server->waitForSend( m_job );
                MY_INT_QVERIFY(res, res == IOSendJob::Success);
                res = m_t->m_server->getSendResult( m_job );
                MY_INT_QVERIFY(res, res == IOSendJob::Success);
            }

            MY_INT_QVERIFY(res, res == IOSendJob::Success);
        }

        // If wait for response
        if ( m_waitForResponse ) {
            IOSendJob::Result res;
            if ( m_senderIsClient )
                res = m_client->waitForResponse( m_job, MaxSleep );
            else
                res = m_t->m_server->waitForResponse( m_job, MaxSleep );

            MY_INT_QVERIFY(res, res == IOSendJob::Success);

            IOSendJob::Response resp;
            if ( m_senderIsClient )
                resp = m_client->takeResponse( m_job );
            else
                resp = m_t->m_server->takeResponse( m_job );

            m_mutex.lock();

            MY_INT_QVERIFY(resp.responseResult,
                           resp.responseResult == IOSendJob::Success ||
                           resp.responseResult == IOSendJob::NoResponse);

            // We are the first! Check response
            if ( resp.responseResult == IOSendJob::Success ) {
                QVERIFY(resp.responseResult == IOSendJob::Success);
                QVERIFY(resp.responsePackages.size() == 1);
                QVERIFY(resp.responsePackages[0]->header.type ==
                        CommunicationTest::ResponseType);
                QVERIFY(!m_responseHasBeenTaken);
                m_responseHasBeenTaken = true;
            }
            // Other thread has taken our response, check some fields
            else {
                QVERIFY(resp.responseResult == IOSendJob::NoResponse);
                QVERIFY(resp.responsePackages.size() == 0);
            }

            m_mutex.unlock();
        }

        // Finish
        m_mutex.lock();
        --m_countThreads;
        m_mutex.unlock();
    }
};

void CommunicationTest::sendBigMessageAndWaitFromThreads (
    bool senderIsClient,
    bool waitSend,
    bool waitResponse )
{
    m_clientsMutex.lock();
    m_clientsPackages.clear();
    m_clientsMutex.unlock();

    m_serverMutex.lock();
    m_serverPackages.clear();
    m_serverMutex.unlock();

    m_jobMutex.lock();
    m_jobHash.clear();
    m_jobMutex.unlock();

    m_serverResponses = 0;
    m_clientResponses = 0;

    // Get first client
    QVERIFY( m_clientList.size() > 0 );
    IOClient* client = m_clientList[0];
    Q_ASSERT( client );
    Q_ASSERT( m_clientsUuids.contains(client) );
    IOSender::Handle h = m_clientsUuids[client];

    // Create request package
    SmartPtr<IOPackage> p =
        IOPackage::createInstance( CommunicationTest::RequestType2,
                                   BuffersNumber );

    // Fill sender uuid if sender is client
    if ( senderIsClient )
        Uuid::dump( h, p->header.senderUuid );

    // Fill buffers
    for ( quint32 i = 0; i < BuffersNumber; ++i ) {
        QString uuid = Uuid::createUuid().toString();
        QByteArray data;
        while ( (quint32)data.size() < MaxBytesToSend )
            data.append(uuid);

        p->fillBuffer(i, randEncoding(), data.data(), data.size());
    }

    QMutex mutex;
    QWaitCondition wait;
    volatile quint32 startedThreads = 0;
    volatile bool sharedResponseHasBeenTakenFlag = false;

    IOSendJob::Handle job;

    QList< SmartPtr<ThreadWaiter> > threadPool;
    for ( quint32 i = 0; i < ThreadsNumber; ++i ) {
        SmartPtr<ThreadWaiter> thread(
            new ThreadWaiter( this,
                              mutex, wait, startedThreads,
                              sharedResponseHasBeenTakenFlag,
                              senderIsClient,
                              waitSend,
                              waitResponse,
                              client,
                              job ) );

        thread->start();
        threadPool.append( thread );
    }

    // Ready
    while ( startedThreads != ThreadsNumber ) {
        sleepMsecs( MinSleep );
        QCoreApplication::processEvents();
    }

    QVERIFY( startedThreads == ThreadsNumber );

    // Steady
    sleepMsecs( 2000 );

    // Go. Send package
    IOSender::Handle senderHandle;
    if ( senderIsClient ) {
        senderHandle = client->senderHandle();
        IOSendJob::Handle localJob = client->sendPackage( p );
        IOSendJob::Result res = client->getSendResult( localJob );
        if ( res != IOSendJob::SendQueueIsFull ) {
            MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                                res == IOSendJob::SendPended);
            job = localJob;
        }
    }
    else {
        senderHandle = m_server->senderHandle();
        IOSendJob::Handle localJob = m_server->sendPackage( h, p );
        IOSendJob::Result res = m_server->getSendResult( localJob );
        if ( res != IOSendJob::SendQueueIsFull ) {
            MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                                res == IOSendJob::SendPended);
            job = localJob;
        }
    }

    // Save job for correct response
    m_jobMutex.lock();
    m_jobHash[senderHandle] = job;
    m_jobMutex.unlock();

    // Go
    wait.wakeAll();

    // Wait for finish
    while ( startedThreads != 0 ) {
        sleepMsecs( MinSleep );
        QCoreApplication::processEvents();
    }

    // Wait for finish threads
    for ( quint32 i = 0; i < ThreadsNumber; ++i ) {
        threadPool[i]->wait();
    }

    if ( waitResponse )
        QVERIFY( sharedResponseHasBeenTakenFlag == true );

    // Clean all threads
    threadPool.clear();

    m_jobMutex.lock();
    m_jobHash.clear();
    m_jobMutex.unlock();
}

void CommunicationTest::compareJobs ()
{
    // Get first client
    QVERIFY( m_clientList.size() > 0 );
    IOClient* client = m_clientList[0];
    Q_ASSERT( client );
    Q_ASSERT( m_clientsUuids.contains(client) );
    IOSender::Handle h = m_clientsUuids[client];
    Q_ASSERT( h == client->senderHandle() );

    // Client is a sender
    {
        // Create request package
        SmartPtr<IOPackage> p =
            IOPackage::createInstance( CommunicationTest::RequestType,
                                       BuffersNumber );

        // Fill sender uuid with client handle
        Uuid::dump( h, p->header.senderUuid );

        // Fill buffers
        for ( quint32 i = 0; i < BuffersNumber; ++i ) {
            QString uuid = Uuid::createUuid().toString();
            QByteArray data;
            while ( (quint32)data.size() < MaxBytesToSend / 1024 )
                data.append(uuid);

            p->fillBuffer(i, randEncoding(), data.data(), data.size());
        }

        m_jobMutex.lock();
        m_compareJobs = true;
        m_jobHash.clear();
        m_jobMutex.unlock();

        IOSendJob::Handle job = client->sendPackage( p );
        IOSendJob::Result res = client->getSendResult( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                            res == IOSendJob::SendPended);

        // Compare jobs
        QMutexLocker locker( &m_jobMutex );

        const QString& uuid = client->senderHandle();

        if ( m_jobHash.contains(uuid) ) {
            const IOSendJob::Handle& jobH = m_jobHash[uuid];
            QVERIFY( client->getJobUuid(jobH) == client->getJobUuid(job) );
        }
        else {
            m_jobHash[uuid] = job;
            m_jobWait.wait( &m_jobMutex );
        }

        m_compareJobs = false;
    }

    // Server is a sender
    {
        // Create request package
        SmartPtr<IOPackage> p =
            IOPackage::createInstance( CommunicationTest::RequestType,
                                       BuffersNumber );

        // Fill buffers
        for ( quint32 i = 0; i < BuffersNumber; ++i ) {
            QString uuid = Uuid::createUuid().toString();
            QByteArray data;
            while ( (quint32)data.size() < MaxBytesToSend / 1024 )
                data.append(uuid);

            p->fillBuffer(i, randEncoding(), data.data(), data.size());
        }

        m_jobMutex.lock();
        m_compareJobs = true;
        m_jobHash.clear();
        m_jobMutex.unlock();

        IOSendJob::Handle job = m_server->sendPackage( h, p );
        IOSendJob::Result res = m_server->getSendResult( job );
        MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                            res == IOSendJob::SendPended);

        // Compare jobs
        QMutexLocker locker( &m_jobMutex );

        const QString& uuid = m_server->senderHandle();

        if ( m_jobHash.contains(uuid) ) {
            const IOSendJob::Handle& jobH = m_jobHash[uuid];
            QVERIFY( m_server->getJobUuid(jobH) == m_server->getJobUuid(job) );
        }
        else {
            m_jobHash[uuid] = job;
            m_jobWait.wait( &m_jobMutex );
        }

        m_compareJobs = false;
    }
}

void CommunicationTest::detachAndSendDetachedClient ()
{
    // Get last client
    QVERIFY( m_clientList.size() > 0 );

    QMutexLocker locker( &m_clientsMutex );
    IOClient* client = m_clientList[ClientsNumber - 1];
    Q_ASSERT( client );
    Q_ASSERT( m_clientsUuids.contains(client) );
    IOSender::Handle h = m_clientsUuids[client];
    Q_ASSERT( h == client->senderHandle() );
    locker.unlock();

    int specificArg = 0;

#ifdef _WIN_
    specificArg = GetCurrentProcessId();
#endif

    {
        IOServer server2(
                IORoutingTableHelper::GetServerRoutingTable(PSL_LOW_SECURITY),
                IOSender::Dispatcher );
        QObject::connect( &server2,
                     SIGNAL(onPackageReceived(IOServerInterface*,
                                              IOSender::Handle,
                                              const SmartPtr<IOPackage>)),
                     SLOT(onPackageToServer(IOServerInterface*,
                                            IOSender::Handle,
                                            const SmartPtr<IOPackage>)),
                     Qt::DirectConnection );
        QObject::connect( &server2,
                     SIGNAL(onResponsePackageReceived(IOSender::Handle,
                                                      IOSendJob::Handle,
                                              const SmartPtr<IOPackage>)),
                     SLOT(onResponsePackageToServer(IOSender::Handle,
                                                    IOSendJob::Handle,
                                            const SmartPtr<IOPackage>)),
                     Qt::DirectConnection );
        QObject::connect( &server2,
                     SIGNAL(onDetachClient(IOServerInterface*,
                                                     IOSender::Handle,
                                      const IOCommunication::DetachedClient)),
                     SLOT(onDetachClientOnServer(IOServerInterface*,
                                            IOSender::Handle,
                                      const IOCommunication::DetachedClient)),
                     Qt::DirectConnection );
        QObject::connect( &server2,
                     SIGNAL(onDetachedClientReceived(IOServerInterface*,
                                                     IOSender::Handle,
                                      const SmartPtr<IOPackage>,
                                      const IOCommunication::DetachedClient)),
                     SLOT(onDetachedClientToServer(IOServerInterface*,
                                            IOSender::Handle,
                                      const SmartPtr<IOPackage>,
                                      const IOCommunication::DetachedClient)),
                     Qt::DirectConnection );


        IOSender::State st = server2.listen();
        MY_INT_QVERIFY(st, st == IOSender::Connected);

        IOCommunication::DetachedClient detachedClient;
        bool res = false;

        // Detach
        QMutexLocker locker( &m_serverMutex );
        m_detachedClients.clear();
        locker.unlock();
        res = m_server->detachClient( h, specificArg );
        QVERIFY(res);

        res = waitForDetachedClient( h, m_server, detachedClient,
                                     ClientsNumber - 1 );
        QVERIFY( res );
        QVERIFY( detachedClient.isValid() );
        QVERIFY( m_server->countClients() == ClientsNumber - 1 );

        // Attach
        res = server2.attachClient( detachedClient );
        QVERIFY( res );
        res = server2.attachClient( detachedClient );
        QVERIFY( ! res );

        for ( quint32 i = 0; server2.countClients() != 1 &&
                  i < WaitIterations; ++i ) {
            QCoreApplication::processEvents();
            sleepMsecs( MinSleep );
        }
        QVERIFY( server2.countClients() == 1 );
        QVERIFY( server2.getClientsHandles()[0] == h );

        // Send smth
        SmartPtr<IOPackage> p =
            IOPackage::createInstance( CommunicationTest::AttachDetachRequest,
                                       0 );
        IOSendJob::Handle job = client->sendPackage( p );
        IOSendJob::Result waitRes = client->waitForResponse(job, MaxSleep);
        MY_INT_QVERIFY(waitRes, waitRes == IOSendJob::Success);
        IOSendJob::Response resp = client->takeResponse(job);
        QVERIFY( resp.responseResult == IOSendJob::Success );
        QVERIFY( resp.responsePackages.size() == 1 );
        p = resp.responsePackages[0];
        QVERIFY( p->header.type == CommunicationTest::AttachDetachResponse );
        QVERIFY( p->header.buffersNumber == 1 );
        IOServer* servPtr = 0;
        ::memcpy(&servPtr, p->buffers[0].getImpl(), sizeof(servPtr));
        QVERIFY(servPtr == &server2);

        // Detach
        locker.relock();
        m_detachedClients.clear();
        locker.unlock();
        res = server2.detachClient( h, specificArg );
        QVERIFY( res );

        res = waitForDetachedClient( h, &server2, detachedClient, 0 );
        QVERIFY( res );
        QVERIFY( detachedClient.isValid() );
        QVERIFY( server2.countClients() == 0 );
        QVERIFY( client->state() == IOSender::Connected );

        // Clear all detached clients
        locker.relock();
        m_detachedClients.clear();
        locker.unlock();
    }

#ifndef _WIN_
    // Only on Unix we can close detached handles,
    // so client must be in disconnected state

    for ( quint32 i = 0; client->state() != IOSender::Disconnected &&
              i < WaitIterations; ++i ) {
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }
#else
    // On Windows because of ::WSADuplicateSocket we can't just close
    // duplicated socket, so we should explictly disconnect client

    client->disconnectClient();
#endif
    QVERIFY(client->state() == IOSender::Disconnected);
    sleepMsecs( MinSleep );

    // We should reconnect to pass other tests
    quint32 cliNumber = m_server->countClients();
    client->connectClient();
    IOSender::State state = client->waitForConnection();
    MY_INT_QVERIFY(state, state == IOSender::Connected);
    m_clientsUuids[client] = client->senderHandle();
    for ( quint32 i = 0; m_server->countClients() != cliNumber + 1 &&
              i < WaitIterations; ++i ) {
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }
    MY_INT_QVERIFY( m_server->countClients(),
                    m_server->countClients() == ClientsNumber );

    // Wait for real connection on client side
    for ( quint32 i = 0;
          i < WaitIterations && m_connectedClients != ClientsNumber;
          ++i ) {
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }
    MY_INT_QVERIFY(m_connectedClients, m_connectedClients == ClientsNumber);
}

class Helper
{
public:
    Helper ( bool& isOuterProcess,
             QHash<IOSender::Handle,IOSender::Handle>& outerClients ) :
        m_isOuterProcess(isOuterProcess),
        m_outerClients(outerClients)
    {
        m_isOuterProcess = true;
        m_outerClients.clear();
    }

    ~Helper ()
    {
        m_isOuterProcess = false;
        m_outerClients.clear();
    }

private:
    bool& m_isOuterProcess;
    QHash<IOSender::Handle,IOSender::Handle>& m_outerClients;
};

void CommunicationTest::startProcessAndSendDetachedClient ()
{
    // Should mark as outer process creation
    Helper helper( m_isOuterProcess, m_outerClients );
    // Init out-process clients to full clients number
    m_outProcessConnClients = ClientsNumber;

    MY_INT_QVERIFY( m_connectedClients, m_connectedClients == ClientsNumber );
    QVERIFY( m_clientList.size() > 0 );

    QMutexLocker locker( &m_clientsMutex );
    QList<IOClient*> clientList = m_clientList;
    locker.unlock();

    QHash<IOClient*, IOSendJob::Handle> jobList;

    // Do all detach job:
    //    1. send specific package.
    //    2. receiver must detach client
    //    3. catcher of detached client state must start process
    //    4. new process must send response
    foreach ( IOClient* client, clientList ) {
        Q_ASSERT( client );

        locker.relock();
        IOSender::Handle h = m_clientsUuids[client];
        locker.unlock();

        QVERIFY( h == client->senderHandle() );
        QVERIFY( client->state() == IOSender::Connected );

        IOCommunication::DetachedClient detachedClient;
        SmartPtr<IOPackage> p;

        // Start another process, detach client and send detached state.
        p = IOPackage::createInstance( CommunicationTest::CreateProcessRequest,
                                       1 );
        p->fillBuffer( 0, IOPackage::RawEncoding,
                       qPrintable(h), h.length() + 1);
        IOSendJob::Handle job = client->sendPackage( p );
        jobList[client] = job;
    }
    // Wait for clients disconnection
    //     (clients are now sent to the new process)
    for ( quint32 i = 0; m_server->countClients() != 0 &&
              i < WaitIterations; ++i ) {
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }
    MY_INT_QVERIFY(m_server->countClients(), m_server->countClients() == 0);

    // Wait for correct response of another process
    foreach ( IOClient* client, jobList.keys() ) {
        locker.relock();
        IOSender::Handle h = m_clientsUuids[client];
        locker.unlock();

        IOSendJob::Handle job = jobList[client];

        IOSendJob::Response resp;
        for ( quint32 i = 0; i < WaitIterations; ++i ) {
            IOSendJob::Response r = client->takeResponse(job);
            if ( r.responseResult != IOSendJob::NoResponse ) {
                resp = r;
                break;
            }
            QCoreApplication::processEvents();
            sleepMsecs( MinSleep );
        }
        MY_INT_QVERIFY(resp.responseResult,
                       resp.responseResult == IOSendJob::Success );
        MY_INT_QVERIFY(resp.responsePackages.size(),
                       resp.responsePackages.size() == 1);
        SmartPtr<IOPackage> p = resp.responsePackages[0];
        MY_INT_QVERIFY(p->header.type,
                       p->header.type ==
                             CommunicationTest::AttachDetachResponse );
        MY_INT_QVERIFY(p->header.buffersNumber, p->header.buffersNumber == 1);
        IOSender::Handle hResp = p->buffers[0].getImpl();
        QVERIFY(h == hResp);
    }

    // Wait for clients disconnection (because other process dies)
    foreach ( IOClient* client, clientList ) {
        Q_ASSERT( client );
        for ( quint32 i = 0; i < WaitIterations; ++i ) {
            if ( client->state() == IOSender::Disconnected )
                break;
            QCoreApplication::processEvents();
            sleepMsecs( MinSleep );
        }
        QVERIFY(client->state() == IOSender::Disconnected);
    }
    MY_INT_QVERIFY(m_outProcessConnClients, m_outProcessConnClients == 0);

    // We should reconnect to pass other tests
    foreach ( IOClient* client, clientList ) {
        quint32 cliNumber = m_server->countClients();
        client->connectClient();
        IOSender::State state = client->waitForConnection();
        MY_INT_QVERIFY(state, state == IOSender::Connected);
        m_clientsUuids[client] = client->senderHandle();
        for ( quint32 i = 0; m_server->countClients() != cliNumber + 1 &&
                  i < WaitIterations; ++i ) {
            QCoreApplication::processEvents();
            sleepMsecs( MinSleep );
        }
        QVERIFY( m_server->countClients() == cliNumber + 1 );
    }
    MY_INT_QVERIFY( m_server->countClients(),
                    m_server->countClients() == ClientsNumber );

    // Wait for real connection on client side
    for ( quint32 i = 0;
          i < WaitIterations && m_outProcessConnClients != ClientsNumber;
          ++i ) {
        QCoreApplication::processEvents();
        sleepMsecs( MinSleep );
    }
    MY_INT_QVERIFY(m_outProcessConnClients,
                   m_outProcessConnClients == ClientsNumber);
    // Drop the flag
    m_outProcessConnClients = 0;
}

class ThreadSender2 : public QThread
{
private:
    CommunicationTest* m_t;
    QMutex& m_mutex;
    QWaitCondition& m_wait;
    bool m_senderIsClient;
    bool m_waitForSend;

public:
    ThreadSender2 ( CommunicationTest* t,
                   QMutex& mutex,
                   QWaitCondition& wait,
                   bool senderIsClient,
                   bool waitForSend ) :
        m_t(t),
        m_mutex(mutex),
        m_wait(wait),
        m_senderIsClient(senderIsClient),
        m_waitForSend(waitForSend)
    {}

    void run ()
    {
        m_mutex.lock();

        // Get first client
        QVERIFY( m_t->m_clientList.size() > 0 );
        IOClient* client = m_t->m_clientList[ClientsNumber - 1];
        Q_ASSERT( client );
        Q_ASSERT( m_t->m_clientsUuids.contains(client) );
        IOSender::Handle h = m_t->m_clientsUuids[client];
        Q_ASSERT( m_t->m_clientsUuids[client] == client->senderHandle() );

        // Create package
        SmartPtr<IOPackage> p =
            IOPackage::createInstance( CommunicationTest::UnknownType, 1 );

        // Fill sender uuid if sender is client
        if ( m_senderIsClient )
            Uuid::dump( h, p->header.senderUuid );

        QString uuid = Uuid::createUuid().toString();
        QByteArray data;
        while ( (quint32)data.size() < MaxBytesToSend )
            data.append(uuid);

        p->fillBuffer(0, randEncoding(), data.data(), data.size());

        // Start sending

        IOSendJob::Handle cliJob, servJob;

        cliJob = client->sendPackage( p );
        Q_ASSERT(cliJob.isValid());
        if ( ! m_senderIsClient ) {
            servJob = m_t->m_server->sendPackage( h, p );
            QVERIFY(m_t->m_server->state() == IOSender::Connected);
            Q_ASSERT(servJob.isValid());
        }

        m_mutex.unlock();
        m_wait.wakeAll();

        // If wait for send
        if ( m_waitForSend ) {
            IOSendJob::Result res;

            if ( m_senderIsClient ) {
                res = client->waitForSend( cliJob );
                MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                                    res == IOSendJob::Fail);
            }

            res = client->getSendResult( cliJob );
            MY_INT_QVERIFY(res, ( ! m_senderIsClient &&
                                  res == IOSendJob::SendPended) ||
                                res == IOSendJob::Success ||
                                res == IOSendJob::Fail);

            if ( ! m_senderIsClient ) {
                res = m_t->m_server->waitForSend( servJob );
                MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                                    res == IOSendJob::Fail);
                res = m_t->m_server->getSendResult( servJob );
                MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                                    res == IOSendJob::Fail);
            }
        }
        // If wait for response
        else {
            IOSendJob::Result res;

            if ( m_senderIsClient ) {
                res = client->waitForResponse( cliJob, MaxSleep );
                MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                                    res == IOSendJob::Fail);
            }

            IOSendJob::Response resp = client->takeResponse( cliJob );
            MY_INT_QVERIFY(resp.responseResult,
                           resp.responseResult == IOSendJob::Fail ||
                           resp.responseResult == IOSendJob::NoResponse);

            if ( ! m_senderIsClient ) {
                res = m_t->m_server->waitForResponse( servJob, MaxSleep );
                MY_INT_QVERIFY(res, res == IOSendJob::Success ||
                                    res == IOSendJob::Fail);
                IOSendJob::Response resp =
                    m_t->m_server->takeResponse( servJob );
                MY_INT_QVERIFY(resp.responseResult,
                               resp.responseResult == IOSendJob::Fail ||
                               resp.responseResult == IOSendJob::NoResponse);
            }
        }
    }
};

void CommunicationTest::stopClientOrServerWhileWaitingForSendOrResponseResults (
    bool senderIsClient,
    bool waitSend )
{
    QVERIFY( m_clientList.size() > 0 );
    IOClient* client = m_clientList[ClientsNumber - 1];
    Q_ASSERT( client );

    QVERIFY( client->state() == IOSender::Connected );
    QVERIFY( m_server->state() == IOSender::Connected );

    m_clientsMutex.lock();
    m_clientsPackages.clear();
    m_clientsMutex.unlock();

    m_serverMutex.lock();
    m_serverPackages.clear();
    m_serverMutex.unlock();

    QMutex mutex;
    QWaitCondition wait;

    m_serverResponses = 0;
    m_clientResponses = 0;

    ThreadSender2 sender( this, mutex, wait, senderIsClient, waitSend );

    Q_ASSERT( client->state() == IOSender::Connected );
    Q_ASSERT( m_server->state() == IOSender::Connected );

    mutex.lock();
    sender.start();

    wait.wait( &mutex );
    mutex.unlock();

    if ( ! waitSend )
        sleepMsecs( 2000 );

    if ( senderIsClient ) {
        client->disconnectClient();

        do {
            m_clientsMutex.lock();
            quint32 connectedClients = m_connectedClients;
            m_clientsMutex.unlock();

            if ( connectedClients == ClientsNumber - 1 &&
                 client->state() == IOSender::Disconnected )
                break;

            sleepMsecs( MinSleep );
            QCoreApplication::processEvents();

        } while ( 1 );

        // Wait for disconnection on server
        sleepMsecs( 1000 );
    }
    else {
        m_server->disconnectServer();

        while ( 1 ) {
            quint32 disconnClients = 0;
            for ( uint i = 0; i < ClientsNumber; ++i ) {
                IOSender::State state = m_clientList[i]->state();
                if ( state == IOSender::Disconnected ) {
                    m_clientList[i]->disconnectClient();
                    ++disconnClients;
                }
            }

            if ( disconnClients == ClientsNumber )
                break;

            sleepMsecs( MinSleep );
            QCoreApplication::processEvents();
        }
    }

    sender.wait();

    // Do reconnect
    if ( senderIsClient ) {
        Q_ASSERT(client->state() == IOSender::Disconnected);
        client->connectClient();
        IOSender::State state = client->waitForConnection();
        MY_INT_QVERIFY(state, state == IOSender::Connected);
        m_clientsUuids[client] = client->senderHandle();

        for ( uint i = 0; i < WaitIterations; ++i ) {
            if ( m_connectedClients == ClientsNumber )
                break;
            sleepMsecs( MinSleep );
            QCoreApplication::processEvents();
        }
        MY_INT_QVERIFY(m_connectedClients, m_connectedClients == ClientsNumber);
    }
    else {
        Q_ASSERT(m_server->state() == IOSender::Disconnected);
        IOSender::State state = m_server->listen();
        if ( m_connMode == IOSender::ProxyConnectionMode )
            state = m_server->waitForProxyConnection();

        MY_INT_QVERIFY(state, state == IOSender::Connected);

        for ( uint i = 0; i < ClientsNumber; ++i ) {
            m_clientList[i]->connectClient();
            IOSender::State state = m_clientList[i]->waitForConnection();
            // Should be connected
            MY_INT_QVERIFY(state, state == IOSender::Connected);
            IOClient* client = m_clientList[i];
            m_clientsUuids[client] = client->senderHandle();
        }

        for ( uint i = 0; i < WaitIterations; ++i ) {
            if ( m_connectedClients == ClientsNumber )
                break;
            sleepMsecs( MinSleep );
            QCoreApplication::processEvents();
        }
        MY_INT_QVERIFY(m_connectedClients, m_connectedClients == ClientsNumber);
    }
}

/***** REMOTE TEST CASES *****************************************************/

void CommunicationTest::Remote_initServerAndClients ()
{
    initServerAndClients();
}

void CommunicationTest::Remote_cleanupServerAndClients ()
{
    cleanupServerAndClients();
}

void CommunicationTest::Remote_sendBigMessageFromServerToAllClients ()
{
    sendBigMessageFromServerToAllClients();
}

void CommunicationTest::Remote_sendBigMessageFromAllClientsToServer ()
{
    sendBigMessageFromAllClientsToServer();
}

void CommunicationTest::Remote_sendShortMessageFromServerToExactClients ()
{
    sendShortMessageFromServerToExactClients();
}

void CommunicationTest::Remote_checkUuids ()
{
    checkUuids();
}

void CommunicationTest::Remote_sendShortMessageFromClientToServerAndWaitForResponse_MultiThread ()
{
    sendShortMessageAndWait_MultiThread( true, false, true );
}

void CommunicationTest::Remote_sendShortMessageFromClientToServerAndWaitForSendAndResponse_MultiThread ()
{
    sendShortMessageAndWait_MultiThread( true, true, true );
}

void CommunicationTest::Remote_sendShortMessageFromServerToAllClientsAndWaitForResponse_MultiThread ()
{
    sendShortMessageAndWait_MultiThread( false, false, true );
}

void CommunicationTest::Remote_sendShortMessageFromServerToAllClientsAndWaitForSendAndResponse_MultiThread ()
{
    sendShortMessageAndWait_MultiThread( false, true, true );
}

void CommunicationTest::Remote_sendBigMessageFromClientToServerAndWaitForResponseFromThreads ()
{
    sendBigMessageAndWaitFromThreads( true, false, true );
}

void CommunicationTest::Remote_sendBigMessageFromClientToServerAndWaitForSendAndResponseFromThreads ()
{
    sendBigMessageAndWaitFromThreads( true, true, true );
}

void CommunicationTest::Remote_sendBigMessageFromServerToAllClientsAndWaitForResponseFromThreads ()
{
    sendBigMessageAndWaitFromThreads( false, false, true );
}

void CommunicationTest::Remote_sendBigMessageFromServerToAllClientsAndWaitForSendAndResponseFromThreads ()
{
    sendBigMessageAndWaitFromThreads( false, true, true );
}

void CommunicationTest::Remote_compareJobs ()
{
    compareJobs();
}

void CommunicationTest::Remote_detachAndSendDetachedClient ()
{
    detachAndSendDetachedClient();
}

void CommunicationTest::Remote_startProcessAndSendDetachedClient ()
{
    startProcessAndSendDetachedClient();
}

void CommunicationTest::Remote_stopClientWhileWaitingForSendResults ()
{
    stopClientOrServerWhileWaitingForSendOrResponseResults( true, true );
}

void CommunicationTest::Remote_stopClientWhileWaitingForResponseResults ()
{
    stopClientOrServerWhileWaitingForSendOrResponseResults( true, false );
}

void CommunicationTest::Remote_stopServerWhileWaitingForSendResults ()
{
    stopClientOrServerWhileWaitingForSendOrResponseResults( false, true );
}

void CommunicationTest::Remote_stopServerWhileWaitingForResponseResults ()
{
    stopClientOrServerWhileWaitingForSendOrResponseResults( false, false );
}

/*****************************************************************************/

int main ( int argc, char *argv[] )
{
    QCoreApplication a(argc, argv);
    if ( a.arguments().contains("test") ) {
        Receiver receiver;

        bool res = false;
#ifdef _WIN_
        //
        // Read duplicated socket STDIN
        //

        HANDLE hStdin = ::GetStdHandle(STD_INPUT_HANDLE);
        if ( hStdin == INVALID_HANDLE_VALUE )  {
            WRITE_TRACE(DBG_FATAL, "Wrong STDIN handle");
            return -1;
        }

        IOCommunication::SocketDuplicatedState dupSt;
        quint32 stateSz = 0;
        void* stateBuff = dupSt.getStateBuffer(stateSz);

        DWORD dwRead = 0;
        BOOL bSuccess = ::ReadFile( hStdin, stateBuff, stateSz, &dwRead, NULL );
        if ( ! bSuccess || dwRead != stateSz ) {
            WRITE_TRACE(DBG_FATAL, "Can't read duplicated socket from pipe '%d'!",
                        ::GetLastError());
            return -1;
        }

        IOCommunication::SocketHandle sock;
        if ( ! dupSt.toSocket(sock) ) {
            WRITE_TRACE(DBG_FATAL, "Can't create socket from duplicated one '%d'!",
                        ::WSAGetLastError());
            return -1;
        }

#else
        bool ok = false;
        QString socketH( argv[1] );
        IOCommunication::SocketHandle sock(
            new IOCommunication::SocketHandlePrivate(socketH.toInt(&ok)));
        if ( ! ok ) {
            WRITE_TRACE(DBG_FATAL, "Conversion failed!");
            return -1;
        }
#endif

        IOClient client(
                IORoutingTableHelper::GetClientRoutingTable(PSL_HIGH_SECURITY),
                IOSender::IOClient, sock );

        // Connect
        res = QObject::connect( &client,
                                SIGNAL(onDetachedClientReceived(
                                    IOClientInterface*,
                                    const SmartPtr<IOPackage>,
                                    const IOCommunication::DetachedClient )),
                                &receiver,
                                SLOT(onDetachedClientReceivedToClient(
                                    IOClientInterface*,
                                    const SmartPtr<IOPackage>,
                                    const IOCommunication::DetachedClient )),
                                Qt::DirectConnection );
        if ( ! res ) {
            WRITE_TRACE(DBG_FATAL, "Error in signal connection to client (1)");
            return -1;
        }
        res = QObject::connect( &client,
                                SIGNAL(onPackageReceived(
                                    IOClientInterface*,
                                    const SmartPtr<IOPackage> )),
                                &receiver,
                                SLOT(onPackageReceivedToClient(
                                    IOClientInterface*,
                                    const SmartPtr<IOPackage> )),
                                Qt::DirectConnection );
        if ( ! res ) {
            WRITE_TRACE(DBG_FATAL, "Error in signal connection to client (2)");
            return -1;
        }

        client.connectClient( MaxSleep  );
        IOSender::State state = client.waitForConnection();
        if ( state != IOSender::Connected ) {
            WRITE_TRACE(DBG_FATAL, "Can't connect to server!");
            return -1;
        }

        IOServer server(
                IORoutingTableHelper::GetServerRoutingTable(PSL_LOW_SECURITY),
                IOSender::Vm );

        // Connect
        res = QObject::connect( &server,
                                SIGNAL(onClientAttached(
                                    IOSender::Handle,
                                    const SmartPtr<IOPackage>)),
                                &receiver,
                                SLOT(onClientAttachedToServer(
                                    IOSender::Handle,
                                    const SmartPtr<IOPackage>)),
                                Qt::DirectConnection );
        if ( ! res ) {
            WRITE_TRACE(DBG_FATAL, "Error in signal connection to server");
            return -1;
        }

        IOSender::State servState = server.listen();
        if ( servState != IOSender::Connected ) {
            WRITE_TRACE(DBG_FATAL, "Can't create server!");
            return -1;
        }

        SmartPtr<IOPackage> p =
            receiver.waitForPackage( CommunicationTest::FirstPackage );
        if ( ! p.isValid() ) {
            WRITE_TRACE(DBG_FATAL, "Receive wrong first package!");
            return -1;
        }

        p = IOPackage::createInstance(
                              CommunicationTest::GetDetachedClientRequest, 0 );
        IOSendJob::Handle job = client.sendPackage( p );
        IOSendJob::Result jRes = client.waitForResponse( job, MaxSleep );
        if ( jRes != IOSendJob::Success ) {
            WRITE_TRACE(DBG_FATAL, "Wait for response failed!");
            return -1;
        }
        IOSendJob::Response resp = client.takeResponse(job);
        if ( resp.responseResult != IOSendJob::Success ) {
            WRITE_TRACE(DBG_FATAL, "Response receive failed!");
            return -1;
        }
        if ( resp.responsePackages.size() != 1 ) {
            WRITE_TRACE(DBG_FATAL, "Wrong response!");
            return -1;
        }

        IOCommunication::DetachedClient detachedClient =
            receiver.getDetachedClient();
        if ( ! detachedClient.isValid() ) {
            WRITE_TRACE(DBG_FATAL, "Can't find detached client!");
            return -1;
        }

        res = server.attachClient( detachedClient );
        if ( ! res ) {
            WRITE_TRACE(DBG_FATAL, "Attach failed!");
            return -1;
        }
        SmartPtr<IOPackage> additionalPkg;
        IOSender::Handle h = receiver.waitForClientAttached( additionalPkg );
        if ( h.isEmpty() ) {
            WRITE_TRACE(DBG_FATAL, "Wait or attach failed!");
            return -1;
        }
        if ( additionalPkg->header.type !=
             CommunicationTest::CreateProcessRequest ||
             additionalPkg->header.buffersNumber != 1 ) {
            WRITE_TRACE(DBG_FATAL, "Wrong response package! (server)");
            return -1;
        }

        p = IOPackage::createInstance( CommunicationTest::AttachDetachRequest,
                                       0 );
        job = server.sendPackage( h, p );
        jRes = server.waitForResponse( job, MaxSleep );
        if ( jRes != IOSendJob::Success ) {
            WRITE_TRACE(DBG_FATAL, "Wait for response failed! (server)");
            return -1;
        }
        resp = client.takeResponse(job);
        if ( resp.responseResult != IOSendJob::Success ) {
            WRITE_TRACE(DBG_FATAL, "Response receive failed! (server)");
            return -1;
        }
        if ( resp.responsePackages.size() != 1 ) {
            WRITE_TRACE(DBG_FATAL, "Wrong response! (server)");
            return -1;
        }

        p = resp.responsePackages[0];

        if ( p->header.type != CommunicationTest::CreateProcessRequest ||
             p->header.buffersNumber != 1 ) {
            WRITE_TRACE(DBG_FATAL, "Wrong response package! (server)");
            return -1;
        }
        IOSender::Handle hReq = p->buffers[0].getImpl();
        if ( h != hReq ) {
            WRITE_TRACE(DBG_FATAL, "Wrong response package handle! (server)");
            return -1;
        }

        p = IOPackage::createInstance( CommunicationTest::AttachDetachResponse,
                                       1, p );
        p->fillBuffer( 0, IOPackage::RawEncoding,
                       qPrintable(h), h.length() + 1);
        job = server.sendPackage( h, p );
        res = server.waitForSend( job );
        if ( res != IOSendJob::Success ) {
            WRITE_TRACE(DBG_FATAL, "Can't send!");
            return -1;
        }

        // Hurray!! Success!!

        return 0;
    }
    else {
        IOSender::ConnectionMode connMode = IOSender::DirectConnectionMode;
        if ( a.arguments().contains("proxy") )
            connMode = IOSender::ProxyConnectionMode;

        CommunicationTest test( connMode );
        return QTest::qExec(&test, 1, argv);
    }
}

/*****************************************************************************/
#include "CommunicationTest.moc"
