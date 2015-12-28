/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
/// @file
///		TestDspCmdDirVmClone.h
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdDirVmClone dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef H_TestDspCmdDirVmClone_H
#define H_TestDspCmdDirVmClone_H

#include "TestDispatcherBase.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include <prlcommon/Std/SmartPtr.h>

class TestDspCmdDirVmClone : public TestDispatcherBase
{
   Q_OBJECT

private slots:
	void init();
	void cleanup();

private slots:
	void TestOnValidParams();

private:
	SmartPtr<CVmConfiguration> m_pVmConfig;
	SmartPtr<CVmConfiguration> m_pVmCloneConfig;
};

#endif //H_TestDspCmdDirVmClone_H
