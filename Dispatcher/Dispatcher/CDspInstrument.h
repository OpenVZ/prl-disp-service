///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspInstrument.h
///
/// Declaration of generic dispatcher instruments
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

#ifndef __CDSPINSTRUMENT_H__
#define __CDSPINSTRUMENT_H__

#include <QList>
#include <QPair>
#include <prlsdk/PrlErrors.h>
#include <boost/function.hpp>

namespace Instrument
{
namespace Command
{
///////////////////////////////////////////////////////////////////////////////
// struct Base

struct Base
{
	typedef PRL_RESULT result_type;
};

///////////////////////////////////////////////////////////////////////////////
// struct Batch

struct Batch
{
	typedef boost::function<void ()> undo_type;
	typedef boost::function<PRL_RESULT ()> redo_type;

	PRL_RESULT execute();
	void rollback();

	void addItem(const Batch& another_);
	void addItem(const redo_type& redo_)
	{
		addItem(redo_, undo_type());
	}
	void addItem(const redo_type& redo_, const undo_type& undo_);

private:
	typedef QList<undo_type> undoList_type;
	typedef QList<QPair<redo_type, undo_type> > transaction_type;

	undoList_type m_undo;
	transaction_type m_transaction;
};

} // namespace Command

namespace Chain
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T>
struct Unit
{
	typedef T request_type;
	typedef PRL_RESULT result_type;
	typedef boost::function<PRL_RESULT (const request_type& )> redo_type;

	Unit()
	{
	}
	explicit Unit(const redo_type& redo_): m_redo(redo_)
	{
	}

	PRL_RESULT operator()(const request_type& request_)
	{
		return m_redo.empty() ? PRL_ERR_SUCCESS : m_redo(request_);
	}

private:
	redo_type m_redo;
};

} // namespace Chain
} // namespace Instrument

#endif // __CDSPINSTRUMENT_H__

