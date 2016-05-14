///////////////////////////////////////////////////////////////////////////////
///
/// @file CVmMigrateTargetDisk.h
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef CVmMigrateTargetDisk_H
#define CVmMigrateTargetDisk_H

#include <prlsdk/PrlTypes.h>
#include <Libraries/DispToDispProtocols/CVmMigrationProto.h>
#include <prlcommon/VirtualDisk/Qcow2Disk.h>
#include <BlockingQueue.h>

#include <QThread>
#include <QMutex>
#include <queue>

using namespace Parallels;

///////////////////////////////////////////////////////////////////////////////
// class CVmMigrateTargetDisk

class CVmMigrateTargetDisk : public QThread
{
	Q_OBJECT

public:

	typedef struct
	{
		CVmMigrateDiskBlock_t hdr;
		SmartPtr<char> buf;
	} DiskQueueElem_t;

	CVmMigrateTargetDisk();
	virtual ~CVmMigrateTargetDisk();

	PRL_RESULT init(VirtualDisk::Qcow2* disk);
	void add_data(const CVmMigrateDiskBlock_t& hdr, const SmartPtr<char>& buf);
	void stop();
	void wait_queued();

	PRL_RESULT getError() const;

	virtual bool AdevInCurrentThread() const
	{
		return (QThread::currentThread() == this);
	}

private:
	virtual void run();
	PRL_RESULT write_data(DiskQueueElem_t* data);

private:
	QSemaphore m_sem_init;

	PRL_RESULT m_result;

	QAtomicInt m_wait;

	Cancellation::Token m_token;
	BlockingQueue<DiskQueueElem_t> m_queue;
	QScopedPointer<VirtualDisk::Qcow2> m_disk;
};

#endif
