///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_AutoStart.cpp
///
/// Dispatcher task for
///
/// @author sergeyt
/// @owner sergeym
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

#include "Task_AutoStart.h"
#include "Task_CommonHeaders.h"
#include "CDspStarter.h"
#include "CProtoSerializer.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/Std/PrlTime.h"
#include "CDspService.h"
#include "CDspVmManager.h"
#include "CDspVmSuspendHelper.h"

#include <qtconcurrentmap.h>
#include <boost/bind.hpp>

#ifdef _CT_
#include "CDspVzHelper.h"
#include "Dispatcher/Tasks/Task_VzManager.h"
#ifdef _LIN_
#include <sys/vfs.h>
#endif
#endif

namespace
{
#ifdef _CT_
void resumeCt(const SmartPtr<CVmConfiguration>& config_)
{
	QString u = config_->getVmIdentification()->getVmUuid();
	WRITE_TRACE(DBG_FATAL, "Auto resume CT %s", QSTR2UTF8(u));

	/* Destroy suspend in PRAM mark */
	QFile f(CDspVzHelper::getVzrebootMarkFile(config_));
	f.remove();

	CVzOperationHelper z;
	if (PRL_FAILED(z.resume_env(u, PNSF_CT_SKIP_ARPDETECT)))
		z.start_env(u, 0);
}

void startCt(const SmartPtr<CVmConfiguration>& config_)
{
	QString u = config_->getVmIdentification()->getVmUuid();
	WRITE_TRACE(DBG_FATAL, "Auto start CT %s", QSTR2UTF8(u));

	CVzOperationHelper().start_env(u, 0);
}

#endif // _CT_
} // namespace

using namespace Parallels;

Task_AutoStart::Task_AutoStart(SmartPtr<CDspClient> &pUser,
        const SmartPtr<IOPackage> &pRequestPkg)
: CDspTaskHelper(pUser, pRequestPkg)
{}

bool sortByBootOrderPrio(const SmartPtr<CVmConfiguration> &s1, const SmartPtr<CVmConfiguration> &s2)
{
	return s1->getVmSettings()->getVmStartupOptions()->getBootOrderPrio() >
		s2->getVmSettings()->getVmStartupOptions()->getBootOrderPrio();
}

static bool isFirstStart()
{
#ifdef _LIN_
	struct statfs fs;

#ifndef TMPFS_MAGIC
#define TMPFS_MAGIC	0x01021994
#endif
	if (statfs("/run", &fs) || fs.f_type != TMPFS_MAGIC)
		return true;

	QDir().mkdir("/run/prl-disp");

	QFile f("/run/prl-disp/.ct_start.lck");
	if (f.exists())
		return false;

	f.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);
#endif

	return true;
}

void Task_AutoStart::startCts()
{
	if (!isFirstStart())
		return;

	QList<SmartPtr<CVmConfiguration> > configs, resumeList;

	CDspService::instance()->getVzHelper()->getCtConfigList(getClient(), 0, configs);

	qSort(configs.begin(), configs.end(), sortByBootOrderPrio);

	resumeList = CDspService::instance()->getVzHelper()->getAutoResumeCtList(configs);

	if (!resumeList.empty())
	{
		/* PRAM is shared resource between VM & CT
		 * so it needed to synchronize PRAM destroy,
		 */
		SmartPtr<CDspVmSuspendMounter> pSuspendMounter = CDspService::instance()->
			getVmManager().getSuspendHelper()->prepareCtForResume();

		QtConcurrent::blockingMap(resumeList, &resumeCt);
	}
	else
	{
		QList<SmartPtr<CVmConfiguration> > startList;
		
		foreach(SmartPtr<CVmConfiguration> pConfig, configs)
			if (pConfig->getVmSettings()->getVmStartupOptions()->getAutoStart() != PAO_VM_START_MANUAL)
				startList += pConfig;

		QtConcurrent::blockingMap(startList, &startCt);
	}
}

PRL_RESULT Task_AutoStart::run_body()
{
	startCts();

	CDspService::instance()->getVmManager().getSuspendHelper()->freeFastRebootResources();

	return PRL_ERR_SUCCESS;
}
