///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHwMonitorHandler_lin.cpp
///
/// Platform dependent hardware configuration changes handler implementation for Linux
///
/// @author ilya
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

#include "CDspHwMonitorHandler.h"
#include <CDspService.h>
#include <Libraries/HostInfo/CHostInfo.h>
#include <prlcommon/Logging/Logging.h>
//#include "Devices/Usb/UsbSysfs.h"  // Devices/Usb commented out by request from CP team
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <errno.h>

#define MSECS_IN_SEC 1000
#define USECS_IN_MSEC 1000
#define USECS_IN_SEC (MSECS_IN_SEC * USECS_IN_MSEC)

/**
* Class constructor
*/
CDspHwMonitorHandler::CDspHwMonitorHandler( QObject * parent )
: QThread (parent)
, m_thread_id(0)
{
	WRITE_TRACE( DBG_DEBUG, "Create Harware Monitor Handler" );
}

/**
* Starts handle device configuration changes
*/
void CDspHwMonitorHandler::startHandleDevices()
{
	WRITE_TRACE( DBG_DEBUG, "Start Harware Monitor Handler" );
	m_bRunFlag = TRUE;
	QThread::start();
}

/**
* Stop handle device configuration changes
*/
void CDspHwMonitorHandler::stopHandleDevices()
{
	m_bRunFlag = FALSE;
	if (0 != m_thread_id)
		pthread_kill(m_thread_id, SIGUSR1);
	QThread::wait();
	WRITE_TRACE( DBG_DEBUG, "Stop Harware Monitor Handler" );
}

static const int NL_RCVBUF_SIZE = 8092;

// Devices/Usb commented out by request from CP team
//static int openNetlinkSock()
//{
//	int sock = ::socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
//	if (sock < 0)
//		return -1;
//
//	int rcvbuf_size = NL_RCVBUF_SIZE;
//	if ( ::setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
//			&rcvbuf_size, sizeof(rcvbuf_size)) < 0 ) {
//		// Don't return an error. Just log the event.
//		WRITE_TRACE_RL(3600, DBG_FATAL, "setsockopt for SO_RCVBUF failed with %d", errno);
//	}
//
//	struct sockaddr_nl local;
//	memset(&local, 0, sizeof(local));
//	local.nl_family = AF_NETLINK;
//	local.nl_groups = RTMGRP_LINK;
//	if (::bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
//		WRITE_TRACE_RL(3600, DBG_FATAL, "Failed to bind netlink_sock: %d", errno);
//		::close(sock);
//		return -1;
//	}
//
//	return sock;
//}

// Devices/Usb commented out by request from CP team
//// Read all messages from the network-link socket.
//// @return -1 if some error is occured on socket.
//static int readNetlinkSock(int sock)
//{
//	struct sockaddr_nl nladdr;
//	struct iovec iov;
//	struct msghdr msg;
//	char buf[NL_RCVBUF_SIZE];
//
//	msg.msg_name = &nladdr;
//	msg.msg_namelen = sizeof(nladdr);
//	msg.msg_iov = &iov;
//	msg.msg_iovlen = 1;
//
//	memset(&nladdr, 0, sizeof(nladdr));
//	nladdr.nl_family = AF_NETLINK;
//	iov.iov_base = buf;
//
//	// Note: we are not interested in the data and discarding the message
//	while(1) {
//		iov.iov_len = sizeof(buf);
//		int s = ::recvmsg(sock, &msg, MSG_DONTWAIT);
//		if (s < 0) {
//			if (errno == EINTR || errno == EAGAIN)
//				return 0;
//			return -1;
//		}
//		if (s == 0) {
//			WRITE_TRACE(DBG_DEBUG, "EOF on netlink socket.");
//			return -1;
//		}
//	}
//}

/**
* Overridden method of thread working body
*
* We use poll() on /dev/prl_usb_connect and /proc/bus/usb/devices,
* if these things exist. Also we check mtime sysfs /sys/bus/usb/devices/.
*/
void CDspHwMonitorHandler::run()
{
// Devices/Usb commented out by request from CP team
//	WRITE_TRACE( DBG_FATAL, "Started Hardware Monitor thread" );
//	m_thread_id = pthread_self();
//
//	enum {
//		devs_poll_idx,
//		prlusb_poll_idx,
//		nl_poll_idx,
//		poll_entries_num
//	};
//
//	struct pollfd p[poll_entries_num];
//
//	for (int i = 0; i < poll_entries_num; ++i)
//		p[i].fd = -1;
//
//	// do stat of sysfs periodically
//	int sysfs_fd = open(PRL_USB_SYSFS_PATH, O_RDONLY);
//
//	time_t ts_sec = 0;
//	int ts_nsec = 0;
//
//	// timeouts in milliseconds
//	const int netEventTimeout = 100;
//	const int pollTimeout = 1000;
//
//	// Network events usually come in group,
//	// So we must not send HWChanged on each event.
//	// On the other side, we must not keep the HwChanged event for too long.
//	int netEventsCount = 0;
//
//	// Maximum number of unsent network events
//	const int MAX_NETEVENT_COUNT = 10;
//
//	/* Workaround for bug/feature in linux>2.6.32: sysfs does not maintain inode attributes,
//	 * unless someone changed the attributes with explicit vfs operation.
//	 */
//	(void)utimes(PRL_USB_SYSFS_PATH, NULL);
//
//	while (m_bRunFlag) {
//
//		if (p[prlusb_poll_idx].fd < 0)
//			p[prlusb_poll_idx].fd = open(PRL_USB_CONNECT_PATH, O_RDONLY);
//
//		if (p[devs_poll_idx].fd < 0)
//			p[devs_poll_idx].fd = open(PRL_USB_USBFS_PATH "/devices", O_RDONLY);
//
//		if (p[nl_poll_idx].fd < 0)
//			p[nl_poll_idx].fd = openNetlinkSock();
//
//		// use fact that if fd is <0, it is ignored.
//		// So we don't check whether descriptors are valid.
//		for (int i = 0; i<poll_entries_num; ++i) {
//			p[i].events = POLLIN;
//			p[i].revents = 0;
//		}
//
//		BOOL wake_usb = FALSE;
//
//		int res = poll(p, poll_entries_num,
//						netEventsCount ? netEventTimeout : pollTimeout);
//
//		if (sysfs_fd >= 0) {
//			struct stat stb;
//
//			if (fstat(sysfs_fd, &stb) == 0 &&
//				(stb.st_mtim.tv_sec  != ts_sec ||
//				 stb.st_mtim.tv_nsec != ts_nsec)) {
//				ts_sec  = stb.st_mtim.tv_sec;
//				ts_nsec = stb.st_mtim.tv_nsec;
//				wake_usb = TRUE;
//			}
//		}
//
//		if (res == 0) {
//			if (netEventsCount)
//				netEventsCount = MAX_NETEVENT_COUNT;
//			goto continue_loop;
//		}
//
//		// not consume 100% CPU on accidental fail of poll
//		if (res < 0) {
//			// Errors other then EINTR are unexpected.
//			// To process them, we are closing all fds and do sleeping.
//			if (errno == EINTR) {
//				res = 0;
//				goto continue_loop;
//			}
//			for (int i = 0; i<poll_entries_num; ++i) {
//				::close(p[i].fd);
//				p[i].fd = -1;
//			}
//
//			if (netEventsCount)
//				netEventsCount = MAX_NETEVENT_COUNT;
//
//			goto continue_loop;
//		}
//
//		if ( p[prlusb_poll_idx].revents & POLLIN
//			|| p[devs_poll_idx].revents & POLLIN ) {
//			wake_usb = TRUE;
//		}
//
//		if (p[nl_poll_idx].revents & POLLIN) {
//			// ToDo: Usually, there will be several network events;
//			// We should wait some time before signaling changes.
//			if (readNetlinkSock(p[nl_poll_idx].fd) < 0) {
//				::close(p[nl_poll_idx].fd);
//				p[nl_poll_idx].fd = -1;
//			}
//			else
//				++netEventsCount;
//		}
//
//continue_loop:
//		if (wake_usb)
//			onDeviceChange(PDE_USB_DEVICE, "", 2);
//
//		if (netEventsCount >= MAX_NETEVENT_COUNT) {
//			netEventsCount = 0;
//			onDeviceChange(PDE_GENERIC_NETWORK_ADAPTER, "", 1);
//		}
//		if (res < 0)
//			QThread::msleep(1000);
//	}
//
//	close(p[prlusb_poll_idx].fd);
//	close(sysfs_fd);
//	close(p[devs_poll_idx].fd);
//	close(p[nl_poll_idx].fd);
}

/**
* Device changed handler function
* @param dev_name platform dependent device name
* @param event_code new device state connected/disconnected
*/
void CDspHwMonitorHandler::onDeviceChange(PRL_DEVICE_TYPE dev_type, const QString &dev_name, unsigned int event_code)
{
	emit deviceChanged(dev_type, dev_name, event_code );
}

CDspHwMonitorHandler::~CDspHwMonitorHandler()
{
}
