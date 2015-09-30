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

#include "snapshot_enum.h"
#include <boost/assign/list_of.hpp>

namespace ba = boost::assign;
namespace Libvirt
{
template<>
Enum<Snapshot::Xml::EState>::data_type Enum<Snapshot::Xml::EState>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Snapshot::Xml::EStateNostate, "nostate"))
			(data_type::value_type(Snapshot::Xml::EStateRunning, "running"))
			(data_type::value_type(Snapshot::Xml::EStateBlocked, "blocked"))
			(data_type::value_type(Snapshot::Xml::EStatePaused, "paused"))
			(data_type::value_type(Snapshot::Xml::EStateShutdown, "shutdown"))
			(data_type::value_type(Snapshot::Xml::EStateShutoff, "shutoff"))
			(data_type::value_type(Snapshot::Xml::EStateCrashed, "crashed"))
			(data_type::value_type(Snapshot::Xml::EStateDiskSnapshot, "disk-snapshot"));
}

template<>
Enum<Snapshot::Xml::ESnapshot>::data_type Enum<Snapshot::Xml::ESnapshot>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Snapshot::Xml::ESnapshotNo, "no"))
			(data_type::value_type(Snapshot::Xml::ESnapshotInternal, "internal"));
}

template<>
Enum<Snapshot::Xml::EStartupPolicy>::data_type Enum<Snapshot::Xml::EStartupPolicy>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Snapshot::Xml::EStartupPolicyMandatory, "mandatory"))
			(data_type::value_type(Snapshot::Xml::EStartupPolicyRequisite, "requisite"))
			(data_type::value_type(Snapshot::Xml::EStartupPolicyOptional, "optional"));
}

template<>
Enum<Snapshot::Xml::EStorageFormatBacking>::data_type Enum<Snapshot::Xml::EStorageFormatBacking>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Snapshot::Xml::EStorageFormatBackingCow, "cow"))
			(data_type::value_type(Snapshot::Xml::EStorageFormatBackingQcow, "qcow"))
			(data_type::value_type(Snapshot::Xml::EStorageFormatBackingQcow2, "qcow2"))
			(data_type::value_type(Snapshot::Xml::EStorageFormatBackingQed, "qed"))
			(data_type::value_type(Snapshot::Xml::EStorageFormatBackingVmdk, "vmdk"));
}

template<>
Enum<Snapshot::Xml::EProtocol>::data_type Enum<Snapshot::Xml::EProtocol>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Snapshot::Xml::EProtocolNbd, "nbd"))
			(data_type::value_type(Snapshot::Xml::EProtocolRbd, "rbd"))
			(data_type::value_type(Snapshot::Xml::EProtocolSheepdog, "sheepdog"))
			(data_type::value_type(Snapshot::Xml::EProtocolGluster, "gluster"))
			(data_type::value_type(Snapshot::Xml::EProtocolIscsi, "iscsi"))
			(data_type::value_type(Snapshot::Xml::EProtocolHttp, "http"))
			(data_type::value_type(Snapshot::Xml::EProtocolHttps, "https"))
			(data_type::value_type(Snapshot::Xml::EProtocolFtp, "ftp"))
			(data_type::value_type(Snapshot::Xml::EProtocolFtps, "ftps"))
			(data_type::value_type(Snapshot::Xml::EProtocolTftp, "tftp"));
}

template<>
Enum<Snapshot::Xml::ETransport>::data_type Enum<Snapshot::Xml::ETransport>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Snapshot::Xml::ETransportTcp, "tcp"))
			(data_type::value_type(Snapshot::Xml::ETransportRdma, "rdma"));
}

template<>
Enum<Snapshot::Xml::EActive>::data_type Enum<Snapshot::Xml::EActive>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Snapshot::Xml::EActive0, "0"))
			(data_type::value_type(Snapshot::Xml::EActive1, "1"));
}

} // namespace Libvirt
