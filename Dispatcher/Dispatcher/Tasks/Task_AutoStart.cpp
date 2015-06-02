///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_AutoStart.cpp
///
/// Dispatcher task for
///
/// @author sergeyt
/// @owner sergeym
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

#include "Task_AutoStart.h"
#include "Task_CommonHeaders.h"
#include "CDspStarter.h"
#include "CProtoSerializer.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/Std/PrlTime.h"
#include "CDspService.h"
#include "CDspVmManager.h"
#include "CDspVmSuspendHelper.h"

#ifdef _CT_
#include "CDspVzHelper.h"
#include "Dispatcher/Tasks/Task_VzManager.h"
#endif

using namespace Parallels;

Task_AutoStart::Task_AutoStart(SmartPtr<CDspClient> &pUser,
        const SmartPtr<IOPackage> &pRequestPkg)
: CDspTaskHelper(pUser, pRequestPkg)
{}

PRL_RESULT Task_AutoStart::startVms()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	QHash< SmartPtr<CVmConfiguration>, QString > vmList, fastRebootVmList;
	QSet<CVmIdent> setRunVms;
	bool bFastReboot = CDspService::instance()->getDispConfigGuard()
		.getDispCommonPrefs()->getFastRebootPreferences()->isFastReboot();

	if ( bFastReboot )
	{
		//clear flag FastReboot in dispatcher config
		CDspLockedPointer<CDispCommonPreferences> pCommonPrefs =
					CDspService::instance()->getDispConfigGuard().getDispCommonPrefs();
		pCommonPrefs->getFastRebootPreferences()->setFastReboot(false);
		CDspService::instance()->getDispConfigGuard().saveConfig();
	}


	QMultiHash< QString, SmartPtr<CVmConfiguration> > vmHash = CDspService::instance()->getVmDirHelper().getAllVmList();
	foreach ( QString vmDirUuid, vmHash.uniqueKeys() )
	{
		foreach( SmartPtr<CVmConfiguration> pVmConfig, vmHash.values( vmDirUuid ) )
		{
			PRL_ASSERT( pVmConfig );
			if( !pVmConfig )
				continue;
			if (PRL_FAILED(pVmConfig->getValidRc()))
				continue;
			if (pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
				continue;

			if ( ! CDspService::instance()->getVmDirHelper().loadSecureData(pVmConfig) )
				continue;

			QString vmUuid = pVmConfig->getVmIdentification()->getVmUuid();
			QString vmName = pVmConfig->getVmIdentification()->getVmName();

			PRL_VM_AUTOSTART_OPTION ctAutoStart = pVmConfig->getVmSettings()->getVmStartupOptions()->getAutoStart();
			unsigned int delay = pVmConfig->getVmSettings()->getVmStartupOptions()->getAutoStartDelay();
			bool bVmFastReboot = pVmConfig->getVmSettings()->getVmStartupOptions()->isFastReboot();

			WRITE_TRACE(DBG_FATAL, "AUTOSTART_OPTION %s %s: AutoStart %d : Fast Reboot %d: Delay %u",
				QSTR2UTF8(vmUuid), QSTR2UTF8(vmName), ctAutoStart, bVmFastReboot, delay);

			bool bStartVm = false;

			CDspLockedPointer<CVmDirectoryItem> pVmDirItem =
				CDspService::instance()->getVmDirManager().getVmDirItemByUuid( vmDirUuid, vmUuid );
			if ( !pVmDirItem )
			{
				WRITE_TRACE(DBG_FATAL, "Error getDirectoryItem  %s %s", QSTR2UTF8(vmDirUuid), QSTR2UTF8(vmUuid) );
				continue;
			}

			if ( PAO_VM_START_ON_LOAD == ctAutoStart )
				bStartVm = true;
			else if ( PAO_VM_START_ON_RELOAD == ctAutoStart )
			{
				bStartVm = pVmDirItem->isLastRunningState();
				WRITE_TRACE(DBG_FATAL, "isLastRunningState %s %s: %d", QSTR2UTF8(vmUuid), QSTR2UTF8(vmName), bStartVm );
			}
			else if (!bVmFastReboot) //PAO_VM_START_MANUAL
				continue;

			if (bStartVm || bVmFastReboot )
			{
				pVmConfig->getVmIdentification()->setHomePath(pVmDirItem->getVmHome());
				setRunVms.insert(MakeVmIdent(vmUuid, vmDirUuid));
			}

			if ( bStartVm )
				vmList[pVmConfig] = vmDirUuid;

			if ( bFastReboot && bVmFastReboot )
				fastRebootVmList[pVmConfig] = vmDirUuid;
		}
	}

	setRunVms = getVmsWithAliveProcess(setRunVms);
	if ( bFastReboot )
	{
		WRITE_TRACE(DBG_FATAL, "Restore VMs from fast reboot.");
		vmList = fastRebootVmList;
	}
	else
		WRITE_TRACE(DBG_INFO, "Restore VMs regular way.");

	PRL_UINT64 initTimeStamp = PrlGetTickCount64();
	while(!vmList.isEmpty())
	{
		foreach( SmartPtr<CVmConfiguration> pVmConfig, vmList.uniqueKeys() )
		{
			if (!bFastReboot)
			{
				PRL_UINT64 delay = pVmConfig->getVmSettings()->
						getVmStartupOptions()->getAutoStartDelay();
				if ( PrlGetTickCount64() <= initTimeStamp + delay * PrlGetTicksPerSecond() )
					continue;
			}

			QString vmUuid = pVmConfig->getVmIdentification()->getVmUuid();
			QString vmDirUuid = vmList.value( pVmConfig );

			getClient()->setVmDirectoryUuid(vmDirUuid);

			PRL_RESULT outCreateError = PRL_ERR_SUCCESS;
			bool bNew = false;
			SmartPtr<CDspClient> pRunUser;
			SmartPtr<CDspVm> pVm =
				CDspVm::CreateInstance( vmUuid, vmDirUuid, outCreateError, bNew, getClient());
			if ( pVm )
			{
				PRL_RESULT ret = CDspService::instance()->getVmDirHelper()
									.getVmStartUser( pVmConfig, pRunUser, true );
				if ( ! pRunUser || PRL_FAILED(ret) )
				{
					WRITE_TRACE(DBG_FATAL, "Cannot obtain auto start user for VM: %s. Error 0x%x (%s)",
								QSTR2UTF8(pVmConfig->getVmIdentification()->getVmName()),
								ret, PRL_RESULT_TO_STRING(ret));

					CDspVm::UnregisterVmObject(pVm);
					pVm.reset();
				}
			}

			if ( pVm && bFastReboot)
			{
				//update fast reboot before filter
				pVm->storeFastRebootState(false, pRunUser);
			}

			if ( setRunVms.contains(MakeVmIdent(vmUuid, vmDirUuid)) )
			{
				WRITE_TRACE(DBG_FATAL, "Auto start: VM \"%s\" with uuid=%s has already been started"
							" and will be reconnected !"
							, QSTR2UTF8(pVmConfig->getVmIdentification()->getVmName())
							, QSTR2UTF8(vmUuid) );

				vmList.remove( pVmConfig );
				break;
			}

			if ( pVm )
			{
				if ( bFastReboot )
				{
					QString path = pVm->prepareFastReboot(false);
					if (path.isEmpty())
					{
						//failed to mount
						WRITE_TRACE(DBG_FATAL, "Failed to mount, start VM");
					}
				}

				PVE::IDispatcherCommands cmdStartNum = PVE::DspCmdVmStart;
				CProtoCommandPtr pStartRequest = CProtoSerializer::CreateProtoBasicVmCommand( cmdStartNum, vmUuid );

				SmartPtr<IOPackage>
					pStartPackage = DispatcherPackage::createInstance(
						cmdStartNum,
						pStartRequest->GetCommand()->toString() );

				pRunUser->setVmDirectoryUuid(vmDirUuid);
				if( !pVm->start( pRunUser , pStartPackage ) && bNew )
				{
					CDspVm::UnregisterVmObject(pVm);
				}
			}

			vmList.remove( pVmConfig );
			break;

		}
		if (!bFastReboot)
			HostUtils::Sleep(1000);
	}

	return ret;
}

static SmartPtr<IOPackage> getStartPkg(SmartPtr<CVmConfiguration> &pConfig)
{
	return DispatcherPackage::createInstance(
			PVE::DspCmdVmStartEx,
			CProtoSerializer::CreateVmStartExProtoCommand(
				pConfig->getVmIdentification()->getVmUuid(), 0, 0));
}

static SmartPtr<IOPackage> getResumePkg(SmartPtr<CVmConfiguration> &pConfig)
{
	 return DispatcherPackage::createInstance(
			 PVE::DspCmdVmResume,
			 CProtoSerializer::CreateProtoBasicVmCommand(
				 PVE::DspCmdVmResume,
				 pConfig->getVmIdentification()->getVmUuid(),
				 PNSF_CT_SKIP_ARPDETECT));
}

PRL_RESULT Task_AutoStart::runTask(SmartPtr<IOPackage> p)
{
	SmartPtr<CDspTaskHelper> pTask(new Task_VzManager(getClient(), p));
	return runExternalTask(pTask.getImpl());
}

bool sortByBootOrderPrio(const SmartPtr<CVmConfiguration> &s1, const SmartPtr<CVmConfiguration> &s2)
{
	return s1->getVmSettings()->getVmStartupOptions()->getBootOrderPrio() >
		s2->getVmSettings()->getVmStartupOptions()->getBootOrderPrio();
}

void Task_AutoStart::startCts()
{
#ifdef _CT_
	QList<SmartPtr<CVmConfiguration> > configs, startList, resumeList;

	CDspService::instance()->getVzHelper()->getCtConfigList(getClient(), 0, configs);

	qSort(configs.begin(), configs.end(), sortByBootOrderPrio);

	resumeList = CDspService::instance()->getVzHelper()->getAutoResumeCtList(configs);

	SmartPtr<CDspVmSuspendMounter> pSuspendMounter;

	if (!resumeList.empty())
	{
		/* PRAM is shared resource between VM & CT
		 * so it needed to synchronize PRAM destroy,
		 */
		pSuspendMounter = CDspService::instance()->
			getVmManager().getSuspendHelper()->prepareCtForResume();

		foreach(SmartPtr<CVmConfiguration> pConfig, resumeList)
		{
			WRITE_TRACE(DBG_FATAL, "Auto resume CT %s",
					QSTR2UTF8(pConfig->getVmIdentification()->getVmUuid()));
			/* CT will started in case resume failure */
			if (PRL_FAILED(runTask(getResumePkg(pConfig))))
				startList += pConfig;

			/* Destroy suspend in PRAM mark */
			QFile f(CDspVzHelper::getVzrebootMarkFile(pConfig));
			f.remove();
		}
	}
	else
	{
		foreach(SmartPtr<CVmConfiguration> pConfig, configs)
			if (pConfig->getVmSettings()->getVmStartupOptions()->getAutoStart() != PAO_VM_START_MANUAL)
				startList += pConfig;
	}

	foreach(SmartPtr<CVmConfiguration> pConfig, startList)
	{
		WRITE_TRACE(DBG_FATAL, "Auto start CT %s",
				QSTR2UTF8(pConfig->getVmIdentification()->getVmUuid()));

		runTask(getStartPkg(pConfig));
	}
#endif
}

PRL_RESULT Task_AutoStart::run_body()
{
	PRL_RESULT res;

	res = startVms();
	startCts();

	CDspService::instance()->getVmManager().getSuspendHelper()->freeFastRebootResources();

	return res;
}
