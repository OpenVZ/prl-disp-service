///////////////////////////////////////////////////////////////////////////////
///
/// @file CVmMigrateTargetDisk.cpp
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

#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlsdk/PrlErrors.h>
#include <prlsdk/PrlEnums.h>
#include <CVmMigrateTargetDisk.h>
#include <prlcommon/Std/PrlAssert.h>

CVmMigrateTargetDisk::CVmMigrateTargetDisk() :
	m_sem_init(1),
	m_result(PRL_ERR_UNINITIALIZED)
{
	m_sem_init.acquire();
}

CVmMigrateTargetDisk::~CVmMigrateTargetDisk()
{
	stop();
}

PRL_RESULT CVmMigrateTargetDisk::init(VirtualDisk::Qcow2* disk)
{
	WRITE_TRACE(DBG_DEBUG, "[Disk migration] Initializing new virtual disk task");
	m_result = PRL_ERR_SUCCESS;

	if ( !disk )
		return (m_result = PRL_ERR_INVALID_ARG);

	m_disk.reset(disk);

	QThread::start();

	m_sem_init.acquire();
	return m_result;
}

void CVmMigrateTargetDisk::stop()
{
	m_token.signal();
	WRITE_TRACE(DBG_DEBUG, "[Disk migration] Stopping(%u)!!!", m_queue.size() );
	QThread::wait();
}

void CVmMigrateTargetDisk::add_data(const CVmMigrateDiskBlock_t& hdr, const SmartPtr<char>& buf)
{
	DiskQueueElem_t data;
	data.hdr = hdr;
	data.buf = buf;
	m_queue.add(data, m_token);
}

void CVmMigrateTargetDisk::run()
{
	m_sem_init.release();
	m_result = PRL_ERR_SUCCESS;

	while (PRL_SUCCEEDED(m_result) && !(m_queue.empty() && m_wait))
	{
		DiskQueueElem_t data;
		if (m_queue.take(data, m_token))
			m_result = write_data(&data);
		else
			break;
		if (PRL_FAILED(m_result))
		{
			WRITE_TRACE(DBG_FATAL, "[Disk migration] Failed to write block to disk %s,\
					address %llu, size %u",
				qPrintable(Uuid::fromGuid(data.hdr.disk_id).toString()),
				data.hdr.lba, data.hdr.nsect);
		}
	}
}

PRL_RESULT CVmMigrateTargetDisk::write_data(DiskQueueElem_t* data)
{
	WRITE_TRACE(DBG_DEBUG, "write block %llu", data->hdr.lba);
	return m_disk->write(data->buf.getImpl(), data->hdr.nsect * 512, data->hdr.lba);
}

void CVmMigrateTargetDisk::wait_queued()
{
	m_wait.fetchAndStoreAcquire(1);
	m_token.signal();
	QThread::wait();
}

PRL_RESULT CVmMigrateTargetDisk::getError() const
{
	if (isRunning())
		return PRL_ERR_SERVICE_BUSY;

	return m_result;
}

