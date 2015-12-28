/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <QString>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlsdk/PrlOses.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include "UuidMap.h"
#include "CVzHelper.h"
#include "PrlIOStructs.h"
#include "CVzHardDisk.h"
#include "CVzTemplateHelper.h"
#include <numeric>
#include "vzcapi/h/vzcapi.h"
#include "vzcapi/h/vpsconfig.h"
#include "vzsrvapi/h/vzsrvapi.h"
#include <tchar.h>

QMutex CVzHelper::s_mtxEnvUuidMap(QMutex::Recursive);
QHash< QString, unsigned int > CVzHelper::s_envUuidMap;

static HMODULE hVzcapi = NULL;
static HMODULE hVzsrvapi = NULL;

#define ENUM_SYMBOLS							\
	SYM_DECL(void,  VZSrvApiInit, (void));				\
	SYM_DECL(void,  VZC_LogInit, (const char *));			\
	SYM_DECL(void,  VZC_GenerateLogContext, (void));		\
	SYM_DECL(void,  VZC_NewActivity, (PVZC_JOBID));			\
	SYM_DECL(PVOID, VZC_AllocCommand, (VZC_COMMAND_TYPE));		\
	SYM_DECL(void,  VZC_FreeCommand, (PVOID));			\
	SYM_DECL(VZC_RESULT, VZC_ExecCommand, (VZC_JOBID, PVOID));	\
	SYM_DECL(PCSTR, VZC_StrState, (DWORD));				\
	SYM_DECL(PCSTR, VZC_StrSubstate, (DWORD));			\
	SYM_DECL(void,	VZC_RegisterCommandCallback, (VZC_COMMAND_CALLBACK)); \
	SYM_DECL(void,	VZC_ReadConfig, (PVZC_CONF));			\
	SYM_DECL(PVZCFG,VZCfg_ReadConfigVps, (DWORD));			\
	SYM_DECL(PVZCFG,VZCfg_ReadConfigFile, (PCWSTR));			\
	SYM_DECL(PVZCFG,VZCfg_ReadCreateConfigFile, (PCWSTR));			\
	SYM_DECL(BOOL,	VZCfg_CopyConfig, (PVZCFG src, PVZCFG dst));	\
	SYM_DECL(BOOL,	VZCfg_DeleteConfig, (PVZCFG cfg));		\
	SYM_DECL(void,	VZCfg_FreeConfig, (PVZCFG cfg));		\
	SYM_DECL(void,  VZC_GetVpsPrivatePathW, (PWSTR, ULONG, ULONG, PCWSTR)); \
	SYM_DECL(void,  VZC_GetVpsConfPathW, (PWSTR, ULONG, ULONG)); \
	SYM_DECL(BOOL,	VZCfg_GetValueStrListW, (PVZCFG, const char *, PWSTR **, int *)); \
	SYM_DECL(BOOL,	VZCfg_GetValueStrW, (PVZCFG, const char *, PWSTR  *)); \
	SYM_DECL(BOOL,	VZCfg_GetValueDword, (PVZCFG, const char *, DWORD  *)); \
	SYM_DECL(BOOL,	VZCfg_SetValueDword, (PVZCFG, const char *, DWORD)); \
	SYM_DECL(const WCHAR *, VZCfg_GetValue, (PVZCFG, const char *)); \
	SYM_DECL(BOOL,	VZCfg_SetValue,	(PVZCFG, const char *, const WCHAR *)); \
	SYM_DECL(BOOL,	VZCfg_DeleteValue, (PVZCFG, const char *)); \
	SYM_DECL(BOOL,	VZCfg_GetValueQword, (PVZCFG, const char *, ULONGLONG  *)); \
	SYM_DECL(BOOLEAN, VZC_GetCurrentLog, (PCHAR pcBuff, ULONG dwSizeIn, PULONG pdwSizeOut)); \
	SYM_DECL(VOID,	VZC_ParseLogEntry, (PCHAR pcBuff, ULONG dwSize, PLOG_ENTRY ptLEntry)); \


#define SYM_DECL(res, name, args) \
	typedef res  (VZCAPI * type##name) args

ENUM_SYMBOLS;

#undef	SYM_DECL
#define SYM_DECL(res, name, args) \
	static type##name p##name

ENUM_SYMBOLS;

static VZC_CONF g_vzConf;

#include "CVzCmd_win.h"

static PRL_RESULT get_firewall(CVmGenericNetworkAdapter *pNet, unsigned int envid);
static PRL_RESULT set_firewall(ULONG envId, SmartPtr<CVmConfiguration> &pConfig,
			       SmartPtr<CVmConfiguration> &pOldConfig, QString& errMsg);

static QString get_error_info()
{
	char _buff[0x10000];
	char *buff = _buff;
	ULONG len, out_len, msg_len;
	QString qs;

	if (!pVZC_GetCurrentLog(buff, sizeof(_buff), &out_len))
		return qs;

	for (len=0; len < out_len; len += msg_len) {
		LOG_ENTRY tLEntry;

		msg_len = strlen(buff + len)+1;
		pVZC_ParseLogEntry(buff + len, msg_len, &tLEntry);

		if (!tLEntry.pSeverity)
			continue;

		if (tLEntry.uSeverity <= VZC_R_WARNING)
			continue;

		qs += "\n";
		qs += tLEntry.pMsg;
	}

	if (!qs.isEmpty())
		WRITE_TRACE(DBG_FATAL, "VZWIN ERROR: %s", QSTR2UTF8(qs));

	return qs;
}

static int  get_free_env_id(unsigned int * id)
{
	static volatile LONG ctid = 100 - 1;
	ULONG next_ctid;

	do {
		next_ctid = (ULONG)InterlockedIncrement(&ctid);

		VzCmd<VZC_QUERY_VPS_COMMAND> pCmd(VZC_CMD_QUERY_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;

		pCmd->vpsIdList = &next_ctid;
		pCmd->vpsIdListCount = 1;
		pCmd->vpsInfoClass = STATUS_VPS_INFO_CLASS;

		PRL_RESULT res = pCmd.Execute();
		if (res == PRL_ERR_SUCCESS && pCmd->vpsInfoListCount == 1)
			next_ctid = 0;
	} while (next_ctid == 0);

	*id = (unsigned int) next_ctid;

	return PRL_ERR_SUCCESS;
}

static QString get_env_xml_config_path(SmartPtr<CVmConfiguration> &pConfig)
{
	QString cfg = pConfig->getVmIdentification()->getHomePath();

	if (cfg.isEmpty())
		return QString();
	return (cfg + "/ve.xml");
}

static bool parse_ipaddr(const char * str, PVZC_IPADDR addr)
{
	bool success = false;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints = { 0 };
	DWORD dwRet;
	struct sockaddr_in * sa_in;
	struct sockaddr_in6 * sa_in6;

	ZeroMemory(addr, sizeof(*addr));

	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_protocol = IPPROTO_TCP;

	dwRet = getaddrinfo(str, NULL, &hints, &ptr);
	if (dwRet != 0)
		return false;

	switch (ptr->ai_family) {
	case AF_INET:
		sa_in = (struct sockaddr_in *)ptr->ai_addr;
		addr->family = VZC_AF_INET;
		addr->ipv4_addr = sa_in->sin_addr.S_un.S_addr;
		success = true;
		break;
	case AF_INET6:
		sa_in6 = (struct sockaddr_in6 *)ptr->ai_addr;
		addr->family = VZC_AF_INET6;
		memcpy(addr->ipv6_addr_byte, sa_in6->sin6_addr.u.Byte, 16);
		success = true;
		break;
	default:
		break;
	}

	freeaddrinfo(ptr);

	return success;
}

template <typename T>
static void format_ipaddr(QString& qs, const T& addr, boolean v6)
{
	if (!v6) {
		if (addr.netAddrV4 == 0)
			qs = "*";
		else
			qs.sprintf("%d.%d.%d.%d/%d",
				   (addr.netAddrV4)       & 255,
				   (addr.netAddrV4 >> 8)  & 255,
				   (addr.netAddrV4 >> 16) & 255,
				   (addr.netAddrV4 >> 24) & 255,
				   addr.ones);
	} else {
		boolean isZero = true;
		qs = "";
		for (int i=0; i < 16; i+=2) {
			// network byte order
			unsigned short a = addr.netAddrV6[i+1] | (addr.netAddrV6[i] << 8);
			if (a)
				isZero = false;
			if (i != 0)
				qs += ":";
			qs += QString().sprintf("%X", (int)a);
		}
		qs += QString().sprintf("/%d", addr.ones);
		if (isZero)
			qs = "*";
	}
}

static bool parse_ipaddrmask(const char* str, PVZC_IPADDR pAddr, bool bHost)
{
	bool success = false;
	char* maskStr, maskDelim;
	VZC_IPADDR addr;
	VZC_IPADDR mask;
	int i, bits, zero;
	char ch;

	maskStr = strchr((char*)str, '/');
	if (maskStr) {
		maskDelim = *str;
		*maskStr++ = 0;
	}

	if (!parse_ipaddr(str, &addr))
		goto out;
	if (addr.family == VZC_AF_INET &&
	    sscanf(str, "%d.%d.%d.%d%c", &i, &i, &i, &i, &ch) != 4)
		goto out;

	if (maskStr) {
		if (sscanf(maskStr, "%d%c", &bits, &ch) == 1) {
			if (addr.family == VZC_AF_INET) {
				if (bHost && (bits <= 0 || bits >= 32) ||
					!bHost && (bits < 0 || bits > 32))
					goto out;
				addr.mask = bits;
			} else if (addr.family == VZC_AF_INET6) {
				if (bHost && (bits <= 0 || bits >= 128) ||
					 !bHost && (bits < 0 || bits > 128))
					goto out;
				addr.mask = bits;
			} else
				goto out;
		} else {
			ULONG umask;
			if (!parse_ipaddr(maskStr, &mask))
				goto out;

			/* allow d.d.d.d/d.d.d.d only  for ipv4 */
			if (addr.family != VZC_AF_INET)
				goto out;
			if (addr.family != mask.family)
				goto out;
			umask = ntohl(mask.ipv4_addr);
			for (zero=bits=i=0; i < 32; ++i) {
				if ((1UL << (31-i)) & umask) {
					if (zero)
						goto out;
					++bits;
				} else {
					zero = TRUE;
				}
			}
			addr.mask = bits;
		}
	} else {
		if (addr.family == VZC_AF_INET)
			addr.mask = bHost ? 24 : 32;
		if (addr.family == VZC_AF_INET6)
			addr.mask = bHost ? 24 : 128;  // todo: what is ipv6 default mask?
	}

	success = true;
	*pAddr = addr;

out:
	if (maskStr)
		*--maskStr = maskDelim;

	return success;
}

bool CVzHelper::initialized()
{
	return (hVzcapi != NULL);
}

CVzHelper::CVzHelper()
{
	WRITE_TRACE(DBG_FATAL, "CVzHelper::CVzHelper()");
	init_lib();

	// enable global firewall by default
	if (hVzcapi) {
		VzCmd<VZC_SET_NODE_COMMAND> pCmd(VZC_CMD_SET_NODE);
		VZC_FIREWALL_STATUS_INFO info;
		info.firewallEnabled = TRUE;
		pCmd->infoClass = FIREWALL_STATUS_NODE_INFO_CLASS;
		pCmd->infoEntry = &info;
		pCmd.Execute();
	}
}

CVzHelper::~CVzHelper()
{
}

QString CVzHelper::get_cpu_mask(const SmartPtr<CVmConfiguration> &pVmConfig, bool bOvercommit)
{
	Q_UNUSED(bOvercommit);
	return pVmConfig->getVmHardwareList()->getCpu()->getCpuMask();
}

QString CVzHelper::getVzPrivateDir(void)
{
	return UTF16_2QSTR(g_vzConf.vzConfPrivate);
}

QString CVzHelper::getVzRootDir(void)
{
	return UTF16_2QSTR(g_vzConf.vzConfRoot);
}

QString CVzHelper::getVzTemplateDir(void)
{
	return UTF16_2QSTR(g_vzConf.vzConfTemplate);
}

QString CVzHelper::getVzInstallDir(void)
{
	return UTF16_2QSTR(g_vzConf.vzConfInstall);
}

int CVzHelper::init_lib()
{
	WRITE_TRACE(DBG_FATAL, "Initialize vzcapi");

	// is it initialized already?
	if (hVzcapi) {
		WRITE_TRACE(DBG_FATAL, "Already initialized");
		return PRL_ERR_SUCCESS;
	}

	hVzcapi = LoadLibraryA("vzcapi.dll");
	if (!hVzcapi) {
		WRITE_TRACE(DBG_FATAL, "==> LoadLibrary(\"vzcapi.dll\") = %u", GetLastError());
		return PRL_ERR_API_WASNT_INITIALIZED;
	}

	hVzsrvapi = LoadLibraryA("vzsrvapi.dll");
	if (!hVzsrvapi) {
		WRITE_TRACE(DBG_FATAL, "==> LoadLibrary(\"vzsrvapi.dll\") = %u", GetLastError());
		FreeLibrary(hVzcapi);
		hVzcapi = NULL;
		return PRL_ERR_API_WASNT_INITIALIZED;
	}

#undef	SYM_DECL
#define SYM_DECL(res, name, args)						\
	do {									\
		p##name = (type##name) GetProcAddress(hVzcapi, #name);		\
		if (p##name == NULL) {						\
			p##name = (type##name) GetProcAddress(hVzsrvapi, #name); \
		}								\
		if (p##name == NULL) {						\
			WRITE_TRACE(DBG_FATAL, "==> GetProcAddress(\"%hs\") = %u", #name, GetLastError()); \
			FreeLibrary(hVzsrvapi);					\
			FreeLibrary(hVzcapi);					\
			hVzcapi = NULL;						\
			return PRL_ERR_API_WASNT_INITIALIZED;			\
		}								\
	} while (0)

	ENUM_SYMBOLS;

	pVZSrvApiInit();
	pVZC_LogInit("prl_dsp_service");
	pVZC_GenerateLogContext();

	pVZC_ReadConfig(&g_vzConf);

	return PRL_ERR_SUCCESS;
}

int CVzHelper::lock_envid(unsigned int *id, const QString &dst)
{
	Q_UNUSED(id);
	Q_UNUSED(dst);
	return -1;
}

void CVzHelper::unlock_envid(unsigned int id)
{
	Q_UNUSED(id);
}

int CVzHelper::create_ctx(const QString&, const QString&, int, bool)
{
	return 0;
}

int CVzHelper::update_uuidmap(const QString &uuid, bool del)
{
	int ret;
	unsigned int id;
	Uuid_t u;

	if (!is_vz_running())
		return 0;

	Uuid::dump(uuid, u);
	id = get_envid_by_uuid(uuid);

	ret = ::update_uuidmap(u, id, del);

	return ret;
}

int CVzHelper::init_uuidmap()
{
	if (!is_vz_running())
		return 0;

	return ::init_uuidmap();
}



int CVzHelper::is_vz_running()
{
	HANDLE h;
	h = CreateFileW(L"\\\\.\\vzkrnl", 0,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		CloseHandle(h);
		return 1;
	}
	return 0;
}

PRL_STAT_NET_TRAFFIC *CVzHelper::get_net_stat_vm(const QString &uuid)
{
	if (hVzcapi == NULL)
		return NULL;

	unsigned int id = get_envid_by_uuid(uuid);
	if (id == 0)
		return NULL;

	return get_net_stat(id);
}

PRL_STAT_NET_TRAFFIC *CVzHelper::get_net_stat(unsigned int id)
{
	ULONG ctid = (ULONG) id;
	VzCmd<VZC_QUERY_VPS_COMMAND> pCmd(VZC_CMD_QUERY_VPS);
	VzCmd<VZC_QUERY_VPS_COMMAND> pCmdV6(VZC_CMD_QUERY_VPS);
	if (pCmd == NULL || pCmdV6 == NULL)
		return NULL;

	QScopedPointer<PRL_STAT_NET_TRAFFIC> n(new PRL_STAT_NET_TRAFFIC());

	pCmd->vpsIdList = &ctid;
	pCmd->vpsIdListCount = 1;
	pCmd->vpsInfoClass = NETSTATS_VPS_INFO_CLASS;

	pCmdV6->vpsIdList = &ctid;
	pCmdV6->vpsIdListCount = 1;
	pCmdV6->vpsInfoClass = NETSTATS_VPS_INFO_CLASS_V6;

	PVZC_NETSTATS_INFO pInfo = NULL;
	PVZC_NETSTATS_INFO pInfoV6 = NULL;

	if (pCmd.Execute() == PRL_ERR_SUCCESS && pCmd->vpsInfoListCount == 1)
		pInfo = (PVZC_NETSTATS_INFO)pCmd->vpsInfoList[0].vpsInfo;

	if (pCmdV6.Execute() == PRL_ERR_SUCCESS && pCmdV6->vpsInfoListCount == 1)
		pInfoV6 = (PVZC_NETSTATS_INFO)pCmdV6->vpsInfoList[0].vpsInfo;

	ULONG i;
	for (i = 0; pInfo != NULL && i < pInfo->uItems; ++i) {
		if (pInfo->netStats[i].classId >= PRL_TC_CLASS_MAX)
			continue;
		n->outgoing[pInfo->netStats[i].classId] += pInfo->netStats[i].bytesSent.QuadPart;
		n->incoming[pInfo->netStats[i].classId] += pInfo->netStats[i].bytesReceived.QuadPart;
		n->outgoing_pkt[pInfo->netStats[i].classId] +=
			(PRL_UINT32)pInfo->netStats[i].packetsSent.QuadPart;
		n->incoming_pkt[pInfo->netStats[i].classId] +=
			(PRL_UINT32)pInfo->netStats[i].packetsReceived.QuadPart;
	}

	for (i = 0; pInfoV6 != NULL && i < pInfoV6->uItems; ++i) {
		if (pInfoV6->netStats[i].classId >= PRL_TC_CLASS_MAX)
			continue;
		n->outgoing[pInfoV6->netStats[i].classId] += pInfoV6->netStats[i].bytesSent.QuadPart;
		n->incoming[pInfoV6->netStats[i].classId] += pInfoV6->netStats[i].bytesReceived.QuadPart;
		n->outgoing_pkt[pInfoV6->netStats[i].classId] +=
			(PRL_UINT32)pInfoV6->netStats[i].packetsSent.QuadPart;
		n->incoming_pkt[pInfoV6->netStats[i].classId] +=
			(PRL_UINT32)pInfoV6->netStats[i].packetsReceived.QuadPart;
	}

	return n.take();
}

int CVzHelper::update_network_classes_config(const CNetworkClassesConfig &conf)
{
	int ret = PRL_ERR_SUCCESS;

	for(int nClassCount = 0; nClassCount < VZNETIF_MAX_TRAFFIC_CLASSES; nClassCount++) {

		// 1 is special class and cannot be edited (all traffic)
		if (nClassCount == 1)
			continue;

		VzCmd<VZC_SET_NODE_COMMAND> pCmd(VZC_CMD_SET_NODE);
		VZC_TRAFFIC_CLASS_NODE_INFO_EX tc = { 0 };

		pCmd->infoClass = REMOVE_TRAFFIC_CLASS_NODE_INFO_CLASS_EX;
		pCmd->infoEntry = &tc;
		tc.classId = nClassCount;

		pCmd.Execute();
	}

	foreach(CNetworkClass *entry, conf.m_lstNetworkClasses) {

		// 1 is special class and cannot be edited (all traffic)
		if (entry->getClassId() == 1)
			continue;

		VzCmd<VZC_SET_NODE_COMMAND> pCmd(VZC_CMD_SET_NODE);
		VZC_TRAFFIC_CLASS_NODE_INFO_EX tc = { 0 };
		ULONG uItems, uSize;

		pCmd->infoClass = ADD_TRAFFIC_CLASS_NODE_INFO_CLASS_EX;
		pCmd->infoEntry = &tc;
		tc.classId = entry->getClassId();
		uItems = entry->getNetworkList().size();

		WRITE_TRACE(DBG_FATAL, "Add network class %u items %u", tc.classId, uItems);

		uSize = FIELD_OFFSET(VZC_IPRANGE_LIST_EX, iprange) +  sizeof(VZC_IPRANGE_EX)*uItems;
		tc.pInclude = (PVZC_IPRANGE_LIST_EX)_alloca(uSize);
		tc.pInclude->uItems = uItems;
		tc.pInclude->uSize =  uSize;

		int j = 0;
		foreach(QString sNet, entry->getNetworkList()) {
			VZC_IPADDR addr;

			WRITE_TRACE(DBG_FATAL, "   IP range %s", QSTR2UTF8(sNet));

			if (!parse_ipaddrmask(QSTR2UTF8(sNet), &addr, FALSE)) {
				WRITE_TRACE(DBG_FATAL, "Invalid network IP range format - %s", QSTR2UTF8(sNet));
				return PRL_ERR_INVALID_ARG;
			}

			switch(addr.family){
			case VZC_AF_INET:
				tc.pInclude->iprange[j].v6 = FALSE;
				tc.pInclude->iprange[j].netAddrV4 = addr.ipv4_addr;
				tc.pInclude->iprange[j].ones = addr.mask;
				break;
			case VZC_AF_INET6:
				tc.pInclude->iprange[j].v6 = TRUE;
				memcpy(tc.pInclude->iprange[j].netAddrV6, addr.ipv6_addr_byte, 16);
				tc.pInclude->iprange[j].ones = addr.mask;
				break;
			default:
				WRITE_TRACE(DBG_FATAL, "Invalid network IP range format - %s", QSTR2UTF8(sNet));
				return PRL_ERR_INVALID_ARG;
			}

			++j;
		}

		int ret2 = pCmd.Execute();
		if (ret2 != PRL_ERR_SUCCESS)
			ret = ret2;
	}

	return ret;
}

static void add_network_class_entry(CNetworkClassesConfig &conf, PRL_UINT32 nClassId, const VZC_IPRANGE_EX& range)
{
	QString qs;
	format_ipaddr(qs, range, range.v6);

	// add to existing class
	foreach(CNetworkClass *entry, conf.m_lstNetworkClasses) {
		if (entry->getClassId() == nClassId) {
			QList<QString> lst = entry->getNetworkList();
			lst += qs;
			entry->setNetworkList(lst);
			return;
		}
	}

	// add new class
	CNetworkClass *entry = new CNetworkClass();
	entry->setClassId(nClassId);
	QList<QString> lst;
	lst += qs;
	entry->setNetworkList(lst);
	conf.m_lstNetworkClasses += entry;

	return;
}

int CVzHelper::get_network_classes_config(CNetworkClassesConfig &conf)
{
	VzCmd<VZC_QUERY_NODE_COMMAND> pCmd(VZC_CMD_QUERY_NODE);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->infoClass = QUERY_TRAFFIC_CLASS_NODE_INFO_CLASS_EX;
	int ret = pCmd.Execute();
	if (ret != PRL_ERR_SUCCESS)
		return ret;

	conf.ClearLists();

	PVZC_TRAFFIC_CLASSES_NODE_INFO_EX pInfo = (PVZC_TRAFFIC_CLASSES_NODE_INFO_EX)pCmd->infoEntry;
	for(int nClassCount = 0; nClassCount < VZNETIF_MAX_TRAFFIC_CLASSES; nClassCount++) {
		ULONG i, j;
		for (i = 0; i < pInfo->uItems;i++) {
			if (pInfo->classes[i].classId != nClassCount)
				continue;
			if (!pInfo->classes[i].pInclude)
				continue;
			for (j = 0; j < pInfo->classes[i].pInclude->uItems; j++) {
				add_network_class_entry(
					conf,
					pInfo->classes[i].classId,
					pInfo->classes[i].pInclude->iprange[j]);
			}
		}
	}

	return PRL_ERR_SUCCESS;
}

int CVzHelper::update_network_shaping_config(const CNetworkShapingConfig &conf)
{
	VZC_TRAFFIC_SHAPER_STATUS_NODE_INFO info;
	info.shaperIsOn = conf.isEnabled();
	VzCmd<VZC_SET_NODE_COMMAND> pCmd(VZC_CMD_SET_NODE);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->infoClass = TRAFFIC_SHAPER_STATUS_NODE_INFO_CLASS;
	pCmd->infoEntry = &info;
	return pCmd.Execute();
}

int CVzHelper::get_network_shaping_config(CNetworkShapingConfig &conf)
{
	VzCmd<VZC_QUERY_NODE_COMMAND> pCmd(VZC_CMD_QUERY_NODE);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->infoClass = TRAFFIC_SHAPER_STATUS_NODE_INFO_CLASS;
	int ret = pCmd.Execute();
	if (ret != PRL_ERR_SUCCESS)
		return ret;
	PVZC_TRAFFIC_SHAPER_STATUS_NODE_INFO pInfo = (PVZC_TRAFFIC_SHAPER_STATUS_NODE_INFO)pCmd->infoEntry;
	conf.setEnabled(pInfo->shaperIsOn);
        return 0;
}

int CVzHelper::set_rate(const QString &uuid, const CVmNetworkRates &rates)
{
	unsigned int id = get_envid_by_uuid(uuid);
	if (id == 0)
		return PRL_ERR_CT_NOT_FOUND;

	return set_rate(id, rates);
}

int CVzHelper::set_rate(ULONG id, const CVmNetworkRates &rates)
{
	int ret = PRL_ERR_SUCCESS;

	VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);

	ULONG uItems = rates.m_lstNetworkRates.size();
	ULONG uSize = FIELD_OFFSET(VZC_TRAFFIC_RATE_VPS_INFO, rates) + sizeof(VZC_TRAFFIC_RATE_INFO)*uItems;
	PVZC_TRAFFIC_RATE_VPS_INFO pInfo = (PVZC_TRAFFIC_RATE_VPS_INFO)_alloca(uSize);
	pInfo->uItems = uItems;
	pInfo->uSize = uSize;

	int i=0;
	foreach(CVmNetworkRate *rate, rates.m_lstNetworkRates) {
		WRITE_TRACE(DBG_FATAL, "TRAFFIC RATE: %u = %u Kbits/sec", rate->getClassId(), rate->getRate());
		pInfo->rates[i].classId = rate->getClassId();
		// Kbits to bytes
		pInfo->rates[i].outRate = rate->getRate()*(1024/8);
		++i;
	}

	pCmd->vpsInfoEntry.vpsId = id;
	pCmd->vpsInfoEntry.vpsInfo = pInfo;
	pCmd->vpsInfoClass = TRAFFIC_RATE_VPS_INFO_CLASS;
	pCmd->bPermanent = TRUE;

	ret = pCmd.Execute();

	return ret;
}

int CVzHelper::add_privnet(unsigned int id, QList<QString> &ips)
{
	VZC_PRIVNET_INFO privNet;
	VZC_PRIVNET_WEAKNESS_NODE_INFO info = { 0 };
	int ret;

	VzCmd<VZC_SET_NODE_COMMAND> pCmd(VZC_CMD_SET_NODE);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	privNet.privnetId = id;

	ULONG uItems = ips.count();
	ULONG uSize = FIELD_OFFSET(VZC_IPRANGE_LIST_EX,iprange) +  sizeof(VZC_IPRANGE_EX)*uItems;

	privNet.pInclude = (PVZC_IPRANGE_LIST_EX)_alloca(uSize);

	int j = 0;
	foreach(QString ip, ips) {
		VZC_IPADDR addr;


		if (ip == QString("*")) {
			info.weakPrivnet = TRUE;
		} else {
			if (!parse_ipaddrmask(QSTR2UTF8(ip), &addr, TRUE)) {
				WRITE_TRACE(DBG_FATAL, "Invalid network IP range format - %s", QSTR2UTF8(ip));
				return PRL_ERR_INVALID_ARG;
			}

			switch(addr.family){
			case VZC_AF_INET:
				privNet.pInclude->iprange[j].v6 = FALSE;
				privNet.pInclude->iprange[j].netAddrV4 = addr.ipv4_addr;
				privNet.pInclude->iprange[j].ones = addr.mask;
				break;
			case VZC_AF_INET6:
				privNet.pInclude->iprange[j].v6 = TRUE;
				memcpy(privNet.pInclude->iprange[j].netAddrV6, addr.ipv6_addr_byte, 16);
				privNet.pInclude->iprange[j].ones = addr.mask;
				break;
			default:
				WRITE_TRACE(DBG_FATAL, "Invalid network IP range format - %s", QSTR2UTF8(ip));
				return PRL_ERR_INVALID_ARG;
			}

			++j;
		}
	}

	privNet.pInclude->uItems = j;
	privNet.pInclude->uSize =
		FIELD_OFFSET(VZC_IPRANGE_LIST_EX,iprange) + sizeof(VZC_IPRANGE_EX)*privNet.pInclude->uItems;

	pCmd->infoEntry = &privNet;
	pCmd->infoClass = ADD_PRIVNET_NODE_INFO_CLASS;

	ret = pCmd.Execute();
	if (ret != PRL_ERR_SUCCESS)
		return ret;

	if (info.weakPrivnet) {
		VzCmd<VZC_SET_NODE_COMMAND> pCmd(VZC_CMD_SET_NODE);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->infoEntry = &info;
		pCmd->infoClass = PRIVNET_WEAKNESS_NODE_INFO_CLASS;
		ret = pCmd.Execute();
	}

	return ret;
}

int CVzHelper::remove_privnet(unsigned int id)
{
	VZC_PRIVNET_INFO privNet;

	VzCmd<VZC_SET_NODE_COMMAND> pCmd(VZC_CMD_SET_NODE);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	pCmd->infoEntry = &privNet;
	pCmd->infoClass = REMOVE_PRIVNET_NODE_INFO_CLASS;

	privNet.privnetId = id;
	privNet.pInclude = NULL;

	return pCmd.Execute();
}

int CVzHelper::enable_privnet(bool enabled)
{
	VZC_PRIVNET_STATUS_NODE_INFO info;

	VzCmd<VZC_SET_NODE_COMMAND> pCmd(VZC_CMD_SET_NODE);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	pCmd->infoEntry = &info;
	pCmd->infoClass = PRIVNET_STATUS_NODE_INFO_CLASS;

	info.privnetEnabled = enabled;

	return pCmd.Execute();
}

CVzOperationHelper::~CVzOperationHelper()
{
}

#define PRL_CMD_WORK_TIMEOUT  1000 * 60 * 60 * 24
PRL_RESULT CVzOperationHelper::run_prg(const char *name, const QStringList &lstArgs, CVmEvent *pEvent)
{
	QProcess proc;
	static BOOL (WINAPI *pfnWow64DisableWow64FsRedirection) (PVOID *) =
		(BOOL (WINAPI*)(PVOID*)) GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Wow64DisableWow64FsRedirection");
	static BOOL (WINAPI *pfnWow64RevertWow64FsRedirection) (PVOID ) =
		(BOOL (WINAPI*)(PVOID)) GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Wow64RevertWow64FsRedirection");
	PVOID oldWow64;

	if (pfnWow64DisableWow64FsRedirection)
		pfnWow64DisableWow64FsRedirection(&oldWow64);

	WRITE_TRACE(DBG_INFO, "%s %s", name, lstArgs.join(" ").toUtf8().constData());
	proc.start(name, lstArgs);

	if (pfnWow64RevertWow64FsRedirection)
		pfnWow64RevertWow64FsRedirection(oldWow64);

	bool bOk = proc.waitForFinished(PRL_CMD_WORK_TIMEOUT);
	if (!bOk) {
		WRITE_TRACE(DBG_FATAL, "%s tool not responding. Terminate it now.", name);
		proc.kill();
		return PRL_ERR_OPERATION_FAILED;
	}
	int ret = proc.exitCode();
	if (ret != 0) {
		QString msg = proc.readAll();
		WRITE_TRACE(DBG_FATAL, "%s utility failed: %s %s [%d]\nout=%s",
			    name, name,
			    lstArgs.join(" ").toUtf8().constData(),
			    proc.exitCode(),
			    QSTR2UTF8(msg));
		if (pEvent)
			pEvent->addEventParameter(new CVmEventParameter(PVE::String, QSTR2UTF8(msg), EVT_PARAM_MESSAGE_PARAM_0));
		return PRL_ERR_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

static QList<unsigned int> get_ids_list()
{
	QList<unsigned int> lst;

	QDir dir("/etc/vz/conf");
	QFileInfoList dirs = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
	foreach(QFileInfo entry, dirs)
	{
		unsigned int id;
		if (entry.suffix() != "conf")
			continue;
		if (sscanf(QSTR2UTF8(entry.fileName()), "%d.conf", &id) != 1)
			continue;
		if (id == 0)
			continue;
		lst += id;
	}
	qSort(lst.begin(), lst.end());
	return lst;
}

int CVzHelper::get_envid_list(QList<unsigned int> &lst)
{
	VzCmd<VZC_ENUMERATE_VPS_COMMAND> pCmd(VZC_CMD_ENUMERATE_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	if (pCmd.Execute() == PRL_ERR_SUCCESS) {
		for(DWORD i=0; i < pCmd->vpsIdListCount; i++) {
			WRITE_TRACE(DBG_FATAL, "VpsId=%u state=%d substate=%d",
				    pCmd->vpsIdList[i],
				    pCmd->vpsStateList[i].state,
				    pCmd->vpsStateList[i].substate);
			lst += pCmd->vpsIdList[i];
		}
	}

	return PRL_ERR_SUCCESS;
}

int CVzHelper::get_env_status(unsigned int id, VIRTUAL_MACHINE_STATE &nState)
{
	VzCmd<VZC_QUERY_VPS_COMMAND> pCmd(VZC_CMD_QUERY_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	ULONG ctid = (ULONG)id;
	pCmd->vpsIdList = &ctid;
	pCmd->vpsIdListCount = 1;
	pCmd->vpsInfoClass = STATUS_VPS_INFO_CLASS;
	nState = VMS_UNKNOWN;

	int ret = pCmd.Execute();
	if (ret == PRL_ERR_SUCCESS && pCmd->vpsInfoListCount == 1) {
		PVZC_STATUS_VPS_INFO  pi = (PVZC_STATUS_VPS_INFO)pCmd->vpsInfoList[0].vpsInfo;

		if (pi->transitionStatus == VE_SUBSTATE_NONE) {
			switch (pi->status)
			{
			case VE_STATE_RUNNING:
				nState = VMS_RUNNING;
				break;
			case VE_STATE_STOPPED:
				nState = VMS_STOPPED;
				break;
			case VE_STATE_MOUNTED:
				/* PS has no VM mounted state, mapped mounted as paused */
				nState = VMS_STOPPED;
				break;
			}
		} else {
			switch (pi->transitionStatus)
			{
			case VE_SUBSTATE_STARTING:
				nState = VMS_STARTING;
				break;
			case VE_SUBSTATE_STOPPING:
				nState = VMS_STOPPING;
				break;
			case VE_SUBSTATE_DELETING:
				nState = VMS_DELETING_STATE;
				break;
			case VE_SUBSTATE_MIGRATING:
				nState = VMS_MIGRATING;
				break;
			case VE_SUBSTATE_RESTORING:
				nState = VMS_RESTORING;
				break;
			case VE_SUBSTATE_BACKUPING:
			case VE_SUBSTATE_CLONING_SRC:
				nState = VMS_SNAPSHOTING;
				break;
			}
		}
	}

	return ret;
}

int CVzHelper::get_env_status(const QString &uuid, VIRTUAL_MACHINE_STATE &nState)
{
	if (hVzcapi == NULL)
		return PRL_ERR_VZ_API_NOT_INITIALIZED;

	unsigned int id = get_envid_by_uuid(uuid);
	if (id == 0)
		return PRL_ERR_CT_NOT_FOUND;

	return get_env_status(get_envid_by_uuid(uuid), nState);
}

int CVzHelper::get_vz_config_param(const char *param, QString &out)
{
	Q_UNUSED(param);
	Q_UNUSED(out);
	return -1;
}

SmartPtr<CVmConfiguration> CVzHelper::get_env_config(const QString &uuid)
{
	unsigned int id = get_envid_by_uuid(uuid);
	if (id == 0)
		return SmartPtr<CVmConfiguration>();
	return get_env_config(id);
}

SmartPtr<CVmConfiguration> CVzHelper::get_env_config(unsigned int envid)
{
	ULONG id = envid;
	PWSTR * strList, str;
	int count = 0;
	ULONGLONG qw;
	DWORD dw;
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	SmartPtr<CVmConfiguration> pConfig;

	// check for API inited once
	if (hVzcapi == NULL)
		return pConfig;

	pConfig = SmartPtr<CVmConfiguration> (new CVmConfiguration);
	pConfig->setVmType(PVT_CT);

	VzCfg pCfg(id);

	QString privatePath;

	// VmIdentification
	{
		CVmIdentification *p = pConfig->getVmIdentification();

		// GUID
		if (pCfg.GetValueStrW(VZCFG_VPS_GUID, &str)) {
			p->setVmUuid(UTF16_2QSTR(str).toLower());
			p->setEnvId(envid);
		} else {
			return pConfig;
		}

		// VE_PRIVATE
		WCHAR buf[MAX_PATH+1];
		pVZC_GetVpsPrivatePathW(buf, MAX_PATH, id, NULL);
		privatePath = UTF16_2QSTR(buf);
		p->setHomePath(privatePath);

		// NAME
		if (pCfg.GetValueStrListW(VZCFG_VPS_NAME_ALIASES, &strList, &count) && count > 0)
			p->setVmName(UTF16_2QSTR(strList[0]));
		else
			p->setVmName(QString("%1").arg(id));

		// Uptime
		{
			VzCmd<VZC_QUERY_VPS_COMMAND> pCmd(VZC_CMD_QUERY_VPS);
			pCmd->vpsIdList = &id;
			pCmd->vpsIdListCount = 1;
			pCmd->vpsInfoClass = UPTIME_VPS_INFO_CLASS;
			if (pCmd.Execute() == PRL_ERR_SUCCESS && pCmd->vpsInfoListCount == 1) {
				PVZC_UPTIME_VPS_INFO info = (PVZC_UPTIME_VPS_INFO)pCmd->vpsInfoList[0].vpsInfo;
				p->setVmUptimeInSeconds(info->uptime.QuadPart);
			}
		}
	}

	// VmSettings
	{
		CVmCommonOptions *pCo = pConfig->getVmSettings()->getVmCommonOptions();

		// DESCRIPTION
		PWSTR str;
		if (pCfg.GetValueStrW(VZCFG_VPS_DESCRIPTION, &str))
			pCo->setVmDescription(UTF16_2QSTR(str));

		 pCo->setOsType(PVS_GUEST_TYPE_WINDOWS);
		 pCo->setOsVersion(PVS_GUEST_VER_WIN_2008);

		 // ONBOOT
		 CVmStartupOptions *pSo = pConfig->getVmSettings()->getVmStartupOptions();
		 DWORD enabled = 0;
		 pCfg.GetValueDword(VZCFG_FLAG_AUTOBOOT, &enabled);
		 pSo->setAutoStart(enabled ? PAO_VM_START_ON_LOAD : PAO_VM_START_MANUAL);

		 // VE_TYPE (template - true/false)
		 enabled = 0;
		 pCfg.GetValueDword(VZCFG_FLAG_CT_TEMPLATE, &enabled);
		 pCo->setTemplate(enabled);
	 }

	// CtSettings
	{
		CCtSettings *pCt = pConfig->getCtSettings();

		// BASIC_GROUP_VPS_INFO_CLASS
		VzCmd<VZC_QUERY_VPS_COMMAND> pCmd(VZC_CMD_QUERY_VPS);
		pCmd->vpsIdList = &id;
		pCmd->vpsIdListCount = 1;
		pCmd->vpsInfoClass = BASIC_GROUP_VPS_INFO_CLASS;
		if (pCmd.Execute() == PRL_ERR_SUCCESS && pCmd->vpsInfoListCount == 1) {
			PVZC_BASIC_GROUP_VPS_INFO  pi = (PVZC_BASIC_GROUP_VPS_INFO)pCmd->vpsInfoList[0].vpsInfo;

			// OSTEMPLATE
			pCt->setOsTemplate(UTF16_2QSTR(pi->templateInfo.realOsTemplate));

			// APP TEMPLATES
			QStringList tmpl_set;
			for (DWORD i=0; i < pi->templateInfo.appTemplatesListCount; ++i)
				tmpl_set += UTF16_2QSTR(pi->templateInfo.appTemplatesList[i]);
			pCt->setAppTemplate(tmpl_set);
		}
	}

	// Network
	{
		CVmGlobalNetwork * pNet = pConfig->getVmSettings()->getGlobalNetwork();

		// HOSTNAME
		if (pCfg.GetValueStrW(VZCFG_HOSTNAME, &str))
			pNet->setHostName(UTF16_2QSTR(str));

		// SEARCHDOMAINS
		QList<QString> searchdomains;
		if (pCfg.GetValueStrListW(VZCFG_SEARCHDOMAINS, &strList, &count)) {
			for (int k=0; k < count; ++k)
				searchdomains += UTF16_2QSTR(strList[k]);
			pNet->setSearchDomains(searchdomains);
		}

		// DNSSERVERS
		QList<QString> nameservers;
		if (pCfg.GetValueStrListW(VZCFG_NETIF_DNSSERVERS, &strList, &count)) {
			for (int k=0; k < count; ++k)
				nameservers += UTF16_2QSTR(strList[k]);
			pNet->setDnsIPAddresses(nameservers);
		}

		// RATE
		CVmNetworkRates *pRates = pNet->getNetworkRates();

		VzCmd<VZC_QUERY_VPS_COMMAND> pCmd(VZC_CMD_QUERY_VPS);
		pCmd->vpsIdList = &id;
		pCmd->vpsIdListCount = 1;
		pCmd->vpsInfoClass = TRAFFIC_RATE_VPS_INFO_CLASS;
		if (pCmd.Execute() == PRL_ERR_SUCCESS && pCmd->vpsInfoListCount == 1) {
			PVZC_TRAFFIC_RATE_VPS_INFO pInfo = (PVZC_TRAFFIC_RATE_VPS_INFO)pCmd->vpsInfoList[0].vpsInfo;
			for (ULONG i=0; i < pInfo->uItems; ++i) {
				CVmNetworkRate *pRate = new CVmNetworkRate;
				pRate->setClassId(pInfo->rates[i].classId);
				pRate->setRate(pInfo->rates[i].outRate*8/1024);
				pRates->m_lstNetworkRates += pRate;
			}
		}

		// RATEBOUND - VZWIN always bound
		pRates->setRateBound(true);
	}

	// HARDWARE
	{
		CVmCpu *pCpu = pConfig->getVmHardwareList()->getCpu();
		CVmMemory *pMem = pConfig->getVmHardwareList()->getMemory();

		// RAM SIZE (PRIVATE MEMORY)
		if (pCfg.GetValueQword(VZCFG_VZRES_MEMLIMIT, &qw))
			pMem->setRamSize(qw / (1024*1024));
		else
			pMem->setRamSize(0);


		// CPUS
		DWORD cpus = 0;
		if (pCfg.GetValueDword(VZCFG_VZRES_CPUS, &cpus) && (cpus != 0))
			pCpu->setNumber(cpus);
		else
			//zero mean unlimited
			//error - set unlimited
			pCpu->setNumber(PRL_CPU_UNLIMITED);

		if (cpus == 0)
			cpus = si.dwNumberOfProcessors;

		// CPULIMIT
		if (pCfg.GetValueDword(VZCFG_VZRES_CPULIMIT, &dw)) {
			PRL_CPULIMIT_DATA cpulimit;
			cpulimit.type = PRL_CPULIMIT_PERCENTS;
			cpulimit.value = dw * cpus;
			pCpu->setCpuLimitData(&cpulimit);
		}
		if (pCfg.GetValueDword(VZCFG_VZRES_CPULIMITMHZ, &dw)) {
			PRL_CPULIMIT_DATA cpulimit;
			cpulimit.type = PRL_CPULIMIT_MHZ;
			cpulimit.value = dw * cpus;
			pCpu->setCpuLimitData(&cpulimit);
		}

		// CPUUNITS
		if (pCfg.GetValueDword(VZCFG_VZRES_CPUUNITS, &dw))
			pCpu->setCpuUnits(dw);

		// DISKSPACE
 		/* FIXME:
		   handle DISKINODES
		   handle multiple drives
		*/
		if (pCfg.GetValueQword(VZCFG_DISKQUOTA, &qw))
		{
			CVzHardDisk *pDisk = new CVzHardDisk;
			// bytes -> Mbytes
			qw /= (1024*1024);
			if (qw == 0)
				qw = 1;

			// GUID
			pCfg.GetValueStrW(VZCFG_VPS_GUID, &str);
			pDisk->setSystemUuid(UTF16_2QSTR(str).toLower());
			pDisk->setSize(qw);
			pDisk->setInterfaceType(PMS_UNKNOWN_DEVICE);
			pDisk->setEmulatedType(PDT_USE_IMAGE_FILE);
			pDisk->setSystemName(privatePath + QString("\\root.efd"));
			pDisk->setUserFriendlyName(privatePath + QString("\\root.efd"));
			pConfig->getVmHardwareList()->addHardDisk(pDisk);
		}

		// NETIF
		for (int idx = 0; idx < 64; idx++) {
			QString key;


			// GUID
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_ADAPTER_GUID);
			if (!pCfg.GetValueStrW(key.toAscii().constData(), &str))
				continue;

			CVmGenericNetworkAdapter *pNet = new CVmGenericNetworkAdapter;

			pNet->setIndex(idx);

			// ENABLED
			DWORD disabled = 0;
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_DISABLED);
			pCfg.GetValueDword(key.toAscii().constData(), &disabled);
			pNet->setEnabled(!disabled);

			// BRIDGED mode
			DWORD isBridged = 0;
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_IS_BRIDGED);
			pCfg.GetValueDword(key.toAscii().constData(), &isBridged);
			if (isBridged) {
				// Bridged Network Id
				key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_BRIDGED_NETID);
				if (pCfg.GetValueStrW(key.toAscii().constData(), &str))
					// TODO: may be need to map different parameter
					pNet->setVirtualNetworkID(UTF16_2QSTR(str));
				// TODO: check if we need new type, CT_BRIDGED
				pNet->setEmulatedType(PNA_BRIDGED_ETHERNET);
			} else {
				pNet->setEmulatedType(PNA_ROUTED);
			}

			// IPv4 DHCP
			DWORD isDhcp = 0, isDhcpV6 = 0;
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_DHCP_IS_ON);
			pCfg.GetValueDword(key.toAscii().constData(), &isDhcp);
			// IPv6 DHCP
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_DHCPV6_IS_ON);
			pCfg.GetValueDword(key.toAscii().constData(), &isDhcpV6);

			if (isDhcp)
				pNet->setConfigureWithDhcpEx(true);
			if (isDhcpV6)
				pNet->setConfigureWithDhcpIPv6(true);

			// IPv4 ADDRESSES
			QList<QString> ips;
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_IPADDR);
			if (pCfg.GetValueStrListW(key.toAscii().constData(), &strList, &count)) {
				for (int k=0; k < count; ++k)
					ips += UTF16_2QSTR(strList[k]);
			}

			// IPv6 ADDRESSES
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_IPV6ADDR);
			if (pCfg.GetValueStrListW(key.toAscii().constData(), &strList, &count)) {
				for (int k=0; k < count; ++k)
					ips += UTF16_2QSTR(strList[k]);
			}

			pNet->setNetAddresses(ips);

			// NAME
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_NAME);
			if (pCfg.GetValueStrW(key.toAscii().constData(), &str))
				pNet->setSystemName(UTF16_2QSTR(str));

			// MAC
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_ADAPTER_MACADDR);
			if (pCfg.GetValueStrW(key.toAscii().constData(), &str))
				pNet->setMacAddress(UTF16_2QSTR(str).remove(QChar(':')));

			// IPv4 GATEWAY
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_IPGATEWAY);
			if (pCfg.GetValueStrW(key.toAscii().constData(), &str))
				pNet->setDefaultGateway(UTF16_2QSTR(str));

			// IPv6 GATEWAY
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_IPV6GATEWAY);
			if (pCfg.GetValueStrW(key.toAscii().constData(), &str))
				pNet->setDefaultGatewayIPv6(UTF16_2QSTR(str));

			// SEARCHDOMAINS
			QList<QString> searchdomains;
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_SEARCHDOMAINS);
			if (pCfg.GetValueStrListW(key.toAscii().constData(), &strList, &count)) {
				for (int k=0; k < count; ++k)
					searchdomains += UTF16_2QSTR(strList[k]);
				pNet->setSearchDomains(searchdomains);
			}

			// IPv4 DNSSERVERS
			QList<QString> nameservers;
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_DNSSERVERS);
			if (pCfg.GetValueStrListW(key.toAscii().constData(), &strList, &count)) {
				for (int k=0; k < count; ++k)
					nameservers += UTF16_2QSTR(strList[k]);
			}

			// IPv6 DNSSERVERS
			key.sprintf(VZCFG_NET_ROOT_GROUP "%d.%hs", idx, VZCFG_NETIF_IPV6DNSSERVERS);
			if (pCfg.GetValueStrListW(key.toAscii().constData(), &strList, &count)) {
				for (int k=0; k < count; ++k)
					nameservers += UTF16_2QSTR(strList[k]);
			}

			pNet->setDnsIPAddresses(nameservers);

			get_firewall(pNet, envid);

			pConfig->getVmHardwareList()->addNetworkAdapter(pNet);
		}
	}

	// Store xml copy
	QString path = get_env_xml_config_path(pConfig);
	pConfig->saveToFile(path, true, true);

	return pConfig;
}

SmartPtr<CVmConfiguration> CVzHelper::get_env_config_sample(const QString &name, int &err)
{
	SmartPtr<CVmConfiguration> pConfig;
	Q_UNUSED(name);
	Q_UNUSED(err);

	if (hVzcapi == NULL)
		return pConfig;

	pConfig = SmartPtr<CVmConfiguration> (new CVmConfiguration);

	return pConfig;
}

int CVzOperationHelper::set_env_name(const QString &uuid, const QString &name)
{
	return set_env_name(CVzHelper::get_envid_by_uuid(uuid), name);
}

int CVzOperationHelper::set_env_name(unsigned int envid, const QString &name)
{
	ULONG id = envid;
	VZC_NAME_ALIASES_VPS_INFO info;
	WCHAR* aliases[1];

	VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	pCmd->vpsInfoEntry.vpsId = id;
	pCmd->vpsInfoEntry.vpsInfo = &info;
	pCmd->vpsInfoClass = NAME_ALIASES_VPS_INFO_CLASS;
	pCmd->bPermanent = TRUE;

	if (name.isEmpty()) {
		info.aliasesCount = 0;
	} else {
		info.aliasesCount = 1;
		aliases[0] = (PWSTR)name.utf16();
		info.aliases = aliases;
	}

	return pCmd.Execute(m_sErrorMsg);
}

static void ipaddr_to_fwaddr(const VZC_IPADDR& addr, PVZC_FIREWALL_ADDR pOutAddr)
{
	switch (addr.family)
	{
	case VZC_AF_INET:
		pOutAddr->netAddrV4 = addr.ipv4_addr;
		pOutAddr->ones = addr.mask;
		break;
	case VZC_AF_INET6:
		memcpy(pOutAddr->netAddrV6, addr.ipv6_addr_byte, 16);
		pOutAddr->ones = addr.mask;
		break;
	}
}

static PRL_RESULT add_rule(VZC_FIREWALL_RULE* rule, /* by ref*/ int& i,
			   const CVmNetFirewallRule* pRule,
			   PRL_FIREWALL_DIRECTION nDirection)
{
	VZC_IPADDR srcAddr = { VZC_AF_INET };
	VZC_IPADDR dstAddr = { VZC_AF_INET };

	memset(rule, 0, sizeof(*rule));

	rule->incoming = (nDirection == PFD_INCOMING);

	WRITE_TRACE(DBG_FATAL, "fw-rule: dir=%u %s %s [%s %u] [%s %u]",
		    nDirection,
		    rule->incoming ? "in" : "out", QSTR2UTF8(pRule->getProtocol()),
		    QSTR2UTF8(pRule->getLocalNetAddress()),
		    pRule->getLocalPort(),
		    QSTR2UTF8(pRule->getRemoteNetAddress()),
		    pRule->getRemotePort()
		);

	QString qs = pRule->getProtocol();
	if (qs.compare("tcp", Qt::CaseInsensitive) == 0) {
		rule->protocol = VZC_FIREWALL_PROTO_TCP;
	} else if (qs.compare("udp", Qt::CaseInsensitive) == 0) {
		rule->protocol = VZC_FIREWALL_PROTO_UDP;
	} else if (qs.compare("icmp", Qt::CaseInsensitive) == 0) {
		rule->protocol = VZC_FIREWALL_PROTO_ICMP;
	} else {
		WRITE_TRACE(DBG_FATAL, "Invalid protocol value '%s'", QSTR2UTF8(qs));
		return PRL_ERR_INVALID_ARG;
	}

	qs = rule->incoming ? pRule->getRemoteNetAddress() : pRule->getLocalNetAddress();
	bool src_isEmpty = qs.isEmpty() || qs == "*";
	if (!src_isEmpty && !parse_ipaddrmask(QSTR2UTF8(qs), &srcAddr, FALSE)) {
		WRITE_TRACE(DBG_FATAL, "Invalid IP address/mask format - %s", QSTR2UTF8(qs));
		return PRL_ERR_INVALID_ARG;
	}
	rule->v6 = (srcAddr.family == VZC_AF_INET6);
	ipaddr_to_fwaddr(srcAddr, &rule->src);

	qs = rule->incoming ? pRule->getLocalNetAddress() : pRule->getRemoteNetAddress();
	bool dst_isEmpty = qs.isEmpty() || qs == "*";
	if (!dst_isEmpty && !parse_ipaddrmask(QSTR2UTF8(qs), &dstAddr, FALSE)) {
		WRITE_TRACE(DBG_FATAL, "Invalid IP address/mask format - %s", QSTR2UTF8(qs));
		return PRL_ERR_INVALID_ARG;
	}

	// check addr family matching
	if (rule->v6 && dstAddr.family != VZC_AF_INET6) {
		// any can be both ipv4/ipv6
		if (dstAddr.ipv4_addr == 0 && dstAddr.mask == 0) {
			dstAddr.family = VZC_AF_INET6;
		} else {
			WRITE_TRACE(DBG_FATAL, "Local IP and remote IP are from different address family");
			return PRL_ERR_INVALID_ARG;
		}
	}
	if (!rule->v6 && dstAddr.family != VZC_AF_INET) {
		// any can be both ipv4/ipv6
		if (rule->src.netAddrV4 == 0 && rule->src.ones == 0) {
			memset(&rule->src, sizeof(rule->src), 0);
			rule->v6 = TRUE;
		} else {
			WRITE_TRACE(DBG_FATAL, "Local IP and remote IP are from different address family");
			return PRL_ERR_INVALID_ARG;
		}
	}

	ipaddr_to_fwaddr(dstAddr, &rule->dst);

	rule->src.port = rule->incoming ? pRule->getRemotePort() : pRule->getLocalPort();
	if (rule->src.port > 0xffff) {
		WRITE_TRACE(DBG_FATAL, "Invalid IP port format - %s", QSTR2UTF8(qs));
		return PRL_ERR_INVALID_ARG;
	}

	rule->dst.port = rule->incoming ? pRule->getLocalPort() : pRule->getRemotePort();
	if (rule->dst.port > 0xffff) {
		WRITE_TRACE(DBG_FATAL, "Invalid IP port format - %s", QSTR2UTF8(qs));
		return PRL_ERR_INVALID_ARG;
	}

	i++;
	
	// #CCU-4047: make ipv4/ipv6 twin rule if both addrs is empty
	if (src_isEmpty && dst_isEmpty) {
		*(rule + 1) = *rule;
		i++;
		rule++;
		rule->v6 = !rule->v6;
	}

	return PRL_ERR_SUCCESS;
}

static PRL_RESULT set_firewall(ULONG envId, SmartPtr<CVmConfiguration> &pConfig,
			       SmartPtr<CVmConfiguration> &pOldConfig, QString& errMsg)
{
	Q_UNUSED(pOldConfig);
	PRL_RESULT ret;

	if (pConfig->getVmType() != PVT_CT)
		return PRL_ERR_SUCCESS;

	// Add new rules and update policy/status
	foreach(CVmGenericNetworkAdapter* pAdapter, pConfig->getVmHardwareList()->m_lstNetworkAdapters) {

		CVmNetFirewallRules* inRules = pAdapter->getFirewall()->getIncoming()->getDirection()->getFirewallRules();
		CVmNetFirewallRules* outRules = pAdapter->getFirewall()->getOutgoing()->getDirection()->getFirewallRules();

		// Build new rule list
		// Make space twice larger for possible ipv4/ipv6 combos
		ULONG uItems = 2 * (inRules->m_lstFirewallRules.count() + outRules->m_lstFirewallRules.count());
		ULONG uSize = FIELD_OFFSET(VZC_FIREWALL_RULE_INFO, rules) + sizeof(VZC_FIREWALL_RULE) * uItems;
		PVZC_FIREWALL_RULE_INFO pInfo = (PVZC_FIREWALL_RULE_INFO)_alloca(uSize);
		memset(pInfo, 0, uSize);
		int i = 0;
		foreach(const CVmNetFirewallRule* pRule, inRules->m_lstFirewallRules) {
			ret = add_rule(&pInfo->rules[i], i, pRule, PFD_INCOMING);
			if (ret != PRL_ERR_SUCCESS)
				return ret;
		}
		foreach(const CVmNetFirewallRule* pRule, outRules->m_lstFirewallRules) {
			ret = add_rule(&pInfo->rules[i], i, pRule, PFD_OUTGOING);
			if (ret != PRL_ERR_SUCCESS)
				return ret;
		}
		pInfo->uItems = i;
		pInfo->uSize =  FIELD_OFFSET(VZC_FIREWALL_RULE_INFO, rules) + sizeof(VZC_FIREWALL_RULE) * i;

		// Query old rules for deletion
		VzCmd<VZC_QUERY_EX_VPS_COMMAND> pQuery(VZC_CMD_QUERY_EX_VPS);
		if (pQuery == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		VZC_VPS_INFO_KEY ifkey[1];
		ifkey->vpsId = envId;
		ifkey->type = VZC_VPS_INFO_KEY_IFNAME;
		wcscpy(ifkey->ifname, (PCWSTR)QSTR2UTF16(pAdapter->getSystemName()));
		pQuery->inList = ifkey;
		pQuery->inListCount = 1;
		pQuery->vpsInfoClass = QUERY_RULE_FIREWALL_NETIF_VPS_INFO_CLASS;

		if (pQuery.Execute() != PRL_ERR_SUCCESS)
			return ret;

		// Delete old rules
		if (pQuery->outListCount == 1 &&
		    ((PVZC_FIREWALL_RULE_INFO)pQuery->outList[0].vpsInfo)->uItems > 0)
		{
			VzCmd<VZC_SET_EX_VPS_COMMAND> pDel(VZC_CMD_SET_EX_VPS);
			if (pDel == NULL)
				return PRL_ERR_API_WASNT_INITIALIZED;
			pDel->bPermanent = TRUE;
			pDel->vpsInfo = pQuery->outList[0].vpsInfo;
			pDel->vpsInfoClass = DEL_RULE_FIREWALL_NETIF_VPS_INFO_CLASS;
			pDel->vpsInfoKey.vpsId = envId;
			pDel->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
			wcscpy(pDel->vpsInfoKey.ifname, (PCWSTR)QSTR2UTF16(pAdapter->getSystemName()));

			ret = pDel.Execute(errMsg);
			if (ret != PRL_ERR_SUCCESS)
				return ret;
		}

		// Add new rules list
		VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->bPermanent = TRUE;
		pCmd->vpsInfo = pInfo;
		pCmd->vpsInfoClass = ADD_RULE_FIREWALL_NETIF_VPS_INFO_CLASS;
		pCmd->vpsInfoKey.vpsId = envId;
		pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
		wcscpy(pCmd->vpsInfoKey.ifname, (PCWSTR)QSTR2UTF16(pAdapter->getSystemName()));

		ret = pCmd.Execute(errMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;

		// Set status
		VZC_FIREWALL_STATUS_INFO statusInfo;
		statusInfo.firewallEnabled = pAdapter->getFirewall()->isEnabled();
		pCmd->bPermanent = TRUE;
		pCmd->vpsInfo = &statusInfo;
		pCmd->vpsInfoClass = STATUS_FIREWALL_NETIF_VPS_INFO_CLASS;
		pCmd->vpsInfoKey.vpsId = envId;
		pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
		wcscpy(pCmd->vpsInfoKey.ifname, (PCWSTR)QSTR2UTF16(pAdapter->getSystemName()));

		ret = pCmd.Execute(errMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;

		// Set policy
		VZC_FIREWALL_POLICY_NETIF_VPS_INFO policyInfo;
		policyInfo.policyIn = pAdapter->getFirewall()->getIncoming()->getDirection()->getDefaultPolicy();
		policyInfo.policyOut = pAdapter->getFirewall()->getOutgoing()->getDirection()->getDefaultPolicy();
		pCmd->bPermanent = TRUE;
		pCmd->vpsInfo = &policyInfo;
		pCmd->vpsInfoClass = POLICY_FIREWALL_NETIF_VPS_INFO_CLASS;
		pCmd->vpsInfoKey.vpsId = envId;
		pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
		wcscpy(pCmd->vpsInfoKey.ifname, (PCWSTR)QSTR2UTF16(pAdapter->getSystemName()));

		ret = pCmd.Execute(errMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;

	}

	return PRL_ERR_SUCCESS;
}

static PRL_RESULT get_firewall(CVmGenericNetworkAdapter *pNet, unsigned int envid)
{
	boolean bEnabled = false;
	PRL_FIREWALL_POLICY fwPolicy_in = PFP_ACCEPT;
	PRL_FIREWALL_POLICY fwPolicy_out = PFP_ACCEPT;

	VZC_VPS_INFO_KEY ifkey[1];
	ifkey->vpsId = envid;
	ifkey->type = VZC_VPS_INFO_KEY_IFNAME;
	wcscpy(ifkey->ifname, (PCWSTR)QSTR2UTF16(pNet->getSystemName()));

	// Query status
	{
		VzCmd<VZC_QUERY_EX_VPS_COMMAND> pCmd(VZC_CMD_QUERY_EX_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->inList = ifkey;
		pCmd->inListCount = 1;
		pCmd->vpsInfoClass = STATUS_FIREWALL_NETIF_VPS_INFO_CLASS;
		if (pCmd.Execute() == PRL_ERR_SUCCESS && pCmd->outListCount == 1) {
			PVZC_FIREWALL_STATUS_INFO info =
				(PVZC_FIREWALL_STATUS_INFO)pCmd->outList[0].vpsInfo;
			bEnabled = info->firewallEnabled;
		}
	}

	// Query policy
	{
		VzCmd<VZC_QUERY_EX_VPS_COMMAND> pCmd(VZC_CMD_QUERY_EX_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->inList = ifkey;
		pCmd->inListCount = 1;
		pCmd->vpsInfoClass = POLICY_FIREWALL_NETIF_VPS_INFO_CLASS;
		if (pCmd.Execute() == PRL_ERR_SUCCESS && pCmd->outListCount == 1) {
			PVZC_FIREWALL_POLICY_NETIF_VPS_INFO info =
				(PVZC_FIREWALL_POLICY_NETIF_VPS_INFO)pCmd->outList[0].vpsInfo;
			fwPolicy_in = (PRL_FIREWALL_POLICY)info->policyIn;
			fwPolicy_out = (PRL_FIREWALL_POLICY)info->policyOut;
		}
	}

	// Query rules
	{
		VzCmd<VZC_QUERY_EX_VPS_COMMAND> pCmd(VZC_CMD_QUERY_EX_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->inList = ifkey;
		pCmd->inListCount = 1;
		pCmd->vpsInfoClass = QUERY_RULE_FIREWALL_NETIF_VPS_INFO_CLASS;
		if (pCmd.Execute() == PRL_ERR_SUCCESS && pCmd->outListCount == 1) {
			PRL_FIREWALL_POLICY* pFwPolicy;
			CVmNetFirewallRules* pFwRules;

			CVmNetFirewall* pFw = new CVmNetFirewall();
			CVmNetFirewallRules* pFwRules_in = new CVmNetFirewallRules();
			CVmNetFirewallRules* pFwRules_out = new CVmNetFirewallRules();

			PVZC_FIREWALL_RULE_INFO info = (PVZC_FIREWALL_RULE_INFO)pCmd->outList[0].vpsInfo;
			for (unsigned int i=0; i < info->uItems; ++i) {
				const VZC_FIREWALL_RULE& rule = info->rules[i];

				if (rule.incoming) {
					pFwRules = pFwRules_in;
					pFwPolicy = &fwPolicy_in;
				} else {
					pFwRules = pFwRules_out;
					pFwPolicy = &fwPolicy_out;
				}

				CVmNetFirewallRule * pRule = new CVmNetFirewallRule();
				QString qs;

				switch (rule.protocol)
				{
				case VZC_FIREWALL_PROTO_TCP:  qs = "tcp"; break;
				case VZC_FIREWALL_PROTO_UDP:  qs = "udp"; break;
				case VZC_FIREWALL_PROTO_ICMP: qs = "icmp"; break;
				}
				pRule->setProtocol(qs);

				pRule->setLocalPort(rule.incoming ? rule.dst.port : rule.src.port);
				pRule->setRemotePort(rule.incoming ? rule.src.port : rule.dst.port);

				format_ipaddr(qs, rule.incoming ? rule.dst : rule.src, rule.v6);
				pRule->setLocalNetAddress(qs);

				format_ipaddr(qs, rule.incoming ? rule.src : rule.dst, rule.v6);
				pRule->setRemoteNetAddress(qs);

				pFwRules->m_lstFirewallRules.push_back(pRule);
			}
			pFw->setEnabled(bEnabled);
			pFw->getIncoming()->getDirection()->setDefaultPolicy(fwPolicy_in);
			pFw->getOutgoing()->getDirection()->setDefaultPolicy(fwPolicy_out);
			pFw->getIncoming()->getDirection()->setFirewallRules(pFwRules_in);
			pFw->getOutgoing()->getDirection()->setFirewallRules(pFwRules_out);
			pNet->setFirewall(pFw);
		}
	}

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::apply_env_config_internal(
	SmartPtr<CVmConfiguration> &pConfig,
	SmartPtr<CVmConfiguration> &pOldConfig,
	unsigned int nFlags)
{
	Q_UNUSED(nFlags);
	int ret = PRL_ERR_SUCCESS;
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	if (hVzcapi == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	QString uuid = pConfig->getVmIdentification()->getVmUuid();
	ULONG id = pConfig->getVmIdentification()->getEnvId();

	WRITE_TRACE(DBG_FATAL, "CTID: %u", id);

	VzCfg pCfg(id);

	// DESCRIPTION_VPS_INFO_CLASS
	QString olddesc = pOldConfig->getVmSettings()->getVmCommonOptions()->getVmDescription();
	QString desc = pConfig->getVmSettings()->getVmCommonOptions()->getVmDescription();
	if (olddesc != desc) {
		VZC_DESCRIPTION_VPS_INFO info = { 0 };
		info.description = (PWSTR)QSTR2UTF16(desc);
		WRITE_TRACE(DBG_FATAL, "DESCRIPTION: %ws", info.description);

		VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsInfoClass = DESCRIPTION_VPS_INFO_CLASS;
		pCmd->vpsInfoEntry.vpsId = id;
		pCmd->vpsInfoEntry.vpsInfo = &info;
		pCmd->bPermanent = true;
		ret = pCmd.Execute(m_sErrorMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;
	}

	VZC_RESOURCE_CONTROL_VPS_INFO rc_info = { 0 };
	unsigned int oldramsize = pOldConfig->getVmHardwareList()->getMemory()->getRamSize();
	unsigned int ramsize = pConfig->getVmHardwareList()->getMemory()->getRamSize();
	if (oldramsize != ramsize) {
		// mb -> pages
		WRITE_TRACE(DBG_FATAL, "RAM: %d MB", ramsize);
		rc_info.appVpsMemoryLimit = (ULONG)(ramsize * (1024*1024/si.dwPageSize));
		rc_info.flag.appVpsMemoryLimit = true;
	}

	unsigned long oldcount = pOldConfig->getVmHardwareList()->getCpu()->getNumber();
	unsigned long count = pConfig->getVmHardwareList()->getCpu()->getNumber();
	if (oldcount != count) {
		WRITE_TRACE(DBG_FATAL, "CPU COUNT: %d", count);
		rc_info.cpus = count;
		rc_info.flag.cpus = true;
	}

	PRL_CPULIMIT_DATA oldcpulimit, cpulimit;
	pOldConfig->getVmHardwareList()->getCpu()->getCpuLimitData(&oldcpulimit);
	pConfig->getVmHardwareList()->getCpu()->getCpuLimitData(&cpulimit);
	if (oldcpulimit.value != cpulimit.value || oldcpulimit.type != cpulimit.type || oldcount != count)
	{
		VZC_RESOURCE_CONTROL_VPS_INFO rc_cpu = { 0 };
		unsigned int cpu_nums = pConfig->getVmHardwareList()->getCpu()->getNumber();
		if (cpu_nums == 0)
			cpu_nums = si.dwNumberOfProcessors;
		WRITE_TRACE(DBG_FATAL, "CPU LIMIT: %d %s NUMS: %u",
			    cpulimit.value, (cpulimit.type == PRL_CPULIMIT_PERCENTS ? "%%" : "MHz"), cpu_nums);
		if (cpulimit.type == PRL_CPULIMIT_PERCENTS) {
			rc_cpu.cpuLimit = cpulimit.value / cpu_nums;
			rc_cpu.flag.cpuLimit = true;
		} else {
			rc_cpu.cpuLimitMhz = cpulimit.value / cpu_nums;
			rc_cpu.flag.cpuLimitMhz = true;
		}
		// Set CPU limit separately to handle CpuLimitMhz config value
		VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsInfoClass = RESOURCE_CONTROL_VPS_INFO_CLASS;
		pCmd->vpsInfoEntry.vpsId = id;
		pCmd->vpsInfoEntry.vpsInfo = &rc_cpu;
		pCmd->bPermanent = true;
		ret = pCmd.Execute(m_sErrorMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;
		if (cpulimit.type == PRL_CPULIMIT_PERCENTS) {
			pCfg.DeleteValue("CpuLimitMhz");
		} else {
			pCfg.SetValueDword("CpuLimitMhz", cpulimit.value);
		}
	}

	unsigned long oldunits = pOldConfig->getVmHardwareList()->getCpu()->getCpuUnits();
	unsigned long units = pConfig->getVmHardwareList()->getCpu()->getCpuUnits();
	if (oldunits != units) {
		if (units == PRL_CPU_UNLIMITED)
			units = 0;
		WRITE_TRACE(DBG_FATAL, "CPU UNITS: %d", units);
		rc_info.cpuUnits = units;
		rc_info.flag.cpuUnits = true;
	}

	// RESOURCE_CONTROL_VPS_INFO_CLASS
	if (*(PULONG*)(&rc_info.flag) != 0) {
		VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsInfoClass = RESOURCE_CONTROL_VPS_INFO_CLASS;
		pCmd->vpsInfoEntry.vpsId = id;
		pCmd->vpsInfoEntry.vpsInfo = &rc_info;
		pCmd->bPermanent = true;
		ret = pCmd.Execute(m_sErrorMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;
	}

	VZC_FLAGS_VPS_INFO flags_info = { 0 };
	PRL_VM_AUTOSTART_OPTION oldmode = pOldConfig->getVmSettings()->getVmStartupOptions()->getAutoStart();
	PRL_VM_AUTOSTART_OPTION mode = pConfig->getVmSettings()->getVmStartupOptions()->getAutoStart();
	if (oldmode != mode) {
		WRITE_TRACE(DBG_FATAL, "AUTOSTART: %d %s", mode, (mode == PAO_VM_START_ON_LOAD ? "auto" : "demand"));
		flags_info.flags.autoBoot = (mode == PAO_VM_START_ON_LOAD);
		flags_info.changed.autoBoot = true;
	}
	// VE_TYPE: template or not
	bool tmpl = pConfig->getVmSettings()->getVmCommonOptions()->isTemplate();
	bool oldtmpl = pOldConfig->getVmSettings()->getVmCommonOptions()->isTemplate();
	if (oldtmpl != tmpl) {
		pCfg.SetValueDword(VZCFG_FLAG_CT_TEMPLATE, tmpl);
		pCfg.SetValueDword(VZCFG_FLAG_BOOTDISABLED, tmpl);
	}

	// FLAGS_VPS_INFO_CLASS
	if (flags_info.changed.asUlong != 0) {
		VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsInfoClass = FLAGS_VPS_INFO_CLASS;
		pCmd->vpsInfoEntry.vpsId = id;
		pCmd->vpsInfoEntry.vpsInfo = &flags_info;
		pCmd->bPermanent = true;
		ret = pCmd.Execute(m_sErrorMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;
	}

	// TODO: TEMPLATE CT
	//bool oldtmpl = pOldConfig->getVmSettings()->getVmCommonOptions()->isTemplate();
	//bool tmpl = pConfig->getVmSettings()->getVmCommonOptions()->isTemplate();

	// DISKSPACE
	{
		QList<CVmHardDisk*> lstDisk = pConfig->getVmHardwareList()->m_lstHardDisks;
		QList<CVmHardDisk*> lstOldDisk = pOldConfig->getVmHardwareList()->m_lstHardDisks;
		unsigned long diskSize = 0;
		unsigned long oldDiskSize = 0;
		foreach(CVmHardDisk* pDisk, lstDisk) {
			WRITE_TRACE(DBG_FATAL, "VM DISK size: %uMB type: %u", pDisk->getSize(), pDisk->getEmulatedType());
			//if (pDisk->getEmulatedType() == PVE::ContainerHardDisk) {
			diskSize = pDisk->getSize();
			if (diskSize)
				break;
			//}
		}
		foreach(CVmHardDisk* pDisk, lstOldDisk) {
			WRITE_TRACE(DBG_FATAL, "VM OLDDISK size: %uMB type: %u", pDisk->getSize(), pDisk->getEmulatedType());
			//if (pDisk->getEmulatedType() == PVE::ContainerHardDisk) {
			oldDiskSize = pDisk->getSize();
			if (oldDiskSize)
				break;
			//}
		}
		WRITE_TRACE(DBG_FATAL, "diskSize: %uMB oldDiskSize: %uMB", diskSize, oldDiskSize);
		if (diskSize != 0 && oldDiskSize != 0 && diskSize != oldDiskSize) {
			VZC_DISK_QUOTA_VPS_INFO info = { 0 };
			info.diskQuotaInBytes = (ULONGLONG)diskSize * 1024*1024;
			info.flag.diskQuotaInBytes = 1;
			VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
			if (pCmd == NULL)
				return PRL_ERR_API_WASNT_INITIALIZED;
			pCmd->vpsInfoClass = DISK_QUOTA_VPS_INFO_CLASS;
			pCmd->vpsInfoEntry.vpsId = id;
			pCmd->vpsInfoEntry.vpsInfo = &info;
			pCmd->bPermanent = true;
			ret = pCmd.Execute(m_sErrorMsg);
			if (ret != PRL_ERR_SUCCESS)
				return ret;
		}
	}

	// HOSTNAME
	QString hostname = pConfig->getVmSettings()->getGlobalNetwork()->getHostName();
	QString oldhostname = pOldConfig->getVmSettings()->getGlobalNetwork()->getHostName();
	if (!hostname.isEmpty() && oldhostname != hostname) {
		VZC_HOSTNAME_VPS_INFO info = { 0 };
		WRITE_TRACE(DBG_FATAL, "hostname: %ws", QSTR2UTF16(hostname));
		info.vpsHostname = (PWSTR)QSTR2UTF16(hostname);
		VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsInfoClass = HOSTNAME_VPS_INFO_CLASS;
		pCmd->vpsInfoEntry.vpsId = id;
		pCmd->vpsInfoEntry.vpsInfo = &info;
		pCmd->bPermanent = true;
		ret = pCmd.Execute(m_sErrorMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;
	}

	// NETIF
	QList<CVmGenericNetworkAdapter* > lstNet = pConfig->getVmHardwareList()->m_lstNetworkAdapters;
	QList<CVmGenericNetworkAdapter* > lstOldNet = pOldConfig->getVmHardwareList()->m_lstNetworkAdapters;

	// Check if old adapter was removed
	foreach(CVmGenericNetworkAdapter* pOldNet, lstOldNet) {
		bool found = false;
		QString oldName = pOldNet->getSystemName();
		foreach(CVmGenericNetworkAdapter* pNet, lstNet) {
			if (oldName == pNet->getSystemName()) {
				found = true;
				break;
			}
		}
		if (!found) {
			WRITE_TRACE(DBG_FATAL, "VENET REMOVE: %ws", QSTR2UTF16(oldName));
			VzCmd<VZC_NETADAPTER_VPS_COMMAND> pCmd(VZC_CMD_REMOVE_NETADAPTER_VPS);
			if (pCmd == NULL)
				return PRL_ERR_API_WASNT_INITIALIZED;
			pCmd->vpsId = id;
			pCmd->keyType = VZC_VPS_INFO_KEY_IFNAME;
			pCmd->ifname = (PWSTR)QSTR2UTF16(oldName);
			ret = pCmd.Execute(m_sErrorMsg);
			if (ret != PRL_ERR_SUCCESS)
				return ret;
		}
	}

	// Check if new adapter was added
	int idx = -1;
	foreach(CVmGenericNetworkAdapter* pNet, lstNet) {
		bool found = false;
		QString name = pNet->getSystemName();
		idx++;
		foreach(CVmGenericNetworkAdapter* pOldNet, lstOldNet) {
			if (name == pOldNet->getSystemName()) {
				found = true;
				break;
			}
		}
		if (!found) {
			if (name.isEmpty()) {
				name = QString("venet%1").arg(idx);
				pNet->setSystemName(name);
			}
			WRITE_TRACE(DBG_FATAL, "VENET ADD: %ws", QSTR2UTF16(name));
			VzCmd<VZC_NETADAPTER_VPS_COMMAND> pCmd(VZC_CMD_ADD_NETADAPTER_VPS);
			if (pCmd == NULL)
				return PRL_ERR_API_WASNT_INITIALIZED;
			pCmd->vpsId = id;
			pCmd->keyType = VZC_VPS_INFO_KEY_IFNAME;
			pCmd->ifname = (PWSTR)QSTR2UTF16(name);
			ret = pCmd.Execute(m_sErrorMsg);
			if (ret != PRL_ERR_SUCCESS)
				return ret;
		}
	}

	// Check if adapter params has been changed
	foreach(CVmGenericNetworkAdapter* pNet, lstNet) {
		bool changed = true;
		QString name = pNet->getSystemName();
		CVmGenericNetworkAdapter* pOldNet = NULL;
		foreach(CVmGenericNetworkAdapter* _pOldNet, lstOldNet) {
			pOldNet = _pOldNet;
			if (name == pOldNet->getSystemName()) {
				if (pNet->getVirtualNetworkID() == pOldNet->getVirtualNetworkID() &&
				    pNet->getEmulatedType() == pOldNet->getEmulatedType() &&
				    pNet->getMacAddress() == pOldNet->getMacAddress() &&
				    pNet->getNetAddresses() == pOldNet->getNetAddresses() &&
				    pNet->getDefaultGateway() == pOldNet->getDefaultGateway() &&
				    pNet->getDefaultGatewayIPv6() == pOldNet->getDefaultGatewayIPv6() &&
				    pNet->getDnsIPAddresses() == pOldNet->getDnsIPAddresses() &&
				    pNet->getSearchDomains() == pOldNet->getSearchDomains() &&
				    pNet->isConfigureWithDhcp() == pOldNet->isConfigureWithDhcp() &&
				    pNet->isConfigureWithDhcpIPv6() == pOldNet->isConfigureWithDhcpIPv6())
				{
					changed = false;
				}
				break;
			}
		}
		if (changed) {
			WRITE_TRACE(DBG_FATAL, "VENET CHANGE: %ws", QSTR2UTF16(name));

			VZC_FLAGS_NETIF_VPS_INFO flags_info = { 0 };
			if (pOldNet == NULL || pNet->isConfigureWithDhcp() != pOldNet->isConfigureWithDhcp()) {
				flags_info.flags.dhcp = pNet->isConfigureWithDhcp();
				WRITE_TRACE(DBG_FATAL, "VENET DHCP: %d", flags_info.flags.dhcp);
				flags_info.changed.dhcp = true;
			}
			if (pOldNet == NULL || pNet->isConfigureWithDhcpIPv6() != pOldNet->isConfigureWithDhcpIPv6()) {
				flags_info.flags.dhcpv6 = pNet->isConfigureWithDhcpIPv6();
				WRITE_TRACE(DBG_FATAL, "VENET DHCPV6: %d", flags_info.flags.dhcpv6);
				flags_info.changed.dhcpv6 = true;
			}
			if (pOldNet == NULL || pNet->getEmulatedType() != pOldNet->getEmulatedType()) {
				flags_info.flags.bridged = (pNet->getEmulatedType() == PNA_BRIDGED_ETHERNET);
				WRITE_TRACE(DBG_FATAL, "VENET TYPE: %s", (flags_info.flags.bridged ? "bridged" : "routed"));
				flags_info.changed.bridged = true;
			}

			// FLAGS_NETIF_VPS_INFO_CLASS
			if (flags_info.changed.asUlong) {
				VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
				if (pCmd == NULL)
					return PRL_ERR_API_WASNT_INITIALIZED;
				pCmd->vpsInfoClass = FLAGS_NETIF_VPS_INFO_CLASS;
				pCmd->vpsInfoKey.vpsId = id;
				pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
				wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
				pCmd->vpsInfo = &flags_info;
				pCmd->bPermanent = true;
				ret = pCmd.Execute(m_sErrorMsg);
				if (ret != PRL_ERR_SUCCESS)
					return ret;
			}

			// IPADDRESS
			if (pOldNet == NULL || pNet->getNetAddresses() != pOldNet->getNetAddresses()) {
				if (!pNet->isConfigureWithDhcp() || !pNet->isConfigureWithDhcpIPv6()) {
					QList<QString> lstIps = pNet->getNetAddresses();
					ULONG * ipAddrs = new ULONG[lstIps.size()];
					ULONG * ipMasks = new ULONG[lstIps.size()];
					VZC_IPADDR * ipv6Addrs = new VZC_IPADDR[lstIps.size()];
					int count_v4 = 0;
					int count_v6 = 0;
					foreach(QString ip, lstIps) {
						VZC_IPADDR addr;
						WRITE_TRACE(DBG_FATAL, "IPADDR: %ws", QSTR2UTF16(ip));
						if (parse_ipaddrmask(QSTR2UTF8(ip), &addr, TRUE)) {
							if (addr.family == VZC_AF_INET) {
								ipAddrs[count_v4] = addr.ipv4_addr;
								ipMasks[count_v4++] = ntohl(-1 << (32 - addr.mask));
							}
							if (addr.family == VZC_AF_INET6) {
								ipv6Addrs[count_v6++] = addr;
							}
						}
					}

					if (!pNet->isConfigureWithDhcp())
					{
						VZC_IP_ADDRESS_EX_VPS_INFO info;
						info.ipAddr = ipAddrs;
						info.ipMask = ipMasks;
						info.ipAddrCount = info.ipMaskCount = count_v4;
						VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
						if (pCmd == NULL)
							return PRL_ERR_API_WASNT_INITIALIZED;
						pCmd->vpsInfoClass = IPADDRESS_NETIF_VPS_INFO_CLASS;
						pCmd->vpsInfoKey.vpsId = id;
						pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
						wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
						pCmd->vpsInfo = &info;
						pCmd->bPermanent = true;
						ret = pCmd.Execute(m_sErrorMsg);
						if (ret != PRL_ERR_SUCCESS)
							return ret;
					}

					if (!pNet->isConfigureWithDhcpIPv6())
					{
						VZC_IPV6_ADDRESS_VPS_INFO info;
						info.ipAddr = ipv6Addrs;
						info.ipAddrCount = count_v6;
						VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
						if (pCmd == NULL)
							return PRL_ERR_API_WASNT_INITIALIZED;
						pCmd->vpsInfoClass = IPV6ADDRESS_NETIF_VPS_INFO_CLASS;
						pCmd->vpsInfoKey.vpsId = id;
						pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
						wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
						pCmd->vpsInfo = &info;
						pCmd->bPermanent = true;
						ret = pCmd.Execute(m_sErrorMsg);
						if (ret != PRL_ERR_SUCCESS)
							return ret;
					}

					delete [] ipAddrs;
					delete [] ipMasks;
					delete [] ipv6Addrs;
				}
			}

			// BRIDGED NETID
			if (pOldNet == NULL || pNet->getVirtualNetworkID() != pOldNet->getVirtualNetworkID()) {
				VZC_BRIDGED_NETIF_VPS_INFO info;
				QString netid = pNet->getVirtualNetworkID();
				if (netid.isEmpty())
					netid = QString("Bridged");
				info.netId = (PWSTR)QSTR2UTF16(netid);
				WRITE_TRACE(DBG_FATAL, "BRIDGED NETNAME: %ws", info.netId);
				VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
				if (pCmd == NULL)
					return PRL_ERR_API_WASNT_INITIALIZED;
				pCmd->vpsInfoClass = BRIDGED_NETIF_VPS_INFO_CLASS;
				pCmd->vpsInfoKey.vpsId = id;
				pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
				wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
				pCmd->vpsInfo = &info;
				pCmd->bPermanent = true;
				ret = pCmd.Execute(m_sErrorMsg);
				if (ret != PRL_ERR_SUCCESS)
					return ret;
			}

			// MAC ADDRESS
			if (pOldNet == NULL || pNet->getMacAddress() != pOldNet->getMacAddress()) {
				QString mac = pNet->getMacAddress();
				WRITE_TRACE(DBG_FATAL, "MAC: %ws", QSTR2UTF16(mac));
				VZC_MAC_ADDRESS_INFO info = { 0 };
				for (int i=0; i < 6; ++i) {
					int byte;
					sscanf(QSTR2UTF8(mac.mid(i*2, 2)), "%x", &byte);
					info.addr[i] = (UCHAR)byte;
				}
				VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
				if (pCmd == NULL)
					return PRL_ERR_API_WASNT_INITIALIZED;
				pCmd->vpsInfoClass = MACADDRESS_NETIF_VPS_INFO_CLASS;
				pCmd->vpsInfoKey.vpsId = id;
				pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
				wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
				pCmd->vpsInfo = &info;
				pCmd->bPermanent = true;
				ret = pCmd.Execute(m_sErrorMsg);
				if (ret != PRL_ERR_SUCCESS)
					return ret;
			}

			// GATEWAY v4
			if ((pOldNet == NULL || pNet->getDefaultGateway() != pOldNet->getDefaultGateway()) &&
			    !pNet->isConfigureWithDhcp())
			{
				VZC_IPADDR addr;
				if (parse_ipaddrmask(QSTR2UTF8(pNet->getDefaultGateway()), &addr, TRUE) &&
				    addr.family == VZC_AF_INET)
				{
					WRITE_TRACE(DBG_FATAL, "GW: %ws", QSTR2UTF16(pNet->getDefaultGateway()));
					VZC_IP_ADDRESS_VPS_INFO info;
					info.ipList = &addr.ipv4_addr;
					info.ipListCount = 1;
					VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
					if (pCmd == NULL)
						return PRL_ERR_API_WASNT_INITIALIZED;
					pCmd->vpsInfoClass = IPGATEWAY_NETIF_VPS_INFO_CLASS;
					pCmd->vpsInfoKey.vpsId = id;
					pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
					wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
					pCmd->vpsInfo = &info;
					pCmd->bPermanent = true;
					ret = pCmd.Execute(m_sErrorMsg);
					if (ret != PRL_ERR_SUCCESS)
						return ret;
				}
			}

			// GATEWAY v6
			if ((pOldNet == NULL || pNet->getDefaultGatewayIPv6() != pOldNet->getDefaultGatewayIPv6()) &&
			    !pNet->isConfigureWithDhcpIPv6())
			{
				VZC_IPADDR addr;
				if (parse_ipaddrmask(QSTR2UTF8(pNet->getDefaultGatewayIPv6()), &addr, TRUE) &&
				    addr.family == VZC_AF_INET6)
				{
					WRITE_TRACE(DBG_FATAL, "GWv6: %ws", QSTR2UTF16(pNet->getDefaultGatewayIPv6()));
					VZC_IPV6_ADDRESS_VPS_INFO info;
					info.ipAddr = &addr;
					info.ipAddrCount = 1;
					VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
					if (pCmd == NULL)
						return PRL_ERR_API_WASNT_INITIALIZED;
					pCmd->vpsInfoClass = IPV6GATEWAY_NETIF_VPS_INFO_CLASS;
					pCmd->vpsInfoKey.vpsId = id;
					pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
					wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
					pCmd->vpsInfo = &info;
					pCmd->bPermanent = true;
					ret = pCmd.Execute(m_sErrorMsg);
					if (ret != PRL_ERR_SUCCESS)
						return ret;
				}
			}

			// SEARCHDOMAINS
			if (pOldNet == NULL || pNet->getSearchDomains() != pOldNet->getSearchDomains()) {
				VZC_SEARCHDOMAIN_VPS_INFO info;
				QList<QString> lst = pNet->getSearchDomains();
				info.searchDomainList = new PWSTR [lst.size()];
				info.searchDomainListCount = lst.size();
				int i = 0;
				foreach(QString str, lst) {
					WRITE_TRACE(DBG_FATAL, "SEARCHDOMAINS: %ws", QSTR2UTF16(str));
					info.searchDomainList[i++] = _wcsdup(QSTR2UTF16(str));
				}
				VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
				if (pCmd == NULL)
					return PRL_ERR_API_WASNT_INITIALIZED;
				pCmd->vpsInfoClass = SEARCHDOMAINS_VPS_INFO_CLASS;
				pCmd->vpsInfoKey.vpsId = id;
				pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
				wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
				pCmd->vpsInfo = &info;
				pCmd->bPermanent = true;
				ret = pCmd.Execute(m_sErrorMsg);
				while (i > 0)
					free(info.searchDomainList[--i]);
				delete [] info.searchDomainList;
				if (ret != PRL_ERR_SUCCESS)
					return ret;
			}

			// DNSSERVERS
			if (pOldNet == NULL || pNet->getDnsIPAddresses() != pOldNet->getDnsIPAddresses()) {
				QList<QString> lst = pNet->getDnsIPAddresses();
				VZC_IP_ADDRESS_VPS_INFO info;
				info.ipList = new ULONG [lst.size()];
				int i = 0;
				foreach(QString str, lst) {
					VZC_IPADDR addr;
					WRITE_TRACE(DBG_FATAL, "DNSSERVERS: %ws", QSTR2UTF16(str));
					if (parse_ipaddrmask(QSTR2UTF8(str), &addr, TRUE) &&
					    addr.family == VZC_AF_INET)
					{
						info.ipList[i++] = addr.ipv4_addr;
					}
				}
				info.ipListCount = i;
				if (info.ipListCount) {
					VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
					if (pCmd == NULL)
						return PRL_ERR_API_WASNT_INITIALIZED;
					pCmd->vpsInfoClass = DNS_SERVERS_NETIF_VPS_INFO_CLASS;
					pCmd->vpsInfoKey.vpsId = id;
					pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
					wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
					pCmd->vpsInfo = &info;
					pCmd->bPermanent = true;
					ret = pCmd.Execute(m_sErrorMsg);
					if (ret != PRL_ERR_SUCCESS) {
						delete [] info.ipList;
						return ret;
					}
				}
				delete [] info.ipList;
			}
		}
	}

	// SEARCHDOMAINS
	QList<QString> searchdomains = pConfig->getVmSettings()->getGlobalNetwork()->getSearchDomains();
	QList<QString> oldsearchdomains = pOldConfig->getVmSettings()->getGlobalNetwork()->getSearchDomains();
	if (oldsearchdomains != searchdomains) {
		VZC_SEARCHDOMAIN_VPS_INFO info;
		info.searchDomainList = new PWSTR [searchdomains.size()];
		info.searchDomainListCount = searchdomains.size();
		int i = 0;
		foreach(QString str, searchdomains) {
			WRITE_TRACE(DBG_FATAL, "GLOBAL SEARCHDOMAINS: %ws", QSTR2UTF16(str));
			info.searchDomainList[i++] = _wcsdup(QSTR2UTF16(str));
		}
		VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsInfoClass = SEARCHDOMAINS_VPS_INFO_CLASS;
		pCmd->vpsInfoEntry.vpsId = id;
		pCmd->vpsInfoEntry.vpsInfo = &info;
		pCmd->bPermanent = true;
		ret = pCmd.Execute(m_sErrorMsg);
		while (i > 0)
			free(info.searchDomainList[--i]);
		delete [] info.searchDomainList;
		if (ret != PRL_ERR_SUCCESS)
			return ret;
	}

	// DNSSERVERS
	QList<QString> dnsservers = pConfig->getVmSettings()->getGlobalNetwork()->getDnsIPAddresses();
	QList<QString> olddnsservers = pOldConfig->getVmSettings()->getGlobalNetwork()->getDnsIPAddresses();
	if (olddnsservers != dnsservers) {
		VZC_IP_ADDRESS_VPS_INFO info;
		info.ipList = new ULONG [dnsservers.size()];
		int i = 0;
		foreach(QString str, dnsservers) {
			VZC_IPADDR addr;
			WRITE_TRACE(DBG_FATAL, "GLOBAL DNSSERVERS: %ws", QSTR2UTF16(str));
			if (parse_ipaddrmask(QSTR2UTF8(str), &addr, TRUE) &&
			    addr.family == VZC_AF_INET)
			{
				info.ipList[i++] = addr.ipv4_addr;
			}
		}
		info.ipListCount = i;
		if (info.ipListCount) {
			foreach(CVmGenericNetworkAdapter* pNet, lstNet) {
				QString name = pNet->getSystemName();
				VzCmd<VZC_SET_EX_VPS_COMMAND> pCmd(VZC_CMD_SET_EX_VPS);
				if (pCmd == NULL)
					return PRL_ERR_API_WASNT_INITIALIZED;
				pCmd->vpsInfoClass = DNS_SERVERS_NETIF_VPS_INFO_CLASS;
				pCmd->vpsInfoKey.vpsId = id;
				pCmd->vpsInfoKey.type = VZC_VPS_INFO_KEY_IFNAME;
				wcscpy(pCmd->vpsInfoKey.ifname, (PWSTR)QSTR2UTF16(name));
				pCmd->vpsInfo = &info;
				pCmd->bPermanent = true;
				ret = pCmd.Execute(m_sErrorMsg);
				if (ret != PRL_ERR_SUCCESS) {
					delete [] info.ipList;
					return ret;
				}
			}
		}
		delete [] info.ipList;
	}

	// RATE
	CVmNetworkRates *pRates = pConfig->getVmSettings()->getGlobalNetwork()->getNetworkRates();
	CVmNetworkRates *pOldRates = pOldConfig->getVmSettings()->getGlobalNetwork()->getNetworkRates();
	if (pRates->toString() != pOldRates->toString())
		ret = CVzHelper::set_rate(id, pRates);

	// Handle the Basic Firewall settings change
	QStringList lstFullItemIds;
	pConfig->diffDocuments(pOldConfig.getImpl(), lstFullItemIds);
	QString qsDiff = lstFullItemIds.join(" ");
	if (qsDiff.contains(".Firewall."))
		ret = set_firewall(id, pConfig, pOldConfig, m_sErrorMsg);

	return ret;
}

int CVzOperationHelper::apply_env_config(SmartPtr<CVmConfiguration> &pConfig, SmartPtr<CVmConfiguration> &pOldConfig,
			unsigned int nFlags)
{
	int ret = CVzOperationHelper::apply_env_config_internal(pConfig, pOldConfig, nFlags);
	if (ret != PRL_ERR_SUCCESS) {
		// try to restore old configuration
		pConfig = CVzHelper::get_env_config(pOldConfig->getVmIdentification()->getEnvId());
		CVzOperationHelper::apply_env_config_internal(pOldConfig, pConfig, nFlags);
	}
	return ret;
}

int CVzOperationHelper::register_env(const QString &sPath, unsigned int envid,
		const QString &sUuid, PRL_UINT32 nFlags, SmartPtr<CVmConfiguration> &pConfig)
{
	Q_UNUSED(sUuid);
	Q_UNUSED(nFlags);

	WRITE_TRACE(DBG_FATAL, "Register Container at '%s'", QSTR2UTF8(sPath));

	if (hVzcapi == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	if (envid == 0) {
		int ret = get_free_env_id(&envid);
		if (ret)
			return ret;
	}

	// Store config at envid location
	VzCfg pCfg(envid, QSTR2UTF16(QString("%1\\" VZ_CT_CONFIG_FILE).arg(sPath)));
	if (pCfg == NULL)
		return PRL_ERR_VZ_OPERATION_FAILED;

	// Update private path
	VZC_PRIVATE_VPS_INFO info = { 0 };
	// need Windows path separator here
	QString sTmp = sPath;
	sTmp.replace('/', '\\');
	info.privatePath = (PWSTR)QSTR2UTF16(sTmp);
	VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsInfoClass = PRIVATE_VPS_INFO_CLASS;
	pCmd->vpsInfoEntry.vpsId = envid;
	pCmd->vpsInfoEntry.vpsInfo = &info;
	pCmd->bPermanent = true;
	int ret = pCmd.Execute(m_sErrorMsg);
	if (ret != PRL_ERR_SUCCESS) {
		pCfg.DeleteConfig();
		return ret;
	}

	WRITE_TRACE(DBG_FATAL, "Assigned Container id %d", envid);
	pConfig = CVzHelper::get_env_config(envid);

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::unregister_env(const QString &uuid, int flags)
{
	Q_UNUSED(flags);

	if (hVzcapi == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	WRITE_TRACE(DBG_FATAL, "Unregister Container %s", QSTR2UTF8(uuid));

	unsigned int envid = CVzHelper::get_envid_by_uuid(uuid);
	if (envid == 0)
		return PRL_ERR_CT_NOT_FOUND;

	VzCfg pCfg(envid);
	if (pCfg == NULL)
		return PRL_ERR_CT_NOT_FOUND;
	WCHAR path[MAX_PATH+1];
	pVZC_GetVpsPrivatePathW(path, MAX_PATH, envid, L"ve.conf");
	if (!pCfg.WriteConfig(path))
		return PRL_ERR_VZ_OPERATION_FAILED;

	int ret = pCfg.DeleteConfig();
	if (ret)
		return ret;

	WRITE_TRACE(DBG_FATAL, "Unregister Container %s with ctid %u completed", QSTR2UTF8(uuid), envid);

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::create_env(const QString &dst, SmartPtr<CVmConfiguration> &pConfig,
		PRL_UINT32 flags)
{
	Q_UNUSED(flags)
	QStringList args;
	QString name = pConfig->getVmIdentification()->getVmName();
	QString ostemplate = pConfig->getCtSettings()->getOsTemplate();
	unsigned int id = 0;
	bool bIsNum = 0;
	int ret = PRL_ERR_SUCCESS;
	QString uuid;

	WRITE_TRACE(DBG_FATAL, "CREATE name: %s template: %s", QSTR2UTF8(name), QSTR2UTF8(ostemplate));

	id = name.toUInt(&bIsNum);
	if (!bIsNum) {
		// generate id;
		ret = get_free_env_id(&id);
		if (ret)
			return ret;
	}

	if (id == 0) {
		WRITE_TRACE(DBG_FATAL, "Invalid Container id");
		return PRL_ERR_OPERATION_FAILED;
	}

	if (!bIsNum) {
		int res = set_env_name(id, name);
		if (PRL_FAILED(res))
			return res;
	}

	// PRIVATE path
	if (!dst.isEmpty()) {
		VZC_PRIVATE_VPS_INFO info = { 0 };
		// need Windows path separator here
		QString sTmp = dst;
		sTmp.replace('/', '\\');
		info.privatePath = (PWSTR)QSTR2UTF16(sTmp);
		VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsInfoClass = PRIVATE_VPS_INFO_CLASS;
		pCmd->vpsInfoEntry.vpsId = id;
		pCmd->vpsInfoEntry.vpsInfo = &info;
		pCmd->bPermanent = true;
		ret = pCmd.Execute(m_sErrorMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;
	}

	// CREATE command
	{
		VzCmd<VZC_CREATE_VPS_COMMAND> pCmd(VZC_CMD_CREATE_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsId = id;

		// TEMPLATE
		pCmd->pkgSet = (PWSTR)ostemplate.utf16();

		// DISKSIZE
		// Same as linux CT - find size of first disk
		{
			QList<CVmHardDisk*> lstDisk = pConfig->getVmHardwareList()->m_lstHardDisks;
			unsigned long diskSize = 0;

			foreach(CVmHardDisk* pDisk, lstDisk) {
				WRITE_TRACE(DBG_FATAL, "VM DISK size: %uMB type: %u", pDisk->getSize(), pDisk->getEmulatedType());
				//if (pDisk->getEmulatedType() == PVE::ContainerHardDisk) {
				diskSize = pDisk->getSize();
				if (diskSize) {
					// Mb -> bytes
					WRITE_TRACE(DBG_FATAL, "CREATE size: %uMB", diskSize);
					pCmd->diskQuotaInBytes = (ULONGLONG)diskSize * 1024*1024;
				}
				//}
			}
		}

		ret = pCmd.Execute(m_sErrorMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;
	}

	// Remove default VZWIN netadapter
	{
		VzCmd<VZC_NETADAPTER_VPS_COMMAND> pCmd(VZC_CMD_REMOVE_NETADAPTER_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsId = id;
		pCmd->keyType = VZC_VPS_INFO_KEY_IFNAME;
		pCmd->ifname = L"venet0";
		ret = pCmd.Execute(m_sErrorMsg);
		if (ret != PRL_ERR_SUCCESS)
			return ret;
	}

	// Apply all other params
	SmartPtr<CVmConfiguration> pOldConfig = CVzHelper::get_env_config(id);

	pConfig->getVmIdentification()->setVmUuid(pOldConfig->getVmIdentification()->getVmUuid());
	pConfig->getVmIdentification()->setVmName(name);
	pConfig->getVmIdentification()->setEnvId(id);

	ret = apply_env_config(pConfig, pOldConfig);
	if (ret != PRL_ERR_SUCCESS)
		delete_env(id);

	return ret;
}

int CVzOperationHelper::clone_env(const QString &uuid, const QString &sNewHome,
				  const QString &sNewName, PRL_UINT32 nFlags, SmartPtr<CVmConfiguration> &pNewConfig)
{
	unsigned int srcid, dstid;
	QStringList args;
	bool bIsNum;
	int ret;

	if (hVzcapi == NULL)
		return PRL_ERR_VZ_API_NOT_INITIALIZED;

	dstid = sNewName.toUInt(&bIsNum);
	if (!bIsNum) {
		// generate id;
		ret = get_free_env_id(&dstid);
		if (ret)
			return PRL_ERR_OPERATION_FAILED;
	}
	if (dstid == 0) {
		WRITE_TRACE(DBG_FATAL, "Invalid Container id");
		return PRL_ERR_OPERATION_FAILED;
	}

	srcid = CVzHelper::get_envid_by_uuid(uuid);

	VzCmd<VZC_CLONE_VPS_COMMAND> pCmd(VZC_CMD_CLONE_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = srcid;
	pCmd->newVpsId = dstid;
	if (!sNewHome.isEmpty()) {
		QString path = QString("%1\\%2").arg(sNewHome).arg(dstid);
		pCmd->newPrivate = (PWSTR)path.utf16();
	}

	int res = pCmd.Execute(m_sErrorMsg);

	if (PRL_FAILED(res))
		return res;

	if (!bIsNum) {
		res = set_env_name(dstid, sNewName);
		if (PRL_FAILED(res)) {
			delete_env(dstid);
			return res;
		}
	}


	pNewConfig = CVzHelper::get_env_config(dstid);
	SmartPtr<CVmConfiguration> pOldConfig(new CVmConfiguration(pNewConfig->toString()));

	bool isTemplate = (nFlags & PCVF_CLONE_TO_TEMPLATE);
	pNewConfig->getVmSettings()->getVmCommonOptions()->setTemplate(isTemplate);

	// Update Config
	foreach(CVmGenericNetworkAdapter *pNetAdapter, pNewConfig->getVmHardwareList()->m_lstNetworkAdapters) {
		// regenerate mac address for cloned ct
		pNetAdapter->setMacAddress(HostUtils::generateMacAddress());

		// reset IP addresses for templates
		if (isTemplate)
			pNetAdapter->setNetAddresses();
	}

	ret = apply_env_config(pNewConfig, pOldConfig);
	pNewConfig = CVzHelper::get_env_config(dstid);

	return ret;
}

int CVzOperationHelper::get_resize_env_info(const QString &uuid, CDiskImageInfo &di)
{
	Q_UNUSED(uuid);
	Q_UNUSED(di);
	return PRL_ERR_UNIMPLEMENTED;
}

int CVzOperationHelper::create_env_disk(const QString &uuid, const CVmHardDisk &disk)
{
	Q_UNUSED(uuid);
	Q_UNUSED(disk);
	return PRL_ERR_UNIMPLEMENTED;
}

int CVzOperationHelper::create_disk_image(const QString &path, quint64 sizeBytes);
{
	Q_UNUSED(path);
	Q_UNUSED(sizeBytes);
	return PRL_ERR_UNIMPLEMENTED;
}

int CVzOperationHelper::resize_env_disk(const QString &uuid, const QString &sPath,
		unsigned int nNewSize, unsigned int flags)
{
	Q_UNUSED(sPath);
	Q_UNUSED(flags);

	VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	VZC_DISK_QUOTA_VPS_INFO info = { 0 };
	// Mb -> bytes
	info.diskQuotaInBytes = (ULONGLONG)nNewSize * 1024*1024;
	info.flag.diskQuotaInBytes = 1;

	pCmd->vpsInfoClass = DISK_QUOTA_VPS_INFO_CLASS;
	pCmd->vpsInfoEntry.vpsId = CVzHelper::get_envid_by_uuid(uuid);
	pCmd->vpsInfoEntry.vpsInfo = &info;
	pCmd->bPermanent = true;

	return pCmd.Execute(m_sErrorMsg);
}

int CVzOperationHelper::start_env(const QString &uuid, PRL_UINT32 nFlags)
{
	Q_UNUSED(nFlags);
	VzCmd<VZC_START_VPS_COMMAND> pCmd(VZC_CMD_START_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = CVzHelper::get_envid_by_uuid(uuid);
	return pCmd.Execute(m_sErrorMsg);
}

int CVzOperationHelper::stop_env(const QString &uuid, PRL_UINT32 nMode)
{
	Q_UNUSED(nMode);
	VzCmd<VZC_STOP_VPS_COMMAND> pCmd(VZC_CMD_STOP_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = CVzHelper::get_envid_by_uuid(uuid);
	return pCmd.Execute(m_sErrorMsg);
}

int CVzOperationHelper::restart_env(const QString &uuid)
{
	int ret;
	VIRTUAL_MACHINE_STATE nState;

	ret = CVzHelper::get_env_status(uuid, nState);
	if (ret)
		return ret;
	if (nState == VMS_RUNNING) {
		ret = stop_env(uuid, PSM_SHUTDOWN);
		if (ret)
			return ret;
	}
	return CVzOperationHelper::start_env(uuid, 0);
}

int CVzOperationHelper::mount_env(const QString &uuid)
{
	VzCmd<VZC_MOUNT_VPS_COMMAND> pCmd(VZC_CMD_MOUNT_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = CVzHelper::get_envid_by_uuid(uuid);
	return pCmd.Execute(m_sErrorMsg);
}

int CVzOperationHelper::umount_env(const QString &uuid)
{
	VzCmd<VZC_MOUNT_VPS_COMMAND> pCmd(VZC_CMD_UMOUNT_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = CVzHelper::get_envid_by_uuid(uuid);
	return pCmd.Execute(m_sErrorMsg);
}

int CVzOperationHelper::suspend_env(const QString &uuid)
{
	Q_UNUSED(uuid);
	return PRL_ERR_UNIMPLEMENTED;
}

int CVzOperationHelper::resume_env(const QString &uuid)
{
	Q_UNUSED(uuid);
	return PRL_ERR_UNIMPLEMENTED;
}

int CVzOperationHelper::delete_env(const QString &uuid)
{
	return delete_env(CVzHelper::get_envid_by_uuid(uuid));
}

int CVzOperationHelper::delete_env(unsigned int id)
{
	VzCmd<VZC_DESTROY_VPS_COMMAND> pCmd(VZC_CMD_DESTROY_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = id;
	return pCmd.Execute(m_sErrorMsg);
}

int CVzOperationHelper::set_env_userpasswd(const QString &uuid, const QString &user,
		const QString &pw, PRL_UINT32 nFlags)
{
	Q_UNUSED(nFlags);

	VZC_USERPASSWORD userpwd[1];
	VZC_USERPASSWORD_VPS_INFO info = { 0 };

	VzCmd<VZC_SET_VPS_COMMAND> pCmd(VZC_CMD_SET_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	pCmd->vpsInfoEntry.vpsId = CVzHelper::get_envid_by_uuid(uuid);
	pCmd->vpsInfoEntry.vpsInfo = &info;
	pCmd->vpsInfoClass = USERPASSWORD_VPS_INFO_CLASS;
	pCmd->bPermanent = TRUE;
	info.userPasswordList = userpwd;
	info.userPasswordCount = 1;
	userpwd[0].username = (PWSTR)user.utf16();
	userpwd[0].password = (PWSTR)pw.utf16();

	return pCmd.Execute(m_sErrorMsg);
}

int CVzOperationHelper::auth_env_user(const QString &uuid, const QString &user, const QString &pw)
{
	VzCmd<VZC_AUTHORIZE_VPS_COMMAND> pCmd(VZC_CMD_AUTHORIZE_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = CVzHelper::get_envid_by_uuid(uuid);
	pCmd->login = (PWSTR)user.utf16();
	pCmd->password = (PWSTR)pw.utf16();
	return pCmd.Execute(m_sErrorMsg);
}

int CVzHelper::set_env_uptime(const QString &uuid, const quint64 uptime, const QDateTime & date)
{
	Q_UNUSED(uuid);
	Q_UNUSED(uptime);
	Q_UNUSED(date);
	return PRL_ERR_UNIMPLEMENTED;
}

int CVzHelper::reset_env_uptime(const QString &uuid)
{
	Q_UNUSED(uuid);
	return PRL_ERR_UNIMPLEMENTED;
}

int CVzHelper::install_templates_env(QString uuid, QStringList &lstVzTmpl)
{
	unsigned int id = get_envid_by_uuid(uuid);
	int ret= PRL_ERR_SUCCESS;

	if (lstVzTmpl.isEmpty())
		return ret;

	VzCmd<VZC_TEMPLATES_ADD_VPS_COMMAND> pCmd(VZC_CMD_TEMPLATES_ADD_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	pCmd->pkgSets = (PWSTR*) _alloca(sizeof(PWSTR) * lstVzTmpl.size());
	if (pCmd->pkgSets == NULL)
		return PRL_ERR_OUT_OF_MEMORY;
	int i = 0;
	foreach(QString str, lstVzTmpl) {
		PCWSTR pwStr = (PCWSTR)QSTR2UTF16(str);
		pCmd->pkgSets[i] = (PWSTR) _alloca(sizeof(WCHAR) * (wcslen(pwStr) + 1));
		if (pCmd->pkgSets[i] == NULL)
			return PRL_ERR_OUT_OF_MEMORY;
		wcscpy(pCmd->pkgSets[i++], pwStr);
	}
	pCmd->count = i;
	pCmd->vpsId = (ULONG) id;
	ret = pCmd.Execute();

	return ret;
}

int CVzHelper::remove_templates_env(QString uuid, QStringList &lstVzTmpl)
{
	unsigned int id = get_envid_by_uuid(uuid);
	int ret  = PRL_ERR_SUCCESS;

	if (lstVzTmpl.isEmpty())
		return ret;

	foreach(QString str, lstVzTmpl) {
		VzCmd<VZC_TEMPLATE_REMOVE_VPS_COMMAND> pCmd(VZC_CMD_TEMPLATE_REMOVE_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;

		pCmd->vpsId = (ULONG) id;
		pCmd->pkgSet = (PWSTR)QSTR2UTF16(str);
		ret = pCmd.Execute();
		if (ret != PRL_ERR_SUCCESS)
			return ret;
	}

	return ret;
}

int CVzHelper::convert_os_ver(const QString& osname, const QString& osver)
{
	/* VZWIN template has no version info, instead it matches host OS version */
	Q_UNUSED(osname);
	Q_UNUSED(osver);
	OSVERSIONINFO info = { sizeof OSVERSIONINFO };
	GetVersionEx(&info);
	int osVer;
	if (info.dwMajorVersion == 5 && info.dwMinorVersion == 2)
		osVer = PVS_GUEST_VER_WIN_2003;
	else if (info.dwMajorVersion == 6 && info.dwMinorVersion <= 1)
		osVer = PVS_GUEST_VER_WIN_2008;
	else if (info.dwMajorVersion == 6 && info.dwMinorVersion == 2)
		osVer = PVS_GUEST_VER_WIN_2012;
	else
		osVer = PVS_GUEST_VER_WIN_OTHER;
	return osVer;
}

static SmartPtr<CtTemplate> fill_template_info(PVZC_TEMPLATE_EX_ENTRY info)
{
	SmartPtr<CtTemplate> pTmpl(new CtTemplate);
	pTmpl->setName(UTF16_2QSTR(info->templateName));
	pTmpl->setVersion(UTF16_2QSTR(info->templateVersion));
	pTmpl->setDescription(UTF16_2QSTR(info->templateDescription));
	if (info->isOsTemplate) {
		pTmpl->setType(PCT_TYPE_EZ_OS);
		pTmpl->setOsVersion(
			CVzHelper::convert_os_ver(UTF16_2QSTR(info->templateName),
						  UTF16_2QSTR(info->templateVendorVersion)));
	} else {
		pTmpl->setType(PCT_TYPE_EZ_APP);
		/* VZWIN app is not bound to os template */
		//pTmpl->setOsTemplate(QString(*os));
	}
	if (info->arch && !_wcsicmp(info->arch, L"x86_64"))
		pTmpl->setCpuMode(PCM_CPU_MODE_64);
	pTmpl->setOsType(PVS_GUEST_TYPE_WINDOWS);
	if (info->isInstalled)
		pTmpl->setCached(true);

	return pTmpl;
}

int CVzHelper::get_templates(QList<SmartPtr<CtTemplate> > &lstVzTmpl)
{
	VzCmd<VZC_ENUMERATE_TEMPLATES_EX_COMMAND> pCmd(VZC_CMD_ENUMERATE_TEMPLATES_EX);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	int ret = pCmd.Execute();
	if (ret != PRL_ERR_SUCCESS)
		return ret;

	for (ULONG i=0; i < pCmd->entryListCount; ++i) {
		SmartPtr<CtTemplate> pTmpl = fill_template_info(pCmd->entryList[i]);
		lstVzTmpl.append(pTmpl);
	}

	return 0;
}

PRL_RESULT CVzHelper::install_template(QString sName, QString sOsTmplName)
{
	if (sName.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Template name is empty");
		return PRL_ERR_INVALID_ARG;
	}
	VzCmd<VZC_TEMPLATE_INSTALL_COMMAND> pCmd(VZC_CMD_TEMPLATE_INSTALL);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->templateName = (PWSTR)QSTR2UTF16(sName);
	return pCmd.Execute();
}

PRL_RESULT CVzHelper::remove_template(QString sName, QString sOsTmplName)
{
	if (sName.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Template name is empty");
		return PRL_ERR_INVALID_ARG;
	}
	VzCmd<VZC_TEMPLATE_UNINSTALL_COMMAND> pCmd(VZC_CMD_TEMPLATE_UNINSTALL);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->templateName = (PWSTR)QSTR2UTF16(sName);
	return pCmd.Execute();
}


PRL_RESULT CVzHelper::is_ostemplate_exists(const QString &sOsTemplate)
{
	if (sOsTemplate.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "OS template name is empty");
		return PRL_ERR_INVALID_ARG;
	}
	VzCmd<VZC_QUERY_TEMPLATE_INFO_COMMAND> pCmd(VZC_CMD_QUERY_TEMPLATE_INFO);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->templateName = (PWSTR)QSTR2UTF16(sOsTemplate);
	PRL_RESULT res = pCmd.Execute();
	if (res == PRL_ERR_SUCCESS && pCmd->templateInfo != NULL) {
		if (!pCmd->templateInfo->isOsTemplate)
			res = PRL_ERR_VZ_OSTEMPLATE_NOT_FOUND;
	}
	return  res;
}

int CVzHelper::getTemplateInfo(
		const QString &name,
		SmartPtr<CtTemplate> *ctTemplate,
		QString *installPath,
		QString *archivePath,
		bool *isInstalled,
		bool *isArchived)
{
	PCWSTR pName = QSTR2UTF16(name);
	if (name.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Template name is empty");
		return PRL_ERR_INVALID_ARG;
	}
	VzCmd<VZC_QUERY_TEMPLATE_INFO_COMMAND> pCmd(VZC_CMD_QUERY_TEMPLATE_INFO);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->templateName = (PWSTR)QSTR2UTF16(name);
	PRL_RESULT res = pCmd.Execute();
	if (res == PRL_ERR_SUCCESS && pCmd->templateInfo != NULL) {
		PVZC_TEMPLATE_EX_ENTRY p = pCmd->templateInfo;

		/* workaround for #PCWIN-11118 bugfix - do not use match that ignores version */
		if (name.contains('/') && _wcsicmp(pName, p->templateName) != 0)
			return  PRL_ERR_TEMPLATE_NOT_FOUND;

		if (ctTemplate)
			*ctTemplate = fill_template_info(p);
		if (installPath)
			*installPath = UTF16_2QSTR(p->installPath);
		if (archivePath)
			*archivePath = UTF16_2QSTR(p->archivePath);
		if (isInstalled)
			*isInstalled = p->isInstalled;
		if (isArchived)
			*isArchived = p->isCached;
		return  PRL_ERR_SUCCESS;
	}
	return  PRL_ERR_TEMPLATE_NOT_FOUND;
}

QString CVzHelper::getCtPrivatePath(unsigned int ctid)
{
	WCHAR buf[MAX_PATH+1];
	pVZC_GetVpsPrivatePathW(buf, MAX_PATH, ctid, NULL);
	return UTF16_2QSTR(buf);
}

QString CVzHelper::getCtConfPath(unsigned int ctid)
{
	WCHAR buf[MAX_PATH+1];
	pVZC_GetVpsConfPathW(buf, MAX_PATH, ctid);
	return UTF16_2QSTR(buf);
}

int CVzHelper::lock_env(unsigned int id, int state, int substate, void ** opaque)
{
	VzCmd<VZC_LOCK_VPS_COMMAND> pCmd(VZC_CMD_LOCK_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = id;
	pCmd->state = state;
	pCmd->substate = substate;
	int ret = pCmd.Execute();
	*opaque = pCmd->opaque;
	return ret;
}

int CVzHelper::unlock_env(unsigned int id, void ** opaque)
{
	VzCmd<VZC_LOCK_VPS_COMMAND> pCmd(VZC_CMD_UNLOCK_VPS);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = id;
	pCmd->opaque = *opaque;
	*opaque = NULL;
	return pCmd.Execute();
}

/*
 * VZLin version of locking - do nothing on vzwin due to different semantic
 * On Windows, use the versions above.
 */
int CVzHelper::lock_env(unsigned int id, const char *status)
{
	Q_UNUSED(id);
	Q_UNUSED(status);
	return 1;
}

void CVzHelper::unlock_env(unsigned int id, int lockfd)
{
	Q_UNUSED(id);
	Q_UNUSED(lockfd);
}

int CVzHelper::store_config_env(unsigned int id)
{
	VzCfg pCfg(id);
	WCHAR path[MAX_PATH+1];
	WCHAR conf[MAX_PATH+1];
	_snwprintf(conf, MAX_PATH, L"%u.conf", id);
	pVZC_GetVpsPrivatePathW(path, MAX_PATH, id, conf);
	if (!pCfg.WriteConfig(path))
		return PRL_ERR_VZ_OPERATION_FAILED;
	return PRL_ERR_SUCCESS;
}

int CVzHelper::restore_config_env(unsigned int id)
{
	WCHAR path[MAX_PATH+1];
	WCHAR conf[MAX_PATH+1];
	_snwprintf(conf, MAX_PATH, L"%u.conf", id);
	pVZC_GetVpsPrivatePathW(path, MAX_PATH, id, conf);
	VzCfg pCfg(id, path);
	if (pCfg == NULL)
		return PRL_ERR_VZ_OPERATION_FAILED;
	return PRL_ERR_SUCCESS;
}

int CVzHelper::src_start_migrate_env(unsigned int id, void ** opaque)
{
	int ret = lock_env(id, VE_STATE_ANY, VE_SUBSTATE_MIGRATING, opaque);
	if (ret)
		return ret;
	// save config to private
	store_config_env(id);
	return ret;
}

int CVzHelper::src_complete_migrate_env(unsigned int id, void ** opaque, bool fDeleteSource)
{
	VzCmd<VZC_COMPLETE_SRC_MIGRATION_COMMAND> pCmd(VZC_CMD_COMPLETE_SRC_MIGRATION);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = id;
	// delete CT if not in clone mode
	pCmd->flag.fDeleteLocalIfSuccess = fDeleteSource;
	// and in clone mode, keep even CT conf
	pCmd->flag.fReallyKeepLocal = !fDeleteSource;
	int ret = pCmd.Execute();

	unlock_env(id, opaque);

	return ret;
}

int CVzHelper::dst_start_migrate_env(unsigned int * pId, void ** opaque)
{
	ULONG id = *pId;
	for (;;)
	{
		// 1st check if id is free
		VzCmd<VZC_QUERY_VPS_COMMAND> pCmd(VZC_CMD_QUERY_VPS);
		if (pCmd == NULL)
			return PRL_ERR_API_WASNT_INITIALIZED;
		pCmd->vpsIdList = &id;
		pCmd->vpsIdListCount = 1;
		pCmd->vpsInfoClass = STATUS_VPS_INFO_CLASS;
		if (pCmd.Execute() == PRL_ERR_SUCCESS && pCmd->vpsInfoListCount == 1) {
			get_free_env_id(pId);
			id = *pId;
			continue;
		}
		// 2nd check try to lock it
		int ret = lock_env(id, VE_STATE_NONE, VE_SUBSTATE_MIGRATING, opaque);
		if (ret != PRL_ERR_SUCCESS) {
			get_free_env_id(pId);
			id = *pId;
			continue;
		}
		// 3rd check if it yet free as locked
		pCmd->vpsIdList = &id;
		pCmd->vpsIdListCount = 1;
		pCmd->vpsInfoClass = STATUS_VPS_INFO_CLASS;
		if (pCmd.Execute() == PRL_ERR_SUCCESS && pCmd->vpsInfoListCount == 1) {
			PVZC_STATUS_VPS_INFO  p = (PVZC_STATUS_VPS_INFO)pCmd->vpsInfoList[0].vpsInfo;
			if (p->status != VE_STATE_NONE) {
				unlock_env(id, opaque);
				*opaque = NULL;
				get_free_env_id(pId);
				id = *pId;
				continue;
			}
		}
		// if all checks passed, it's free
		break;
	}
	return PRL_ERR_SUCCESS;
}

int CVzHelper::dst_complete_migrate_env(
	const QString&uuid,
	unsigned int id, unsigned int origin_id,
	void ** opaque)
{
	WCHAR conf[64];
	_snwprintf(conf, sizeof(conf)/sizeof(*conf), L"%u.conf", id);
	WCHAR buf[MAX_PATH+1];
	pVZC_GetVpsPrivatePathW(buf, MAX_PATH, id, conf);
	// in brackets for free
	{
		VzCfg pCfg(buf);
		// here is a tricky part - change CT and net adapeters guid in clone mode
		if (uuid.compare(UTF16_2QSTR(pCfg.GetValue(VZCFG_VPS_GUID)), Qt::CaseInsensitive) != 0) {
			pCfg.SetValue(VZCFG_VPS_GUID, QSTR2UTF16(uuid.toLower()));
			for (int i=0; i < 64; ++i) {
				char key[64];
				_snprintf(key, sizeof(key), "NetIf%d.AdapterGuid", i);
				if (pCfg.GetValue(key) != NULL)
					pCfg.SetValue(key, QSTR2UTF16(Uuid::createUuid().toString().toUpper()));
			}
			// get rid of PVA eid.conf
			pVZC_GetVpsPrivatePathW(buf, MAX_PATH, id, L".vza\\eid.conf");
			DeleteFile(buf);
			pVZC_GetVpsPrivatePathW(buf, MAX_PATH, id, L".vza");
			RemoveDirectory(buf);
		}
	}
	VzCmd<VZC_COMPLETE_DST_MIGRATION_COMMAND> pCmd(VZC_CMD_COMPLETE_DST_MIGRATION);
	if (pCmd == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;
	pCmd->vpsId = id;
	pCmd->oldVpsId = origin_id;
	pCmd->flag.fDontStartAfterMigration = TRUE;
	int ret = pCmd.Execute();

	unlock_env(id, opaque);

	return ret;
}

/************************************************************************/
void CVzOperationHelper::cancel_operation()
{
	get_cleaner().process();
}

/***********************************************************************/
void CVzOperationCleaner::add(cleanup_callback_FN fn, const QString &sData)
{
	callback_entry entry(fn);

	entry.data.sData = sData;
	m_stack.push(entry);
}

void CVzOperationCleaner::add(cleanup_callback_FN fn, int nInt)
{
	callback_entry entry(fn);

	entry.data.nInt = nInt;
	m_stack.push(entry);
}

void CVzOperationCleaner::add(cleanup_callback_FN fn, void *ptr)
{
	callback_entry entry(fn);

	entry.data.ptr = ptr;
	m_stack.push(entry);
}

void CVzOperationCleaner::pop()
{
	if (!m_stack.isEmpty())
		m_stack.pop();
}

void CVzOperationCleaner::process()
{
	while (!m_stack.isEmpty()) {
		callback_entry &entry = m_stack.top();
		entry.fn(&entry.data);
		m_stack.pop();
	}
}

void CVzOperationCleaner::kill_process(callback_data *data)
{
	int pid = data->nInt;

	if (pid == 0)
		return;

	WRITE_TRACE(DBG_FATAL, "kill_process: %d", pid);
}

void CVzOperationCleaner::vzctl_cancel_operation(callback_data *data)
{
	void *ptr = data->ptr;

	if (ptr == NULL)
		return;

	WRITE_TRACE(DBG_FATAL, "vzctl_cancel_operation %p", ptr);
}

/************************************************************************/
state_event_handler_fn CVzStateMonitor::m_cb;
void * CVzStateMonitor::m_obj;

void CVzStateMonitor::stop()
{
	m_bStopStatusMonitor = true;
}

CVzStateMonitor::~CVzStateMonitor()
{
	stop();
}

static VOID VZCAPI VZCNotificationCallback(VZC_JOBID jobId, PVZC_CALLBACK_ARGS pArgs)
{
	PVZC_NOTIFICATION_ENTRY pEntry =
		(PVZC_NOTIFICATION_ENTRY) pArgs->errorDescription;
	int state = 0;

	if (!pArgs || !pEntry) {
		WRITE_TRACE(DBG_TRACE, "Empty vzc notification entry");
		return;
	}

	WRITE_TRACE(DBG_TRACE, "VZCNotificationCallback activity=%lu command=%lu vpsId=%u type=%u",
		    jobId.activityNum, jobId.commandNum, pEntry->vpsId, pEntry->eventType);

	switch  (pEntry->eventType)
	{
	case NOTIFY_VPS_STATE_CHANGED:
		if (pEntry->eventData.stateChanged.substate == VE_SUBSTATE_NONE) {
			WRITE_TRACE(DBG_TRACE, "VPS_STATE_CHANGED: %hs %hs -> %hs %hs",
			       pVZC_StrState(pEntry->eventData.stateChanged.lastState),
			       pVZC_StrSubstate(pEntry->eventData.stateChanged.lastSubstate),
			       pVZC_StrState(pEntry->eventData.stateChanged.state),
			       pVZC_StrSubstate(pEntry->eventData.stateChanged.substate));
		} else {
			WRITE_TRACE(DBG_TRACE, "VPS_STATE_CHANGED: %hs %hs",
			       pVZC_StrState(pEntry->eventData.stateChanged.state),
			       pVZC_StrSubstate(pEntry->eventData.stateChanged.substate));
		}
		if (pEntry->eventData.stateChanged.state != pEntry->eventData.stateChanged.lastState)
		{
			switch (pEntry->eventData.stateChanged.state)
			{
			case VE_STATE_RUNNING:
				state = VZCTL_ENV_STARTED;
				break;
			case VE_STATE_STOPPED:
				switch (pEntry->eventData.stateChanged.lastState)
				{
				case VE_STATE_NONE:
					state = VZCTL_ENV_CREATED;
					break;
				default:
					state = VZCTL_ENV_STOPPED;
					break;
				}
				break;
			case VE_STATE_NONE:
				state = VZCTL_ENV_DELETED;
				break;
			}

			if (state && pEntry->eventData.stateChanged.substate == VE_SUBSTATE_NONE) {
				try {
					CVzStateMonitor::m_cb(CVzStateMonitor::m_obj, pEntry->vpsId, state);
				} catch (...) {
					WRITE_TRACE(DBG_FATAL, "ERROR CVzStateMonitor::m_cb exception");
				}
			}
		}

		break;

	case NOTIFY_VPS_OPERATION_FAILED:
		WRITE_TRACE(DBG_TRACE, "VPS_OPERATION_FAILED: %u %u",
			    pEntry->eventData.operationFailed.operation,
			    VZC_R_RESULT(pEntry->eventData.operationFailed.result));
		break;
	}
}

void CVzStateMonitor::start(state_event_handler_fn cb, void *obj)
{
	VZC_JOBID jobId;
	PVZC_NOTIFICATION_COMMAND pCmd;

	if (!hVzcapi || pVZC_AllocCommand == NULL)
		return;

	m_cb = cb;
	m_obj = obj;

	/* register callback to trace changes in status ct  */
	// bug workaround:
	//	need to do it in loop since vzsrv sometimes stops errorneusly removes
	//	our thread id from listeners list
	for (;;) {
		pCmd = (PVZC_NOTIFICATION_COMMAND) pVZC_AllocCommand(VZC_CMD_NOTIFICATION);
		if (pCmd == NULL)
			return;
		pVZC_NewActivity(&jobId);
		pVZC_RegisterCommandCallback(VZCNotificationCallback);
		pVZC_ExecCommand(jobId, pCmd);
		pVZC_FreeCommand(pCmd);
	}
}

int CVzOperationHelper::move_env(const QString &sUuid, const QString &sNewHome,
	const QString &sName)
{
	Q_UNUSED(sUuid);
	Q_UNUSED(sNewHome);
	Q_UNUSED(sName);
	return PRL_ERR_UNIMPLEMENTED;
}

Ct::Statistics::Aggregate *CVzHelper::get_env_stat(const QString& uuid_)
{
	if (NULL == hVzcapi)
		return NULL;

	unsigned int id = CVzHelper::get_envid_by_uuid(uuid_);
	if (0 == id)
		return NULL;

	using Ct::Statistics::Aggregate;
	QScopedPointer<Aggregate> a(new Aggregate());

	QScopedPointer<PRL_STAT_NET_TRAFFIC> n(get_net_stat(id));
	a->net = n.isNull() ? PRL_STAT_NET_TRAFFIC() : *n;

	return a.take();
}

int CVzHelper::get_env_fstat(const QString &uuid, QList<Ct::Statistics::Filesystem>& fs)
{
	Q_UNUSED(uuid);
	Q_UNUSED(fs);
	return PRL_ERR_UNIMPLEMENTED;
}
