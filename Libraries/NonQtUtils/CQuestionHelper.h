/////////////////////////////////////////////////////////////////////////////
///
/// @file CQuestionHelper.h
///
/// Class helper for the question mech definition
///
/// Wiki documentation:
/// http://wiki.parallels.com/index.php/Sync_Requests_Queue
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

#ifndef QUESTION_HELPER_H
#define QUESTION_HELPER_H


#include <map>
#include <prlsdk/PrlErrors.h>
#include <prlsdk/PrlEnums.h>


class CQuestionHelper
{
public:

	/** Returns default answers for given quetion
	 *  @param question
	 *  @return answer or PRL_ERR_INVALID_ARG
	 */
	static PRL_RESULT getDefaultAnswer( PRL_RESULT nQuestion,
										PRL_APPLICATION_MODE nMode,
										PRL_VM_AUTOSTOP_OPTION nAutoStopOpt = PAO_VM_STOP);

	/** Returns map of questions and answers
	 *  @param system flags
	 *  @return map of questions and answers
	 */
	static std::map<PRL_RESULT , PRL_RESULT > getQuestionsAndAnswersFromSysFlags(const char* strFlagName,
																				 const char* strSysFlags);
	/**
	 * Returns sign whether question is related to internal questions which shouldn't go to interactive sessions
	 */
	static bool isInternalQuestion(PRL_RESULT nQuestion);

};


#endif	// QUESTION_HELPER_H
