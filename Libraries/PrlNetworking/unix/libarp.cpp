///////////////////////////////////////////////////////////////////////////////
///
/// @file libarp.c
///
/// ARP service routines.
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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

#include <net/if_arp.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>
#include <prlcommon/Logging/Logging.h>

#define  werror(fmt, args...)   WRITE_TRACE(DBG_FATAL, fmt, ##args)

static int get_netaddr(const char * ip, struct sockaddr *addr)
{
	if (!strchr(ip, ':'))
	{
		struct sockaddr_in *a4 = (struct sockaddr_in *) addr;
		a4->sin_family = AF_INET;
		a4->sin_port = 0;
		if (inet_aton(ip, &a4->sin_addr) == 0) {
			werror("invalid ip address %s", ip);
			return -1;
		}
	}
	else
		return -1;  //ipv6 not supported

	return 0;
}

static int parse_hwaddr(const char *str, unsigned char *addr)
{
	int i;
	char buf[3];
	char *endptr;

	for (i = 0; i < ETH_ALEN; i++) {
		buf[0] = str[3*i];
		buf[1] = str[3*i+1];
		buf[2] = '\0';
		addr[i] = strtoul(buf, &endptr, 16);
		if (*endptr != '\0')
			return -1;
	}
	return 0;
}

static int fill_arpreq(const char *ip, const char *dev, unsigned char *hwaddr, struct arpreq *req)
{
	struct sockaddr sa;

	if (get_netaddr(ip, &sa))
		return -1;

	bzero(req, sizeof(struct arpreq));
	memcpy(&req->arp_pa, &sa, sizeof(req->arp_pa));

	if (dev != NULL)
		strncpy((char *) &req->arp_dev, dev, sizeof(req->arp_dev) - 1);

	if (hwaddr != NULL)
		memcpy(&req->arp_ha.sa_data, hwaddr, ETH_ALEN);
	return 0;
}

int arp_add(const char *ip, const char *dev, unsigned char *hwaddr)
{
	int ret, fd;
	struct arpreq req;

	if ((ret = fill_arpreq(ip, dev, hwaddr, &req)))
		return ret;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		werror("arp_add socket : %m");
		return -1;
	}

	req.arp_flags = ATF_PERM | ATF_COM;
	if ((ret = ioctl(fd, SIOCSARP, &req)) < 0)
		werror("SIOCSARP : %m");
	close(fd);

	return ret;
}

static int arp_del(const char *ip, const char *dev, unsigned char *hwaddr, int flags)
{
	int ret, fd;
	struct arpreq req;

	if ((ret = fill_arpreq(ip, dev, hwaddr, &req)))
		return ret;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		werror("arp_del socket: %m");
		return -1;
	}

	req.arp_flags = flags;
	if ((ret = ioctl(fd, SIOCDARP, &req)) < 0) {
		if (errno == ENXIO) {
			req.arp_flags |= ATF_PUBL;
			if ((ret = ioctl(fd, SIOCDARP, &req)) < 0)
				werror("SIOCDARP : %m");
		} else {
			werror("SIOCDARP : %m");
		}
	}
	close(fd);

	return ret;
}

#define PROC_ARP	"/proc/net/arp"
int arp_del_by_hwaddr(const char *dev, unsigned char *hwaddr)
{
	char _line[255];
	char _ip[100];
	char _hwaddr_txt[100];
	char _dev[17];
	unsigned char _hwaddr[ETH_ALEN];
	int type, flags, num;
	int ret = 0;
	FILE *fp;

	if ((fp = fopen(PROC_ARP, "r")) == NULL) {
		werror("open" PROC_ARP " : %m");
		return -1;
	}

	while (fgets(_line, sizeof(_line), fp) != NULL) {
		num = sscanf(_line, "%99s 0x%x 0x%x %99s %*s %16s\n",
				_ip, &type, &flags, _hwaddr_txt, _dev);
		if (num < 5)
			continue;

		if (parse_hwaddr(_hwaddr_txt, _hwaddr))
			continue;
		if (memcmp(_hwaddr, hwaddr, ETH_ALEN) != 0)
			continue;
		if (strncmp(_dev, dev, sizeof(_dev)-1))
			continue;

		ret = arp_del(_ip, _dev, _hwaddr, flags);
	}
	fclose(fp);
	return ret;
}

#if 0
int main()
{
	unsigned char hwaddr[ETH_ALEN];

	bzero(hwaddr, sizeof(hwaddr));
	hwaddr[0] = 1;
	arp_add("1.1.1.1", "eth0", hwaddr);
//	arp_del_by_hwaddr(hwaddr);
}

#endif
