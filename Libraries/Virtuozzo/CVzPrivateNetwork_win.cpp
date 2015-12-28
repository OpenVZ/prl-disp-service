/*
 *  Copyright (c) 2015 Parallels IP Holdings GmbH
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
#include <windows.h>
#include <QHostAddress>

#include "CVzHelper.h"
#include "CVzPrivateNetwork.h"
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>

#include "vzcapi/h/vzcapi.h"
#include "vzcapi/h/vpsconfig.h"
#include "vzsrvapi/h/vzsrvapi.h"

static bool isPrivateNetworkDisabled(void)
{
	bool fDisabled = false;
	HKEY hKey;
	LONG ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\SWSoft\\Virtuozzo", 0, KEY_READ|KEY_WOW64_64KEY, &hKey);
	if (ret == ERROR_SUCCESS) {
		DWORD value, type, len;
		len = sizeof(DWORD);
		ret = RegQueryValueEx(hKey, L"PrivateNetworkDisabled",
				      NULL, &type, (LPBYTE)&value, &len);
		if (type == REG_DWORD && value)
			fDisabled = true;
		RegCloseKey(hKey);
	}
	return fDisabled;
}

/* add or remove IP subnets for new network */
static PRL_RESULT add_remove_privnet(
	CPrivateNetwork *pNewNet,
	const CPrivateNetwork *pOldNet,
	CVmEvent *pEvent)
{
	Q_UNUSED(pEvent);
	PRL_RESULT res = PRL_ERR_SUCCESS;
	bool enabled = !isPrivateNetworkDisabled();

	res = CVzHelper::enable_privnet(enabled);

	if (!enabled)
		return res;

	if (pOldNet)
		res = CVzHelper::remove_privnet(pOldNet->getNetworkID());
	if (pNewNet)
		res = CVzHelper::add_privnet(pNewNet->getNetworkID(), pNewNet->getNetAddresses());

	return res;
}

static int generate_new_id(const CPrivateNetworks *pNets)
{
	QSet<unsigned int> known_ids;

	/* add IDs from configured private networks */
	if (pNets) {
		foreach(CPrivateNetwork *pNetwork, pNets->m_lstPrivateNetwork) {
			unsigned int id = pNetwork->getNetworkID();
			if (id != PRL_PRIVNET_GENERATE_ID && !known_ids.contains(id))
				known_ids.insert(id);
		}
	}

	/* search for first available ID */
	for (unsigned int i = 1; i < (unsigned int)known_ids.size() + 2; i++)
		if (!known_ids.contains(i))
			return i;

	return PRL_PRIVNET_GENERATE_ID;
}

PRL_RESULT CVzPrivateNetwork::AddPrivateNetwork(
	CPrivateNetwork *pNet,
	const CPrivateNetworks *pNets, CVmEvent *pEvent)
{
	if (pNet->getNetworkID() == PRL_PRIVNET_GENERATE_ID) {
		unsigned int id = 0;
		// Global legacy network in VZWIN should have zero id
		if (!pNet->isGlobal()) {
			id = generate_new_id(pNets);
			if (id == PRL_PRIVNET_GENERATE_ID) {
				WRITE_TRACE(DBG_FATAL, "Can't find suitable ID for new IP private network '%s'",
					    QSTR2UTF8(pNet->getName()));
				return PRL_ERR_UNEXPECTED;
			}
		}
		pNet->setNetworkID(id);
	}
	return add_remove_privnet(pNet, NULL, pEvent);
}

PRL_RESULT CVzPrivateNetwork::RemovePrivateNetwork(const CPrivateNetwork *pOldNet, CVmEvent *pEvent)
{
	return add_remove_privnet(NULL, pOldNet, pEvent);
}

PRL_RESULT CVzPrivateNetwork::UpdatePrivateNetwork(
	CPrivateNetwork *pNet, const CPrivateNetwork *pOldNet,
	CVmEvent *pEvent)
{
	return add_remove_privnet(pNet, pOldNet, pEvent);
}
