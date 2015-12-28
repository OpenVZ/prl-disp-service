///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmAutoTaskManagerBase.cpp
///
/// The Vm AutoTask Manager is responsible for
/// automatic compress (aka compact) VM image disks
/// according to schedule
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
///////////////////////////////////////////////////////////////////////////////

#include <prlcommon/Std/PrlAssert.h>
#include "CDspService.h"
#include "CDspVmAutoTaskManagerBase.h"


CDspVmAutoTaskManagerBase::CDspVmAutoTaskManagerBase()
: m_bInitialized( false )
{
	qRegisterMetaType< CVmIdent >("CVmIdent");

	connect(this, SIGNAL(startVmTimerSignal(CVmIdent , int , int )),
					      SLOT(startVmTimer(CVmIdent , int , int )),
				Qt::QueuedConnection);
	connect(this, SIGNAL(killVmTimerSignal(int )), SLOT(killVmTimer(int )),
				Qt::QueuedConnection);
}

CDspVmAutoTaskManagerBase::~CDspVmAutoTaskManagerBase()
{
}

void CDspVmAutoTaskManagerBase::Init()
{
	WRITE_TRACE( DBG_DEBUG, "Initializing %s manager", getManagerName() );

	QMultiHash<QString , SmartPtr<CVmConfiguration> > hashAllVms
		= CDspService::instance()->getVmDirHelper().getAllVmList();

	QMultiHash<QString , SmartPtr<CVmConfiguration> >::iterator it;
	for(it = hashAllVms.begin(); it != hashAllVms.end(); ++it)
	{
		SmartPtr<CVmConfiguration> pVmConfig = it.value();
		PRL_ASSERT(pVmConfig);

		if (isEnabled(pVmConfig, true))
		{
			PRL_RESULT res = tryToRegisterVm(pVmConfig, it.key());
			if (PRL_FAILED(res))
				WRITE_TRACE(DBG_FATAL,
					"Can't register VM %s in Auto-task Manager during starting dispetcher."
					"Error: 0x%x, (%s).",
					QSTR2UTF8(pVmConfig->getVmIdentification()->getVmUuid()),
					res, PRL_RESULT_TO_STRING(res) );
		}
	}

	m_bInitialized = true;
}

void CDspVmAutoTaskManagerBase::Deinit()
{
	WRITE_TRACE( DBG_DEBUG, "Deinitializing %s manager", getManagerName() );
	m_bInitialized = false;
}

PRL_RESULT CDspVmAutoTaskManagerBase::tryToRegisterVm(const CVmIdent& vmIdent,
													  int iRecommendedNextTime)
{
	PRL_RESULT ret;
	SmartPtr<CVmConfiguration> pVmConfig = CDspService::instance()->getVmDirHelper()
		.getVmConfigByUuid( vmIdent, ret );
	if( !pVmConfig )
		return ret;

	return tryToRegisterVm( pVmConfig, vmIdent.second, iRecommendedNextTime );
}

PRL_RESULT CDspVmAutoTaskManagerBase::tryToRegisterVm(const SmartPtr<CVmConfiguration>& pVmConfig,
													  const QString& qsVmDirUuid,
													  int iRecommendedNextTime)
{
	if ( ! pVmConfig )
		return PRL_ERR_FAILURE;

	CVmIdent vmIdent = MakeVmIdent(pVmConfig->getVmIdentification()->getVmUuid(), qsVmDirUuid);

	// Fake client (calling at starting dispatcher from Init())
	SmartPtr<CDspClient> pClient( new CDspClient(IOSender::Handle()) );
	pClient->getAuthHelper().AuthUserBySelfProcessOwner();
	pClient->setVmDirectoryUuid(vmIdent.second);

	PRL_RESULT res = prepareRegistration( vmIdent, pClient );
	if ( PRL_FAILED( res ) )
		return res;

	if ( !isEnabled( pVmConfig, false ) )
		return unregisterVm(vmIdent);

// Check period
	int iPeriod;
	res = getPeriod( pVmConfig, iPeriod );
	if ( PRL_FAILED( res ) )
		return res;

// Register VM and start timer
	{
		QMutexLocker locker(&m_lockVmIdents);

		if (m_mapVmIdents.contains(vmIdent))
		{
			TimerInfo timerInfo = m_mapVmIdents.value(vmIdent);

			if ( timerInfo.bAutoTask )
			{
				int iOldPeriod = timerInfo.iPeriod;
				if (iOldPeriod && iOldPeriod != iPeriod)
				{
					emit killVmTimerSignal(timerInfo.iTimerId);

					m_mapVmIdents.remove(vmIdent);
				}
				else
				{
					return PRL_ERR_SUCCESS;
				}
			}
		}
	}

// Check timestamp

	QDateTime dtNow = QDateTime::currentDateTime();

	QDateTime dtTimestamp;
	{
		CDspLockedPointer<QSettings> pQSettings = CDspService::instance()->getQSettings();

		QString qsGroup = getSettingsGroupKey(vmIdent);
		pQSettings->beginGroup(qsGroup);

		QString qsKeyTimeStamp = getSettingsKeyTimestamp();
		dtTimestamp = pQSettings->value(qsKeyTimeStamp, QVariant(dtNow)).toDateTime();

		if ( ! pQSettings->contains(qsKeyTimeStamp) )
			pQSettings->setValue(qsKeyTimeStamp, QVariant(dtNow));

		pQSettings->endGroup();
	}

	int iOriginalPeriod = iPeriod;

	int iSkippedInterval = dtTimestamp.secsTo( dtNow );
	if ( iSkippedInterval > 0 )
	{
		iPeriod = (iSkippedInterval > iPeriod ? 0 : iPeriod - iSkippedInterval);
	}
	if ( iRecommendedNextTime )
		iPeriod = qMin(iPeriod, iRecommendedNextTime);

	WRITE_TRACE( DBG_FATAL, "%s manager: register VM %s with period %d (%d)",
				getManagerName(), QSTR2UTF8( pVmConfig->getVmIdentification()->getVmName() ),
				iPeriod, iOriginalPeriod );
	emit startVmTimerSignal(vmIdent, iPeriod, iOriginalPeriod);

	return PRL_ERR_SUCCESS;
}

void CDspVmAutoTaskManagerBase::startVmTimer(CVmIdent vmIdent, int iPeriod, int iOriginalPeriod)
{
	QMutexLocker locker(&m_lockVmIdents);

	int iTimerId = startTimer(iPeriod * 1000);

	TimerInfo ti;
	ti.iTimerId = iTimerId;
	ti.iPeriod = iPeriod;
	ti.iOriginalPeriod = iOriginalPeriod;

	if ( m_mapVmIdents.contains(vmIdent) )
	{
		// Case is usually for non-auto task - task will auto
		ti.bTaskInUse = m_mapVmIdents.value(vmIdent).bTaskInUse;

		m_mapVmIdents.remove(vmIdent);
	}

	m_mapVmIdents.insert(vmIdent, ti);
}

PRL_RESULT CDspVmAutoTaskManagerBase::unregisterVm(const CVmIdent& vmIdent,
												   bool bEditVmConfig,
												   const SmartPtr<CDspClient>& pClient)
{
	WRITE_TRACE( DBG_DEBUG, "Unregistering VM '%s' from %s manager",
			QSTR2UTF8( vmIdent.first ), getManagerName() );

// Clear settings
	{
		CDspLockedPointer<QSettings> pQSettings = CDspService::instance()->getQSettings();

		QString qsGroup = getSettingsGroupKey(vmIdent);
		pQSettings->remove(qsGroup);
	}

	{
		QMutexLocker locker(&m_lockVmIdents);

		if ( ! m_mapVmIdents.contains(vmIdent) )
			return PRL_ERR_SUCCESS;

// Stop timer

		emit killVmTimerSignal(m_mapVmIdents.value(vmIdent).iTimerId);

// TODO: Check running task for this VM. Cancel it ?

// Unregister VM

		m_mapVmIdents.remove(vmIdent);
	}

	return finalizeUnregistration( vmIdent, pClient, bEditVmConfig );
}

void CDspVmAutoTaskManagerBase::killVmTimer(int iTimerId)
{
	killTimer(iTimerId);
}

bool CDspVmAutoTaskManagerBase::updateTaskState(const CVmIdent& vmIdent,
												bool bInUse,
												bool bAutoTask,
												PRL_RESULT ret,
												int iRecommendedNextTime)
{
	bool bRestartTimer = false;

	TimerInfo timerInfo;
	{
		QMutexLocker locker(&m_lockVmIdents);

		QMap<CVmIdent, TimerInfo>::iterator it = m_mapVmIdents.find( vmIdent );
		if ( it != m_mapVmIdents.constEnd() )
		{
			if ( bInUse && it.value().bTaskInUse )
				return false;

			it.value().bTaskInUse = bInUse;

			timerInfo = it.value();
			if ( ! bInUse && ! timerInfo.bAutoTask )
			{
				m_mapVmIdents.remove(vmIdent);
			}
			else
			{
				int iPeriod = timerInfo.iOriginalPeriod;
				if ( PRL_FAILED(ret) )
					iPeriod /= 4;

				if ( iRecommendedNextTime > 0 && iRecommendedNextTime < iPeriod )
					iPeriod = iRecommendedNextTime;

				bRestartTimer = ( iPeriod != timerInfo.iPeriod );
				timerInfo.iPeriod = iPeriod;
			}
		}
		else if (bInUse && ! bAutoTask)
		{
			timerInfo.bTaskInUse = bInUse;
			timerInfo.bAutoTask = bAutoTask;

			m_mapVmIdents.insert(vmIdent, timerInfo);
		}

		if ( ! bRestartTimer )
			return true;

		emit killVmTimerSignal(timerInfo.iTimerId);

		m_mapVmIdents.remove(vmIdent);
	}

	emit startVmTimerSignal(vmIdent, timerInfo.iPeriod, timerInfo.iOriginalPeriod);

	return true;
}

void CDspVmAutoTaskManagerBase::timerEvent(QTimerEvent* te)
{
	if (!IsInitialized())
	{
		WRITE_TRACE( DBG_WARNING, "Timer event on unitialized %s manager", getManagerName() );
		return;
	}

// Looking for proper VM ident.

	bool bRestartTimer = false;
	TimerInfo timerInfo;
	CVmIdent vmIdent;
	{
		QMutexLocker locker(&m_lockVmIdents);

		QMap<CVmIdent, TimerInfo >::iterator it;
		for(it = m_mapVmIdents.begin(); it != m_mapVmIdents.end(); ++it)
		{
			timerInfo = it.value();
			if (timerInfo.iTimerId == te->timerId())
				break;
		}

		if ( it == m_mapVmIdents.end() || timerInfo.bTaskInUse)
			return;

		vmIdent = it.key();

		bRestartTimer = (timerInfo.iPeriod != timerInfo.iOriginalPeriod);

		if (bRestartTimer)
		{
			emit killVmTimerSignal(timerInfo.iTimerId);

			m_mapVmIdents.remove(vmIdent);
		}
	}

	if (bRestartTimer)
	{
		emit startVmTimerSignal(vmIdent, timerInfo.iOriginalPeriod, timerInfo.iOriginalPeriod);
	}

// Store timestamp
	{
		CDspLockedPointer<QSettings> pQSettings = CDspService::instance()->getQSettings();

		QString qsGroup = getSettingsGroupKey(vmIdent);
		pQSettings->beginGroup(qsGroup);

		pQSettings->setValue(getSettingsKeyTimestamp(), QVariant(QDateTime::currentDateTime()));

		pQSettings->endGroup();
	}

// Get client

	QHash< IOSender::Handle, SmartPtr<CDspClient> > hashClients =
		CDspService::instance()->getClientManager().getSessionListByVm(
		vmIdent.second, vmIdent.first, CDspAccessManager::VmAccessRights::makeModeRWX() );
	if ( hashClients.empty() )
	{
		WRITE_TRACE(DBG_FATAL, "No clients connect to VM %s with required permissions",
								QSTR2UTF8(vmIdent.first) );
		return;
	}

// Start auto task

	if ( ! updateTaskState(vmIdent, true) )
		return;

	// Fake client
	SmartPtr<CDspClient> pClient( new CDspClient(IOSender::Handle()) );
	pClient->getAuthHelper().AuthUserBySelfProcessOwner();
	pClient->setVmDirectoryUuid(vmIdent.second);

	startTask( pClient, vmIdent );
}


PRL_RESULT CDspVmAutoTaskManagerBase::prepareRegistration( const CVmIdent& vmIdent,
														   const SmartPtr<CDspClient>& pClient )
{
	Q_UNUSED( vmIdent );
	Q_UNUSED( pClient );
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspVmAutoTaskManagerBase::finalizeUnregistration( const CVmIdent& vmIdent,
															  const SmartPtr<CDspClient>& pClient,
															  bool bEditVmConfig )
{
	Q_UNUSED( vmIdent );
	Q_UNUSED( pClient );
	Q_UNUSED( bEditVmConfig );
	return PRL_ERR_SUCCESS;
}
