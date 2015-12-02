///////////////////////////////////////////////////////////////////////////////
///
/// @file PrlRoutes.cpp
/// @author evg
///
/// Functions for configuration route and arp for Linux
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

#include "PrlNetLibrary.h"

#include <Libraries/Logging/Logging.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
extern "C" {
	#include <libnetlink.h>
}

typedef struct
{
        __u8 family;
        __u8 bytelen;
        __s16 bitlen;
        __u32 flags;
        __u32 data[4];
} inet_prefix;

static int get_addr(inet_prefix *addr, const char *name)
{
	memset(addr, 0, sizeof(*addr));

	if (strchr(name, ':')) {
		addr->family = AF_INET6;
		if (inet_pton(AF_INET6, name, addr->data) <= 0)
			return -1;
		addr->bytelen = 16;
		addr->bitlen = -1;
		return 0;
	}

	if (strchr(name, '.')) {
		addr->family = AF_INET;
		if (inet_pton(AF_INET, name, addr->data) <= 0)
			return -1;
		addr->bytelen = 4;
		addr->bitlen = -1;
		return 0;
	}

	WRITE_TRACE(DBG_FATAL, "Wrong IP address '%s'", name);
	return -1;
}

static int get_prefix(inet_prefix *dst, const char *arg, int family)
{
	int err;

	memset(dst, 0, sizeof(*dst));

	if (strcmp(arg, "default") == 0 ||
	    strcmp(arg, "any") == 0 ||
	    strcmp(arg, "all") == 0) {
		dst->family = family;
		dst->bytelen = (family == AF_INET ? 16 : 4) ;
		dst->bitlen = -1;
		return 0;
	}


	err = get_addr(dst, arg);
	if (err == 0) {
		switch(dst->family) {
			case AF_INET6:
				dst->bitlen = 128;
				break;
			default:
			case AF_INET:
				dst->bitlen = 32;
		}
	} else
		WRITE_TRACE(DBG_FATAL, "Error: an inet prefix is expected rather than '%s'.", arg);

	return err;
}

static unsigned ll_name_to_index(const char *name)
{
        if (name == NULL)
                return 0;
        return if_nametoindex(name);
}

bool PrlNet::SetRouteToDevice(const QString &ip, const QString &devName, bool add, int metric)
{
	struct {
		struct nlmsghdr n;
		struct rtmsg r;
		char buf[1024];
	} req;

	struct rtnl_handle rth;
	int idx;
	inet_prefix dst;

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.r.rtm_table = RT_TABLE_MAIN;

	if (add){
		req.n.nlmsg_flags |= NLM_F_CREATE|NLM_F_REPLACE;
		req.n.nlmsg_type = RTM_NEWROUTE;
		req.r.rtm_protocol = RTPROT_BOOT;
		req.r.rtm_type = RTN_UNICAST;
		req.r.rtm_scope = RT_SCOPE_LINK;
	}else{
		req.n.nlmsg_type = RTM_DELROUTE;
		req.r.rtm_scope = RT_SCOPE_NOWHERE;
	}

	if (get_prefix(&dst, ip.toUtf8().constData(), AF_INET) < 0)
		return false;

	if (metric != -1)
		addattr32(&req.n, sizeof(req), RTA_PRIORITY, metric);
	else if (dst.family == AF_INET6)
		addattr32(&req.n, sizeof(req), RTA_PRIORITY, 1);

	req.r.rtm_family = dst.family;
	req.r.rtm_dst_len = dst.bitlen;

	if (dst.bytelen){
		if (addattr_l(&req.n, sizeof(req), RTA_DST, &dst.data, dst.bytelen) < 0)
			return false;
	}else{
		WRITE_TRACE(DBG_FATAL, "IP is empty '%s'", ip.toUtf8().constData());
		return false;
	}

	if ((idx = ll_name_to_index(devName.toUtf8().constData())) == 0) {
		WRITE_TRACE(DBG_FATAL, "Cannot find device '%s'", devName.toUtf8().constData());
		return false;
	}

	if (addattr32(&req.n, sizeof(req), RTA_OIF, idx) < 0)
		return false;

	if (rtnl_open(&rth, 0) < 0)
		return false;

	WRITE_TRACE(DBG_INFO, "route %s ip=%s device='%s' index=%d ",
			add ? "add" : "del", ip.toUtf8().constData(), devName.toUtf8().constData(), idx );

        int rc = rtnl_talk(&rth, &req.n, 0, 0, NULL);

	rtnl_close(&rth);

	if (rc < 0)
		return false;
	return true ;
}

bool PrlNet::SetArpToDevice(const QString &ip, const QString &devName, bool add)
{
	//WRITE_TRACE(DBG_FATAL, "Set arp %s proxy ip=%s device='%s' ",
	//		add ? "add" : "del", ip.toUtf8().constData(), devName.toUtf8().constData());
        struct {
                struct nlmsghdr         n;
                struct ndmsg            ndm;
                char                    buf[256];
        } req;

	struct rtnl_handle rth;
	inet_prefix dst;
	unsigned int devIndex = 0;

	if ((devIndex = ll_name_to_index(devName.toUtf8().constData())) == 0) {
		WRITE_TRACE(DBG_FATAL, "Cannot find device '%s'", devName.toUtf8().constData());
		return false;
        }

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.ndm.ndm_state = NUD_PERMANENT;
	req.ndm.ndm_flags |= NTF_PROXY;

	if (add){
		req.n.nlmsg_flags |= NLM_F_CREATE|NLM_F_REPLACE;
		req.n.nlmsg_type = RTM_NEWNEIGH;
	}else{
		req.n.nlmsg_type = RTM_DELNEIGH;
	}

	if (get_addr(&dst, ip.toUtf8().constData()) < 0)
		return false;

        req.ndm.ndm_family = dst.family;
	if (addattr_l(&req.n, sizeof(req), NDA_DST, &dst.data, dst.bytelen) < 0)
		return false;

	req.ndm.ndm_ifindex = devIndex;

	if (rtnl_open(&rth, 0) < 0)
		return false;

	WRITE_TRACE(DBG_DEBUG, "arp %s proxy ip=%s device='%s' index=%d ",
			add ? "add" : "del", ip.toUtf8().constData(), devName.toUtf8().constData(), devIndex);

	int rc = rtnl_talk(&rth, &req.n, 0, 0, NULL);

	rtnl_close(&rth);

	if (rc < 0)
		return false;
	return true ;
}

bool PrlNet::SetArpToNodeDevices(const QString &ip, const QString &srcMac, bool add, bool annonce)
{
	QString old;

	QFile f("/proc/sys/net/ipv4/ip_nonlocal_bind");
	if (!f.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to enable nonlocal bind. Cannot configure arp for ip %s",
			qPrintable(ip));
		return false;
	}
	QTextStream(&f) >> old << 1;

	QStringList names = makePhysicalAdapterList();

	foreach(const QString& name, names)
	{
		QString adapter = getBridgeName(name);
		if (adapter.isEmpty())
			adapter = name;

		bool rc = SetArpToDevice(ip, adapter, add);

		if (!rc) {
			WRITE_TRACE(DBG_FATAL, "Failed to %s arp for device %s",
				add ? "set" : "remove", qPrintable(adapter));
			QTextStream(&f) << old;
			return false;
		}
		else
		{
			WRITE_TRACE(DBG_INFO, "Arp on device '%s' for ip %s is %s",
				qPrintable(adapter), qPrintable(ip),
				add ? "configured" : "disabled");
		}


		if (annonce && add) {
			WRITE_TRACE(DBG_INFO, "Arp on device '%s' for ip %s is annonced",
				qPrintable(adapter), qPrintable(ip));
			PrlNet::updateArp(ip, srcMac, adapter);
		}
	}

	// Return old value
	QTextStream(&f) << old;
	return true;
}
