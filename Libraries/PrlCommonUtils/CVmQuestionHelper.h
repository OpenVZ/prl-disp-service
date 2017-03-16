/////////////////////////////////////////////////////////////////////////////
///
/// @file CQuestionHelper.h
///
/// Class helper for the question mech definition
///
/// Wiki documentation:
/// http://wiki.parallels.com/index.php/Sync_Requests_Queue
///
/// @author avagin@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef VM_QUESTION_HELPER_H
#define VM_QUESTION_HELPER_H

#include <prlcommon/IOService/IOCommunication/IOProtocol.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Std/SmartPtr.h>

class CVmQuestionHelper
{
public:
	static SmartPtr<IOService::IOPackage> getDefaultAnswerToVm(CVmConfiguration *pVmConfig, \
													const SmartPtr<IOService::IOPackage>& pQuestionPacket);
};


#endif	// QUESTION_HELPER_H
