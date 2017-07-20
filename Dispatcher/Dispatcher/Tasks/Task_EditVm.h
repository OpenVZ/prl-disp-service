///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_EditVm.h
///
/// Edit VM configuration
///
/// @author myakhin@
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_EditVm_H__
#define __Task_EditVm_H__


#include "CDspTaskHelper.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>


class Task_EditVm
	: public CDspTaskHelper
	, protected ProgressHelper
{
	Q_OBJECT
public:

	Task_EditVm(const SmartPtr<CDspClient>& pClient,
				const SmartPtr<IOPackage>& p);

	// return true when config was updated otherwise return false
	static bool atomicEditVmConfigByVm(const QString &sClientVmDirUuid,
		const QString& vmUuid,
		const CVmEvent& evtFromVm,
		SmartPtr<CDspClient> pUserSession
		);

	static void beginEditVm ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	/**
	* Corrects device paths in view of VM name was changed
	* @param old VM home path
	* @param new VM home path
	*/
	static void CorrectDevicePathsInVmConfig(
		SmartPtr<CVmConfiguration> pVmConfig,
		const QString &sOldVmHomePath,
		const QString &sNewVmHomePath);

	static PRL_RESULT configureVzParameters(SmartPtr<CVmConfiguration> pNewVmConfig,
			SmartPtr<CVmConfiguration> pOldVmConfig = SmartPtr<CVmConfiguration>());

	/**
	* reset network addresses from config if template
	*/
	static void resetNetworkAddressesFromVmConfig( SmartPtr<CVmConfiguration> pNewVmConfig,
					SmartPtr<CVmConfiguration> pOldVmConfig );

private:
	virtual void cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p);
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

	virtual QString  getVmUuid();

	PRL_RESULT mergeSampleConfig(SmartPtr<CVmConfiguration> pConfigNew,
				SmartPtr<CVmConfiguration> pConfigOld);

	PRL_RESULT editVm();
	//
	// Prepare applying new config to a running VM.
	void applyVmConfig(SmartPtr<CVmConfiguration> old_, SmartPtr<CVmConfiguration> new_);

	/**
	* apply new networking setting on host if adapters are changed
	*/
	static void updateNetworkSettings(const SmartPtr<CVmConfiguration> pNewVmConfig,
					const SmartPtr<CVmConfiguration> pOldVmConfig);

	// check/create serial/paralells ports output files
	/**
	* Check/create serial ports output files.
	* @param pUserSession An user creating VM config
	* @param pVmConfigNew A new VM config
	* @param pWSError A pointer to CWSError class
	* @param portType = PDE_SERIAL_PORT==portType || PDE_PARALLEL_PORT==portType
	* @return Error code
	*/
	PRL_RESULT createPortsOutputFiles(
		SmartPtr<CDspClient> pUserSession,
		SmartPtr<CVmConfiguration> pVmConfigNew,
		SmartPtr<CVmConfiguration> pVmConfigOld,
		CVmEvent  *pWSError,
		PRL_DEVICE_TYPE portType);

	// create port output file
	static PRL_RESULT createPortOutputFile( const QString& strFullPath,
											SmartPtr<CDspClient> pUser);

	static bool isNetworkRatesChanged(const CVmNetworkRates *oldRates, const CVmNetworkRates *newRates);
	PRL_RESULT renameExtDisks(const CVmConfiguration& config, const QString& qsOldDirName,
			const QString& qsNewDirName, QList<QPair<QString, QString> > renamedBundles, const QString& vmName,
			CVmEvent &error);
	void renameExtDisksRollback(QList<QPair<QString, QString> > renamedBundles);

	PRL_RESULT editFirewall(
		SmartPtr<CVmConfiguration> pVmConfigNew,
		SmartPtr<CVmConfiguration> pVmConfigOld,
		VIRTUAL_MACHINE_STATE nState,
		bool& flgExclusiveFirewallChangedWasRegistred);

	static void patchConfigOnOSChanged( SmartPtr<CVmConfiguration> pVmConfig,
										SmartPtr<CVmConfiguration> pVmConfigOld);

private:
	// internal callback

private:
	QString m_sVmUuid;
};


#endif	// __Task_EditVm_H__
