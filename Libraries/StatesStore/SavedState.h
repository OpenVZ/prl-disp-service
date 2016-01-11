////////////////////////////////////////////////////////////////////////////////
///
/// @file SavedState.h
///
/// This class implements a description of checkpoint
///
/// @author MaximKi
/// @owner SergeyM
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
////////////////////////////////////////////////////////////////////////////////
#ifndef SAVEDSTATE_H
#define SAVEDSTATE_H

#include <QString>
#include <prlcommon/Interfaces/ParallelsNamespace.h>
//#include <Libraries/VirtualDisk/DiskStates.h>  // VirtualDisk commented out by request from CP team

/**
 * Description of the basic part of the saved states struct.
 * Class describe properties of one snapshot element.
 * Class don't contains references to other snapshot elements.
 */
class CSavedState
{
public:


	/// Standard class constructor
	CSavedState();

	/// Copy constructor
	CSavedState(const CSavedState& original);

	/// Class destructor
	~CSavedState();

	/**
	 * Get guid of checkpoint record
	 */
	QString GetGuid() const { return m_sGuid; }
	/**
	 * Set guid of checkpoint record
	 */
	void SetGuid(const QString& Guid) { m_sGuid = Guid; }

	/**
	 * Get name of checkpoint record
	 */
	QString GetName() const { return m_sName; }
	/**
	 * Set name of checkpoint record
	 */
	void SetName(const QString& Name) { m_sName = Name; }

	/**
	 * Get time of checkpoint creating
	 */
	QString GetCreateTime() const { return m_sDateTime; }
	/**
	 * Set time of checkpoint creating
	 */
	void SetCreateTime(const QString& Time) { m_sDateTime = Time; }

	/**
	 * Get creator of checkpoint
	 */
	QString GetCreator() const { return m_sCreator; }
	/**
	 * Set creator of checkpoint
	 */
	void SetCreator(const QString& UserName) { m_sCreator = UserName; }

	/**
	 * Get screenshot file name. Also this attribute can contains
	 * screenshot data encoded in base64.
	 */
	QString GetScreenShot() const { return m_sScreenShot; }
	/**
	 * Set screenshot file name or screenshot data encoded in base64
	 */
	void SetScreenShot(const QString& ScreenShotFile) { m_sScreenShot = ScreenShotFile; }

	/**
	 * Get a checkpoint description
	 */
	QString GetDescription() const { return m_sDescription; }
	/**
	 * Set a checkpoint description
	 */
	void SetDescription(const QString& Description) { m_sDescription = Description; }

	/**
	 * Get last VM state(stopped, started, suspended, etc.)
	 */
	PVE::SnapshotedVmState GetVmState() const { return m_iVmState; }
	/**
	 * Set last VM state(stopped, started, suspended, etc.)
	 */
	void SetVmState(PVE::SnapshotedVmState iVmState) { m_iVmState = iVmState; }

	/**
	 * Is this checkpoint current?
	 */
	bool IsCurrent() const { return m_bCurrent; }

	/**
	 * Is this record describe "You are here" node?
	 */
	bool IsCurrentState() const { return m_bCurrentState; }

	struct Runtime
	{
		/// Snapshot size
		quint64 nSize;

		/// Vm checkpoint OS version
		unsigned int nOsVersion;

// VirtualDisk commented out by request from CP team
//		// Unfinished disk operation type
//		PROCESS_TYPE	nUnfinishedOpType;
//		bool IsUnfinished() { return nUnfinishedOpType != PROCESS_NONE; }

	public:
		void Cleanup();
		Runtime(){ Cleanup(); }
	};

	Runtime GetRuntime() const { return m_runtime; }
	void SetRuntime( const Runtime& val ) { m_runtime = val; }

protected:

	/**
	 * Set all properties to default values
	 */
	void CleanupStateProperties();

	/**
	 * Set/unset this node as a current node
	 */
	void SetCurrent(bool Current = true) { m_bCurrent = Current; }

	/**
	 * Set/unset this node as a "You are here" node
	 */
	void SetCurrentState(bool CurrentState = true);


private:

	/// Checkpoint guid
	QString m_sGuid;

	/// Checkpoint name
	QString m_sName;

	/// Checkpoint creating time
	QString m_sDateTime;

	/// Checkpoint creator
	QString m_sCreator;

	/// Checkpoint screenshot
	QString m_sScreenShot;

	/// Checkpoint description
	QString m_sDescription;

	/// Is this checkpoint current?
	bool m_bCurrent;

	/// Is this record describe "You are here" node?
	bool m_bCurrentState;

	/// Vm state
	PVE::SnapshotedVmState m_iVmState;

	/// Runtime info
	Runtime m_runtime;
};


#endif // SAVEDSTATE_H
