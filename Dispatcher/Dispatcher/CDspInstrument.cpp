///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspInstrument.cpp
///
/// Definition of generic dispatcher instruments
///
/// @author shrike
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

#include "CDspInstrument.h"

namespace Instrument
{
namespace Command
{
///////////////////////////////////////////////////////////////////////////////
// struct Batch

void Batch::addItem(const Batch& another_)
{
	m_transaction << another_.m_transaction;
}

void Batch::addItem(const redo_type& redo_, const undo_type& undo_)
{
	m_transaction << qMakePair(redo_, undo_);
}

PRL_RESULT Batch::execute()
{
	m_undo.clear();
	if (m_transaction.isEmpty())
		return PRL_ERR_UNINITIALIZED;

	PRL_RESULT output = PRL_ERR_SUCCESS;
	foreach (transaction_type::const_reference i, m_transaction)
	{
		output = i.first();
		if (PRL_FAILED(output))
			break;

		if (!i.second.empty())
			m_undo.push_front(i.second);
	}
	if (PRL_FAILED(output))
		rollback();

	return output;
}

void Batch::rollback()
{       
	foreach (undoList_type::const_reference u, m_undo)
	{
		u();
	}
	m_undo.clear();
}

} // namespace Command
} // namespace Instrument

