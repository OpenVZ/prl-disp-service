///////////////////////////////////////////////////////////////////////////////
///
/// @file IOSendJob.cpp
///
/// Special class to handle send results, or response results.
///
/// @author romanp
/// @owner sergeym
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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

#include "IOSendJob.h"
#include "Libraries/Logging/Logging.h"

using namespace IOService;

/*****************************************************************************/

bool IOSendJob::Handle::isValid () const
{
    return m_job.isValid();
}

IOSendJob::Handle::Handle ()
{}

IOSendJob::Handle::Handle ( const SmartPtr<IOSendJob>& job ) :
    m_job(job)
{}

IOSendJob::Response::Response () :
    responseResult(IOSendJob::NoResponse)
{}

IOSendJob::Handle IOSendJob::InvalidHandle;

/*****************************************************************************/

IOSendJob::IOSendJob () :
    m_isSendUrgentlyWaked(false),
    m_isResponseUrgentlyWaked(false),
    m_sendResult(SendPended),
    m_sendWaitingsNum(0),
    m_responseWaitingsNum(0)
{}

IOSendJob::~IOSendJob ()
{}

void IOSendJob::reinit ()
{
    m_sendResult = SendPended;
    m_response = Response();
    m_packageUuid = Uuid();

    Q_ASSERT( m_sendWaitingsNum == 0 );
    Q_ASSERT( m_responseWaitingsNum == 0 );
}

void IOSendJob::registerPackageUuid ( const Uuid_t pkgUuid )
{
    m_packageUuid = Uuid::toUuid(pkgUuid);
}

const Uuid& IOSendJob::getPackageUuid () const
{
    return m_packageUuid;
}

bool IOSendJob::isResponsibleForPackageUuid ( const Uuid_t uuid ) const
{
    Uuid pkgUuid = Uuid::toUuid( uuid );
    if ( m_packageUuid.isNull() || pkgUuid.isNull() )
        return false;

    return m_packageUuid == pkgUuid;
}

bool IOSendJob::waitForSend ( quint32 timeout, bool& urgentlyWaked )
{
    QMutexLocker locker( &m_mutex );

    urgentlyWaked = false;

    if ( m_isSendUrgentlyWaked ) {
        m_isSendUrgentlyWaked = false;
        urgentlyWaked = true;
        return true;
    }

    if ( m_sendResult != SendPended ) {
        return true;
    }

    ++m_sendWaitingsNum;

    bool res = false;

    if ( timeout == UINT_MAX )
        res = m_sendWait.wait( &m_mutex );
    else
        res = m_sendWait.wait( &m_mutex, timeout );

    --m_sendWaitingsNum;

    if ( m_isSendUrgentlyWaked ) {
        m_isSendUrgentlyWaked = false;
        urgentlyWaked = true;
    }

    return res;
}

bool IOSendJob::waitForResponse ( quint32 timeout, bool& urgentlyWaked )
{
    QMutexLocker locker( &m_mutex );

    urgentlyWaked = false;

    if ( m_isResponseUrgentlyWaked ) {
        m_isResponseUrgentlyWaked = false;
        urgentlyWaked = true;
        return true;
    }

    if ( m_response.responseResult != NoResponse ) {
        return true;
    }

    ++m_responseWaitingsNum;

    bool res = false;

    if ( timeout == UINT_MAX )
        res = m_responseWait.wait( &m_mutex );
    else
        res = m_responseWait.wait( &m_mutex, timeout );

    --m_responseWaitingsNum;

    if ( m_isResponseUrgentlyWaked ) {
        m_isResponseUrgentlyWaked = false;
        urgentlyWaked = true;
    }

    return res;
}

void IOSendJob::wakeSendWaitings ( IOSendJob::Result res )
{
    Q_ASSERT(res != SendPended);

    QMutexLocker locker( &m_mutex );
    // Urgently waking
    if ( res == UrgentlyWaked )
        m_isSendUrgentlyWaked = true;
    // Generic result set
    else
        m_sendResult = res;
    m_sendWait.wakeAll();
}

void IOSendJob::wakeResponseWaitings ( IOSendJob::Result res,
                                       const IOSender::Handle& h,
                                       const SmartPtr<IOPackage>& p )
{
    Q_ASSERT(res != NoResponse);

    QMutexLocker locker( &m_mutex );
    // Urgently waking
    if ( res == UrgentlyWaked )
        m_isResponseUrgentlyWaked = true;
    // Generic result set
    else {
        m_response.responseResult = res;
        m_response.senderHandle = h;
        m_response.responsePackages.append( p );
    }
    m_responseWait.wakeAll();
}

IOSendJob::Result IOSendJob::getSendResult () const
{
    QMutexLocker locker( &m_mutex );
    return m_sendResult;
}

IOSendJob::Response IOSendJob::takeResponse ()
{
    Response response;

    QMutexLocker locker( &m_mutex );
    // Make a copy
    response = m_response;
    // Clean all responses
    m_response.responsePackages.clear();
    // Again no responses
    m_response.responseResult = NoResponse;

    return response;
}

bool IOSendJob::clearResponse ()
{
    QMutexLocker locker( &m_mutex );
    // Clean all responses
    m_response.responsePackages.clear();
    // Again no responses
    m_response.responseResult = NoResponse;

    return true;
}

bool IOSendJob::sendWaitingsWereWaken () const
{
    QMutexLocker locker( &m_mutex );
    return (m_sendResult != SendPended);
}

bool IOSendJob::responseWaitingsWereWaken () const
{
    QMutexLocker locker( &m_mutex );
    return (m_response.responseResult != NoResponse);
}

quint32 IOSendJob::getSendWaitingsNumber () const
{
    return m_sendWaitingsNum;
}

quint32 IOSendJob::getResponseWaitingsNumber () const
{
    return m_responseWaitingsNum;
}

/*****************************************************************************/

IOJobManager::Job::Job () :
    sendJob( new IOSendJob ),
    isActive(false)
{
    ::memset( &pkgHeader, 0, sizeof(pkgHeader) );
}

/*****************************************************************************/

IOJobManager::JobPool::JobPool ( const Uuid& u ) :
    uuid(u),
    activeJobsSize(),
    heartBeatJob( new Job )
{
    jobList.reserve( IOJobManager::OptimalPoolSize );

    // Preallocate jobs
    for ( quint32 i = 0; i < IOJobManager::OptimalPoolSize; ++i )
        jobList << QSharedPointer<Job>(new Job);

    // Init heartbeat job
    SmartPtr<IOPackage> heartPkg =
        IOPackage::createInstance(IOCommunicationMngPackage::HeartBeat, 0);
    if ( ! heartPkg.isValid() )
        throw std::bad_alloc();

    heartPkg->header.crc16 = IOPackage::headerChecksumCRC16( heartPkg->header );
    heartBeatJob->pkgHeader = heartPkg->header;
    heartBeatJob->pkg = heartPkg;
}

/*****************************************************************************/

SmartPtr<IOJobManager::JobPool> IOJobManager::initJobPool ()
{
    Uuid uuid = Uuid::createUuid();
    SmartPtr<JobPool> jobPool;
    try { jobPool = SmartPtr<JobPool>(new JobPool(uuid)); }
    catch ( ... ) {}
    return jobPool;
}

SmartPtr<IOSendJob> IOJobManager::findJobByResponsePackage (
    const SmartPtr<JobPool>& jobPool,
    const SmartPtr<IOPackage>& p ) const
{
    Q_ASSERT( jobPool.isValid() );
    Q_ASSERT( p.isValid() );

    // Lock pool
    QReadLocker rdLocker( &jobPool->rwLock );

    foreach ( const QSharedPointer<Job>& j, jobPool->jobList) {
        const SmartPtr<IOSendJob>& job = j->sendJob;
        if ( job.countRefs() > 1 &&
             job->isResponsibleForPackageUuid(p->header.parentUuid) ) {
            return job;
        }
    }

    return SmartPtr<IOSendJob>();
}

bool IOJobManager::initActiveJob (
    SmartPtr<JobPool>& jobPool,
    const IOPackage::PODHeader& pkgHeader,
    const SmartPtr<IOPackage>& package,
    JobRefType& job,
    bool urgent )
{
    Q_ASSERT( jobPool.isValid() );

    // Lock
    QWriteLocker wrLocker( &jobPool->rwLock );

    QSharedPointer<Job> freeJob;

    qint32 freeJobsNum = -1;

    QVector<QSharedPointer<Job> >::Iterator it = jobPool->jobList.begin();
    QVector<QSharedPointer<Job> >::Iterator firstFreeJobIter = it;
    while ( it != jobPool->jobList.end() ) {
        if ( isJobFree(it->data()) ) {
            ++freeJobsNum;
            // Save first job
            if ( freeJobsNum == 0 ) {
                freeJob = *it;
                firstFreeJobIter = it;
            }
        }
        ++it;
    }

    // Can't find free jobs, so allocate one more time
    if ( freeJob.isNull() ) {
        try { freeJob = QSharedPointer<Job>(new Job); }
        catch ( ... ) {}
        if ( freeJob.isNull() ) {
            job.clear();
            return false;
        }
        jobPool->jobList.append( freeJob );
    }
    // Try to free
    else {
        // Reinit free send job
        freeJob->sendJob->reinit();

        // Remove some free elements
        if ( jobPool->jobList.size() > OptimalPoolSize ) {
            freeJobsNum = (freeJobsNum == -1 ? 0 : freeJobsNum);
            const quint32 nonFreeJobsNum =
                jobPool->jobList.size() - freeJobsNum;
            if ( OptimalNonFreePoolSize >= nonFreeJobsNum ) {
                const quint32 toBeFreed =
                    jobPool->jobList.size() - OptimalPoolSize;

                quint32 freed = 0;
                QVector<QSharedPointer<Job> >::Iterator it = firstFreeJobIter + 1;
                while ( it != jobPool->jobList.end() && freed < toBeFreed ) {
                    if ( isJobFree(it->data()) ) {
                        it = jobPool->jobList.erase(it);
                        ++freed;
                    }
                    else
                        ++it;
                }
            }
        }
    }

    // Allocation problems? Must be checked above
    Q_ASSERT(!freeJob.isNull());
    // Is not active yet
    Q_ASSERT(freeJob->isActive == false);

    // Save outer job
    job = freeJob;

    // Check max active jobs size for non urgent init
    if ( !urgent && jobPool->activeJobsSize >= m_activeJobsLimit )
        return false;

    // Change last job
    QSharedPointer<Job> h = jobPool->lastActiveJob.toStrongRef();
    if ( !h.isNull() ) {
        Q_ASSERT(h->nextJob.isNull());
        h->nextJob = freeJob;
        jobPool->lastActiveJob = freeJob;
    }
    // Init first/last ptrs
    else {
        Q_ASSERT(jobPool->lastActiveJob.isNull());
        Q_ASSERT(jobPool->firstActiveJob.isNull());

        jobPool->lastActiveJob = freeJob;
        jobPool->firstActiveJob = freeJob;
    }

    // Increase jobs size
    ++jobPool->activeJobsSize;

    // Mark job as active now
    freeJob->isActive = true;

    // Init job
    freeJob->pkgHeader = pkgHeader;
    freeJob->pkg = package;

    return true;
}

IOJobManager::JobRefType IOJobManager::getNextActiveJob (
    const SmartPtr<JobPool>& jobPool ) const
{
    Q_ASSERT( jobPool.isValid() );

    // Lock
    QReadLocker rdLocker( &jobPool->rwLock );
    return jobPool->firstActiveJob;
}

IOJobManager::JobRefType IOJobManager::getHeartBeatJob (
    const SmartPtr<JobPool>& jobPool ) const
{
    return jobPool->heartBeatJob;
}

void IOJobManager::putActiveJob ( SmartPtr<JobPool> jobPool,
                                  const JobRefType& job )
{
    Q_ASSERT( jobPool.isValid() );
    QSharedPointer<Job> h = job.toStrongRef();
    Q_ASSERT( h.data() );

    // Nothing to do if job is not active or is a fake heartbeat job
    if ( ! h->isActive || h == jobPool->heartBeatJob ) {
        return;
    }

    // Lock
    QWriteLocker wrLocker( &jobPool->rwLock );
    Q_ASSERT(jobPool->firstActiveJob == job);
    Q_ASSERT(jobPool->activeJobsSize > 0);

    // Decrease jobs size
    --jobPool->activeJobsSize;

    // Init first active job
    jobPool->firstActiveJob = h->nextJob;
    h->nextJob.clear();

    // Mark job as inactive
    h->isActive = false;

    // Free package
    h->pkg = SmartPtr<IOPackage>();

    // Last active job should be zeroed if this job is the last
    if ( jobPool->firstActiveJob.isNull() ) {
        Q_ASSERT(jobPool->activeJobsSize == 0);
        jobPool->lastActiveJob.clear();
    }

    // Debug check
    if ( jobPool->activeJobsSize == 0 ) {
        Q_ASSERT(jobPool->firstActiveJob.isNull());
        Q_ASSERT(jobPool->lastActiveJob.isNull());
    }
}

QList<IOJobManager::JobRefType> IOJobManager::getBusySendJobs (
    const SmartPtr<JobPool>& jobPool ) const
{
    Q_ASSERT( jobPool.isValid() );

    QList<JobRefType> res;

    // Lock
    QReadLocker rdLocker( &jobPool->rwLock );

    // Firstly get correctly ordered active jobs
    JobRefType w = jobPool->firstActiveJob;
    for ( QSharedPointer<Job> h;; w = h->nextJob ) {
        h = w.toStrongRef();
        if ( h.isNull() ) {
            break;
        }
        res << w;
    }

    // Secondary get all not active but externally owned jobs
    foreach ( const QSharedPointer<Job>& j, jobPool->jobList) {
        if ( ! isJobFree(j.data()) && ! j->isActive ) {
            res << j;
        }
    }

    return res;
}

bool IOJobManager::isJobFree ( const Job* job ) const
{
    Q_ASSERT(job);

    if ( job->sendJob.countRefs() > 1 || job->isActive )
        return false;

    return true;
}

/*****************************************************************************/
