///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_ConfigureGenericPci.h
///
/// Dispatcher task for configuration generic PCI devices.
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_ConfigureGenericPci_H_
#define __Task_ConfigureGenericPci_H_


#include <QString>

#include "CDspTaskHelper.h"
#include "CDspClient.h"
#include <prlxmlmodel/HostHardwareInfo/GenericPciDevices.h>


class GenericPciDevices;


class Task_ConfigureGenericPci : public CDspTaskHelper
{
	Q_OBJECT
public:

	Task_ConfigureGenericPci(SmartPtr<CDspClient> &pUser, const SmartPtr<IOPackage> &pRequestPkg);

private:

	/**
	 * Structure for storing changes applied to device in the dialog
	 */
	struct PciDeviceConfigure 
	{
		PciDeviceConfigure() : state(PGS_CONNECTED_TO_HOST) {}

		PRL_GENERIC_DEVICE_STATE state;
		QString					 driverName;
		QString					 serviceName;
	};

	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

	bool getPciDeviceNumbers(const QString& qsDeviceId, int& nNum1, int& nNum2, int& nNum3);
	PRL_RESULT updateVtdHook();

	GenericPciDevices							m_Devices;
	QMap<QString, PciDeviceConfigure >			m_mapHwDevices;

	bool			m_bIsOperationFailed;
	QStringList		m_lstDevErrors;
	bool			m_bNeedHostReboot;

	static bool m_bIsRunning;
};


#endif	// __Task_ConfigureGenericPci_H_
