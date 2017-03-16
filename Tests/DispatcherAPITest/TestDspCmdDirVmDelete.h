/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
///		TestDspCmdDirVmDelete.h
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdDirVmDelete dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef H_TestDspCmdDirVmDelete_H
#define H_TestDspCmdDirVmDelete_H

#include "TestDispatcherBase.h"

class TestDspCmdDirVmDelete : public TestDispatcherBase
{
   Q_OBJECT

public:
	TestDspCmdDirVmDelete();

private slots:
	void initTestCase();
	void init();
	void cleanup();
	void cleanupTestCase();

private slots:
	void TestOnValidParams();
	void TestOnNonAccessVmDir();
	void TestOnInvalidVmUuid();
	void TestOnEmptyVmUuid();
	void TestOnTooLongVmUuid();

private:
	CPveControl *m_pPveControl2;
};

#endif //H_TestDspCmdDirVmDelete_H
