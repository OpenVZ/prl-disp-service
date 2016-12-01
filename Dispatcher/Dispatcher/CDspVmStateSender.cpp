////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2008-2015 Parallels IP Holdings GmbH
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
/// @file
///  CDspVmStateSender.cpp
///
/// @brief
///  Implementation of the class CDspVmStateSender
///
/// @brief
///  This class implements class to send vm state in different event loop
///
/// @author sergeyt
///  Sergeyt
///
/// @date
///  2008-10-01
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#define FORCE_LOGGING_PREFIX "DspVmStateSender"

#include <prlcommon/Logging/Logging.h>

#include "CDspVmStateSender.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Interfaces/ParallelsQt.h>
#include "CDspCommon.h"
#include "CDspService.h"
#include "CDspClientManager.h"

#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameter.h>
#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

CDspVmStateSender::CDspVmStateSender()
{
	bool bConnected = connect( this
		, SIGNAL( signalSendVmStateChanged( unsigned int, QString ,QString, bool ) )
		, this
		, SLOT( slotSendVmStateChanged( unsigned int, QString, QString, bool ) )
		, Qt::QueuedConnection
		);
	PRL_ASSERT(bConnected);

	bConnected = connect( this
		, SIGNAL( signalSendVmAdditionStateChanged( unsigned int, QString ,QString ) )
		, this
		, SLOT( slotSendVmAdditionStateChanged( unsigned int, QString, QString ) )
		, Qt::QueuedConnection
		);
	PRL_ASSERT(bConnected);

	Q_UNUSED(bConnected);
}

VIRTUAL_MACHINE_STATE CDspVmStateSender::tell(const CVmIdent& vm_) const
{
	cache_type::const_iterator p = m_cache.find(vm_);
	return m_cache.end() == p ? VMS_UNKNOWN : p.value();
}

void CDspVmStateSender::onVmStateChanged( VIRTUAL_MACHINE_STATE nVmOldState,
								VIRTUAL_MACHINE_STATE nVmNewState,
								QString vmUuid, QString dirUuid, bool notifyVm )
{
	WRITE_TRACE( DBG_DEBUG, "%s: prev = %s, state = %s, vm_uuid=%s"
		, __FUNCTION__
		, PRL_VM_STATE_TO_STRING(nVmOldState)
		, PRL_VM_STATE_TO_STRING(nVmNewState)
		, QSTR2UTF8(vmUuid)
		);

	emit signalVmStateChanged( nVmOldState, nVmNewState, vmUuid, dirUuid );
	emit signalSendVmStateChanged( nVmNewState, vmUuid, dirUuid, notifyVm );
}

void CDspVmStateSender::onVmAdditionStateChanged( VIRTUAL_MACHINE_ADDITION_STATE nVmAdditionState,
								QString vmUuid,
								QString dirUuid )
{
	WRITE_TRACE( DBG_DEBUG, "%s: addition state = %X, vm_uuid=%s"
		, __FUNCTION__
		, nVmAdditionState
		, QSTR2UTF8(vmUuid)
		);

	emit signalSendVmAdditionStateChanged( nVmAdditionState, vmUuid, dirUuid );
}

void CDspVmStateSender::onVmConfigChanged(QString vmDirUuid_, QString vmUuid_)
{
	emit signalSendVmConfigChanged(vmDirUuid_, vmUuid_);
}

void CDspVmStateSender::onVmPersonalityChanged(QString vmDirUuid_, QString vmUuid_)
{
	emit signalSendVmPersonalityChanged(vmDirUuid_, vmUuid_);
}

void CDspVmStateSender::onVmDeviceDetached(QString vmUuid_, QString device_)
{
	emit signalVmDeviceDetached(vmUuid_, device_);
}

void CDspVmStateSender::onVmCreated
	(const QString& directory_, const QString& uuid_)
{
	emit signalVmCreated(directory_, uuid_);
}

void CDspVmStateSender::onVmRegistered
	(const QString& directory_, const QString& uuid_, const QString& name_)
{
	emit signalVmRegistered(directory_, uuid_, name_);
}

CDspVmStateSenderThread::CDspVmStateSenderThread(CDspVmStateSender* ready_)
:	m_mtx( QMutex::Recursive), m_pVmStateSender(), m_bin(ready_)
{
	if (!m_bin.isNull())
		m_bin->moveToThread(this);
}

CDspLockedPointer<CDspVmStateSender> CDspVmStateSenderThread::getVmStateSender()
{
	LOG_MESSAGE( DBG_DEBUG, "%s", __FUNCTION__ );

	return CDspLockedPointer<CDspVmStateSender>( &m_mtx, m_pVmStateSender);
}

void CDspVmStateSenderThread::run()
{
	m_mtx.lock();
	m_pVmStateSender = m_bin.data();
	m_mtx.unlock();

	COMMON_TRY
	{
		exec();
	}
	COMMON_CATCH;

	m_mtx.lock();
	m_pVmStateSender = NULL;
	m_mtx.unlock();
}

void CDspVmStateSender::slotSendVmStateChanged(
	unsigned int nVmState,
	QString vmUuid,
	QString dirUuid,
	bool)
{
	WRITE_TRACE( DBG_DEBUG, "%s: state = %s, vm_uuid=%s"
		, __FUNCTION__
		, PRL_VM_STATE_TO_STRING(nVmState)
		, QSTR2UTF8(vmUuid)
		);

	if (VMS_UNKNOWN == nVmState)
		m_cache.remove(MakeVmIdent(vmUuid, dirUuid));
	else
		m_cache[MakeVmIdent(vmUuid, dirUuid)] = (VIRTUAL_MACHINE_STATE)nVmState;

	// Send to all VM client new VM state
	CVmEvent event( PET_DSP_EVT_VM_STATE_CHANGED, vmUuid , PIE_DISPATCHER );

	event.addEventParameter( new CVmEventParameter (PVE::Integer
		, QString("%1").arg((int)nVmState)
		, EVT_PARAM_VMINFO_VM_STATE ) );

	SmartPtr<IOPackage> pUpdateVmStatePkg = DispatcherPackage::createInstance( PVE::DspVmEvent, event );

	CDspService::instance()->getClientManager()
		.sendPackageToVmClients(
			pUpdateVmStatePkg,
			dirUuid,
			vmUuid
			);
}

void CDspVmStateSender::slotSendVmAdditionStateChanged( unsigned int nVmAdditionState,
													   QString vmUuid,
													   QString dirUuid )
{
	WRITE_TRACE( DBG_DEBUG, "%s: state = %X, vm_uuid=%s"
		, __FUNCTION__
		, nVmAdditionState
		, QSTR2UTF8(vmUuid)
		);

	// Send to all VM client new VM state
	CVmEvent event( PET_DSP_EVT_VM_ADDITION_STATE_CHANGED, vmUuid , PIE_DISPATCHER );

	event.addEventParameter( new CVmEventParameter (PVE::Integer
		, QString("%1").arg((int)nVmAdditionState)
		, EVT_PARAM_VMINFO_VM_ADDITION_STATE ) );

	SmartPtr<IOPackage> pUpdateVmAdditionStatePkg = DispatcherPackage::createInstance( PVE::DspVmEvent, event );

	CDspService::instance()->getClientManager()
		.sendPackageToVmClients(
		pUpdateVmAdditionStatePkg,
		dirUuid,
		vmUuid
		);
}

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Proclamation

void Proclamation::reactRegistered(QString directory_, QString uuid_, QString name_)
{
	// Generate "VM added" event
	CVmEvent e(PET_DSP_EVT_VM_ADDED, uuid_, PIE_DISPATCHER);

	if (!name_.isEmpty())
	{
		e.addEventParameter(
			new CVmEventParameter(PVE::String, name_, EVT_PARAM_GENERATED_VM_NAME));
	}
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, e);
	m_driver->sendPackageToVmClients(p, directory_, uuid_);
}

} // namespace Vm

