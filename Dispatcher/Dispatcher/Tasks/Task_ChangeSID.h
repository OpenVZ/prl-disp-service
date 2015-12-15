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
/// @author sergeyt
///	igor@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_ChangeSID_H__
#define __Task_ChangeSID_H__

#include "CDspVm.h"
#include "CDspTaskHelper.h"
#include "CDspLibvirt.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"

class Task_ChangeSID : public CDspTaskHelper
{
	Q_OBJECT
public:
	Task_ChangeSID(const SmartPtr<CDspClient> &user,
			const SmartPtr<IOPackage> &p,
			const SmartPtr<CVmConfiguration> &pVmConfig,
			bool bStandAlone = false);
	~Task_ChangeSID();

	virtual QString getVmUuid();

protected:
	virtual PRL_RESULT prepareTask();
	virtual void finalizeTask();
	virtual PRL_RESULT run_body();

private:
	void jobProgressEvent(unsigned int progress);
	PRL_RESULT save_config(SmartPtr<CVmConfiguration> &pVmConfig);
	PRL_RESULT change_sid(Libvirt::Tools::Agent::Vm::Unit& u);
	PRL_RESULT run_changeSID_cmd(Libvirt::Tools::Agent::Vm::Unit& u);

private:
	SmartPtr<CVmConfiguration> m_pVmConfig;
	/** Sign which specified whether task was initiated as separate command or a part of clone VM operation */
	bool m_bStandAlone;
};

#define CHANGESID_TIMEOUT	(60 * 60 * 1000)

#endif //__Task_ChangeSID_H__
