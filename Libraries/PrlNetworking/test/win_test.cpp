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
#include <windows.h>

//#include <prlcommon/Interfaces/ParallelsDirs.h>

#include <iostream>

#include "../PrlNetLibrary.h"

#include "../IpStatistics.h"

#include "../win/ethlist.h"
#include "../prlnet.h"

#include <prlcommon/Interfaces/ParallelsPlatform.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>


using namespace PrlNet;

int main( int argc, char *argv[] )
{
	(void)argc;
	(void)argv;

	EthAdaptersList ethList;
	if (0 != makeBindableAdapterList(ethList, false)) {
		printf("make ethlist failed\n");
		return -1;
	}
	if (ethList.isEmpty()) {
		printf("ethlist empty\n");
		return -1;
	}

	prlnet_handle h;
	if (0 != PrlNet::prlnet_open(&h))
	{
		printf("drvopen failed\n");
		return -1;
	}

	prlnet_generic_io_t io;
	memset(&io, 0, sizeof(io));
	io.code = 0;
	int bytes_returned = -1;
	io.in_size = 1024;
	io.in_ptr = (UINT64)-1;
	//io.out_size = 1024;
	//io.out_ptr = 23;//(ULONGLONG)(-1);
	DWORD err = PrlNet::PrlNetInternal::prlnet_generic_ioctl(h, &io, &bytes_returned);
	printf("err = %u, br = %d\n", err, bytes_returned);

	return 0;
}
