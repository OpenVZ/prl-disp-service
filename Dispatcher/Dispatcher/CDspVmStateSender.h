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
///	CDspVmStateSender.h
///
/// @brief
///	Definition of the class CDspVmStateSender
///
/// @brief
///	This class implements class to send vm state in different event loop
///
/// @author sergeyt
///	Sergeyt
///
/// @date
///	2008-10-01
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspVmStateSender_H_
#define __CDspVmStateSender_H_

#include <QHash>
#include <QThread>
#include "CVmIdent.h"
#include "CDspSync.h"
#include <prlcommon/Std/SmartPtr.h>
#include <prlsdk/PrlEnums.h>

class CDspVmStateSenderThread;
class CDspVmStateSender: public QObject
{
	friend class CDspVmStateSenderThread;
	Q_OBJECT

	CDspVmStateSender();

public:
	VIRTUAL_MACHINE_STATE tell(const CVmIdent& vm_) const;

	void onVmStateChanged( VIRTUAL_MACHINE_STATE nVmOldState, VIRTUAL_MACHINE_STATE nVmNewState,
						   QString vmUuid, QString dirUuid, bool notifyVm );

	void onVmAdditionStateChanged( VIRTUAL_MACHINE_ADDITION_STATE nVmAdditionState, QString vmUuid, QString dirUuid );
	void onVmConfigChanged(QString vmDirUuid_, QString vmUuid_);
	void onVmPersonalityChanged(QString vmDirUuid_, QString vmUuid_);
	void onVmDeviceDetached(QString vmUuid_, QString device_);

signals:
	void signalVmStateChanged( unsigned int nVmOldState, unsigned int nVmNewState,
							   QString vmUuid, QString dirUuid );
	void signalSendVmStateChanged( unsigned int nVmState, QString vmUuid, QString dirUuid, bool notifyVm );
	void signalSendVmAdditionStateChanged( unsigned int nVmAdditionState, QString vmUuid, QString dirUuid );
	void signalSendVmConfigChanged(QString, QString);
	void signalSendVmPersonalityChanged(QString, QString);
	void signalVmDeviceDetached(QString vmUuid, QString device);

public slots:
	void slotSendVmStateChanged( unsigned int nVmState, QString vmUuid, QString dirUuid, bool notifyVm );
	void slotSendVmAdditionStateChanged( unsigned int nVmAdditionState, QString vmUuid, QString dirUuid );

private:
	typedef QHash<CVmIdent, VIRTUAL_MACHINE_STATE> cache_type;

	cache_type m_cache;
};


class CDspVmStateSenderThread : public QThread
{
public:
	CDspVmStateSenderThread();

	CDspLockedPointer<CDspVmStateSender> getVmStateSender();

private:
	virtual void run();

private:
	QMutex m_mtx;

	// should be created inside thread ( to process slots in this eventloop )
	CDspVmStateSender* m_pVmStateSender;
};
#endif // __CDspVmStateSender_H_
