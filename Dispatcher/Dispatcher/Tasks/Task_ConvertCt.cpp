//////////////////////////////////////////////////////////////////////////////
///
/// @file Task_ConvertCt.cpp
///
/// Copyright (c) 2005-2024, Parallels International GmbH
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
//////////////////////////////////////////////////////////////////////////////

#include <prlcommon/Interfaces/VirtuozzoQt.h>
#include <prlcommon/Interfaces/VirtuozzoNamespace.h>
#include <prlsdk/PrlErrorsValues.h>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Messaging/CVmEventParameterList.h>

#include "CDspService.h"
#include "CDspLibvirt.h"
#include "Task_ConvertCt.h"
#include "Task_RegisterVm.h"
#include "CDspVmDirHelper.h"
#include "Libraries/Virtuozzo/CVzHelper.h"

///////////////////////////////////////////////////////////////////////////////
// class Task_ConvertCt

Task_ConvertCt::Task_ConvertCt(const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p,
		const SmartPtr<CVmConfiguration>& pCtConfig,
		Registry::Public& registry_) :
		CDspTaskHelper(client, p),
		mpCtConfig(pCtConfig),
		mCtDirUuid(getClient()->getVmDirectoryUuid()),
		mCtHome(mpCtConfig->getVmIdentification()->getHomePath()),
		mCtConf(mCtHome + QDir::separator() + VZ_CT_CONFIG_FILE),
		mCtPvsConf(mCtHome + QDir::separator() + VZ_CT_XML_CONFIG_FILE),
		mpVzHelper(new CVzOperationHelper()),
		mRegistry(registry_)
{
	CProtoVmMigrateCommand *pCmd = CProtoSerializer::
				CastToProtoCommand<CProtoVmMigrateCommand>(cmd);
	PRL_ASSERT(pCmd->IsValid());
	mCtUuid = pCmd->GetVmUuid();

	//prepare folder for new VM
	QString strDefaultVmFolder;
	QString sUserProfile = CDspService::instance()->getDispConfigGuard().getDispUserByUuidAsString(getClient()->getUserSettingsUuid());
	if (!sUserProfile.isEmpty())
	{
		CDispUser userProfile( sUserProfile );
		strDefaultVmFolder = userProfile.getUserWorkspace()->getDefaultVmFolder( );
	}
	if (strDefaultVmFolder.isEmpty())
	{
		strDefaultVmFolder = CDspService::instance()->getVmDirManager().getVmDirectory(getClient()->getVmDirectoryUuid())->getDefaultVmFolder();
	}
	if (strDefaultVmFolder.isEmpty())
	{
		strDefaultVmFolder = VirtuozzoDirs::getCommonDefaultVmCatalogue();
		WRITE_TRACE(DBG_WARNING, "Can't find default vm folder, use '%s'", QSTR2UTF8(strDefaultVmFolder));
	}

	QString ctUuid(mCtUuid);
	mNewVmDir = strDefaultVmFolder + QDir::separator() + ctUuid.remove('{').remove('}');

	WRITE_TRACE(DBG_DEBUG, "Starting convert CT [%s] to VM into '%s'",
			QSTR2UTF8(mCtUuid), QSTR2UTF8(mNewVmDir));
}

PRL_RESULT Task_ConvertCt::prepareTask()
{
	WRITE_TRACE(DBG_DEBUG, "Converting CT '%s'", QSTR2UTF8(mCtUuid));

	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - CT converting rejected!");
		return CDspTaskFailure (*this)(PRL_ERR_DISP_SHUTDOWN_IN_PROCESS);
	}

	//stop CT
	if (CVzHelper::is_env_running(mCtUuid))
	{
		WRITE_TRACE(DBG_DEBUG, "Converting CT '%s' is in running state. Stop it", QSTR2UTF8(mCtUuid));
		PRL_RESULT res = mpVzHelper->stop_env(mCtUuid, 0);
		if (PRL_FAILED(res))
		{
			WRITE_TRACE(DBG_FATAL, "Converting CT '%s': Failed to stop.", QSTR2UTF8(mCtUuid));
			return res;
		}
	}

	if (!CFileHelper::CreateDirectoryPath(mNewVmDir, &getClient()->getAuthHelper())) {
		WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(mNewVmDir));
		return CDspTaskFailure (*this)(PRL_ERR_CANT_CREATE_DIRECTORY);
	}

	//create reserve copy of current config
	if (!QFile::exists(mCtConf))
	{
		WRITE_TRACE(DBG_FATAL, "Config file '%s' not found", QSTR2UTF8(mCtConf));
		return CDspTaskFailure (*this)(PRL_ERR_FILE_NOT_FOUND);
	}
	if (!QFile::exists(mCtPvsConf))
	{
		WRITE_TRACE(DBG_FATAL, "Config xml file '%s' not found", QSTR2UTF8(mCtPvsConf));
		return CDspTaskFailure (*this)(PRL_ERR_FILE_NOT_FOUND);
	}
	QString tmpConf(mCtConf + ".ct2vm.tmp");
	if (!QFile::copy(mCtConf, tmpConf))
	{
		WRITE_TRACE(DBG_FATAL, "Can't copy config file '%s' -> '%s'", QSTR2UTF8(mCtConf), QSTR2UTF8(tmpConf));
		return CDspTaskFailure (*this)(PRL_ERR_COPY_VM_INFO_FILE);
	}
	QString tmpVeXml(mCtPvsConf + ".ct2vm.tmp");
	if (!QFile::copy(mCtPvsConf, tmpVeXml))
	{
		WRITE_TRACE(DBG_FATAL, "Can't copy config xml file '%s' -> '%s'", QSTR2UTF8(mCtPvsConf), QSTR2UTF8(tmpVeXml));
		return CDspTaskFailure (*this)(PRL_ERR_COPY_VM_INFO_FILE);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ConvertCt::convert()
{
	mpCtConfig->setVmType(PVT_VM);
	mpCtConfig->getVmIdentification()->setSourceVmUuid(mCtUuid);
	mpCtConfig->getVmIdentification()->setHomePath(mNewVmDir);

	//fix cpu count
	mpCtConfig->getVmHardwareList()->getCpu()->setCpuUnits(1);
	mpCtConfig->getVmHardwareList()->getCpu()->setNumber(1);
	mpCtConfig->getVmHardwareList()->getCpu()->setEnableHotplug(false);

	//copy all hdd
	for(const CVmHardDisk *d : mpCtConfig->getVmHardwareList()->m_lstHardDisks)
	{

		QDir hddDir(d->getSystemName());
		if (!hddDir.exists())
		{
			WRITE_TRACE(DBG_FATAL, "Disk directory does not exist '%s'", QSTR2UTF8(d->getSystemName()));
			return CDspTaskFailure(*this)(PRL_ERR_DIRECTORY_DOES_NOT_EXIST);
		}

		QString dstPath = mNewVmDir + QDir::separator() + hddDir.dirName();
		if (!CFileHelper::CreateDirectoryPath(dstPath, &getClient()->getAuthHelper()))
		{
			WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(dstPath));
			return CDspTaskFailure (*this)(PRL_ERR_CANT_CREATE_DIRECTORY);
		}

		for (QString i : hddDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
		{
			QString targetPath = dstPath + QDir::separator() + i;
			if (!CFileHelper::CreateDirectoryPath(targetPath, &getClient()->getAuthHelper()))
			{
				WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(targetPath));
				return CDspTaskFailure (*this)(PRL_ERR_CANT_CREATE_DIRECTORY);
			}
		}

		for (QString i : hddDir.entryList(QDir::Files))
		{
			QString src = d->getSystemName() + QDir::separator() + i;
			QString dst = dstPath + QDir::separator() + i;
			if (!QFile::copy(src, dst ))
			{
				WRITE_TRACE(DBG_FATAL, "Can't copy hdd disk file '%s' -> '%s'", QSTR2UTF8(src), QSTR2UTF8(dst));
				return CDspTaskFailure(*this)(PRL_ERR_COPY_VM_INFO_FILE);
			}
		}
	}

	mpCtConfig->saveToFile(mNewVmDir + QDir::separator() +"config.pvs");


	//unregister CT
	mpVzHelper->unregister_env(mpCtConfig->getVmIdentification()->getVmUuid(), 0);

	CProtoCommandPtr r =
		CProtoSerializer::CreateProtoCommandWithOneStrParam(
			PVE::DspCmdDirRegVm, mNewVmDir, false, PRVF_KEEP_OTHERS_PERMISSIONS);
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspCmdDirRegVm, r);

	// do not call checkAndLockNotExistsExclusiveVmParameters
	CVmEvent e;
	SmartPtr<CDspClient> c = getClient();
	CDspService::instance()->getTaskManager().schedule(new Task_RegisterVm(mRegistry,
		c, p, mNewVmDir, PACF_NON_INTERACTIVE_MODE, QString(), QString(),
		REG_SKIP_VM_PARAMS_LOCK)).wait().getResult(&e);


	// wait finishing thread and return task result
	PRL_RESULT output = e.getEventCode();
	if (PRL_FAILED(output))
	{
		WRITE_TRACE(DBG_FATAL,
			"Error occurred while registering Vm '%s' from CT code [%#x][%s]",
			QSTR2UTF8(mCtUuid), output, PRL_RESULT_TO_STRING(output));
		return CDspTaskFailure(*this) (PRL_ERR_REGISTER_CONVERTED_VM_FAILED);
	}

	return output;
}

PRL_RESULT Task_ConvertCt::run_body()
{
	WRITE_TRACE(DBG_DEBUG, "Converting CT '%s'", QSTR2UTF8(mCtUuid));
	if (PRL_FAILED(getLastErrorCode()))
		return getLastErrorCode();

	PRL_RESULT e = CDspService::instance()->getVmDirHelper().
		registerExclusiveVmOperation(mCtUuid,
				getClient()->getVmDirectoryUuid(),
				PVE::DspCmdCtConvert, getClient());

	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "registerExclusiveVmOperation failed. Reason: %#x (%s)",
				e, PRL_RESULT_TO_STRING(e));
		return CDspTaskFailure(*this)(PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED);
	}

	//convert config (copy current to new folder)
	e = convert();
	if (PRL_FAILED(e))
	{
		return e;
	}

	CDspService::instance()->getVmDirHelper().
		unregisterExclusiveVmOperation(mCtUuid,
				getClient()->getVmDirectoryUuid(),
				PVE::DspCmdCtConvert, getClient());

	return getLastErrorCode();
}

void Task_ConvertCt::finalizeTask()
{
	//unregister CT and remove it if it was succesfull (keep old config)
	WRITE_TRACE(DBG_DEBUG, "Converting CT '%s'", QSTR2UTF8(mCtUuid));
	if (PRL_FAILED(getLastErrorCode()))
	{
		getClient()->sendResponseError(getLastError(),
				getRequestPackage());
		//restore back config
		QString tmpConf(mCtConf + ".ct2vm.tmp");
		if (!QFile::copy(tmpConf, mCtConf))
		{
			WRITE_TRACE(DBG_FATAL, "Can't restore back config file '%s' -> '%s'", QSTR2UTF8(tmpConf), QSTR2UTF8(mCtConf));
		}
		if (!QFile::remove(tmpConf))
		{
			WRITE_TRACE(DBG_FATAL, "Can't remove temporary config file '%s'", QSTR2UTF8(tmpConf));
		}
		QString tmpVeXml(mCtPvsConf + ".ct2vm.tmp");
		if (!QFile::copy(tmpVeXml, mCtPvsConf))
		{
			WRITE_TRACE(DBG_FATAL, "Can't restore back config xml file '%s' -> '%s'", QSTR2UTF8(tmpVeXml), QSTR2UTF8(mCtPvsConf));
		}
		if (!QFile::remove(tmpVeXml))
		{
			WRITE_TRACE(DBG_FATAL, "Can't remove temporary xml config file '%s'", QSTR2UTF8(tmpVeXml));
		}
		//register back CT
		SmartPtr<CVmConfiguration> c;
		mpVzHelper->register_env(mCtHome,
			mpCtConfig->getVmIdentification()->getCtId(),
			mpCtConfig->getVmIdentification()->getVmUuid(),
			mpCtConfig->getVmIdentification()->getVmName(),
			0, c);
	}
	else
	{
		getClient()->sendSimpleResponse(getRequestPackage(), PRL_ERR_SUCCESS);
	}
}
