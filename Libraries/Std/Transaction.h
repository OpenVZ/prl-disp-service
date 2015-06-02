//////////////////////////////////////////////////////////////////////////
///
/// @file Transaction.h
///
/// @brief Simple transaction object
///
/// @author OlegV
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
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
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "Interfaces/ParallelsTypes.h"
#include <prlsdk/PrlTypes.h>

class CSimpleTransaction {
//
// Not so simple actually since it has finalize operation
// allowing commit to fail. So rollback may be called after commit.
// The reasons for such overcomplication are pure historical.
//
public:
typedef bool (*progress_cb)(void* ctx, UINT64 processed, UINT64 total);

	virtual ~CSimpleTransaction() {}
	// Execute operation
	virtual PRL_RESULT	execute(progress_cb cb, void* ctx) = 0;
	// Commit after execute
	virtual PRL_RESULT	commit() = 0;
	// Rollback after execute or commit
	virtual PRL_RESULT	rollback() = 0;
	// Finalize transaction (irreversible)
	virtual PRL_RESULT	finalize() = 0;
};
