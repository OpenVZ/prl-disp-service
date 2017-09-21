////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2012-2017, Parallels International GmbH
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
/// @file
///	Task_MoveVm.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author krasnov@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_MoveVm_H_
#define __Task_MoveVm_H_

#include "CDspTaskHelper.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

namespace Vm
{
namespace Directory
{
struct Ephemeral;

} // namespace Directory
} // namespace Vm

enum _PRL_VM_MOVE_STEP {
	MOVE_VM_EXCL_OP_REGISTERED	= (1 << 0),
	MOVE_VM_EXCL_PARAMS_LOCKED	= (1 << 1),
};

class Task_MoveVm : public  CDspTaskHelper
{
   Q_OBJECT
public:
	Task_MoveVm(const SmartPtr<CDspClient>&, const SmartPtr<IOPackage>&,
		Vm::Directory::Ephemeral& ephemeral_);

	virtual QString getVmUuid();
	const QString& getVmDirectory() const
	{
		return m_sVmDirUuid;
	}
	const SmartPtr<CVmConfiguration>& getVmConfig() const
	{
		return m_pVmConfig;
	}

protected:
	virtual PRL_RESULT prepareTask();
	virtual void finalizeTask();
	virtual PRL_RESULT run_body();

	virtual void cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p);

private:
	SmartPtr<CVmConfiguration> m_pVmConfig;
	QString m_sVmDirUuid;
	QString m_sVmUuid;
	QString m_sNewVmHome;
	QString m_sNewVmBundle; // = m_sNewVmHome + VmName
	PRL_UINT32 m_nFlags;

	QString m_sOldVmConfigPath;
	QString m_sOldVmBundle;
	QString m_sNewVmConfigPath;
	bool m_isFsSupportPermsAndOwner;

	PRL_UINT32 m_nSteps;
	Vm::Directory::Ephemeral* m_ephemeral;
	SmartPtr<CVmDirectory::TemporaryCatalogueItem> m_pVmInfo;
};



#endif //__Task_MoveVm_H_
