///////////////////////////////////////////////////////////////////////////////
///
/// @file CVcmmdInterface
///
/// Implements class VcmmdInterface
///
/// @author mnestratov
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <QString>
#include <libvcmmd/vcmmd.h>

/*
 * This is a helper class to interact with vcmmd daemon.
 * There are three use case:
 * 1. Registering
 * 2. Unregistering
 * 3. Updating
 *
 * Registering/unregistering is always a transaction. I.e.
 * corresponding preparation call should always be followed by
 * commit call in case of success or revert in case of failure.
 * For instance:
 *  - register(init->VM start OK->activate) or
 *  - register(init->VM start FAIL->deinit)
 *  - unregister(deactivate->stop VM OK->unregister) or
 *  - unregister(deactivate->stop VM fails->activate)
 * The class is written in such a way that it requires explicit
 * calling in case of VM operation succeeds, and all the cleanup
 * will be done implicitly in the class destructor.
 *
 */

enum VcmmdState
{
	VcmmdVmActive,
	VcmmdVmInactive,
	VcmmdVmUnregistered
};

class VcmmdInterface
{
public:
	VcmmdInterface(const QString& uuid, VcmmdState state);
	~VcmmdInterface();

	bool init(unsigned long long limit,
			unsigned long long guarantee);
	void deactivate();
	bool update(unsigned long long limit,
			unsigned long long guarantee);
	void commit(VcmmdState state);

private:
	void fixupUuid();
	void activate();
	void deinit();
	QString m_uuid;
	VcmmdState m_state;
	bool m_cleanup;
	int m_err;
	char m_errmsg[80];
};


