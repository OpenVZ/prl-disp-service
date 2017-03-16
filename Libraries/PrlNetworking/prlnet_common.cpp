///////////////////////////////////////////////////////////////////////////////
///
/// @file prlnet_common.cpp
///
/// Implementation of common methods of IPrlNet class
///
/// @author sdmitry
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
#if defined(_WIN_)
#include <windows.h>
#endif

#include <new>
#include <string.h>
#include <prlcommon/Logging/Logging.h>
#include "prlnet.h"
#include "prlnet_drv.h"

#if !defined(_WIN_)
#include "prlnet_tap.h"
#endif

IPrlNet* IPrlNet::create_prlnet(IPrlNet::prlnet_type t)
{
	IPrlNet *ret;

	switch (t) {
	case PVSNET:
		ret = new (std::nothrow) PvsNet;
		break;
#if !defined(_WIN_)
	case TAP:
		ret = new (std::nothrow) TapNet;
		break;
#endif

#if defined(_MAC_)
	case APPLEVISOR:
		ret = new (std::nothrow) ApplevisorNet;
		break;
#endif

	default:
		ret = NULL;
		break;
	}

	if (ret)
		ret->m_type = t;

	return ret;
}

static UINT prlnet_fill_desc_arr(prlnet_desc_arr_t *desc_arr, unsigned arr_sz, NET_BUFFER *netbuf)
{
	UINT head;
	prlnet_desc_t *arr = (prlnet_desc_t *)desc_arr->ptr;
	UINT cnt;
	NET_PACKET *pkt;

	for (head = netbuf->uSendHead, cnt = 0;
		 head != netbuf->uSendTail && cnt < arr_sz;
		 head += NET_PACKET_SIZE(pkt), ++cnt) {
		pkt = (NET_PACKET*)(netbuf->aSendBuffer + (head & (IO_NET_SIZE-1)));
		arr[cnt].flags = PRLNET_DESC_FLAG_EOP;
		arr[cnt].addr = (UINT64)(ULONG_PTR)pkt->aDataBuf;
		arr[cnt].size = NET_PACKET_DATA_LEN(pkt);
	}

	desc_arr->count = cnt;

	return head;
}

unsigned int IPrlNet::send_netbuffer(NET_BUFFER *netbuf)
{
	prlnet_desc_t arr[PRLNET_MAX_FRAGS_COUNT];
	prlnet_desc_arr_t desc_arr = {0, (UINT64)(ULONG_PTR)arr};
	unsigned int sent = 0;

	while (!NET_IS_SENDBUF_EMPTY(netbuf)) {
		UINT head = prlnet_fill_desc_arr(&desc_arr, ARRAY_SIZE(arr), netbuf);

		// Real send, we do not bother about any error of bottom layer
		send_descr_arr(&desc_arr);

		// Commit already sent
		netbuf->uSendHead = head;

		sent += desc_arr.count;
	}

	return sent;
}
