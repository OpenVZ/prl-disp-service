////////////////////////////////////////////////////////////////////////////////
///
/// @file SavedState.cpp
///
/// This class implements a description of checkpoint
///
/// @author MaximKi
/// @owner SergeyM
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
////////////////////////////////////////////////////////////////////////////////

#include "SavedState.h"

void CSavedState::Runtime::Cleanup()
{
	nSize = 0;
	nOsVersion = 0;
// VirtualDisk commented out by request from CP team
//	nUnfinishedOpType = PROCESS_NONE;
}

/**
 * Standart constructor
 */
CSavedState::CSavedState()
{
	CleanupStateProperties();
}

/**
 * Copy constructor
 */
CSavedState::CSavedState(const CSavedState& original)
{
	m_sGuid = original.GetGuid();
	m_sName = original.GetName();
	m_sDateTime = original.GetCreateTime();
	m_sCreator = original.GetCreator();
	m_sScreenShot = original.GetScreenShot();
	m_sDescription = original.GetDescription();
	m_bCurrent = original.IsCurrent();
	m_bCurrentState = original.IsCurrentState();
	m_iVmState = original.GetVmState();
	m_runtime= original.GetRuntime();
}

/**
 * Standart destructor
 */
CSavedState::~CSavedState()
{
}

/**
 * Set all properties to default values
 */
void CSavedState::CleanupStateProperties()
{
	m_sGuid.clear();
	m_sName.clear();
	m_sDateTime.clear();
	m_sCreator.clear();
	m_sScreenShot.clear();
	m_sDescription.clear();
	m_bCurrent = false;
	m_bCurrentState = false;
	m_iVmState = PVE::SnapshotedVmPoweredOff;

	m_runtime.Cleanup();
}

/**
 * Set/unset this node as a "You are here" node
 */
void CSavedState::SetCurrentState(bool CurrentState)
{
	if (CurrentState)
		CleanupStateProperties();

	m_bCurrentState = CurrentState;
}
