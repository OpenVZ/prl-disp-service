/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// @file
///		QtCoreTest.h
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing QtCore functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef QtCoreTest_H
#define QtCoreTest_H

#include <QtTest/QtTest>
#include <QWaitCondition>
#include <QMutex>

class QtCoreTest : public QObject
{

Q_OBJECT

public:

private slots:

	void test_QDir_entryInfoList_withBackSlash();
	void test_QFileSystemWatcher_hangsInRemovePath_afterMultipleCall();
	void test_QFileSystemWatcher_hangsInRemovePath_afterAddPath();

};

class QFileSystemWatcher;
class QFile;

class WorkerThreadBase: public QThread
{
public:
	WorkerThreadBase()
		: m_bStop(false), m_bPingSign( false )
		{}
	virtual ~WorkerThreadBase();

	bool ping( unsigned long timeout );

protected:
	void stop() { m_bStop = true; }
	void tryAnswerToPing();
protected:
	bool m_bStop;
	bool m_bPingSign;
	QMutex m_mtx;
	QWaitCondition m_cond;
};

class WorkerThreadFsWatcher: public WorkerThreadBase
{
public:
	WorkerThreadFsWatcher( QFileSystemWatcher* pFsw, QFile* pFile )
		:m_pFsw(pFsw), m_pFile(pFile)
		{}
protected:
	QFileSystemWatcher* m_pFsw;
	QFile* m_pFile;
};

class WorkerThreadFSW_removePath: public WorkerThreadFsWatcher
{
public:
	WorkerThreadFSW_removePath( QFileSystemWatcher* pFsw, QFile* pFile )
		:WorkerThreadFsWatcher(pFsw, pFile)
		{}
private:
	void run();
};

class WorkerThreadFSW_addPath: public WorkerThreadFsWatcher
{
public:
	enum Type { tAdder, tRemover };
	WorkerThreadFSW_addPath( QFileSystemWatcher* pFsw, QFile* pFile, Type t )
		:WorkerThreadFsWatcher(pFsw, pFile), m_type(t)
		{}
private:
	void run();
private:
	Type m_type;
};


#endif
