///////////////////////////////////////////////////////////////////////////////
///
/// @file IpStatistics.h
/// @author sdmitry
///
/// Functions for collecting network interfaces statistic
///
/// Copyright (c) 2007-2015 Parallels IP Holdings GmbH
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
#ifndef PrlIpStat_h__
#define PrlIpStat_h__

#include "PrlNetInternal.h"
#include "PrlNetLibrary.h"

#include <QString>
#include <QList>

namespace PrlNet
{

struct IfaceStat
{
	QString ifaceName;    ///< name of the interface

	bool	bOperational; ///< true if interface is up and working

	quint64 nBytesSent;
	quint64 nBytesRcvd;
	quint64	nPktsSent;
	quint64	nPktsRcvd;

	quint64 nInErrors;
	quint64 nOutErrors;
};

typedef QList<IfaceStat> IfStatList;

/// Fills IpStatList with statistics for the interfaces
PRL_RESULT getIfaceStatistics( IfStatList& ifStatList );


/// Entry of the IfIpList
struct AddressInfo
{
	QString 	ifaceName;
	QString 	address;
	QString		netmask;
	bool		ipv6;
};

typedef QList<AddressInfo> IfIpList;

/// Fills list with ipv4 addresses of interface
/// @param ipList [out] List of the addresses
///
/// @return PRL_ERR_FAILURE if adapter was not found,
///			PRL_ERR_SUCCES on success.
///			other values mean more specific errors.
///
/// Note: it is possible to refactor this function to obtain list of IPv6
///       addresses on Windows hosts;
/// Obtain list of broadcast addresses for this interface etc.
PRL_RESULT getIfaceIpList( IfIpList &ipList, bool includeIpv6 = false );

};


#endif // PrlIpStat_h__
