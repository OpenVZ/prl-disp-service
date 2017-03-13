////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_UpdateVm_H_
#define __Task_UpdateVm_H_

#include "CDspTaskHelper.h"

///////////////////////////////////////////////////////////////////////////////
// class Task_UpdateVm

class Task_UpdateVm: public  CDspTaskHelper
{
	Q_OBJECT
public:
	Task_UpdateVm(const SmartPtr<CDspClient>&,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p);

protected:
	virtual PRL_RESULT prepareTask();
	virtual void finalizeTask();
	virtual PRL_RESULT run_body();

private:
	QString m_sVmUuid;
};

#endif //__Task_UpdateVm_H_
