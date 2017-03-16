///
/// @file CFirewallHelper.cpp
///
/// Firewall rules composer
///
/// @author myakhin
///
/// Copyright (c) 2010-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

#include <QHostAddress>
#include <QTemporaryFile>
#include "Libraries/PrlNetworking/netconfig.h"
#include <prlcommon/Logging/Logging.h>

#include "CFirewallHelper.h"
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>


#define IP_TABLES						"/sbin/iptables"
#define BRIDGE_TABLES						"/sbin/ebtables"
#define IP_6_TABLES						"/sbin/ip6tables"
#define BRIDGE_FIREWALL_PREFIX			"ip-filter-"


QMutex CFirewallHelper::g_IptablesMutex;
QMutex CFirewallHelper::g_BridgeTablesMutex;

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct Action

struct Action
{
	Action(const char* binary_, const char* name_, const QString& chain_):
		m_chain(chain_), m_name(name_), m_binary(binary_)
	{
	}

	operator QString() const;
	Action& operator()(const QString& value_);
	Action& jump(const QString& value_);
private:
	QString m_jump;
	QString m_chain;
	const char* m_name;
	QStringList m_spec;
	const char* m_binary;
};

Action& Action::jump(const QString& value_)
{
	m_jump = value_;
	return *this;
}

Action& Action::operator()(const QString& value_)
{
	if (!value_.isEmpty())
		m_spec << value_;

	return *this;
}

Action::operator QString() const
{
	QStringList x;
	x << m_binary << m_name << m_chain << m_spec;
	if (!m_jump.isEmpty())
		x << "-j" << m_jump;

	return x.join(" ");
}

///////////////////////////////////////////////////////////////////////////////
// struct Api

struct Api
{
	Api(const char* binary_, const QString& chain_):
		m_chain(chain_), m_binary(binary_)
	{
	}

	QString getSentinel() const;
	QString getDefaultDeny() const;
	QString getPassThrough() const;
	Action getAction(const char* name_) const;
	QStringList getIncomings(const char* action_, const QString& device_) const;
	QStringList getOutgoings(const char* action_, const QString& device_) const;

	static QString getIncomingChain(const QString& device_);
	static QString getOutgoingChain(const QString& device_);
private:
	QString m_chain;
	const char* m_binary;
};

Action Api::getAction(const char* name_) const
{
	return Action(m_binary, name_, m_chain);
}

QString Api::getSentinel() const
{
	return getAction("-A").jump("RETURN");
}

QString Api::getDefaultDeny() const
{
	return getAction("-A").jump("DROP");
}

QString Api::getPassThrough() const
{
	return getAction("-A").jump("RETURN")("-m")("state")("--state")
		("ESTABLISHED,RELATED");
}

QStringList Api::getOutgoings(const char* action_, const QString& device_) const
{
	Api a(m_binary, "FORWARD");
	return QStringList()
		<< a.getAction(action_).jump(m_chain)("-i")(device_)
		<< a.getAction(action_).jump(m_chain)
			("-m")("physdev")("--physdev-is-bridged")
			("--physdev-in")(device_);
}

QStringList Api::getIncomings(const char* action_, const QString& device_) const
{
	Api a(m_binary, "FORWARD");
	return QStringList()
		<< a.getAction(action_).jump(m_chain)("-o")(device_)
		<< a.getAction(action_).jump(m_chain)
			("-m")("physdev")("--physdev-is-bridged")
			("--physdev-out")(device_);
}

QString Api::getIncomingChain(const QString& device_)
{
	static const char PREFIX[] = "bf-incoming-";
	return QString(PREFIX).append(device_).replace(".", "-");
}

QString Api::getOutgoingChain(const QString& device_)
{
	static const char PREFIX[] = "bf-outgoing-";
	return QString(PREFIX).append(device_).replace(".", "-");
}

} // namespace

CFirewallHelper::CFirewallHelper(const SmartPtr<CVmConfiguration>& pVmConfig,
								 bool bDeleteRules)
: m_pVmConfig(pVmConfig)
{
#ifdef _LIN_
	ComposeIptablesRules(bDeleteRules);
	ComposeBridgeTablesRules(bDeleteRules);
#else
	Q_UNUSED(bDeleteRules);
#endif
}

void CFirewallHelper::AddRule(CVmNetFirewallRule *pRule, const QString &qsRuleAct,
	const QString &qsChainName, PRL_FIREWALL_DIRECTION nDirection)
{
	bool bIPv6 = ( pRule->getLocalNetAddress().contains(":")
			|| pRule->getRemoteNetAddress().contains(":") );
	bool bBoth = ( pRule->getLocalNetAddress().isEmpty() && pRule->getRemoteNetAddress().isEmpty() );

	QString qsProtocol = ! pRule->getProtocol().isEmpty()
		? QString(" -p %1 ").arg(pRule->getProtocol()) : QString();

	// source port
	QString qsSPort;
	if (nDirection == PFD_INCOMING && pRule->getRemotePort())
		qsSPort = QString(" --sport %1 ").arg(pRule->getRemotePort());
	else if (nDirection == PFD_OUTGOING && pRule->getLocalPort())
		qsSPort = QString(" --sport %1 ").arg(pRule->getLocalPort());

	// destination port
	QString qsDPort;
	if (nDirection == PFD_INCOMING && pRule->getLocalPort())
		qsDPort = QString(" --dport %1 ").arg(pRule->getLocalPort());
	else if (nDirection == PFD_OUTGOING && pRule->getRemotePort())
		qsDPort = QString(" --dport %1 ").arg(pRule->getRemotePort());

	// source IP
	QString qsSrc;
	if (nDirection == PFD_INCOMING && ! pRule->getRemoteNetAddress().isEmpty())
		qsSrc = QString(" -s %1 ").arg(pRule->getRemoteNetAddress());
	else if (nDirection == PFD_OUTGOING && ! pRule->getLocalNetAddress().isEmpty())
		qsSrc = QString(" -s %1 ").arg(pRule->getLocalNetAddress());

	// destination IP
	QString qsDst;
	if (nDirection == PFD_INCOMING && ! pRule->getLocalNetAddress().isEmpty())
		qsDst = QString(" -d %1 ").arg(pRule->getLocalNetAddress());
	else if (nDirection == PFD_OUTGOING && ! pRule->getRemoteNetAddress().isEmpty())
		qsDst = QString(" -d %1 ").arg(pRule->getRemoteNetAddress());

	if ( ! bIPv6 || bBoth )
	{
		QString qsRule = Api(IP_TABLES, qsChainName).getAction("-A")(qsSrc)
					(qsDst)(qsProtocol)(qsSPort)(qsDPort).jump(qsRuleAct);
		if ( ! m_lstRules.contains(qsRule) )
			m_lstRules += qsRule;
	}

	if ( bIPv6 || bBoth )
	{
		QString qsRule = Api(IP_6_TABLES, qsChainName).getAction("-A")(qsSrc)
					(qsDst)(qsProtocol)(qsSPort)(qsDPort).jump(qsRuleAct);
		if ( ! m_lstRules.contains(qsRule) )
			m_lstRules += qsRule;
	}

}

void CFirewallHelper::AddDirectionRules(CVmGenericNetworkAdapter* pAdapter,
		const QString &chain_name, PRL_FIREWALL_DIRECTION nDirection)
{

	CVmNetFirewallDirection *pDirection;

	if (nDirection == PFD_INCOMING)
		pDirection = pAdapter->getFirewall()->getIncoming()->getDirection();
	else
		pDirection = pAdapter->getFirewall()->getOutgoing()->getDirection();

	Api a(IP_TABLES, chain_name), a6(IP_6_TABLES, chain_name);
	// allow established connections to pass through
	// should be before actual rules
	m_lstRules += a.getPassThrough();
	m_lstRules += a6.getPassThrough();

	// setup rules
	QString qsRuleAct = pDirection->getDefaultPolicy() == PFP_ACCEPT ?  "DROP" : "RETURN";
	foreach(CVmNetFirewallRule* pRule, pDirection->getFirewallRules()->m_lstFirewallRules)
		AddRule(pRule, qsRuleAct, chain_name, nDirection);

	// setup default policy
	// do not setup ACCEPT rules as they allow to pass packets, which should
	// be dropped by DROP rules for other VMs
	if (pDirection->getDefaultPolicy() == PFP_DENY) {
		m_lstRules += a.getDefaultDeny();

		// for IPv6, we need to pass ND messages - otherwise no
		// connections will be possible even on allowed rules
		m_lstRules += a6.getAction("-A")("-p")("icmpv6")("--icmpv6-type")
				("133").jump("RETURN");
		m_lstRules += a6.getAction("-A")("-p")("icmpv6")("--icmpv6-type")
				("134").jump("RETURN");
		m_lstRules += a6.getAction("-A")("-p")("icmpv6")("--icmpv6-type")
				("135").jump("RETURN");
		m_lstRules += a6.getAction("-A")("-p")("icmpv6")("--icmpv6-type")
				("136").jump("RETURN");
		m_lstRules += a6.getDefaultDeny();
	}
	m_lstRules += a.getSentinel();
	m_lstRules += a6.getSentinel();
}

void CFirewallHelper::AddRules(CVmGenericNetworkAdapter* pAdapter, const QString &veth_name)
{
	QString x;
	x = Api::getIncomingChain(veth_name);
	{
		Api a(IP_TABLES, x), a6(IP_6_TABLES, x);
		// Set up firewall rules
		m_lstRules += a.getAction("-N");
		m_lstRules += a6.getAction("-N");
		m_lstRules += a.getIncomings("-I", veth_name);
		m_lstRules += a6.getIncomings("-I", veth_name);
		AddDirectionRules(pAdapter, x, PFD_INCOMING);
	}
	x = Api::getOutgoingChain(veth_name);
	{
		Api a(IP_TABLES, x), a6(IP_6_TABLES, x);
		// Set up firewall rules
		m_lstRules += a.getAction("-N");
		m_lstRules += a6.getAction("-N");
		m_lstRules += a.getOutgoings("-I", veth_name);
		m_lstRules += a6.getOutgoings("-I", veth_name);
		AddDirectionRules(pAdapter, x, PFD_OUTGOING);
	}
}

void CFirewallHelper::AddBridgeRules(CVmGenericNetworkAdapter* pAdapter, const QString &veth_name)
{
	QString chain_name = BRIDGE_FIREWALL_PREFIX + veth_name;
	chain_name.replace(".", "-");

	/* rules to create user-defined chain for veth device */
	QString qsRule;
	qsRule = BRIDGE_TABLES " -N " + chain_name + " -P DROP";
	m_lstBridgeRules += qsRule;

	qsRule = BRIDGE_TABLES " -A FORWARD -i " + veth_name + " -j " + chain_name;
	m_lstBridgeRules += qsRule;

	qsRule = BRIDGE_TABLES " -A INPUT -i " + veth_name + " -j " + chain_name;
	m_lstBridgeRules += qsRule;

	/* prepare MAC=IP pairs for the device */
	QString qsAmongSrcValue;
	foreach (QString ip_mask, pAdapter->getNetAddresses()) {
		QString ip;
		if (!NetworkUtils::ParseIpMask(ip_mask, ip))
			continue;

		if (QHostAddress(ip).protocol() == QAbstractSocket::IPv4Protocol) {
			if (!qsAmongSrcValue.isEmpty())
				qsAmongSrcValue += ",";
			QString mac = pAdapter->getMacAddress();
			if (!PrlNet::convertMacAddress(mac))
				continue;
			qsAmongSrcValue += mac + "=" + ip;
		} else if (QHostAddress(ip).protocol() == QAbstractSocket::IPv6Protocol) {
			/* add IPv6 rule */
			qsRule = BRIDGE_TABLES " -A " + chain_name + " -p ip6 --ip6-src " +
				ip + " -j ACCEPT";
			m_lstBridgeRules += qsRule;
		}
	}

	if (pAdapter->isConfigureWithDhcp()) {
		m_lstBridgeRules += (QString)(BRIDGE_TABLES " -A " +
							chain_name + " -p ip4 -j ACCEPT");
		m_lstBridgeRules += (QString)(BRIDGE_TABLES " -A " +
							chain_name + " -p ARP -j ACCEPT");
	} else if (pAdapter->isConfigureWithDhcpIPv6()) {
		m_lstBridgeRules += (QString)(BRIDGE_TABLES " -A " +
							chain_name + " -p ip6 -j ACCEPT");
	}

	/* nothing to do if no IPv4 addresses is specified for the device */
	if (qsAmongSrcValue.isEmpty())
		return;

	/* add IPv4/ARP filtering rule */
	qsRule = BRIDGE_TABLES " -A " + chain_name + " --among-src " +
		qsAmongSrcValue + " -j ACCEPT";
	m_lstBridgeRules += qsRule;
}

void CFirewallHelper::DeleteRules(const QString &veth_name)
{
	QString x;
	x = Api::getIncomingChain(veth_name);
	{
		Api a(IP_TABLES, x), a6(IP_6_TABLES, x);
		m_lstCleanupRules += a.getAction("-F");
		m_lstCleanupRules += a6.getAction("-F");
		m_lstCleanupRules += a.getIncomings("-D", veth_name);
		m_lstCleanupRules += a6.getIncomings("-D", veth_name);
		m_lstCleanupRules += a.getAction("-X");
		m_lstCleanupRules += a6.getAction("-X");
	}
	x = Api::getOutgoingChain(veth_name);
	{
		Api a(IP_TABLES, x), a6(IP_6_TABLES, x);
		m_lstCleanupRules += a.getAction("-F");
		m_lstCleanupRules += a6.getAction("-F");
		m_lstCleanupRules += a.getOutgoings("-D", veth_name);
		m_lstCleanupRules += a6.getOutgoings("-D", veth_name);
		m_lstCleanupRules += a.getAction("-X");
		m_lstCleanupRules += a6.getAction("-X");
	}
}

void CFirewallHelper::DeleteBridgeRules(const QString &veth_name)
{
	QString chain_name = BRIDGE_FIREWALL_PREFIX + veth_name;
	chain_name.replace(".", "-");

	QString qsRule = BRIDGE_TABLES " -F " + chain_name;
	m_lstCleanupBridgeRules += qsRule;

	qsRule = BRIDGE_TABLES " -D FORWARD -i " + veth_name + " -j " + chain_name;
	m_lstCleanupBridgeRules += qsRule;

	qsRule = BRIDGE_TABLES " -D INPUT -i " + veth_name + " -j " + chain_name;
	m_lstCleanupBridgeRules += qsRule;

	qsRule = BRIDGE_TABLES " -X " + chain_name;
	m_lstCleanupBridgeRules += qsRule;
}

void CFirewallHelper::ComposeIptablesRules(bool bDeleteRules)
{
	if ( ! m_pVmConfig )
		// Unexpected error
		return;

	foreach(CVmGenericNetworkAdapter* pAdapter, m_pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
	{
		DeleteRules(pAdapter->getHostInterfaceName());
		if (!bDeleteRules && pAdapter->getFirewall()->isEnabled())
			AddRules(pAdapter, pAdapter->getHostInterfaceName());
	}
}

void CFirewallHelper::ComposeBridgeTablesRules(bool bDeleteRules)
{
	if ( ! m_pVmConfig )
		// Unexpected error
		return;

	foreach(CVmGenericNetworkAdapter* pAdapter, m_pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
	{
		/* setup bridge tables for Containers with bridged networking
		 * only - for VMs necessary filtering is performed by vme device
		 * code */
		if (m_pVmConfig->getVmType() == PVT_VM)
			continue;

		/* skip host-routed device */
		if (pAdapter->isVenetDevice())
			continue;

		DeleteBridgeRules(pAdapter->getHostInterfaceName());
		if (!bDeleteRules && pAdapter->getPktFilter()->isPreventIpSpoof() &&
				pAdapter->getNetAddresses().size() != 0)
			AddBridgeRules(pAdapter, pAdapter->getHostInterfaceName());
	}
}

PRL_RESULT CFirewallHelper::ExecuteRules(QStringList &lstCleanupRules,
		QStringList &lstRules)
{
	m_qsErrorMessage.clear();

	// Cleanup first - ignore errors if such
	foreach(QString qsRule, lstCleanupRules)
	{
		WRITE_TRACE(DBG_FATAL, "Firewall rule: %s", QSTR2UTF8(qsRule));

		QProcess	proc;
		proc.start(qsRule);
		if ( ! proc.waitForStarted() )
		{
			WRITE_TRACE(DBG_FATAL, "Could not start firewall utility !");
			return PRL_ERR_COULD_NOT_START_FIREWALL_TOOL;
		}
		proc.waitForFinished(-1);
	}

	// Set new rules - try all, accumulate errors
	foreach(QString qsRule, lstRules)
	{
		WRITE_TRACE(DBG_FATAL, "Firewall rule: %s", QSTR2UTF8(qsRule));

		QProcess	proc;
		proc.start(qsRule);
		if ( ! proc.waitForStarted() )
		{
			WRITE_TRACE(DBG_FATAL, "Could not start firewall utility !");
			return PRL_ERR_COULD_NOT_START_FIREWALL_TOOL;
		}
		proc.waitForFinished(-1);

		if (proc.exitStatus() != QProcess::NormalExit ||
			proc.exitCode() != 0)
		{
			QString qsErrMsg = UTF8_2QSTR(proc.readAllStandardOutput())
				+ UTF8_2QSTR(proc.readAllStandardError());
			if (!qsErrMsg.isEmpty())
			{
				m_qsErrorMessage += "ERROR: " + qsRule + "\n";
				m_qsErrorMessage += qsErrMsg;
			}
		}
	}

	if ( ! m_qsErrorMessage.isEmpty() )
		return PRL_ERR_FIREWALL_TOOL_EXECUTED_WITH_ERROR;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CFirewallHelper::ExecuteIptables()
{
	PRL_RESULT res;

	QMutexLocker _lock(&g_IptablesMutex);

	WRITE_TRACE(DBG_WARNING, "Start setting basic firewall rules...");

	res = ExecuteRules(m_lstCleanupRules, m_lstRules);

	WRITE_TRACE(DBG_WARNING, "Finished setting basic firewall rules: %s", QSTR2UTF8(m_qsErrorMessage));

	return res;
}

PRL_RESULT CFirewallHelper::ExecuteBridgeTables()
{
	PRL_RESULT res;

	QMutexLocker _lock(&g_BridgeTablesMutex);

	if (m_lstCleanupBridgeRules.isEmpty() && m_lstBridgeRules.isEmpty())
		return PRL_ERR_SUCCESS;

	WRITE_TRACE(DBG_WARNING, "Start setting IP anti-spoof rules...");

	/* Try to make an atomic update of bridge tables */
	QTemporaryFile qfTempFile;
	if (qfTempFile.open()) {
		qfTempFile.close();
		QString qsAtomicCmd = BRIDGE_TABLES " --atomic-file " +
					qfTempFile.fileName();
		QString qsAtomicSaveCmd = qsAtomicCmd + " --atomic-save";
		QString qsAtomicCommitCmd = qsAtomicCmd + " --atomic-commit";
		m_lstCleanupBridgeRules.replaceInStrings(BRIDGE_TABLES, qsAtomicCmd);
		m_lstCleanupBridgeRules.prepend(qsAtomicSaveCmd);
		if (!m_lstBridgeRules.isEmpty()) {
			m_lstBridgeRules.replaceInStrings(BRIDGE_TABLES, qsAtomicCmd);
			m_lstBridgeRules.append(qsAtomicCommitCmd);
		} else
			m_lstCleanupBridgeRules.append(qsAtomicCommitCmd);
	}

	res = ExecuteRules(m_lstCleanupBridgeRules, m_lstBridgeRules);

	WRITE_TRACE(DBG_WARNING, "Finished setting IP anti-spoof rules: %s", QSTR2UTF8(m_qsErrorMessage));

	return res;
}

PRL_RESULT CFirewallHelper::Execute()
{
	PRL_RESULT res;

	res = ExecuteIptables();
	res |= ExecuteBridgeTables();

	return res;
}
