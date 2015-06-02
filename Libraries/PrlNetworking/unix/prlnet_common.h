///////////////////////////////////////////////////////////////////////////////
///
/// @file prlnet_common.g
/// @author sdmitry
///
/// Common internal include of prl_net for Unix OS
///
/// Copyright (c) 2009-2015 Parallels IP Holdings GmbH
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

#include "../unix/ethlist.h"
#include <syslog.h>

#include <QObject>
#include <QHostAddress>
#include <QtAlgorithms>
#include <set>
#include <cassert>

#include <Libraries/Logging/Logging.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>

// #include "System/Network/services/ParallelsNetDaemon.h"

#include <Libraries/Std/ProcessWatcher.h>
#include <Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h>
#include <Libraries/PrlNetworking/netconfig.h>

// #include <System/Network/drivers/lin/pvsvnic/prl_vnic.h>
// Maximum adapters count in system
#define PRLVNIC_MAX_ADAPTER_NUM 16


namespace PrlNet_Private {

const int PARALLELS_MAXIMUM_ADAPTER_INDEX = (PRLVNIC_MAX_ADAPTER_NUM - 1);

// Remember last system error for PrlNet module.
void MODULE_STORE_SYSTEM_ERROR();

// returns true if prl_naptd process is running
bool isNatdRunning();

PRL_RESULT findPrlAdapter(int adapterIndex, PrlNet::EthernetAdapter &adapter);

/// @brief Configures newly installed parallels adapter
/// @param prlDriversDir Directory where parallels drivers are located
/// @param adapterIndex Index of the parallels adapter
/// @param adapterName Name to assign to the adapter
bool configurePrlAdapter(
	const QString &prlDriversDir,
	int adapterIndex, bool bHiddenAdapter, const QString &adapterName );

/// Unconfigures Parallels Adapter
/// @param adapterIndex Adapter to unconfigure. -1 to unconfigure all Parallels Adapter.
void unconfigurePrlAdapter(int adapterIndex);

/// Plugs Parallels Interface into the system
PRL_RESULT plugPrlAdapter( int adapterIndex );

// Unplugs Parallels Interface from the system
PRL_RESULT unplugPrlAdapter( int adapterIndex );

// returns names of the Parallels NATD
void getPrlNatdNames(const QString &parallelsDir, QString &cmd, QString &arg0);

// execute command in arg0
PRL_RESULT execDaemon( const QString &path, const QString & arg0, const char *arg1);
}
