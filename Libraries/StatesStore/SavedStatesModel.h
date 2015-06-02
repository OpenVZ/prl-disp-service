/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

////////////////////////////////////////////////////////////////////////////////
//
// XML parsing return codes
//
////////////////////////////////////////////////////////////////////////////////

#ifndef SAVEDSTATESMODEL_H
#define SAVEDSTATESMODEL_H

/**
 * Snapshots Configuration XML parser return codes
 */
class SnapshotParser
{
public:
	enum SnapshotReturnCode
	{
		RcSuccess = 0,
		BadFileName,
		BadSnapshotTree,
		EmptySnapshotTree,
		CannotWriteFile,
		CannotCreateSnapshot,
		CannotDeleteSnapshot,
		BadSnapshotGuid,
		BadSnapshotName,
		BadSnapshotCreateTime,
		BadSnapshotCreator,
		BadSnapshotScreenshot,
		BadSnapshotDescription
	};
};


/**
 * Macro-definitions used in XML procedures
 */
#define IS_OPERATION_SUCCEEDED(x)	(x?0:1)


/**
 * XML tree basic elements
 */
#define XML_SS_CONFIG_EL_ROOT			"ParallelsSavedStates"
#define XML_SS_CONFIG_EL_ITEM			"SavedStateItem"
#define XML_SS_CONFIG_EL_NAME			"Name"
#define XML_SS_CONFIG_EL_CREATETIME		"DateTime"
#define XML_SS_CONFIG_EL_CREATOR		"Creator"
#define XML_SS_CONFIG_EL_SCREENSHOT		"ScreenShot"
#define XML_SS_CONFIG_EL_DESCRIPTION	"Description"

#define XML_SS_CONFIG_EL_RUNTIME			"Runtime"
#define XML_SS_CONFIG_EL_SIZE					"Size"
#define XML_SS_CONFIG_EL_OS_VERSION		"OsVersion"
#define XML_SS_CONFIG_EL_UNFINISHED_OP	"UnfinishedOp"

#define XML_SS_CONFIG_AT_GUID			"guid"
#define XML_SS_CONFIG_AT_CURRENT		"current"
#define XML_SS_CONFIG_AT_VMSTATE		"state"

#define XML_SS_CONFIG_VAL_IS_CURRENT	"yes"
#define XML_SS_CONFIG_VAL_VMSTATE_OFF	"poweroff"
#define XML_SS_CONFIG_VAL_VMSTATE_ON	"poweron"
#define XML_SS_CONFIG_VAL_VMSTATE_PAUSE	"pause"
#define XML_SS_CONFIG_VAL_VMSTATE_SUSPEND "suspend"

#define XML_SS_CONFIG_HEADER			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"


#endif // SAVEDSTATESMODEL_H
