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
//#include <prlcommon/Interfaces/ParallelsDirs.h>

#include <iostream>

#include "../PrlNetLibrary.h"

#include "../IpStatistics.h"

#include "../win/ethlist.h"

#include <prlcommon/Interfaces/ParallelsPlatform.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include "../unix/MacConfigureAdapter.h"

QString ParallelsInstallDir;
QString ParallelsDriversDir;


int main( int argc, char *argv[] )
{
	QCoreApplication app( argc, argv);

	// Mac_SetAdapterName( 0, "Parallels Shared Networking" );

	CHostOnlyNetwork Net;
	Net.setHostIPAddress(0x0d0c0b0a);
	Net.setIPNetMask(0xffffff);
	PrlNet::setPrlAdapterIpAddresses( 2, false, &Net);

	return 0;
}
