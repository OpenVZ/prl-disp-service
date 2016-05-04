///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateVmBackup_p.h
///
/// Source task for Vm backup creation
///
/// Copyright (c) 2016 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __Task_CreateVmBackup_p_H_
#define __Task_CreateVmBackup_p_H_

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "CDspService.h"
#include "Task_BackupHelper.h"
#include "CDspVmBackupInfrastructure_p.h"

namespace Work
{

typedef boost::function2<PRL_RESULT, const QStringList&, unsigned int> worker_type;

///////////////////////////////////////////////////////////////////////////////
// struct Command

struct Command
{
	Command(Task_BackupHelper& task_, ::Backup::Product::Model& p_,
		const ::Backup::Activity::Object::Model& activity_)
		: m_context(&task_), m_product(p_), m_activity(activity_)
		, m_worker(boost::bind(&Task_BackupHelper::startABackupClient
			, m_context
			, m_product.getObject().getConfig()->getVmIdentification()->getVmName()
			, _1
			, QStringList()
			, m_product.getObject().getConfig()->getVmIdentification()->getVmUuid()
			, _2))
	{
	}

	const QFileInfo * findArchive(const ::Backup::Product::component_type& t_);

protected:
	QStringList buildArgs();

	Task_BackupHelper *m_context;
	::Backup::Product::Model& m_product;
	const ::Backup::Activity::Object::Model& m_activity;
	worker_type m_worker;
};

///////////////////////////////////////////////////////////////////////////////
// struct ACommand

struct ACommand : Command
{
	ACommand(Task_BackupHelper& task_, ::Backup::Product::Model& p_,
		const ::Backup::Activity::Object::Model& activity_)
		: Command(task_, p_, activity_)
	{
	}

	PRL_RESULT do_();

private:
	QStringList buildArgs(const ::Backup::Product::component_type& t_, const QFileInfo* f_);
};

///////////////////////////////////////////////////////////////////////////////
// struct VCommand

struct VCommand : Command
{
	VCommand(Task_BackupHelper& task_, ::Backup::Product::Model& p_,
		const ::Backup::Activity::Object::Model& activity_)
		: Command(task_, p_, activity_)
	{
	}

	PRL_RESULT do_();

private:
	QStringList buildArgs();
};

} // namespace Work

#endif //__Task_CreateVmBackup_p_H_
