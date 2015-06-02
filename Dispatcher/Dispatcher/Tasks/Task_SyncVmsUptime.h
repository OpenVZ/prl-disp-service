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
/// Task implementation which updates periodically VM uptime values a their configurations
///
/// @owner sergeym@
/// @author	sandro@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_SyncVmsUptime_H__
#define __Task_SyncVmsUptime_H__

#include "CDspTaskHelper.h"

class Task_SyncVmsUptime : public CDspTaskHelper
{
	Q_OBJECT
public:
	Task_SyncVmsUptime(const SmartPtr<CDspClient> &user,	const SmartPtr<IOPackage> &p);
	virtual ~Task_SyncVmsUptime();
	/**
	 * Overridden method which processes cancel operation action
	 */
	virtual void cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage> &);

private slots:
	void onTimeoutEvent();

protected:
	virtual PRL_RESULT run_body();
};

#endif //__Task_SyncVmsUptime_H__

