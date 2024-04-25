////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_ConvertCt_H_
#define __Task_ConvertCt_H_

#include <prlcommon/PrlCommonUtilsBase/CFileHelper.h>
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "CDspTaskHelper.h"
#include "CDspRegistry.h"

///////////////////////////////////////////////////////////////////////////////
// class Task_ConvertCt

class Task_ConvertCt: public  CDspTaskHelper
{
	Q_OBJECT
public:
	Task_ConvertCt(const SmartPtr<CDspClient>&,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p,
		const SmartPtr<CVmConfiguration> &pCtConfig,
		Registry::Public& registry_
		);

protected:
	virtual PRL_RESULT prepareTask();
	virtual void finalizeTask();
	virtual PRL_RESULT run_body();
private:
	virtual PRL_RESULT convert();

private:
	/** Pointer to related CT configuration object */
	SmartPtr<CVmConfiguration> mpCtConfig;
	QString mCtUuid;
	QString mCtDirUuid;
	QString mCtHome;
	QString mNewVmDir;
	QString mCtConf;
	QString mCtPvsConf;
	SmartPtr<CVzOperationHelper> mpVzHelper;
	Registry::Public& mRegistry;
};

#endif //__Task_ConvertCt_H_
