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

#include <QCoreApplication>
#include <QDir>

#include <iostream>

#ifdef _DEBUG
#define LOGGING_ON
#endif
#include <prlcommon/Logging/Logging.h>

#include <Libraries/Etrace/Etrace.h>
#include <prlcommon/Std/PrlTime.h>

#ifndef _WIN_
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define closesocket(x)	close(x)
#else
#define socklet_t		int
#endif

#include <errno.h>

#ifdef ETRACE
void usage()
{
	printf("usage: prl_etrace_util <cmd> <args>\n");
	printf("cmd: merge dump1_name dump2_name result_file_name\n");
	printf("     dump -s shmem_name dump_file_name\n");
	printf("     dump -n host:port dump_file_name\n");
	printf("     ctl -s shmem_name <--set-mask [mask]>|<--reset>\n");
	printf("     ctl -n host:port <--set-mask [mask]>|<--reset>\n");
	printf("     parse dump_file_name result_file_name\n");
}


int merge(const char *i1, const char *i2, const char *out)
{
	QFile filein1(i1);
	QFile filein2(i2);
	QFile fileout(out);

	bool r = CEtrace::merge(filein1, filein2, fileout);
	if (!r) {
		printf("operation failed\n");
		return -1;
	}
	return 0;
}

static int parse(const char *input, const char *out)
{
	QFile filein(input);
	QFile fileout(out);

	bool r = CEtrace::parse(filein, fileout);
	if (!r) {
		printf("operation failed\n");
		return -1;
	}
	return 0;
}

static const char *get_shmem_key(const char *name)
{
	if (!strcmp(name, "vmapp"))
		return ETRACE_SHMEM_VMAPP;
	else if (!strcmp(name, "pax"))
		return ETRACE_SHMEM_PAX;
	else {
		printf("unknown shmem id: %s\n", name);
		return NULL;
	}
}

static int dump_from_shmem(const char *input, const char *out)
{
	const char *key;
	QFile fileout(out);

	key = get_shmem_key(input);
	if (key == NULL)
		return -1;

	CEtrace c;
	c.register_shmem(key, ETRACE_DEFAULT_BUF_SIZE, false);
	c.dump(fileout);
	return 0;
}

int net_connect_to_etrace(const char *target)
{
	char host[64];
	char *delim = strchr((char *)target, ':');
	int port, sock;
	struct sockaddr_in sin;
	struct hostent *he;

	strncpy(host, target, MIN(delim - target, (long)sizeof(host)));
	port = atoi(delim + 1);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("socket() failed: %s\n", strerror(errno));
		return sock;
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	if ((he = gethostbyname(host)) != NULL)
		memcpy(&sin.sin_addr, he->h_addr, he->h_length);
	sin.sin_port = htons(port);

	if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		printf("connect() failed: %s\n", strerror(errno));
		return -1;
	}

	return sock;
}

char *net_receive_data(int sock, int *sizeo)
{
	char cmd = ETRACE_CMD_GETDATA;
	int size, rcv;
	char *data, *ptr;

	if (!sizeo)
		return NULL;

	send(sock, &cmd, sizeof(cmd), 0);
	recv(sock, (char *)&size, sizeof(size), 0);

	*sizeo = size;

	data = (char *)malloc(size);
	if (!data)
		return NULL;

	ptr = data;
	while (size) {
		rcv = recv(sock, ptr, size, 0);
		if (rcv < 0)
			return NULL;
		ptr += rcv;
		size -= rcv;
	}

	return data;
}

int dump_using_net(const char *input, const char *out)
{
	int sock;
	char *data;
	int size;

	sock = net_connect_to_etrace(input);
	if (sock < 0)
		return -1;

	data = net_receive_data(sock, &size);
	if (!data)
		return -1;

	QFile fileout(out);
	if (!fileout.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		printf("failed to open file: %s\n", out);
		return -1;
	}

	fileout.write(data, size);

	return 0;
}

int dump(const char *type, const char *input, const char *out)
{
	int ret = -1;
	UINT64 start = PrlTicks();

	if (!strcmp(type, "-s"))
		ret = dump_from_shmem(input, out);
	else if (!strcmp(type, "-n"))
		ret = dump_using_net(input, out);
	else {
		printf("unknown communication type: %s\n", type);
		return -1;
	}

	UINT64 end = PrlTicks();
	(void)start;(void)end;
	LOG_MESSAGE(DBG_FATAL, "time taken %u ms", (UINT)PrlTicksToMilli(end - start));
	return ret;
}

int ctl_using_shmem(const char *target, const char *cmd, const char *strmask)
{
	UINT64 mask;
	const char *key = get_shmem_key(target);
	if (key == NULL)
		return -1;

	CEtrace c;
	c.register_shmem(key, ETRACE_DEFAULT_BUF_SIZE, false);

	if (!strcmp(cmd, "--set-mask")) {
		CEtrace::alias2mask(strmask, &mask);
		c.set_mask(mask);
	} else if (!strcmp(cmd, "--reset")) {
		c.reset();
	} else {
		printf("unknown command: %s\n", cmd);
		return -1;
	}


	return 0;
}

int net_set_etrace_mask(int sock, UINT64 mask)
{
	char cmd = ETRACE_CMD_SETMASK;

	send(sock, &cmd, sizeof(cmd), 0);
	send(sock, (char *)&mask, sizeof(mask), 0);
	return 0;
}

int net_etrace_reset(int sock)
{
	char cmd = ETRACE_CMD_RESET;
	send(sock, &cmd, sizeof(cmd), 0);
	return 0;
}

int ctl_using_net(const char *target, const char *cmd, const char *strmask)
{
	int ret;
	UINT64 mask;
	int sock = net_connect_to_etrace(target);
	if (sock < 0)
		return -1;

	if (!strcmp(cmd, "--set-mask")) {
		CEtrace::alias2mask(strmask, &mask);
		ret = net_set_etrace_mask(sock, mask);
	} else if (!strcmp(cmd, "--reset")) {
		ret = net_etrace_reset(sock);
	} else {
		printf("unknown command: %s\n", cmd);
		ret = -1;
	}

	return ret;
}

int ctl(const char *type, const char *target, const char *cmd, const char *strmask)
{
	int ret = -1;

	if (!strcmp(type, "-s"))
		ret = ctl_using_shmem(target, cmd, strmask);
	else if (!strcmp(type, "-n"))
		ret = ctl_using_net(target, cmd, strmask);
	else {
		printf("unknown communication type: %s\n", type);
		return -1;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	SetConsoleLogging(1);
	if (argc < 2) {
		usage();
		return -1;
	}
	if (strcmp(argv[1], "merge") == 0) {
		if (argc < 5) {
			usage();
			return -1;
		}
		return merge(argv[2], argv[3], argv[4]);
	}
	if (strcmp(argv[1], "dump") == 0) {
		if (argc < 5) {
			usage();
			return -1;
		}
		return dump(argv[2], argv[3], argv[4]);
	}
	if (strcmp(argv[1], "ctl") == 0) {
		if (argc < 5) {
			usage();
			return -1;
		}
		return ctl(argv[2], argv[3], argv[4], (argc > 5) ? argv[5] : "all");
	}
	if (strcmp(argv[1], "parse") == 0) {
		if (argc < 4) {
			usage();
			return -1;
		}
		return parse(argv[2], argv[3]);
	}
	printf("unknown command %s\n", argv[1]);
	return -1;
}
#else
int main(int argc, char *argv[])
{
	(void)argc; (void)argv;
	printf("Project compiled without etrace-support. Nothing to do.\n");

	return 0;
}
#endif
