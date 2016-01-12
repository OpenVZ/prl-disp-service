///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_SearchLostConfigs.cpp
///
/// Implementation of task that order to search VM configurations at specified places those are not registered at
/// user VM directory.
///
/// @author sandro@
/// @owner sergeym@
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

#include "Task_CommonHeaders.h"
#include "Task_SearchLostConfigs.h"
#include "CDspService.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <QDir>

#include <prlxmlmodel/Messaging/CVmEventParameter.h>

#include "Libraries/ProtoSerializer/CProtoSerializer.h"
// ConfigConverter commented out by request from CP team
//#include "Libraries/ConfigConverter/ConfigConverter.h"

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

Task_SearchLostConfigs::Task_SearchLostConfigs( SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &pRequestPkg,
	const QStringList &lstSearchDirs, bool bResultInResponse, bool bPvmDirOnly, int nDepthLimit)
	: CDspTaskHelper(pUser, pRequestPkg),
	m_lstSearchDirs(lstSearchDirs),
	m_lstFoundVms(new MapFoundVm),
	m_bResultInResponse(bResultInResponse),
	m_bPvmDirOnly(bPvmDirOnly),
	m_nDepthLimit(nDepthLimit)
{}

PRL_RESULT Task_SearchLostConfigs::getCancelResult()
{
	return PRL_ERR_SEARCH_CONFIG_OPERATION_CANCELED;
}

PRL_RESULT Task_SearchLostConfigs::prepareTask()
{
	if (!m_lstSearchDirs.size())//Specified empty search paths list: let setup it with local drives paths
	{
#ifdef _WIN_
		TCHAR buf[ MAX_PATH ];
		GetWindowsDirectory( buf, sizeof( buf ) / sizeof( buf[0] ) );
		m_lstSpecialPaths[UTF16_2QSTR( buf )] = true;

#elif defined(_LIN_)
		m_lstSpecialPaths["/bin"] = true;
		m_lstSpecialPaths["/sbin"] = true;
		m_lstSpecialPaths["/boot"] = true;
		m_lstSpecialPaths["/dev"] = true;
		m_lstSpecialPaths["/etc"] = true;
		m_lstSpecialPaths["/proc"] = true;
		// NetApp internal directory
		m_lstSpecialPaths["/.snapshot"] = true;
#endif

		QString strDefaultVmFolder;
		QString sUserProfile = CDspService::instance()->getDispConfigGuard().getDispUserByUuidAsString(getClient()->getUserSettingsUuid());
		if (!sUserProfile.isEmpty())
		{
			CDispUser userProfile( sUserProfile );
			strDefaultVmFolder = userProfile.getUserWorkspace()->getDefaultVmFolder( );
		}

		if ( strDefaultVmFolder.isEmpty() )
			strDefaultVmFolder = CDspService::instance()->getVmDirManager().getVmDirectory(getClient()->getVmDirectoryUuid())->getDefaultVmFolder();

		m_lstSpecialPaths[strDefaultVmFolder] = false;
		m_lstSearchDirs.append(strDefaultVmFolder);

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		QFileInfoList _drives = QDir::drives();
		foreach (QFileInfo _drive, _drives)
			if (_drive.isReadable())//We do not interesting with drives that can't to be readable
				m_lstSearchDirs.append(_drive.absoluteFilePath());
	}

	return (PRL_ERR_SUCCESS);
}

PRL_RESULT Task_SearchLostConfigs::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	while (m_lstSearchDirs.size())//Using non recursive dirs traverse algorithm to prevent possibility of stack overflow
	{
		if (operationIsCancelled())
			break;
		QString operatePath = m_lstSearchDirs.takeFirst();

		if (m_nDepthLimit > 0 && (
#ifdef _WIN_
			QFileInfo(operatePath).absoluteFilePath().count('\\') > m_nDepthLimit ||
#endif
			QFileInfo(operatePath).absoluteFilePath().count('/') > m_nDepthLimit))
		{
			continue;
		}

		QHash<QString, bool>::iterator it = m_lstSpecialPaths.find(operatePath);
		if (it != m_lstSpecialPaths.end())
		{
			if (it.value())
				// Skip path.
				continue;
			else
				m_lstSpecialPaths[operatePath] = true;
		}

		processSearchPath(operatePath);
	}
	return (PRL_ERR_SUCCESS);
}

void Task_SearchLostConfigs::processSearchPath(const QString &sSearchPath)
{
	QDir _dir(sSearchPath);
	if (!_dir.exists())
		return;

	//First stage: add to search dirs child subdirectories
	QFileInfoList _child_dirs = _dir.entryInfoList(QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Dirs);
	foreach(QFileInfo _child_dir_path, _child_dirs)
	{
		// #125021 to prevent infinity recursion by QT bug in QDir::entryInfoList()
		if( QFileInfo(sSearchPath) == _child_dir_path )
			continue;

		m_lstSearchDirs.append(_child_dir_path.absoluteFilePath());
	}

	if ( m_bPvmDirOnly &&
	    !(QFileInfo(sSearchPath).absoluteFilePath().endsWith(VMDIR_DEFAULT_BUNDLE_SUFFIX)))
		return;

	bool isOldConfig = false;
	//Second stage: process current dir on VM configurations
	QFileInfoList _files = _dir.entryInfoList(QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Files);
	foreach(QFileInfo _entry, _files)
	{
		if (_entry.suffix() == "pvs")
		{
			QString sVmConfigPath = _entry.absoluteFilePath();
			if (	CFileHelper::FileCanRead(sVmConfigPath, &getClient()->getAuthHelper())
				&&	CFileHelper::FileCanWrite(sVmConfigPath, &getClient()->getAuthHelper()) )
			{
				//Check whether current VM path already presents in VM catalog
				if (CDspService::instance()->getVmDirManager().getVmDirItemByHome(getClient()->getVmDirectoryUuid(), sVmConfigPath))
					continue;

				isOldConfig = false;
				QString strName;
				unsigned int osNumber = 0;
				bool isTemplate = false;
				SmartPtr<CVmConfiguration> pVmConfig(new CVmConfiguration);
// ConfigConverter commented out by request from CP team
//				SmartPtr<Parallels::ConfigFile> pOldCfgFile( new Parallels::ConfigFile(sVmConfigPath) );
//				if (pOldCfgFile->IsValid())
//				{
//					isOldConfig = true;
//					//Convert old config now
//					SmartPtr<Parallels::CConfigConverter> pConfigConverter( new Parallels::CConfigConverter );
//					pConfigConverter->ConvertConfiguration(pOldCfgFile.getImpl(), *pVmConfig.getImpl(), CDspService::instance()->getHostInfo()->data());
//					// maybe need check that old config was correctly converted?
//				}
//				else
				{
					// get config file
					PRL_RESULT code =
						CDspService::instance()->getVmConfigManager().loadConfig(pVmConfig, sVmConfigPath, getClient(), true );

					if( !IS_OPERATION_SUCCEEDED( code ) || !IS_OPERATION_SUCCEEDED( pVmConfig->m_uiRcInit ))
					{
						PRL_RESULT code = PRL_ERR_PARSE_VM_CONFIG;
						WRITE_TRACE(DBG_FATAL, "Error occurred while loads VM configuration from file with code [%#x (%s)]"
							, code
							, PRL_RESULT_TO_STRING( code ) );
						continue;
					}
				}

				SmartPtr<CVmEvent> pTmpEvent( new CVmEvent() );
				pTmpEvent->setInitRequestId( Uuid::toString(getRequestPackage()->header.uuid) );
				pTmpEvent->setEventIssuerId( pVmConfig->getVmIdentification()->getVmUuid() );

				strName = pVmConfig->getVmIdentification()->getVmName();
				osNumber = pVmConfig->getVmSettings()->getVmCommonOptions()->getOsVersion();
				isTemplate = pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate();

				pTmpEvent->addEventParameter(
					new CVmEventParameter( PVE::String, strName, EVT_PARAM_VM_NAME ) );
				pTmpEvent->addEventParameter(
					new CVmEventParameter( PVE::Boolean, QString::number(isOldConfig), EVT_PARAM_VM_OLD_CONFIG ) );
				pTmpEvent->addEventParameter(
					new CVmEventParameter( PVE::UnsignedInt, QString::number(osNumber), EVT_PARAM_VM_OS_NUMBER ) );
				pTmpEvent->addEventParameter(
					new CVmEventParameter( PVE::Boolean, QString::number(isTemplate), EVT_PARAM_VM_IS_TEMPLATE ) );
				pTmpEvent->addEventParameter(
					new CVmEventParameter( PVE::String, sVmConfigPath, EVT_PARAM_VM_CONFIG_PATH ) );

				QString strVmInfo = pTmpEvent->toString();
				if (strVmInfo.isEmpty())
					continue;

				// if one of parent directories is symlink - search vms from it target
				QFileInfo info(sVmConfigPath);
				info = QFileInfo( info.canonicalFilePath() );

				QList<QFileInfo> lstFileInfo = m_lstFoundVms->values();
				if( !lstFileInfo.contains( info ) )// such config already found skip it!
				{
					 m_lstFoundVms->insert( strVmInfo, info );
				}
				else
					continue;

				if ( m_bResultInResponse )
				{
					SmartPtr<CVmEvent> pVmFoundEvent(
							new CVmEvent(
								PET_DSP_EVT_FOUND_LOST_VM_CONFIG,
								CDspService::instance()->getDispConfigGuard().getDispConfig()->getVmServerIdentification()->getServerUuid(),
								PIE_DISPATCHER
								) );

					pVmFoundEvent->addEventParameter(new CVmEventParameter(PVE::String, strVmInfo, EVT_PARAM_VM_SEARCH_INFO));

					SmartPtr<IOPackage> p = DispatcherPackage::createInstance(PVE::DspVmEvent, pVmFoundEvent->toString(), getRequestPackage());
					getClient()->sendPackage(p);
				}
			}
		}
	}
}

void Task_SearchLostConfigs::finalizeTask()
{
	if ( m_bResultInResponse )
	{
		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), PRL_ERR_SUCCESS );
		CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );

		pResponseCmd->SetParamsList( m_lstFoundVms->keys() );

		getClient()->sendResponse( pCmd, getRequestPackage() );
	}
}
