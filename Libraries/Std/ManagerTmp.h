///////////////////////////////////////////////////////////////////////////////
///
/// @file ManagerTmp.h
///
/// Template of the files processing manager with callback
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
#ifndef __MNGTMPL_INCLUDED__
#define __MNGTMPL_INCLUDED__

#include <QSemaphore>
#include <prlsdk/PrlTypes.h>
#include <Libraries/Std/list.h>
#include <Libraries/Logging/Logging.h>
#include <Libraries/Std/ManagerIf.h>

/*
 * Class handling the group of files processing
 */
template <class EL> class TManager : public IManager
{
protected:
	// Stages of execution
	typedef enum _Stage
	{
		NeedInit = 0,
		Adding,
		Processing,
		Processed,
		Backup,
		Backedup,
		Renaming,
		Renamed,
		Cleaning,
		Cleaned,
		RollingBack,
		Stuck
	} Stage;
	// Element storage (for gcc compiler)
	typedef struct _EHLP
	{
		EL Element;
	} EHLP;
	// Internal entries list
	typedef struct
	{
		EHLP* pHlp;
		// Current stage of execution
		Stage S;
		struct cd_list Lst;
	} Element;
protected:
	Element m_Els;
	PRL_UINT32 m_ListSize;
	PRL_UINT32 m_Current;
	// Callback stuff
	Callback m_Callback;
	PRL_VOID_PTR m_Parameter;
	// Global stage (to check do caller able to commit/rollback)
	Stage m_Stage;
	// Termination request
	bool m_Term;
	// Wait finish of execution via this semaphore
	QSemaphore m_Sem;
	// Global error flag
	bool m_RollbackFailed;
protected:
	/*
	 * Next functions MUST be overloaded by derived classes
	 */
	// Process element
	virtual PRL_RESULT DoExecute(EL& El) = 0;
	// Commit two steps
	virtual PRL_RESULT DoBackup(EL& El) = 0;
	virtual PRL_RESULT DoRename(EL& El) = 0;
	// Finalization step
	virtual PRL_RESULT DoFinalize(EL& El) = 0;
	// Rollbacks
	virtual PRL_RESULT UnrollFinalize(EL& El) = 0;
	virtual PRL_RESULT UnrollRename(EL& El) = 0;
	virtual PRL_RESULT UnrollBackup(EL& El) = 0;
	virtual PRL_RESULT UnrollExecute(EL& El) = 0;
	// Serialize element to string
	virtual QString ElToString(const EL& El) = 0;
	// Completion routine, called before final cleanup
	virtual void DoComplete(bool) { };
public:

	TManager() :
		m_RollbackFailed(false)
	{
		cd_list_init(&m_Els.Lst);
		// Allocate 1 resource
		Unlock();
		SetDefault();
	};

	virtual ~TManager()
	{
		Cleanup();
	};

	// Class destructor
	virtual void Release()
	{
		delete this;
	};

	// Reset to default.
	virtual void Reset()
	{
		if (IsActive())
			Unlock();

		SetDefault();
	};

	// Add element to list
	virtual PRL_RESULT AddElement(const EL& El)
	{
		if (m_Stage != Adding)
		{
			WRITE_TRACE(DBG_FATAL, "Add file called in incorrect filter state (%s)",
						qPrintable(StageStr(m_Stage)));
			return PRL_ERR_INVALID_ACTION_REQUESTED;
		}

		Element* Entry = new (std::nothrow) Element;

		if (!Entry)
		{
			WRITE_TRACE(DBG_FATAL, "Error allocating memory for entry %s",
						qPrintable(ElToString(El)));
			// The filled list will remain and cleaned at destruction or next call
			return PRL_ERR_OUT_OF_MEMORY;
		}

		Entry->pHlp = new (std::nothrow) EHLP;

		if (!Entry->pHlp)
		{
			WRITE_TRACE(DBG_FATAL, "Error allocating memory for data for entry %s",
						qPrintable(ElToString(El)));
			delete Entry;
			return PRL_ERR_OUT_OF_MEMORY;
		}

		Entry->S = Adding;
		Entry->pHlp->Element = El;
		cd_list_add_tail(&Entry->Lst, &m_Els.Lst);

		m_ListSize++;

		return PRL_ERR_SUCCESS;
	};

	// Initialize class
	virtual PRL_RESULT Init(const Callback Func, const PRL_VOID_PTR Param)
	{
		if (IsActive())
		{
			WRITE_TRACE(DBG_FATAL, "Init called in incorrect state (%s)",
						qPrintable(StageStr(m_Stage)));
			return PRL_ERR_INVALID_ACTION_REQUESTED;
		}

		// Lock stage before actual values change
		Lock();

		if (m_RollbackFailed)
		{
			WRITE_TRACE(DBG_FATAL, "Previous rollback operation failed! "
						"Any further work with this manager is impossible!");
			Unlock();
			return PRL_ERR_INVALID_ACTION_REQUESTED;
		}

		m_Callback = Func;
		m_Parameter = Param;
		m_Stage = Adding;

		return PRL_ERR_SUCCESS;
	};

	// Ecxecute operation
	virtual PRL_RESULT Execute()
	{
		if (m_Stage != Adding)
		{
			WRITE_TRACE(DBG_FATAL, "Execute called in incorrect state (%s)",
						qPrintable(StageStr(m_Stage)));
			return PRL_ERR_INVALID_ACTION_REQUESTED;
		}

		m_Stage = Processing;

		Element* Entry;
		PRL_RESULT Err = PRL_ERR_SUCCESS;

		cd_list_for_each_entry(Element, Entry, &m_Els.Lst, Lst)
		{
			// Signal operation start
			DoCallback(ProgressMin);

			if (m_Term)
			{
				WRITE_TRACE(DBG_FATAL, "Termination request found.");
				Err = PRL_ERR_DISK_USER_INTERRUPTED;
				goto Error;
			}

			Entry->S = Processing;

			Err = DoExecute(Entry->pHlp->Element);

			if (PRL_FAILED(Err))
			{
				WRITE_TRACE(DBG_FATAL, "Caught error at execution 0x%x", Err);
				goto Error;
			}

			Entry->S = Processed;
			m_Current++;
		}

		m_Stage = Processed;
		return PRL_ERR_SUCCESS;
	Error:
		DoCallback(Err);
		return Err;
	};

	// Commit executed work
	virtual PRL_RESULT Commit()
	{
		if (m_Stage != Processed)
		{
			WRITE_TRACE(DBG_FATAL, "Invalid class state at commit call %s",
						qPrintable(StageStr(m_Stage)));
			return PRL_ERR_INVALID_ACTION_REQUESTED;
		}

		Element* Entry = NULL;
		m_Stage = Backup;
		PRL_RESULT Err = PRL_ERR_SUCCESS;

		// Do requested operations
		cd_list_for_each_entry(Element, Entry, &m_Els.Lst, Lst)
		{
			Entry->S = Backup;

			Err = DoBackup(Entry->pHlp->Element);

			if (PRL_FAILED(Err))
			{
				WRITE_TRACE(DBG_FATAL, "Error backup entry %s with code 0x%x",
							qPrintable(ElToString(Entry->pHlp->Element)),
							Err);
				return Err;
			}

			Entry->S = Backedup;

			Entry->S = Renaming;

			Err = DoRename(Entry->pHlp->Element);

			if (PRL_FAILED(Err))
			{
				WRITE_TRACE(DBG_FATAL, "Error renaming entry %s with code 0x%x",
							qPrintable(ElToString(Entry->pHlp->Element)), Err);
				return Err;
			}

			Entry->S = Renamed;
		}

		m_Stage = Renamed;

		return PRL_ERR_SUCCESS;
	};

	// Finalize executed work
	virtual PRL_RESULT Finalize()
	{
		if (m_Stage != Renamed)
		{
			WRITE_TRACE(DBG_FATAL, "Invalid class state at finalize call %s",
						qPrintable(StageStr(m_Stage)));
			return PRL_ERR_INVALID_ACTION_REQUESTED;
		}

		Element* Entry = NULL;
		PRL_RESULT Err = PRL_ERR_SUCCESS;

		m_Stage = Cleaning;

		// Do requested operations
		cd_list_for_each_entry(Element, Entry, &m_Els.Lst, Lst)
		{
			Entry->S = Cleaning;

			Err = DoFinalize(Entry->pHlp->Element);

			if (PRL_FAILED(Err))
			{
				WRITE_TRACE(DBG_FATAL, "Error finalizing entry %s with code 0x%x",
							qPrintable(ElToString(Entry->pHlp->Element)), Err);
				continue;
			}

			Entry->S = Cleaned;
		}

		// Do completion procedure
		DoComplete(false);

		// Clean list
		Cleanup();

		m_Stage = Cleaned;

		// Allow waiting threads to execute
		Unlock();
		return PRL_ERR_SUCCESS;
	};

	// Rollback operation
	virtual PRL_RESULT Rollback()
	{
		PRL_RESULT Err = PRL_ERR_SUCCESS;

		Element* Entry = NULL;
		Stage CurStage = m_Stage;

		m_Stage = RollingBack;

		// Rollback can be done for renaming and processing
		switch(CurStage)
		{
		case Cleaned:
			// Already finalized, can't be rolled back
			break;
		case Cleaning:
			// Impossible state
			Q_ASSERT(0);
			break;
			// Fallback
		case Renamed:
		case Backup:
			cd_list_for_each_entry(Element, Entry, &m_Els.Lst, Lst)
			{
				// Do not touch files that do not backed up
				if (Entry->S == Processed)
					continue;

				// Faulty backup, do not touch
				if (Entry->S == Backup)
				{
					// Just mark as processed
					Entry->S = Processed;
					continue;
				}

				// File renamed
				if (Entry->S == Renamed)
				{
					Err = UnrollRename(Entry->pHlp->Element);

					if (PRL_FAILED(Err))
					{
						WRITE_TRACE(DBG_FATAL, "Unrolling commit stage 2 failed with code 0x%x",
									Err);
						goto RollbackErr;
					}

					Entry->S = Backedup;
				}

				// Next part written as if() {} to be conformant with previous code

				// Backed up, but not renamed (renaming is similar to backed up here, see Commit())
				if ((Entry->S == Backedup) || (Entry->S == Renaming))
				{
					Err = UnrollBackup(Entry->pHlp->Element);

					if (PRL_FAILED(Err))
					{
						WRITE_TRACE(DBG_FATAL, "Unrolling commit stage 1 failed with code 0x%x",
									Err);
						goto RollbackErr;
					}

					Entry->S = Processed;
				}
			}
			// Fallback to unroll processed
		case Processing:
		case Processed:
			cd_list_for_each_entry(Element, Entry, &m_Els.Lst, Lst)
			{
				// Do not understand what else states can be here
				if ((Entry->S != Processed) &&
					(Entry->S != Processing))
				{
					WRITE_TRACE(DBG_FATAL, "Unroll Processing strange state %s at element %s",
								qPrintable(StageStr(m_Stage)),
								qPrintable(ElToString(Entry->pHlp->Element)));
					continue;
				}

				Err = UnrollExecute(Entry->pHlp->Element);

				if (PRL_FAILED(Err))
				{
					WRITE_TRACE(DBG_FATAL, "Unrolling execute failed with code 0x%x",
								Err);
					goto RollbackErr;
				}

				Entry->S = Adding;
			}
		// Fallback
		case Adding:
			// Do completion procedure
			DoComplete(true);
			// Do nothing, except list clean
			Cleanup();
			Unlock();
			break;
		default:
			WRITE_TRACE(DBG_FATAL, "Invalid class state at rollback call: %s",
						qPrintable(StageStr(CurStage)));
			return PRL_ERR_INVALID_ACTION_REQUESTED;
		}

		return PRL_ERR_SUCCESS;

	RollbackErr:
		// Mark rollback failed
		// Any further work with this manager is impossible
		m_RollbackFailed = true;

		// Do completion procedure
		DoComplete(true);
		// Clean list
		Cleanup();
		// Final unlock
		Unlock();
		return Err;
	};

	// Just access private member
	virtual bool IsTerminated() const
	{
		return m_Term;
	}

	// Try to stop processing
	virtual void SetTerminate()
	{
		m_Term = true;
	}

	// Is something executed
	virtual bool IsActive() const
	{
		return ((m_Stage != NeedInit) && (m_Stage != Cleaned));
	}

	// Wait completion of all state
	virtual void Wait()
	{
		Lock();
		Unlock();
	}

protected:
	// Execute callback with calculation of overall progress
	virtual void DoCallback(PRL_INT32 Progress)
	{
		if (!m_Callback)
			return;

		PRL_INT32 Prog = 0;

		if ((Progress < 0) ||
			(Progress > ProgressMax))
		{
			Prog = Progress;
			goto Exec;
		}

		// Calculate overall progress
		Prog = ProgressMax * m_Current + Progress;
		Prog /= m_ListSize;

		// Start progress from min value
		if (Prog < ProgressMin)
			Prog = ProgressMin;

	Exec:
		m_Term = !m_Callback(Prog, m_Parameter);
	};

	// Get current processing element number
	virtual PRL_UINT32 GetCurrent() const
	{
		return m_Current;
	};

	// Get elements count
	virtual PRL_UINT32 GetCount() const
	{
		return m_ListSize;
	};

protected:
	// Cleanup class members
	virtual void Cleanup()
	{
		Element* Entry;

		// Cycle through all list and delete all
		while(!cd_list_empty(&m_Els.Lst))
		{
			Entry = cd_list_first_entry(&m_Els.Lst, Element, Lst);
			cd_list_del(&Entry->Lst);
			delete Entry->pHlp;
			delete Entry;
		}

		SetDefault();
	};

	// Set defaults
	virtual void SetDefault()
	{
		m_Callback = NULL;
		m_Parameter = NULL;
		m_ListSize = 0;
		m_Stage = NeedInit;
		m_Current = 0;
		m_Term = false;
	};

	// Convert current stage to string
	static QString StageStr(const Stage Stage)
	{
		static const QString undef = QString("Undefined operation");
		static const QString Stages[] = {
			QString("Awaiting initialization"),
			QString("Adding elements"),
			QString("Processing elements"),
			QString("Elements processed"),
			QString("Backing up elements"),
			QString("Elements backed up"),
			QString("Renaming elements"),
			QString("Elements renamed"),
			QString("Cleaning elements"),
			QString("Elements cleaned"),
			QString("Rolling back elements"),
			QString("Stucked")
		};

		if (Stage > Stuck)
			return undef;

		return Stages[Stage];
	};

	virtual void Lock()
	{
		m_Sem.acquire();
	};

	virtual void Unlock()
	{
		m_Sem.release();
	};
};

// Processing file suffix
static const QString s_SfxProcess(".proc");
// Backup (Remove) file suffix
static const QString s_SfxRemove(".rem");

#endif
