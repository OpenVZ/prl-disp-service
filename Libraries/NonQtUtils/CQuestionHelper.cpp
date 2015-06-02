/////////////////////////////////////////////////////////////////////////////
///
/// @file CQuestionHelper.cpp
///
/// Class helper for the question mech implementation
///
/// @author myakhin@
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "CQuestionHelper.h"
#include "Libraries/Logging/Logging.h"
#include "Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h"
#include <cstdio>
#include <string>
#include <algorithm>


PRL_RESULT CQuestionHelper::getDefaultAnswer(PRL_RESULT nQuestion,
											 PRL_APPLICATION_MODE nMode,
											 PRL_VM_AUTOSTOP_OPTION)
{
	PRL_RESULT nAnswer = PRL_ERR_INVALID_ARG;
	switch(nQuestion)
	{
	/*
	*******************************
	*    Vm Questions part
	*******************************
	*/
	case PRL_WARN_TOO_LOW_HDD_SIZE:
	case PRL_WARN_FAT32_FILE_OVERRUN:
	case PET_QUESTION_COMMON_HDD_ERROR:
		nAnswer = PET_ANSWER_STOP;
		break;

	case PET_QUESTION_DO_YOU_WANT_TO_OVERWRITE_FILE:
		nAnswer =  PET_ANSWER_APPEND;
		break;

	case PRL_ERR_CORE_STATE_INV_SAV_VERSION:
		nAnswer =  PET_ANSWER_NO;
		break;

	case PET_QUESTION_REACH_OVERCOMMIT_STATE:
		if (nMode == PAM_SERVER)
			nAnswer =  PET_ANSWER_STARTVM;
		else
			nAnswer =  PET_ANSWER_CANCEL;
		break;

	case PRL_WARN_VM_DVD_DISCONNECT_LOCKED_DEV:
	case PRL_WARN_VM_DISCONNECT_SATA_HDD:
		nAnswer =  PET_ANSWER_DISCONNECT_ANYWAY;
		break;

	case PRL_ERR_DEV_USB_CHANGEPID:
	case PRL_ERR_DEV_USB_INSTALL_DRIVER_FAILED:
	case PRL_WARN_DEV_USB_CONNECT_FAILED:
	case PRL_ERR_DEV_USB_HARD_DEVICE_INSERTED2:
	case PRL_ERR_DEV_USB_NO_FREE_PORTS:
	case PRL_ERR_DEV_USB_HARD_DEVICE_INSERTED:
	case PRL_ERR_DEV_USB_OPEN_MANAGER_FAILED:
	case PRL_ERR_DEV_USB_BUSY:
	case PRL_ERR_DEV_USB_REUSE:
		nAnswer =  PET_ANSWER_YES;
		break;

	case PET_QUESTION_REBOOT_HOST_ON_PCI_DRIVER_INSTALL_OR_REVERT:
		nAnswer =  PET_ANSWER_LATER;
		break;

	case PET_QUESTION_RESTART_VM_GUEST_TO_COMPACT:
		nAnswer = PET_ANSWER_CANCEL;
		break;

	case PET_QUESTION_CONTINUE_IF_HVT_DISABLED:
	    nAnswer = PET_ANSWER_YES;
	    break;
	case PET_QUESTION_CANCEL_CLONE_TO_TEMPLATE_OPERATION:
	case PET_QUESTION_CANCEL_CLONE_OPERATION:
	case PET_QUESTION_CANCEL_IMPORT_BOOTCAMP_OPERATION:
	case PET_QUESTION_CANCEL_CONVERT_3RD_VM_OPERATION:
		nAnswer = PET_ANSWER_YES;
		break;

	case PET_QUESTION_ON_QUERY_END_SESSION:
		nAnswer = PET_ANSWER_STOP;
		break;

	case PRL_ERR_CORE_STATE_VM_WOULD_STOP:
		nAnswer = PET_ANSWER_STOP_AND_RESTORE;
		break;

	case PET_QUESTION_SUSPEND_STATE_INCOMPATIBLE_CPU:
		nAnswer = PET_ANSWER_RESTART;
		break;
	case PET_QUESTION_SNAPSHOT_STATE_INCOMPATIBLE_CPU:
		nAnswer = PET_ANSWER_STOP_AND_RESTORE;
		break;

	case PET_QUESTION_VM_REBOOT_REQUIRED_BY_PRL_TOOLS:
		nAnswer = PET_ANSWER_LATER;
		break;

	case PET_QUESTION_BOOTCAMP_HELPER_OS_UNSUPPORTED:
		nAnswer = PET_ANSWER_SHUTDOWN;
		break;

	/*
	*******************************
	*    Internal VM Questions
	*******************************
	*/
	case PRL_QUESTION_FINALIZE_VM_PROCESS:
		nAnswer = PET_ANSWER_YES;
		break;

	/*
	*******************************
	*    Server Questions
	*******************************
	*/

	//  -- Dispatcher: Task_RegisterVm
	case PET_QUESTION_VM_COPY_OR_MOVE:
		nAnswer = PET_ANSWER_COPIED;
		break;
	case PET_QUESTION_CREATE_NEW_MAC_ADDRESS:
		nAnswer = PET_ANSWER_CREATE_NEW;
		break;
	case PET_QUESTION_REGISTER_USED_VM:
		nAnswer = PET_ANSWER_NO;
		break;
	case PET_QUESTION_VM_ROOT_DIRECTORY_NOT_EXISTS:
		nAnswer = PET_ANSWER_NO;
		break;
	case PET_QUESTION_RESTORE_VM_CONFIG_FROM_BACKUP:
		nAnswer = PET_ANSWER_YES;
		break;
	case PET_QUESTION_REGISTER_VM_TEMPLATE:
		nAnswer = PET_ANSWER_YES;
		break;
	case PET_QUESTION_CREATE_OS2_GUEST_WITHOUT_FDD_IMAGE:
		nAnswer = PET_ANSWER_CONTINUE;
		break;
	case PET_QUESTION_CREATE_VM_FROM_LION_RECOVERY_PART:
		nAnswer = PET_ANSWER_CONTINUE;
		break;

	//  -- Dispatcher: Task_CreateImage
	case PET_QUESTION_FREE_SIZE_FOR_COMPRESSED_DISK:
		nAnswer = PET_ANSWER_YES;
		break;

	//  -- Dispatcher: Task_Autoprotect
	case PRL_WARN_GOING_TO_TAKE_SMART_GUARD_SNAPSHOT:
		nAnswer = PET_ANSWER_SKIP;
		break;

	//  -- Dispatcher: Task_AutoCompress
	case PET_QUESTION_COMPACT_VM_DISKS:
		nAnswer = PET_ANSWER_NO;
		break;

	//  -- Dispatcher: Task_DownloadAppliance
	case PET_QUESTION_APPLIANCE_CORRUPTED_INSTALLATION:
		nAnswer = PET_ANSWER_YES;
		break;

	//  -- Dispatcher: Task_ConvertThirdPartyVm
	case PRL_QUESTION_CONVERT_3RD_PARTY_CANNOT_MIGRATE:
		nAnswer = PET_ANSWER_YES;
		break;

	default:
		WRITE_TRACE(DBG_FATAL, "Unknown question: %.8X", nQuestion);
		nAnswer = PRL_ERR_INVALID_ARG;
	}

	WRITE_TRACE( DBG_WARNING, "Question = %s (%.8X), default answer = %s (%.8X)"
		, PRL_RESULT_TO_STRING(nQuestion), nQuestion
		, PRL_RESULT_TO_STRING(nAnswer), nAnswer
		);

	return nAnswer;
}

static bool _is_space(const char c)
{
	return ::isspace(c) ? true : false;
}

std::map<PRL_RESULT , PRL_RESULT > CQuestionHelper::getQuestionsAndAnswersFromSysFlags(const char* strFlagName,
																					   const char* strSysFlags)
{
	std::map<PRL_RESULT , PRL_RESULT > mapQA;

	if ( ! strFlagName || ! strSysFlags)
		return mapQA;

	std::string flagName = strFlagName;
	std::string sysFlagsRaw = strSysFlags;
	if (sysFlagsRaw.empty())
		return mapQA;

	std::string::iterator it
		= std::remove_if(sysFlagsRaw.begin(), sysFlagsRaw.end(), _is_space);
	if (it != sysFlagsRaw.end())
		*it = '\0';
	std::string sysFlags = sysFlagsRaw.c_str();

	std::string::size_type idx = sysFlags.find(flagName + "=");
	if (idx == std::string::npos)
		return mapQA;
	idx += (flagName.length() + 1);

	std::string::size_type idx2 = sysFlags.find(";", idx);
	if (idx2 == std::string::npos)
		idx2 = sysFlags.length();

	while(idx != std::string::npos && idx < idx2)
	{
		PRL_RESULT nQuestion = 0;
		PRL_RESULT nAnswer = 0;

		if (std::sscanf(sysFlags.data() + idx, "%i:%i", &nQuestion, &nAnswer) != 2)
			break;

		mapQA.insert(std::map<PRL_RESULT , PRL_RESULT >::value_type(nQuestion, nAnswer));

		idx = sysFlags.find(",", idx);
		if (idx != std::string::npos)
			++idx;
	}

	return mapQA;
}

bool CQuestionHelper::isInternalQuestion(PRL_RESULT nQuestion)
{
	switch (nQuestion)
	{
		case PRL_QUESTION_FINALIZE_VM_PROCESS:
			return true;

		default:
			return false;
	}
}

