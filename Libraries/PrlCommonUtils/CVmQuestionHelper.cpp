/////////////////////////////////////////////////////////////////////////////
///
/// @file CVmQuestionHelper.cpp
///
/// Class helper for the question mech implementation
///
/// @author avagin@
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

#include "CVmQuestionHelper.h"
#include "Libraries/NonQtUtils/CQuestionHelper.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>

using namespace IOService;
using namespace Parallels;

SmartPtr<IOPackage> CVmQuestionHelper::getDefaultAnswerToVm(CVmConfiguration *pVmConfig, \
														const SmartPtr<IOPackage>& pQuestionPacket)
{
	if (!pQuestionPacket.isValid())
	{
		return SmartPtr<IOPackage>();
	}

	CVmEvent eventQuestion(UTF8_2QSTR(pQuestionPacket->buffers[0].getImpl()));

	PRL_RESULT nAnswer = PRL_ERR_INVALID_ARG;
	PRL_VM_AUTOSTOP_OPTION nAutoStopOpt = PAO_VM_STOP;

	if ( pVmConfig )
	{
		/**
		 * $SF$ "vm.default_answers"
		 * Specify list of pairs via comma question and answer delimited by colon
		 *   vm.default_answers = < q_id : a_id >[ , < q_id : a_id > ... ]
		 *   where q_id is a question ID, a_id is an answer ID as integer numbers
		 *   from SDK/Include/PrlErrorsValues.h
		 */

		QMap<PRL_RESULT , PRL_RESULT > mapQA(
			CQuestionHelper::getQuestionsAndAnswersFromSysFlags(
								"vm.default_answers",
								QSTR2UTF8(pVmConfig->getVmSettings()->getVmRuntimeOptions()->getSystemFlags())
								) );

		if ( mapQA.contains(eventQuestion.getEventCode()) )
		{
			WRITE_TRACE( DBG_FATAL, "Default answer on question %s got from system flags",
							PRL_RESULT_TO_STRING(eventQuestion.getEventCode()) );

			nAnswer = mapQA.value(eventQuestion.getEventCode());
		}

		nAutoStopOpt = pVmConfig->getVmSettings()->getShutdown()->getAutoStop();
	}

	if (nAnswer == PRL_ERR_INVALID_ARG)
	{
		nAnswer = CQuestionHelper::getDefaultAnswer( eventQuestion.getEventCode(),
														ParallelsDirs::getAppExecuteMode(),
														nAutoStopOpt);
	}

	if (nAnswer == PRL_ERR_INVALID_ARG)
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to find default answer for question %.8X '%s'",\
						eventQuestion.getEventCode(), PRL_RESULT_TO_STRING(eventQuestion.getEventCode()));
		return SmartPtr<IOPackage>();
	}

	// Prepare answer

	CVmEvent eventAnswer;

	eventAnswer.setEventCode(PET_DSP_EVT_VM_QUESTION);
	eventAnswer.setEventIssuerType(eventQuestion.getEventIssuerType());
	eventAnswer.setEventIssuerId(eventQuestion.getEventIssuerId());	// VM uuid
	eventAnswer.setInitRequestId(Uuid::toString(pQuestionPacket->header.uuid));

	eventAnswer.addEventParameter(
		new CVmEventParameter(
				PVE::String,
				eventQuestion.getEventIssuerId(),
				EVT_PARAM_VM_UUID
				) );

	eventAnswer.addEventParameter(
			new CVmEventParameter(
					PVE::UnsignedInt,
					QString("%1").arg((quint32)nAnswer),
					EVT_PARAM_MESSAGE_CHOICE_0
					) );

	SmartPtr<IOPackage> pAnswerPacket
		= DispatcherPackage::createInstance(PVE::DspCmdVmAnswer, eventAnswer, pQuestionPacket);

	return pAnswerPacket;
}
