///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmAutoTaskManagerBase.h
///
/// The Manager is basic implementation for auto tasks
///
/// @author myakhin@
/// @modifier andreydanin@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __CDSP_VM_AUTO_TASK_MANAGER_BASE_H__
#define __CDSP_VM_AUTO_TASK_MANAGER_BASE_H__


#include <QtCore>
#include "CDspClient.h"


class CVmConfiguration;


class CDspVmAutoTaskManagerBase : public QObject
{
	Q_OBJECT

public:
	CDspVmAutoTaskManagerBase();
	virtual ~CDspVmAutoTaskManagerBase();

	virtual void Init();
	virtual void Deinit();
	bool IsInitialized() const { return m_bInitialized; }

	PRL_RESULT tryToRegisterVm( const CVmIdent& vmIdent, int iRecommendedNextTime = 0 );

	PRL_RESULT tryToRegisterVm( const SmartPtr<CVmConfiguration>& pVmConfig,
								const QString& qsVmDirUuid,
								int iRecommendedNextTime = 0 );

	PRL_RESULT unregisterVm(const CVmIdent& vmIdent,
							bool bEditVmConfig = false,
							const SmartPtr<CDspClient>& pClient = SmartPtr<CDspClient>());

	bool updateTaskState(const CVmIdent& vmIdent,
						 bool bInUse,
						 bool bAutoTask = true,	// useful only if bInUse is true and VM not registered
						 PRL_RESULT ret = PRL_ERR_SUCCESS,
						 int iRecommendedNextTime = 0 );

protected:
	virtual const char* getManagerName() const = 0;
	virtual bool isEnabled( const SmartPtr<CVmConfiguration>& pVmConfig, bool bOnDispStart ) const = 0;

	virtual PRL_RESULT prepareRegistration( const CVmIdent& vmIdent, const SmartPtr<CDspClient>& pClient );
	virtual PRL_RESULT finalizeUnregistration( const CVmIdent& vmIdent, const SmartPtr<CDspClient>& pClient,
											   bool bEditVmConfig );

	virtual PRL_RESULT getPeriod( const SmartPtr<CVmConfiguration>& pVmConfig, int& nPeriod ) const = 0;

	virtual void startTask( const SmartPtr<CDspClient>& pClient,
							const CVmIdent& vmIdent ) = 0;

	virtual QString getSettingsGroupKey( const CVmIdent& vmIdent ) const = 0;
	virtual QString getSettingsKeyTimestamp( ) const = 0;

private slots:

	void startVmTimer(CVmIdent vmIdent, int iPeriod, int iOriginalPeriod);
	void killVmTimer(int iTimerId);

signals:

	void startVmTimerSignal(CVmIdent vmIdent, int iPeriod, int iOriginalPeriod);
	void killVmTimerSignal(int iTimerId);

private:

	struct TimerInfo
	{
		TimerInfo()
			: iTimerId(-1),
			  iPeriod(0),
			  iOriginalPeriod(0),
			  bTaskInUse(false),
			  bAutoTask(true)
		{}

		int		iTimerId;
		int		iPeriod;
		int		iOriginalPeriod;
		bool	bTaskInUse;
		bool	bAutoTask;
	};

	virtual void timerEvent(QTimerEvent* );

	bool						m_bInitialized;
	QMutex						m_lockVmIdents;
	QMap<CVmIdent, TimerInfo >	m_mapVmIdents;

};


#endif	/* __CDSP_VM_AUTO_TASK_MANAGER_BASE_H__ */

