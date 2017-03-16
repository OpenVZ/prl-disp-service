/////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <getopt.h>

#include <string>
#include <vector>

#include <algorithm>
#include <signal.h>

enum TurnMessageType {
  STUN_ALLOCATE_REQUEST                 = 0x0003,
  STUN_ALLOCATE_RESPONSE                = 0x0103,
};

enum RelayAttributeType {
  STUN_ATTR_REQUESTED_TRANSPORT         = 0x0019,  // UInt32
};

enum StunAttributeType {
  STUN_ATTR_USERNAME                    = 0x0006,  // ByteString
};

enum StunProtocolType {
  STUN_PROTO_TCP			= 6,
  STUN_PROTO_UDP			= 17,
};

const uint32_t kStunMagicCookie = 0x2112A442;
const size_t kStunTransactionIdLength = 12;
const size_t kStunLegacyTransactionIdLength = 16;

static int stop = 0;

int connectName(const std::string& host, uint16_t port, int type);
int write_stun_allocate_request(int fd, const std::string& transaction_id,
								const std::string& username, StunProtocolType proto_type);
int read_stun_allocate_response(int fd,  StunProtocolType proto_type);
int write_broker_register( int fd, const std::string& uuid);
int write_broker_unregister( int fd, const std::string& uuid);
int read_broker_response( int fd);

static int broker_oper = 0;

static struct option long_options[] = {
	{"broker_ip", required_argument, 0, 300},
	{"proxy",     required_argument, 0,  'p' },
	{"bport",  required_argument,       0,  'b' },
	{"pudp",  required_argument, 0,  'u' },
	{"ptcp",  required_argument, 0,  't' },
	{"uuid", required_argument, 0, 'U'},
	{"test_count", required_argument, 0, 301},
	{"delete_slot", no_argument, &broker_oper, 2},
	{"create_slot", no_argument, &broker_oper, 1},
	{"help", no_argument,       0,  'h' },
	{0,         0,                 0,  0 }
};

void print_usage()
{
	printf("[--broker_ip <ipv4>] command interface, default is 127.0.0.1\n"
		"[--proxy|p <ipv4>] is optional param describes a proxydaemon ipv4 address, default is 127.0.0.1\n"
		"[--bport|b <port>] - sets a broker command port, default is 64777\n"
		"[--pudp|u <port>] - sets a UDP port for STUN on proxydaemon, default is 19294\n"
		"[--ptcp|t <port>] - sets a TCP port for STUN on proxydaemon, default is 443\n"
		"[--uuid|U <port>] - sets a uuid for STUN and slot, default is B76E93B3A7594D279678662BE390C288\n"
		"[--test_count <count>] -- number of test iterations\n"
		"[--create_slot]\n"
		"[--delete_slot]\n"
		   "[--help|h|?] - prints a help\n");
}

void print_defaults(const std::string& broker_h, uint16_t bport, const std::string& prox_h, uint16_t uport, uint16_t tport)
{
	printf("Settings in use: broker host: %s broker port: %u, proxy host %s, udp port: %u, tcp port: %u\n",
				broker_h.c_str(), bport, prox_h.c_str(), uport, tport);
}

const char* stage_desc = "start";

void sigh(int)
{
	printf("stage %s terminated\n", stage_desc);
	stop = 1;
}


class broker
{
public:
	broker(int fd, const std::string& a_uuid): broker_fd(fd), uuid(a_uuid)
	{
		if (broker_oper == 1 ) {
			stage_desc = "write broker register";
			if (0 > write_broker_register(broker_fd, uuid)) {
				perror("registration on broker failed");
			}
			else {
				stage_desc = "read broker register";
				if (0 > read_broker_response(broker_fd)) {
					perror("read registration response from broker failed");
				}
			}
		}
	}

	~broker()
	{
		if (broker_oper == 2) {
			stage_desc = "write broker unregister";
			if (0 > write_broker_unregister(broker_fd, uuid)) {
				perror("unregistration on broker failed");
			}
			else {
				stage_desc = "read broker response";
				if (0 > read_broker_response(broker_fd)) {
					perror("read response from broker failed");
				}
			}
		}
	}
private:
	int broker_fd;
	std::string uuid;
};

int main(int argc, char *argv[])
{
	std::string uuid("B76E93B3A7594D279678662BE390C288");
	std::string username("7iuTm0ft0GBy8CG5");
	std::string transaction_id("3vUAq3ByutIX");// EMPTY_TRANSACTION_ID;

	int ch;
	int option_index;
	uint16_t udp_port = 19294;
	uint16_t tcp_port = 443;
	uint16_t broker_port = 64777;
	std::string broker_host("127.0.0.1");
	std::string proxy_host("127.0.0.1");

	::signal(SIGTERM, sigh);
	::signal(SIGINT, sigh);
	::signal(SIGQUIT, sigh);

	int test_count = 1;

	while( (ch = getopt_long(argc, argv, "U:u:t:p:b:h", long_options, &option_index)) != -1)
	{
		switch (ch) {
		case 301:
			test_count = strtol(optarg, NULL, 10);
			break;
		case 300:
			broker_host = optarg;
			break;	
		case 'u':
			udp_port = (uint16_t)strtoul(optarg, NULL, 10);
			break;
		case 't':
			tcp_port = (uint16_t)strtoul(optarg, NULL, 10);
			break;
		case 'b':
			broker_port = (uint16_t)strtoul(optarg, NULL, 10);
			break;
		case 'p':
			proxy_host = optarg;
			break;
		case 'U':
			uuid = optarg;
			{
				int k = 0;
				for(unsigned int i = 0; i<uuid.size(); ++i) {
					char c = uuid[i];
					if (c != '-' && c != '{' && c != '}')
						uuid[k++] = c;
				}
			}
			uuid.resize(32);
			break;
		case 'h':
		case '?':
			print_usage();
			return -1;
		default:
			break;
		}
	}
	print_defaults(broker_host, broker_port, proxy_host, udp_port, tcp_port);

	/* part1 agent */
	int broker_fd = -1;
	if (broker_oper != 0)
	{
	stage_desc = "broker connect";
		broker_fd = connectName(broker_host, broker_port, SOCK_STREAM);
	if (0 > broker_fd) {
		perror("connection to broker failed");
		return -1;
	}
	}
	stage_desc = "agent tcp connect";
	int agent_tcp443_fd = connectName(proxy_host, tcp_port, SOCK_STREAM);
	if (0 > agent_tcp443_fd) {
		perror("connection to TCP proxy failed");
		return -1;
	}
	stage_desc = "agent udp connect";
	int agent_udp19294_fd = connectName(proxy_host, udp_port, SOCK_DGRAM);
	if (0 > agent_udp19294_fd) {
		perror("connection to UDP proxy failed");
		return -1;
	}

	std::transform(uuid.begin(), uuid.end(), uuid.begin(), ::tolower);
	username += uuid;
	std::transform(username.begin(), username.end(), username.begin(), ::tolower);
	// register uuid in proxy
	{
		broker service(broker_fd, uuid);

	while(test_count--)
	{
	/* part 2 client */
	stage_desc = "client tcp connect";
	int client_tcp443_fd = connectName(proxy_host, tcp_port, SOCK_STREAM);
	if (0 > agent_tcp443_fd) {
		perror("connection to TCP proxy failed");
		return -1;
	}
	stage_desc = "client udp connect";
	int client_udp19294_fd = connectName(proxy_host, udp_port, SOCK_DGRAM);
	if (0 > client_udp19294_fd) {
		perror("connection to UDP proxy failed");
		return -1;
	}

	int rc = 0;
	int count = 0;

	// push connection udp from client side
	while (rc == 0 && count<5) {
		if ( 0 > write_stun_allocate_request(client_udp19294_fd, transaction_id, username, STUN_PROTO_UDP)) {
			perror("send STUN allocation by UDP failed");
			return -1;
		}

		stage_desc = "read stun udp client";
		if ( 0 > (rc = read_stun_allocate_response(client_udp19294_fd, STUN_PROTO_UDP))) {
			perror("read response of client STUN allocation by UDP failed");
			return -1;
		}
		++count;
	}

	stage_desc = "write stun tcp client";
	if (0 > write_stun_allocate_request(client_tcp443_fd, transaction_id, username, STUN_PROTO_TCP)) {
		perror("send client STUN allocation by TCP failed");
		return -1;
	}

	stage_desc = "read stun tcp client";
	if (0 > read_stun_allocate_response(client_tcp443_fd, STUN_PROTO_TCP)) {
		perror("read response of client STUN allocation by TCP failed");
		return -1;
	}

	if (0 > write_stun_allocate_request(agent_tcp443_fd, transaction_id, username, STUN_PROTO_TCP)) {
		perror("send agent STUN allocation by TCP failed");
		return -1;
	}

	stage_desc = "read stun tcp agent";
	if (0 > read_stun_allocate_response(agent_tcp443_fd, STUN_PROTO_TCP)) {
		perror("read response of agent STUN allocation by TCP failed");
		return -1;
	}

	// push connection udp from agent side
	count = 0;
	rc = 0;
	while (rc == 0 && count<5) {
		if ( 0 > write_stun_allocate_request(agent_udp19294_fd, transaction_id, username, STUN_PROTO_UDP)) {
			perror("send agent STUN allocation by UDP failed");
			return -1;
		}

		stage_desc = "read stun udp agent";
		if ( 0 >(rc = read_stun_allocate_response(agent_udp19294_fd, STUN_PROTO_UDP))) {
			perror("read response of agent STUN allocation by UDP failed");
			return -1;
		}
		++count;
	}

	stage_desc = "close & stop";
	::close(client_tcp443_fd);
	::close(client_udp19294_fd);
	}

	} // finish broker service
	::close(agent_tcp443_fd);
	::close(agent_udp19294_fd);

	if (!(broker_fd<0))
	::close(broker_fd);
	return 0;
}

struct stun_message
{
	uint16_t type;
	uint16_t length;
	char transaction_id[16];
};

struct stun_message_attr
{
	uint16_t type;
	uint16_t length;
};

int write_broker_unregister( int fd, const std::string& uuid)
{
	std::string drop_slot("DropSlot: {" + uuid + "}\n");
	ssize_t rc = ::write(fd, drop_slot.c_str(), drop_slot.length());
	if (rc >= 0 && drop_slot.length() != (size_t)rc)
		return -1;
	else
		return rc;
}

int write_broker_register( int fd, const std::string& uuid)
{
	std::string create_slot("CreateSlot: {" + uuid + "}\n");
	ssize_t rc = ::write(fd, create_slot.c_str(), create_slot.length());
	if (rc >= 0 && create_slot.length() != (size_t)rc)
		return -1;
	else
		return rc;
}

int read_broker_response(int fd)
{
	std::string broker_response;
	struct timespec timeout = { 1, 0 };
	fd_set fds_read;

	FD_ZERO(&fds_read);

	while(!stop) {
		FD_SET(fd, &fds_read);
		int count = ::pselect(fd +1, &fds_read, NULL, NULL, &timeout, NULL);
		if (count >0) {
			if (FD_ISSET(fd, &fds_read)) {
				--count;
				char c;
				ssize_t s = ::read(fd, &c, 1);
				if (s == 1) {
					broker_response += c;
					FD_SET(fd, &fds_read);

					if (c == '\n' && broker_response.length()>1) {
						printf("broker response: %s", broker_response.c_str());
						return 1;
					}
				}
				else if (c == 0)
					return 1;
				else
					return -1;
			}
		}
		else
			return -1;
	}
	return -1;
}

int write_stun_allocate_request(int fd, const std::string& transaction_id_,
								const std::string& username, StunProtocolType proto_type)
{
	struct stun_message msg;
	msg.type = htons(STUN_ALLOCATE_REQUEST);
	msg.length = 0;

	if (transaction_id_.length() == kStunTransactionIdLength) {
		uint32_t value = htonl(kStunMagicCookie);
		::memcpy(msg.transaction_id, &value, sizeof(kStunMagicCookie));
		::memcpy(&msg.transaction_id[sizeof(kStunMagicCookie)], transaction_id_.c_str(), kStunTransactionIdLength);
	}
	else {
		::memcpy(msg.transaction_id, transaction_id_.c_str(), kStunLegacyTransactionIdLength);
	}

	struct stun_message_attr userName;
	userName.type = htons(STUN_ATTR_USERNAME);
	userName.length = htons(username.length());
	msg.length += username.length() + sizeof(userName);

	struct stun_message_attr attrTransport;
	attrTransport.type = htons(STUN_ATTR_REQUESTED_TRANSPORT);
	attrTransport.length = htons(4);
	msg.length += 4 + sizeof(attrTransport);

	std::vector<char> buf(sizeof(msg) + msg.length);

	msg.length = htons(msg.length);
	char* ptr = &buf[0];
	::memcpy(ptr, &msg, sizeof(msg)); ptr += sizeof(msg);
	::memcpy(ptr, &userName, sizeof(userName)); ptr += sizeof(userName);
	::memcpy(ptr, username.c_str(), username.length()); ptr += username.length();
	::memcpy(ptr, &attrTransport, sizeof(attrTransport)); ptr += sizeof(attrTransport);
	uint32_t value = proto_type;
	value = htonl(value);
	::memcpy(ptr, &value, sizeof(value));

	if (proto_type == STUN_PROTO_TCP)
	{
		// google tcp specific
		uint16_t size2 = htons(buf.size());
		::write(fd, &size2, 2);
	}
	int rc = ::write(fd, &buf[0], buf.size());
	return rc;
}

int read_stun_allocate_response(int fd,  StunProtocolType proto_type)
{
	const char* proto = proto_type == STUN_PROTO_TCP ? "TCP" : "UDP";
	struct timespec timeout = { 1, 250000 };
	fd_set fds_read;
	FD_ZERO(&fds_read);

	while(!stop) {
		FD_SET(fd, &fds_read);
		int count = ::pselect(fd +1, &fds_read, NULL, NULL, &timeout, NULL);
		if (count >0) {
			if (FD_ISSET(fd, &fds_read)) {
				--count;
				
				struct stun_message msg;
				int rc;
				int size = 256;
				std::vector<char> buf(size);
				// google specific
				if (proto_type == STUN_PROTO_TCP)
				{
					uint16_t size2;
					rc = ::read(fd, &size2, sizeof(size2));
					rc = ::read(fd, &msg, sizeof(msg));
					msg.length = ntohs(msg.length);
					buf.resize(msg.length);
					rc += ::read(fd, &buf[0], msg.length);
				} 
				else {
					ioctl(fd, FIONREAD, &size);
					buf.resize(size);
					rc = ::read(fd, &buf[0], size);
					::memcpy(&msg, &buf[0], sizeof(msg));
					msg.length = ntohs(msg.length);
				}

				if (msg.length + sizeof(msg) == (size_t)rc)
					return rc;
				else
					return -1;
			}
		}
		else if (count == 0) {
			printf("timeout %s\n", proto);
			return 0;
		}
	}

	return -1;
}

int connectName(const std::string& host, uint16_t port, int type)
{
	struct sockaddr_in sock_addr;
	::memset(&sock_addr, 0, sizeof(sock_addr));
	::inet_aton(host.c_str(), &sock_addr.sin_addr);
	sock_addr.sin_family =  PF_INET;
	sock_addr.sin_port = htons(port);
	int fd = ::socket(AF_INET, type, 0);
	if (type == SOCK_DGRAM) {
		fcntl(fd, F_SETFL, O_ASYNC | O_NONBLOCK);
	}
	int res = ::connect(fd, (const struct sockaddr*)&sock_addr, (socklen_t)(sizeof(sock_addr)));
	if (res < 0)
		return -1;
	else
		return fd;
}

