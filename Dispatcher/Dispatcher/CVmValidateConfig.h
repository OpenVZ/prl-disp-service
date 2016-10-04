////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	CVmValidateConfig.h
///
/// @brief
///	Definition of the class CVmValidateConfig
///
/// @brief
///	This class implements section validation VM configuration
///
/// @author sergeyt
///	myakhin
///
/// @date
///	2008-03-11
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _VM_VALIDATE_CONFIG_H_
#define _VM_VALIDATE_CONFIG_H_


#include <prlcommon/Std/SmartPtr.h>
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Messaging/CVmEvent.h>

#include <QSet>
#include <prlsdk/PrlEnums.h>

#include "CDspClient.h"

class CSavedStateTree;
class CDspClient;

class CVmValidateConfig
{
	struct DeviceInfo
	{
		uint m_devIdx;
		int m_devItemId;
		DeviceInfo( uint idx = 0, int id = 0 )
			:	m_devIdx(idx), m_devItemId(id) {}
	};

public:

	explicit CVmValidateConfig(SmartPtr<CDspClient> pUser, const SmartPtr<CVmConfiguration>& pVmConfig,
			const SmartPtr<CVmConfiguration>& pVmConfigOld = SmartPtr<CVmConfiguration>());

	/**
	 * Check VM configuration
	 */
	QList<PRL_RESULT > CheckVmConfig(PRL_VM_CONFIG_SECTIONS nSection);

	/**
	 * Check CT configuration
	 */
	QList<PRL_RESULT > CheckCtConfig(PRL_VM_CONFIG_SECTIONS nSection);

	typedef enum {
	    PCVAL_ALLOW_DESTROY_HDD_BUNDLE_WITH_SNAPSHOTS = 1 << 0,
	} VM_VALIDATE_FLAGS;

	/**
	 * Check critical errors
	 */
	bool HasCriticalErrors(CVmEvent& evtResult,
			PRL_UINT32 validateInternalFlags = 0);

	/**
	 * Check critical errors for start VM
	 */
	bool HasCriticalErrorsForStartVm(CVmEvent& evtResult);

	/**
	 * Add parameters with additional error info
	 */
	void AddParameters(int nResultIndex, CVmEvent& evtResult);

	/**
	 * Get parameter for error
	 */
	QString GetParameter(PRL_RESULT nResult, int nIndex = 0) const;

	/**
	 * Check on existing error
	 */
	bool HasError(PRL_RESULT nResult) const;

	/** Check VE configuration
	 *
	 */
	static void validateSectionConfig(SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	/**
	 * Check invalid symbols
	 */
	static bool HasSysNameInvalidSymbol(const QString& qsSysName);

	/**
	 * Get file name if invalid symbols present
	 */
	static QString GetFileNameFromInvalidPath(const QString& qsSysName);

	/**
	 * Lets to check whether specified generic PCI device presents at
	 * some VM configuration and whether device already used by some
	 * running VM.
	 * @param generic PCI device type
	 * @param generic PCI device system name
	 * @param optional parameter that let to specify VM uuid which should be skipped during analyze
	 * @param [out] sign whether device using by some running VM
	 * @return sign whether device using at some VM configuration
	 */
	static bool IsDeviceInAnotherVm( PRL_DEVICE_TYPE nDevType,
									 const QString& qsSysName,
									 const QString& qsVmUuidToSkip,
									 bool& bHasRunningAnotherVm);
	/**
	* Check invalid symbols in HDD's serial number
	*/
	static bool IsSerialNumberValid(const QString& qsSerial);

	/**
	* Check kvm_intel is loaded with nested=Y
	*/
	static bool IsNestedVirtEnabled();

private:

	CVmValidateConfig() {}
	CVmValidateConfig(const CVmValidateConfig& ) {}
	CVmValidateConfig& operator=(const CVmValidateConfig& ) { return *this; }

	bool getOldVmConfig();

	void CheckGeneralParameters();
	void CheckBootOption();
	void CheckRemoteDisplay();
	void CheckSharedFolders();
	void CheckCpu();
	void CheckMainMemory();
	void CheckVideoMemory();
	void CheckFloppyDisk();
	void CheckCdDvdRom();
	void CheckHardDisk();
	void CheckNetworkAdapter();
	void CheckIPDuplicates(const QSet<QString>& setNA_ids_);
	void CheckSound();
	void CheckSerialPort();
	void CheckParallelPort();
	void CheckMassStorageDevices(PRL_MASS_STORAGE_INTERFACE_TYPE type);
	void CheckIdeDevices();
	void CheckSataDevices();
	void CheckScsiDevices();
	void CheckVirtioBlockDevices();
	void CheckNetworkShapingRates();

	void CommonDevicesCheck( PRL_VM_CONFIG_SECTIONS nSection );

	void CheckDeviceIndexes( QSet< PRL_DEVICE_TYPE > deviceTypes, PRL_VM_CONFIG_SECTIONS nSection );
	void CheckRemoteDevicesForDesktopMode( QSet< PRL_DEVICE_TYPE > deviceTypes );
	bool IsUrlFormatSysName(const QString& qsSysName) const;
	QString GetPCIDeviceSysNameMainPart(const QString& qsDeviceId);
	void GetAllSupportedPartitionSysNames(QStringList& lstAllPartitions,
										  QStringList& lstSupportedPartitions) const;
	void postValidationErrorsProcessing();
	void leaveErrorsOnlyForChanges();

	// VirtualDisk commented out by request from CP team
	//// format snapshots list for error message
	//static QString GetSnapshotNamesForDisk(const SNAPTREE_ELEMENT& sntElem,
	//									   CSavedStateTree *savedStateTree,
	//									   const Uuid current);
	void CheckSnapshotsOfDisk(const CVmHardDisk &hardDisk);

	SmartPtr<CDspClient>						m_pClient;
	SmartPtr<CVmConfiguration>					m_pVmConfig;
	SmartPtr<CVmConfiguration>					m_pVmConfigOld;
	QList<PRL_RESULT >							m_lstResults;
	QMap<int , QStringList >					m_mapParameters;
	QMap<int , DeviceInfo >						m_mapDevInfo;
	QMap<int , PRL_GENERIC_PCI_DEVICE_CLASS >	m_mapPciDevClasses;
	bool										m_bCheckOnlyChanges;
	QMap<int , QSet<QString > >					m_mapFullItemIds;
};


#endif	/* _VM_VALIDATE_CONFIG_H_ */
