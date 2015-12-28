///
/// @file CFirewallHelper.h
///
/// Firewall rules composer
///
/// @author myakhin
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
///////////////////////////////////////////////////////////////////////////////

#ifndef FIREWALL_HELPER_H
#define FIREWALL_HELPER_H

#include <prlcommon/Std/SmartPtr.h>
#include "XmlModel/VmConfig/CVmConfiguration.h"


class CFirewallHelper
{
public:

	CFirewallHelper(const SmartPtr<CVmConfiguration>& pVmConfig,
					bool bDeleteRules = false);

	PRL_RESULT Execute();

	QString GetErrorMessage() const { return m_qsErrorMessage; }

private:

	void AddRule(CVmNetFirewallRule *pRule,
				const QString &qsRuleAct,
				const QString &qsChainName,
				PRL_FIREWALL_DIRECTION nDirection);
	void AddDirectionRules(CVmGenericNetworkAdapter* pAdapter,
				const QString &chain_name,
				PRL_FIREWALL_DIRECTION nDirection);
	void AddRules( CVmGenericNetworkAdapter* pAdapter,
				const QString &veth_name);
	void AddBridgeRules( CVmGenericNetworkAdapter* pAdapter,
				const QString &veth_name);
	void DeleteRules(const QString &veth_name);
	void DeleteBridgeRules(const QString &veth_name);

	void ComposeIptablesRules(bool bDeleteRules);
	void ComposeBridgeTablesRules(bool bDeleteRules);

	PRL_RESULT ExecuteRules(QStringList &lstCleanupRules,
			QStringList &lstRules);
	PRL_RESULT ExecuteIptables();
	PRL_RESULT ExecuteBridgeTables();

	SmartPtr<CVmConfiguration> m_pVmConfig;
	QStringList	m_lstCleanupRules;
	QStringList	m_lstCleanupBridgeRules;
	QStringList	m_lstRules;
	QStringList	m_lstBridgeRules;
	QString		m_qsErrorMessage;

	static QMutex g_IptablesMutex;
	static QMutex g_BridgeTablesMutex;

};

#endif	// FIREWALL_HELPER_H
