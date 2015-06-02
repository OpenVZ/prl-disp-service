/*
 * libnetlink.c	RTnetlink service routines.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/uio.h>

#include "libnetlink.h"
#include <Libraries/Logging/Logging.h>

#define  werror(fmt, args...)	WRITE_TRACE(DBG_FATAL, fmt, ##args)

static uint64_t rtnl_get_sequence()
{
	static volatile uint64_t s_sequence = 0;
	return __sync_add_and_fetch(&s_sequence, 1);
}

void rtnl_close(struct rtnl_handle *rth)
{
	close(rth->fd);
}

int rtnl_open_byproto(struct rtnl_handle *rth, unsigned subscriptions,
		      int protocol)
{
	socklen_t addr_len;
	int sndbuf = 32768;
	int rcvbuf = 32768;

	memset(rth, 0, sizeof(*rth));

	rth->fd = socket(AF_NETLINK, SOCK_RAW, protocol);
	if (rth->fd < 0) {
		werror("Cannot open netlink socket");
		return -1;
	}

	if (setsockopt(rth->fd,SOL_SOCKET,SO_SNDBUF,&sndbuf,sizeof(sndbuf)) < 0) {
		werror("SO_SNDBUF");
		return -1;
	}

	if (setsockopt(rth->fd,SOL_SOCKET,SO_RCVBUF,&rcvbuf,sizeof(rcvbuf)) < 0) {
		werror("SO_RCVBUF");
		return -1;
	}

	memset(&rth->local, 0, sizeof(rth->local));
	rth->local.nl_family = AF_NETLINK;
	rth->local.nl_groups = subscriptions;

	if (bind(rth->fd, (struct sockaddr*)&rth->local, sizeof(rth->local)) < 0) {
		werror("Cannot bind netlink socket");
		return -1;
	}
	addr_len = sizeof(rth->local);
	if (getsockname(rth->fd, (struct sockaddr*)&rth->local, &addr_len) < 0) {
		werror("Cannot getsockname");
		return -1;
	}
	if (addr_len != sizeof(rth->local)) {
		werror("Wrong address length %d", addr_len);
		return -1;
	}
	if (rth->local.nl_family != AF_NETLINK) {
		werror("Wrong address family %d", rth->local.nl_family);
		return -1;
	}
	rth->seq = 0;
	return 0;
}

int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions)
{
	return rtnl_open_byproto(rth, subscriptions, NETLINK_ROUTE);
}

int rtnl_wilddump_request(struct rtnl_handle *rth, int family, int type)
{
	struct {
		struct nlmsghdr nlh;
		struct rtgenmsg g;
	} req;
	struct sockaddr_nl nladdr;

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = type;
	req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
	req.nlh.nlmsg_pid = 0;
	req.nlh.nlmsg_seq = rth->dump = rth->seq = rtnl_get_sequence();
	req.g.rtgen_family = family;

	return sendto(rth->fd, (void*)&req, sizeof(req), 0,
		      (struct sockaddr*)&nladdr, sizeof(nladdr));
}

int rtnl_send(struct rtnl_handle *rth, const char *buf, int len)
{
	struct sockaddr_nl nladdr;

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	return sendto(rth->fd, buf, len, 0, (struct sockaddr*)&nladdr, sizeof(nladdr));
}

int rtnl_dump_request(struct rtnl_handle *rth, int type, void *req, int len)
{
	struct nlmsghdr nlh;
	struct sockaddr_nl nladdr;
	struct iovec iov[2];

	iov[0].iov_base = &nlh;  iov[0].iov_len = sizeof(nlh);
	iov[1].iov_base = req;  iov[1].iov_len = len;

	struct msghdr msg;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	nlh.nlmsg_len = NLMSG_LENGTH(len);
	nlh.nlmsg_type = type;
	nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
	nlh.nlmsg_pid = 0;
	nlh.nlmsg_seq = rth->dump = rth->seq = rtnl_get_sequence();

	return sendmsg(rth->fd, &msg, 0);
}

int rtnl_dump_filter(struct rtnl_handle *rth,
		     rtnl_filter_t filter,
		     void *arg1,
		     rtnl_filter_t junk,
		     void *arg2)
{
	struct sockaddr_nl nladdr;
	struct iovec iov;
	struct msghdr msg;

	char buf[16384];

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;



	iov.iov_base = buf;
	while (1) {
		int status;
		struct nlmsghdr *h;

		iov.iov_len = sizeof(buf);
		status = recvmsg(rth->fd, &msg, 0);

		if (status < 0) {
			if (errno == EINTR)
				continue;
			werror("OVERRUN");
			continue;
		}

		if (status == 0) {
			werror("EOF on netlink");
			return -1;
		}

		h = (struct nlmsghdr*)buf;
		while (NLMSG_OK(h, status)) {
			int err;

			if (nladdr.nl_pid != 0 ||
			    h->nlmsg_pid != rth->local.nl_pid ||
			    h->nlmsg_seq != rth->dump) {
				if (junk) {
					err = junk(&nladdr, h, arg2);
					if (err < 0)
						return err;
				}
				goto skip_it;
			}

			if (h->nlmsg_type == NLMSG_DONE)
				return 0;
			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(h);
				if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
					werror("ERROR truncated");
				} else {
					errno = -err->error;
					werror("RTNETLINK answers");
				}
				return -1;
			}
			err = filter(&nladdr, h, arg1);
			if (err < 0)
				return err;

skip_it:
			h = NLMSG_NEXT(h, status);
		}
		if (msg.msg_flags & MSG_TRUNC) {
			werror("Message truncated");
			continue;
		}
		if (status) {
			werror("!!!Remnant of size %d", status);
			exit(1);
		}
	}
}

int rtnl_talk(struct rtnl_handle *rtnl, struct nlmsghdr *n, pid_t peer,
	      unsigned groups, struct nlmsghdr *answer,
	      rtnl_filter_t junk,
	      void *jarg)
{
	int status;
	unsigned seq = 0;
	struct nlmsghdr *h;
	struct sockaddr_nl nladdr;
	struct iovec iov ;
	struct msghdr msg;
	char   buf[16384];


	memset(&iov, 0, sizeof(iov));
	iov.iov_base = (void*) n;
	iov.iov_len = n->nlmsg_len;

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = peer;
	nladdr.nl_groups = groups;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n->nlmsg_seq = seq = rtnl->seq = rtnl_get_sequence();

	if (answer == NULL)
		n->nlmsg_flags |= NLM_F_ACK;

	status = sendmsg(rtnl->fd, &msg, 0);

	if (status < 0) {
		werror("Cannot talk to rtnetlink: %s", strerror(errno));
		return -1;
	}

	memset(buf,0,sizeof(buf));

	iov.iov_base = buf;

	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(rtnl->fd, &msg, 0);

		if (status < 0) {
			if (errno == EINTR)
				continue;
			werror("OVERRUN");
			continue;
		}
		if (status == 0) {
			werror("EOF on netlink");
			return -1;
		}
		if (msg.msg_namelen != sizeof(nladdr)) {
			werror("sender address length == %d", msg.msg_namelen);
			exit(1);
		}
		for (h = (struct nlmsghdr*)buf; status >= (int)sizeof(*h); ) {
			int err;
			int len = h->nlmsg_len;
			int l = len - sizeof(*h);

			if (l<0 || len>status) {
				if (msg.msg_flags & MSG_TRUNC) {
					werror("Truncated message");
					return -1;
				}
				werror("!!!malformed message: len=%d", len);
				exit(1);
			}

			if (nladdr.nl_pid != (unsigned) peer ||
			    h->nlmsg_pid != rtnl->local.nl_pid ||
			    h->nlmsg_seq != seq) {
				if (junk) {
					err = junk(&nladdr, h, jarg);
					if (err < 0)
						return err;
				}
				continue;
			}

			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(h);
				if (l < (int) sizeof(struct nlmsgerr)) {
					werror("ERROR truncated");
				} else {
					errno = -err->error;
					if (errno == 0) {
						if (answer)
							memcpy(answer, h, h->nlmsg_len);
						return 0;
					}
					werror("RTNETLINK answers");
				}
				return -1;
			}
			if (answer) {
				memcpy(answer, h, h->nlmsg_len);
				return 0;
			}

			werror("Unexpected reply!!!");

			status -= NLMSG_ALIGN(len);
			h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(len));
		}
		if (msg.msg_flags & MSG_TRUNC) {
			werror("Message truncated");
			continue;
		}
		if (status) {
			werror("!!!Remnant of size %d", status);
			exit(1);
		}
	}
}

int rtnl_listen(struct rtnl_handle *rtnl,
		rtnl_filter_t handler,
		void *jarg)
{
	int status;
	struct nlmsghdr *h;
	struct sockaddr_nl nladdr;
	struct iovec iov;
	struct msghdr msg;/* = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};*/
	char   buf[8192];

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;


	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = 0;

	iov.iov_base = buf;
	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(rtnl->fd, &msg, 0);

		if (status < 0) {
			if (errno == EINTR)
				continue;
			werror("OVERRUN");
			continue;
		}
		if (status == 0) {
			werror("EOF on netlink");
			return -1;
		}
		if (msg.msg_namelen != sizeof(nladdr)) {
			werror("Sender address length == %d", msg.msg_namelen);
			exit(1);
		}
		for (h = (struct nlmsghdr*)buf; status >= (int) sizeof(*h); ) {
			int err;
			int len = h->nlmsg_len;
			int l = len - sizeof(*h);

			if (l<0 || len>status) {
				if (msg.msg_flags & MSG_TRUNC) {
					werror("Truncated message");
					return -1;
				}
				werror("!!!malformed message: len=%d", len);
				exit(1);
			}

			err = handler(&nladdr, h, jarg);
			if (err < 0)
				return err;

			status -= NLMSG_ALIGN(len);
			h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(len));
		}
		if (msg.msg_flags & MSG_TRUNC) {
			werror("Message truncated");
			continue;
		}
		if (status) {
			werror("!!!Remnant of size %d", status);
			exit(1);
		}
	}
}

int rtnl_from_file(FILE *rtnl, rtnl_filter_t handler,
		   void *jarg)
{
	int status;
	struct sockaddr_nl nladdr;
	char   buf[8192];
	struct nlmsghdr *h = (nlmsghdr *)buf;

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = 0;

	while (1) {
		int err, len;
		int l;

		status = fread(&buf, 1, sizeof(*h), rtnl);

		if (status < 0) {
			if (errno == EINTR)
				continue;
			werror("rtnl_from_file: fread");
			return -1;
		}
		if (status == 0)
			return 0;

		len = h->nlmsg_len;
		l = len - sizeof(*h);

		if (l<0 || len> (int)sizeof(buf)) {
			werror("!!!malformed message: len=%d @%lu", len, ftell(rtnl));
			return -1;
		}

		status = fread(NLMSG_DATA(h), 1, NLMSG_ALIGN(l), rtnl);

		if (status < 0) {
			werror("rtnl_from_file: fread");
			return -1;
		}
		if (status < l) {
			werror("rtnl-from_file: truncated message");
			return -1;
		}

		err = handler(&nladdr, h, jarg);
		if (err < 0)
			return err;
	}
}

int addattr32(struct nlmsghdr *n, int maxlen, int type, __u32 data)
{
	int len = RTA_LENGTH(4);
	struct rtattr *rta;
	if ((int)NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
		werror("addattr32: Error! max allowed bound %d exceeded", maxlen);
		return -1;
	}
	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), &data, 4);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;
	return 0;
}

int addattr_l(struct nlmsghdr *n, int maxlen, int type, const void *data,
	      int alen)
{
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;

	if ((int) NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen) {
		werror("addattr_l ERROR: message exceeded bound of %d", maxlen);
		return -1;
	}
	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);
	return 0;
}

int addraw_l(struct nlmsghdr *n, int maxlen, const void *data, int len)
{
	if ((int) (NLMSG_ALIGN(n->nlmsg_len) + NLMSG_ALIGN(len)) > maxlen) {
		werror("addraw_l ERROR: message exceeded bound of %d", maxlen);
		return -1;
	}

	memcpy(NLMSG_TAIL(n), data, len);
	memset((char *) NLMSG_TAIL(n) + len, 0, NLMSG_ALIGN(len) - len);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + NLMSG_ALIGN(len);
	return 0;
}

int rta_addattr32(struct rtattr *rta, int maxlen, int type, __u32 data)
{
	int len = RTA_LENGTH(4);
	struct rtattr *subrta;

	if (RTA_ALIGN(rta->rta_len) + len > maxlen) {
		werror("rta_addattr32: Error! max allowed bound %d exceeded", maxlen);
		return -1;
	}
	subrta = (struct rtattr*)(((char*)rta) + RTA_ALIGN(rta->rta_len));
	subrta->rta_type = type;
	subrta->rta_len = len;
	memcpy(RTA_DATA(subrta), &data, 4);
	rta->rta_len = NLMSG_ALIGN(rta->rta_len) + len;
	return 0;
}

int rta_addattr_l(struct rtattr *rta, int maxlen, int type,
		  const void *data, int alen)
{
	struct rtattr *subrta;
	int len = RTA_LENGTH(alen);

	if (RTA_ALIGN(rta->rta_len) + RTA_ALIGN(len) > maxlen) {
		werror("rta_addattr_l: Error! max allowed bound %d exceeded\n", maxlen);
		return -1;
	}
	subrta = (struct rtattr*)(((char*)rta) + RTA_ALIGN(rta->rta_len));
	subrta->rta_type = type;
	subrta->rta_len = len;
	memcpy(RTA_DATA(subrta), data, alen);
	rta->rta_len = NLMSG_ALIGN(rta->rta_len) + RTA_ALIGN(len);
	return 0;
}

int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
	memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max)
			tb[rta->rta_type] = rta;
		rta = RTA_NEXT(rta,len);
	}
	if (len)
		werror("!!!Deficit %d, rta_len=%d\n", len, rta->rta_len);
	return 0;
}

int parse_rtattr_byindex(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
	int i = 0;

	memset(tb, 0, sizeof(struct rtattr *) * max);
	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max && i < max)
			tb[i++] = rta;
		rta = RTA_NEXT(rta,len);
	}
	if (len)
		werror("!!!Deficit %d, rta_len=%d\n", len, rta->rta_len);
	return i;
}
