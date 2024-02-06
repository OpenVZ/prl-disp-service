/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2021 Virtuozzo International GmbH. All rights reserved.
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
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include "domain_enum.h"
#include <boost/assign/list_of.hpp>

namespace ba = boost::assign;
namespace Libvirt
{
template<>
Enum<Domain::Xml::EType>::data_type Enum<Domain::Xml::EType>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ETypeQemu, "qemu"))
			(data_type::value_type(Domain::Xml::ETypeKqemu, "kqemu"))
			(data_type::value_type(Domain::Xml::ETypeKvm, "kvm"))
			(data_type::value_type(Domain::Xml::ETypeXen, "xen"))
			(data_type::value_type(Domain::Xml::ETypeLxc, "lxc"))
			(data_type::value_type(Domain::Xml::ETypeUml, "uml"))
			(data_type::value_type(Domain::Xml::ETypeOpenvz, "openvz"))
			(data_type::value_type(Domain::Xml::ETypeTest, "test"))
			(data_type::value_type(Domain::Xml::ETypeVmware, "vmware"))
			(data_type::value_type(Domain::Xml::ETypeHyperv, "hyperv"))
			(data_type::value_type(Domain::Xml::ETypeVbox, "vbox"))
			(data_type::value_type(Domain::Xml::ETypePhyp, "phyp"))
			(data_type::value_type(Domain::Xml::ETypeVirtuozzo, "virtuozzo"))
			(data_type::value_type(Domain::Xml::ETypeBhyve, "bhyve"));
}

template<>
Enum<Domain::Xml::EMode>::data_type Enum<Domain::Xml::EMode>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModeCustom, "custom"))
			(data_type::value_type(Domain::Xml::EModeHostModel, "host-model"))
			(data_type::value_type(Domain::Xml::EModeHostPassthrough, "host-passthrough"))
			(data_type::value_type(Domain::Xml::EModeMaximum, "maximum"));
}

template<>
Enum<Domain::Xml::EMatch>::data_type Enum<Domain::Xml::EMatch>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMatchMinimum, "minimum"))
			(data_type::value_type(Domain::Xml::EMatchExact, "exact"))
			(data_type::value_type(Domain::Xml::EMatchStrict, "strict"));
}

template<>
Enum<Domain::Xml::ECheck>::data_type Enum<Domain::Xml::ECheck>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ECheckNone, "none"))
			(data_type::value_type(Domain::Xml::ECheckPartial, "partial"))
			(data_type::value_type(Domain::Xml::ECheckFull, "full"));
}

template<>
Enum<Domain::Xml::EVirOnOff>::data_type Enum<Domain::Xml::EVirOnOff>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EVirOnOffOn, "on"))
			(data_type::value_type(Domain::Xml::EVirOnOffOff, "off"));
}

template<>
Enum<Domain::Xml::EFallback>::data_type Enum<Domain::Xml::EFallback>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EFallbackAllow, "allow"))
			(data_type::value_type(Domain::Xml::EFallbackForbid, "forbid"));
}

template<>
Enum<Domain::Xml::EPolicy>::data_type Enum<Domain::Xml::EPolicy>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EPolicyForce, "force"))
			(data_type::value_type(Domain::Xml::EPolicyRequire, "require"))
			(data_type::value_type(Domain::Xml::EPolicyOptional, "optional"))
			(data_type::value_type(Domain::Xml::EPolicyDisable, "disable"))
			(data_type::value_type(Domain::Xml::EPolicyForbid, "forbid"));
}

template<>
Enum<Domain::Xml::EMemAccess>::data_type Enum<Domain::Xml::EMemAccess>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMemAccessShared, "shared"))
			(data_type::value_type(Domain::Xml::EMemAccessPrivate, "private"));
}

template<>
Enum<Domain::Xml::EVirYesNo>::data_type Enum<Domain::Xml::EVirYesNo>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EVirYesNoYes, "yes"))
			(data_type::value_type(Domain::Xml::EVirYesNoNo, "no"));
}

template<>
Enum<Domain::Xml::EAssociativity>::data_type Enum<Domain::Xml::EAssociativity>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EAssociativityNone, "none"))
			(data_type::value_type(Domain::Xml::EAssociativityDirect, "direct"))
			(data_type::value_type(Domain::Xml::EAssociativityFull, "full"));
}

template<>
Enum<Domain::Xml::EPolicy1>::data_type Enum<Domain::Xml::EPolicy1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EPolicy1None, "none"))
			(data_type::value_type(Domain::Xml::EPolicy1Writeback, "writeback"))
			(data_type::value_type(Domain::Xml::EPolicy1Writethrough, "writethrough"));
}

template<>
Enum<Domain::Xml::EType1>::data_type Enum<Domain::Xml::EType1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType1Access, "access"))
			(data_type::value_type(Domain::Xml::EType1Read, "read"))
			(data_type::value_type(Domain::Xml::EType1Write, "write"));
}

template<>
Enum<Domain::Xml::EType2>::data_type Enum<Domain::Xml::EType2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType2Access, "access"))
			(data_type::value_type(Domain::Xml::EType2Read, "read"))
			(data_type::value_type(Domain::Xml::EType2Write, "write"));
}

template<>
Enum<Domain::Xml::ELevel>::data_type Enum<Domain::Xml::ELevel>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ELevel1, "1"))
			(data_type::value_type(Domain::Xml::ELevel2, "2"))
			(data_type::value_type(Domain::Xml::ELevel3, "3"));
}

template<>
Enum<Domain::Xml::EMode1>::data_type Enum<Domain::Xml::EMode1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode1Emulate, "emulate"))
			(data_type::value_type(Domain::Xml::EMode1Passthrough, "passthrough"))
			(data_type::value_type(Domain::Xml::EMode1Disable, "disable"));
}

template<>
Enum<Domain::Xml::EMode2>::data_type Enum<Domain::Xml::EMode2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode2Emulate, "emulate"))
			(data_type::value_type(Domain::Xml::EMode2Passthrough, "passthrough"));
}

template<>
Enum<Domain::Xml::ESysinfoBiosName>::data_type Enum<Domain::Xml::ESysinfoBiosName>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ESysinfoBiosNameVendor, "vendor"))
			(data_type::value_type(Domain::Xml::ESysinfoBiosNameVersion, "version"))
			(data_type::value_type(Domain::Xml::ESysinfoBiosNameDate, "date"))
			(data_type::value_type(Domain::Xml::ESysinfoBiosNameRelease, "release"));
}

template<>
Enum<Domain::Xml::ESysinfoSystemName>::data_type Enum<Domain::Xml::ESysinfoSystemName>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ESysinfoSystemNameManufacturer, "manufacturer"))
			(data_type::value_type(Domain::Xml::ESysinfoSystemNameProduct, "product"))
			(data_type::value_type(Domain::Xml::ESysinfoSystemNameVersion, "version"))
			(data_type::value_type(Domain::Xml::ESysinfoSystemNameSerial, "serial"))
			(data_type::value_type(Domain::Xml::ESysinfoSystemNameUuid, "uuid"))
			(data_type::value_type(Domain::Xml::ESysinfoSystemNameSku, "sku"))
			(data_type::value_type(Domain::Xml::ESysinfoSystemNameFamily, "family"));
}

template<>
Enum<Domain::Xml::ESysinfoBaseBoardName>::data_type Enum<Domain::Xml::ESysinfoBaseBoardName>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ESysinfoBaseBoardNameManufacturer, "manufacturer"))
			(data_type::value_type(Domain::Xml::ESysinfoBaseBoardNameProduct, "product"))
			(data_type::value_type(Domain::Xml::ESysinfoBaseBoardNameVersion, "version"))
			(data_type::value_type(Domain::Xml::ESysinfoBaseBoardNameSerial, "serial"))
			(data_type::value_type(Domain::Xml::ESysinfoBaseBoardNameAsset, "asset"))
			(data_type::value_type(Domain::Xml::ESysinfoBaseBoardNameLocation, "location"));
}

template<>
Enum<Domain::Xml::ESysinfoChassisName>::data_type Enum<Domain::Xml::ESysinfoChassisName>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ESysinfoChassisNameManufacturer, "manufacturer"))
			(data_type::value_type(Domain::Xml::ESysinfoChassisNameVersion, "version"))
			(data_type::value_type(Domain::Xml::ESysinfoChassisNameSerial, "serial"))
			(data_type::value_type(Domain::Xml::ESysinfoChassisNameAsset, "asset"))
			(data_type::value_type(Domain::Xml::ESysinfoChassisNameSku, "sku"));
}

template<>
Enum<Domain::Xml::EArch>::data_type Enum<Domain::Xml::EArch>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EArchI686, "i686"))
			(data_type::value_type(Domain::Xml::EArchX8664, "x86_64"))
			(data_type::value_type(Domain::Xml::EArchIa64, "ia64"));
}

template<>
Enum<Domain::Xml::EMachine>::data_type Enum<Domain::Xml::EMachine>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMachineXenpv, "xenpv"))
			(data_type::value_type(Domain::Xml::EMachineXenner, "xenner"));
}

template<>
Enum<Domain::Xml::EType3>::data_type Enum<Domain::Xml::EType3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType3Xen, "xen"))
			(data_type::value_type(Domain::Xml::EType3Linux, "linux"));
}

template<>
Enum<Domain::Xml::EFirmware>::data_type Enum<Domain::Xml::EFirmware>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EFirmwareBios, "bios"))
			(data_type::value_type(Domain::Xml::EFirmwareEfi, "efi"));
}

template<>
Enum<Domain::Xml::EArch1>::data_type Enum<Domain::Xml::EArch1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EArch1I686, "i686"))
			(data_type::value_type(Domain::Xml::EArch1X8664, "x86_64"));
}

template<>
Enum<Domain::Xml::EMachine1>::data_type Enum<Domain::Xml::EMachine1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMachine1G3beige, "g3beige"))
			(data_type::value_type(Domain::Xml::EMachine1Mac99, "mac99"))
			(data_type::value_type(Domain::Xml::EMachine1Prep, "prep"))
			(data_type::value_type(Domain::Xml::EMachine1Ppce500, "ppce500"));
}

template<>
Enum<Domain::Xml::EMachine2>::data_type Enum<Domain::Xml::EMachine2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMachine2Pseries, "pseries"));
}

template<>
Enum<Domain::Xml::EArch2>::data_type Enum<Domain::Xml::EArch2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EArch2S390, "s390"))
			(data_type::value_type(Domain::Xml::EArch2S390x, "s390x"));
}

template<>
Enum<Domain::Xml::EMachine3>::data_type Enum<Domain::Xml::EMachine3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMachine3S390, "s390"))
			(data_type::value_type(Domain::Xml::EMachine3S390Virtio, "s390-virtio"))
			(data_type::value_type(Domain::Xml::EMachine3S390Ccw, "s390-ccw"))
			(data_type::value_type(Domain::Xml::EMachine3S390CcwVirtio, "s390-ccw-virtio"));
}

template<>
Enum<Domain::Xml::EArch3>::data_type Enum<Domain::Xml::EArch3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EArch3Armv7l, "armv7l"));
}

template<>
Enum<Domain::Xml::EArch4>::data_type Enum<Domain::Xml::EArch4>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EArch4Aarch64, "aarch64"));
}

template<>
Enum<Domain::Xml::EName>::data_type Enum<Domain::Xml::EName>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ENameEnrolledKeys, "enrolled-keys"))
			(data_type::value_type(Domain::Xml::ENameSecureBoot, "secure-boot"));
}

template<>
Enum<Domain::Xml::EType4>::data_type Enum<Domain::Xml::EType4>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType4Rom, "rom"))
			(data_type::value_type(Domain::Xml::EType4Pflash, "pflash"));
}

template<>
Enum<Domain::Xml::EFormat>::data_type Enum<Domain::Xml::EFormat>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EFormatRaw, "raw"))
			(data_type::value_type(Domain::Xml::EFormatQcow2, "qcow2"));
}

template<>
Enum<Domain::Xml::EStartupPolicy>::data_type Enum<Domain::Xml::EStartupPolicy>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EStartupPolicyMandatory, "mandatory"))
			(data_type::value_type(Domain::Xml::EStartupPolicyRequisite, "requisite"))
			(data_type::value_type(Domain::Xml::EStartupPolicyOptional, "optional"));
}

template<>
Enum<Domain::Xml::EFormat1>::data_type Enum<Domain::Xml::EFormat1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EFormat1Default, "default"))
			(data_type::value_type(Domain::Xml::EFormat1Qcow, "qcow"))
			(data_type::value_type(Domain::Xml::EFormat1Luks, "luks"));
}

template<>
Enum<Domain::Xml::EProtocol>::data_type Enum<Domain::Xml::EProtocol>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EProtocolNbd, "nbd"))
			(data_type::value_type(Domain::Xml::EProtocolRbd, "rbd"))
			(data_type::value_type(Domain::Xml::EProtocolSheepdog, "sheepdog"))
			(data_type::value_type(Domain::Xml::EProtocolGluster, "gluster"))
			(data_type::value_type(Domain::Xml::EProtocolIscsi, "iscsi"))
			(data_type::value_type(Domain::Xml::EProtocolHttp, "http"))
			(data_type::value_type(Domain::Xml::EProtocolHttps, "https"))
			(data_type::value_type(Domain::Xml::EProtocolFtp, "ftp"))
			(data_type::value_type(Domain::Xml::EProtocolFtps, "ftps"))
			(data_type::value_type(Domain::Xml::EProtocolTftp, "tftp"));
}

template<>
Enum<Domain::Xml::ETransport>::data_type Enum<Domain::Xml::ETransport>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ETransportTcp, "tcp"))
			(data_type::value_type(Domain::Xml::ETransportRdma, "rdma"));
}

template<>
Enum<Domain::Xml::EMode3>::data_type Enum<Domain::Xml::EMode3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode3Host, "host"))
			(data_type::value_type(Domain::Xml::EMode3Direct, "direct"));
}

template<>
Enum<Domain::Xml::EDev>::data_type Enum<Domain::Xml::EDev>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EDevHd, "hd"))
			(data_type::value_type(Domain::Xml::EDevFd, "fd"))
			(data_type::value_type(Domain::Xml::EDevCdrom, "cdrom"))
			(data_type::value_type(Domain::Xml::EDevNetwork, "network"));
}

template<>
Enum<Domain::Xml::EMode4>::data_type Enum<Domain::Xml::EMode4>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode4Emulate, "emulate"))
			(data_type::value_type(Domain::Xml::EMode4Host, "host"))
			(data_type::value_type(Domain::Xml::EMode4Sysinfo, "sysinfo"));
}

template<>
Enum<Domain::Xml::EArch5>::data_type Enum<Domain::Xml::EArch5>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EArch5I686, "i686"))
			(data_type::value_type(Domain::Xml::EArch5X8664, "x86_64"))
			(data_type::value_type(Domain::Xml::EArch5Ppc, "ppc"))
			(data_type::value_type(Domain::Xml::EArch5Ppc64, "ppc64"))
			(data_type::value_type(Domain::Xml::EArch5Mips, "mips"))
			(data_type::value_type(Domain::Xml::EArch5Sparc, "sparc"));
}

template<>
Enum<Domain::Xml::EOffset>::data_type Enum<Domain::Xml::EOffset>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EOffsetLocaltime, "localtime"))
			(data_type::value_type(Domain::Xml::EOffsetUtc, "utc"));
}

template<>
Enum<Domain::Xml::EAdjustment>::data_type Enum<Domain::Xml::EAdjustment>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EAdjustmentReset, "reset"));
}

template<>
Enum<Domain::Xml::EBasis>::data_type Enum<Domain::Xml::EBasis>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EBasisUtc, "utc"))
			(data_type::value_type(Domain::Xml::EBasisLocaltime, "localtime"));
}

template<>
Enum<Domain::Xml::EName1>::data_type Enum<Domain::Xml::EName1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName1Platform, "platform"))
			(data_type::value_type(Domain::Xml::EName1Rtc, "rtc"));
}

template<>
Enum<Domain::Xml::ETrack>::data_type Enum<Domain::Xml::ETrack>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ETrackBoot, "boot"))
			(data_type::value_type(Domain::Xml::ETrackGuest, "guest"))
			(data_type::value_type(Domain::Xml::ETrackWall, "wall"));
}

template<>
Enum<Domain::Xml::ETickpolicy>::data_type Enum<Domain::Xml::ETickpolicy>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ETickpolicyDelay, "delay"))
			(data_type::value_type(Domain::Xml::ETickpolicyMerge, "merge"))
			(data_type::value_type(Domain::Xml::ETickpolicyDiscard, "discard"));
}

template<>
Enum<Domain::Xml::EMode5>::data_type Enum<Domain::Xml::EMode5>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode5Auto, "auto"))
			(data_type::value_type(Domain::Xml::EMode5Native, "native"))
			(data_type::value_type(Domain::Xml::EMode5Emulate, "emulate"))
			(data_type::value_type(Domain::Xml::EMode5Paravirt, "paravirt"))
			(data_type::value_type(Domain::Xml::EMode5Smpsafe, "smpsafe"));
}

template<>
Enum<Domain::Xml::EName2>::data_type Enum<Domain::Xml::EName2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName2Hpet, "hpet"))
			(data_type::value_type(Domain::Xml::EName2Pit, "pit"));
}

template<>
Enum<Domain::Xml::EName3>::data_type Enum<Domain::Xml::EName3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName3Kvmclock, "kvmclock"))
			(data_type::value_type(Domain::Xml::EName3Hypervclock, "hypervclock"));
}

template<>
Enum<Domain::Xml::EType5>::data_type Enum<Domain::Xml::EType5>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType5File, "file"))
			(data_type::value_type(Domain::Xml::EType5Anonymous, "anonymous"))
			(data_type::value_type(Domain::Xml::EType5Memfd, "memfd"));
}

template<>
Enum<Domain::Xml::EMode6>::data_type Enum<Domain::Xml::EMode6>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode6Shared, "shared"))
			(data_type::value_type(Domain::Xml::EMode6Private, "private"));
}

template<>
Enum<Domain::Xml::EMode7>::data_type Enum<Domain::Xml::EMode7>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode7Immediate, "immediate"))
			(data_type::value_type(Domain::Xml::EMode7Ondemand, "ondemand"));
}

template<>
Enum<Domain::Xml::EPlacement>::data_type Enum<Domain::Xml::EPlacement>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EPlacementStatic, "static"))
			(data_type::value_type(Domain::Xml::EPlacementAuto, "auto"));
}

template<>
Enum<Domain::Xml::EScheduler>::data_type Enum<Domain::Xml::EScheduler>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ESchedulerBatch, "batch"))
			(data_type::value_type(Domain::Xml::ESchedulerIdle, "idle"));
}

template<>
Enum<Domain::Xml::EScheduler1>::data_type Enum<Domain::Xml::EScheduler1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EScheduler1Fifo, "fifo"))
			(data_type::value_type(Domain::Xml::EScheduler1Rr, "rr"));
}

template<>
Enum<Domain::Xml::EType6>::data_type Enum<Domain::Xml::EType6>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType6Both, "both"))
			(data_type::value_type(Domain::Xml::EType6Code, "code"))
			(data_type::value_type(Domain::Xml::EType6Data, "data"));
}

template<>
Enum<Domain::Xml::EMode8>::data_type Enum<Domain::Xml::EMode8>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode8Strict, "strict"))
			(data_type::value_type(Domain::Xml::EMode8Preferred, "preferred"))
			(data_type::value_type(Domain::Xml::EMode8Interleave, "interleave"));
}

template<>
Enum<Domain::Xml::EMode9>::data_type Enum<Domain::Xml::EMode9>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode9Strict, "strict"))
			(data_type::value_type(Domain::Xml::EMode9Preferred, "preferred"))
			(data_type::value_type(Domain::Xml::EMode9Interleave, "interleave"));
}

template<>
Enum<Domain::Xml::EPolicy2>::data_type Enum<Domain::Xml::EPolicy2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EPolicy2Default, "default"))
			(data_type::value_type(Domain::Xml::EPolicy2Allow, "allow"))
			(data_type::value_type(Domain::Xml::EPolicy2Deny, "deny"));
}

template<>
Enum<Domain::Xml::EDriver>::data_type Enum<Domain::Xml::EDriver>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EDriverQemu, "qemu"))
			(data_type::value_type(Domain::Xml::EDriverKvm, "kvm"));
}

template<>
Enum<Domain::Xml::EResizing>::data_type Enum<Domain::Xml::EResizing>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EResizingEnabled, "enabled"))
			(data_type::value_type(Domain::Xml::EResizingDisabled, "disabled"))
			(data_type::value_type(Domain::Xml::EResizingRequired, "required"));
}

template<>
Enum<Domain::Xml::EUnknown>::data_type Enum<Domain::Xml::EUnknown>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EUnknownIgnore, "ignore"))
			(data_type::value_type(Domain::Xml::EUnknownFault, "fault"));
}

template<>
Enum<Domain::Xml::EValue>::data_type Enum<Domain::Xml::EValue>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EValueBroken, "broken"))
			(data_type::value_type(Domain::Xml::EValueWorkaround, "workaround"))
			(data_type::value_type(Domain::Xml::EValueFixed, "fixed"));
}

template<>
Enum<Domain::Xml::EValue1>::data_type Enum<Domain::Xml::EValue1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EValue1Broken, "broken"))
			(data_type::value_type(Domain::Xml::EValue1Workaround, "workaround"))
			(data_type::value_type(Domain::Xml::EValue1Fixed, "fixed"));
}

template<>
Enum<Domain::Xml::EValue2>::data_type Enum<Domain::Xml::EValue2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EValue2Broken, "broken"))
			(data_type::value_type(Domain::Xml::EValue2Workaround, "workaround"))
			(data_type::value_type(Domain::Xml::EValue2FixedIbs, "fixed-ibs"))
			(data_type::value_type(Domain::Xml::EValue2FixedCcd, "fixed-ccd"))
			(data_type::value_type(Domain::Xml::EValue2FixedNa, "fixed-na"));
}

template<>
Enum<Domain::Xml::EOffOptions>::data_type Enum<Domain::Xml::EOffOptions>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EOffOptionsDestroy, "destroy"))
			(data_type::value_type(Domain::Xml::EOffOptionsRestart, "restart"))
			(data_type::value_type(Domain::Xml::EOffOptionsPreserve, "preserve"))
			(data_type::value_type(Domain::Xml::EOffOptionsRenameRestart, "rename-restart"));
}

template<>
Enum<Domain::Xml::ECrashOptions>::data_type Enum<Domain::Xml::ECrashOptions>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ECrashOptionsDestroy, "destroy"))
			(data_type::value_type(Domain::Xml::ECrashOptionsRestart, "restart"))
			(data_type::value_type(Domain::Xml::ECrashOptionsPreserve, "preserve"))
			(data_type::value_type(Domain::Xml::ECrashOptionsRenameRestart, "rename-restart"))
			(data_type::value_type(Domain::Xml::ECrashOptionsCoredumpDestroy, "coredump-destroy"))
			(data_type::value_type(Domain::Xml::ECrashOptionsCoredumpRestart, "coredump-restart"));
}

template<>
Enum<Domain::Xml::ELockfailureOptions>::data_type Enum<Domain::Xml::ELockfailureOptions>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ELockfailureOptionsPoweroff, "poweroff"))
			(data_type::value_type(Domain::Xml::ELockfailureOptionsRestart, "restart"))
			(data_type::value_type(Domain::Xml::ELockfailureOptionsPause, "pause"))
			(data_type::value_type(Domain::Xml::ELockfailureOptionsIgnore, "ignore"));
}

template<>
Enum<Domain::Xml::EName4>::data_type Enum<Domain::Xml::EName4>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName4Cmt, "cmt"))
			(data_type::value_type(Domain::Xml::EName4Mbmt, "mbmt"))
			(data_type::value_type(Domain::Xml::EName4Mbml, "mbml"))
			(data_type::value_type(Domain::Xml::EName4CpuCycles, "cpu_cycles"))
			(data_type::value_type(Domain::Xml::EName4Instructions, "instructions"))
			(data_type::value_type(Domain::Xml::EName4CacheReferences, "cache_references"))
			(data_type::value_type(Domain::Xml::EName4CacheMisses, "cache_misses"))
			(data_type::value_type(Domain::Xml::EName4BranchInstructions, "branch_instructions"))
			(data_type::value_type(Domain::Xml::EName4BranchMisses, "branch_misses"))
			(data_type::value_type(Domain::Xml::EName4BusCycles, "bus_cycles"))
			(data_type::value_type(Domain::Xml::EName4StalledCyclesFrontend, "stalled_cycles_frontend"))
			(data_type::value_type(Domain::Xml::EName4StalledCyclesBackend, "stalled_cycles_backend"))
			(data_type::value_type(Domain::Xml::EName4RefCpuCycles, "ref_cpu_cycles"))
			(data_type::value_type(Domain::Xml::EName4CpuClock, "cpu_clock"))
			(data_type::value_type(Domain::Xml::EName4TaskClock, "task_clock"))
			(data_type::value_type(Domain::Xml::EName4PageFaults, "page_faults"))
			(data_type::value_type(Domain::Xml::EName4ContextSwitches, "context_switches"))
			(data_type::value_type(Domain::Xml::EName4CpuMigrations, "cpu_migrations"))
			(data_type::value_type(Domain::Xml::EName4PageFaultsMin, "page_faults_min"))
			(data_type::value_type(Domain::Xml::EName4PageFaultsMaj, "page_faults_maj"))
			(data_type::value_type(Domain::Xml::EName4AlignmentFaults, "alignment_faults"))
			(data_type::value_type(Domain::Xml::EName4EmulationFaults, "emulation_faults"));
}

template<>
Enum<Domain::Xml::EDevice>::data_type Enum<Domain::Xml::EDevice>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EDeviceFloppy, "floppy"))
			(data_type::value_type(Domain::Xml::EDeviceDisk, "disk"))
			(data_type::value_type(Domain::Xml::EDeviceCdrom, "cdrom"));
}

template<>
Enum<Domain::Xml::EDevice1>::data_type Enum<Domain::Xml::EDevice1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EDevice1Lun, "lun"));
}

template<>
Enum<Domain::Xml::ESgio>::data_type Enum<Domain::Xml::ESgio>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ESgioFiltered, "filtered"))
			(data_type::value_type(Domain::Xml::ESgioUnfiltered, "unfiltered"))
			(data_type::value_type(Domain::Xml::ESgioVirtioNonTransitional, "virtio-non-transitional"));
}

template<>
Enum<Domain::Xml::ESnapshot>::data_type Enum<Domain::Xml::ESnapshot>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ESnapshotNo, "no"))
			(data_type::value_type(Domain::Xml::ESnapshotInternal, "internal"))
			(data_type::value_type(Domain::Xml::ESnapshotExternal, "external"));
}

template<>
Enum<Domain::Xml::EType7>::data_type Enum<Domain::Xml::EType7>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType7Aio, "aio"));
}

template<>
Enum<Domain::Xml::EStorageFormat>::data_type Enum<Domain::Xml::EStorageFormat>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EStorageFormatRaw, "raw"))
			(data_type::value_type(Domain::Xml::EStorageFormatDir, "dir"))
			(data_type::value_type(Domain::Xml::EStorageFormatBochs, "bochs"))
			(data_type::value_type(Domain::Xml::EStorageFormatCloop, "cloop"))
			(data_type::value_type(Domain::Xml::EStorageFormatDmg, "dmg"))
			(data_type::value_type(Domain::Xml::EStorageFormatIso, "iso"))
			(data_type::value_type(Domain::Xml::EStorageFormatVpc, "vpc"))
			(data_type::value_type(Domain::Xml::EStorageFormatVdi, "vdi"))
			(data_type::value_type(Domain::Xml::EStorageFormatFat, "fat"))
			(data_type::value_type(Domain::Xml::EStorageFormatVhd, "vhd"))
			(data_type::value_type(Domain::Xml::EStorageFormatPloop, "ploop"))
			(data_type::value_type(Domain::Xml::EStorageFormatLuks, "luks"));
}

template<>
Enum<Domain::Xml::EStorageFormatBacking>::data_type Enum<Domain::Xml::EStorageFormatBacking>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EStorageFormatBackingCow, "cow"))
			(data_type::value_type(Domain::Xml::EStorageFormatBackingQcow, "qcow"))
			(data_type::value_type(Domain::Xml::EStorageFormatBackingQcow2, "qcow2"))
			(data_type::value_type(Domain::Xml::EStorageFormatBackingQed, "qed"))
			(data_type::value_type(Domain::Xml::EStorageFormatBackingVmdk, "vmdk"));
}

template<>
Enum<Domain::Xml::ECache>::data_type Enum<Domain::Xml::ECache>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ECacheNone, "none"))
			(data_type::value_type(Domain::Xml::ECacheWriteback, "writeback"))
			(data_type::value_type(Domain::Xml::ECacheWritethrough, "writethrough"))
			(data_type::value_type(Domain::Xml::ECacheDirectsync, "directsync"))
			(data_type::value_type(Domain::Xml::ECacheUnsafe, "unsafe"));
}

template<>
Enum<Domain::Xml::EErrorPolicy>::data_type Enum<Domain::Xml::EErrorPolicy>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EErrorPolicyStop, "stop"))
			(data_type::value_type(Domain::Xml::EErrorPolicyReport, "report"))
			(data_type::value_type(Domain::Xml::EErrorPolicyIgnore, "ignore"))
			(data_type::value_type(Domain::Xml::EErrorPolicyEnospace, "enospace"));
}

template<>
Enum<Domain::Xml::ERerrorPolicy>::data_type Enum<Domain::Xml::ERerrorPolicy>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ERerrorPolicyStop, "stop"))
			(data_type::value_type(Domain::Xml::ERerrorPolicyReport, "report"))
			(data_type::value_type(Domain::Xml::ERerrorPolicyIgnore, "ignore"));
}

template<>
Enum<Domain::Xml::EIo>::data_type Enum<Domain::Xml::EIo>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EIoThreads, "threads"))
			(data_type::value_type(Domain::Xml::EIoNative, "native"));
}

template<>
Enum<Domain::Xml::EDiscard>::data_type Enum<Domain::Xml::EDiscard>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EDiscardUnmap, "unmap"))
			(data_type::value_type(Domain::Xml::EDiscardIgnore, "ignore"));
}

template<>
Enum<Domain::Xml::EDetectZeroes>::data_type Enum<Domain::Xml::EDetectZeroes>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EDetectZeroesOff, "off"))
			(data_type::value_type(Domain::Xml::EDetectZeroesOn, "on"))
			(data_type::value_type(Domain::Xml::EDetectZeroesUnmap, "unmap"));
}

template<>
Enum<Domain::Xml::EJob>::data_type Enum<Domain::Xml::EJob>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EJobCopy, "copy"));
}

template<>
Enum<Domain::Xml::EJob1>::data_type Enum<Domain::Xml::EJob1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EJob1Copy, "copy"))
			(data_type::value_type(Domain::Xml::EJob1ActiveCommit, "active-commit"));
}

template<>
Enum<Domain::Xml::EReady>::data_type Enum<Domain::Xml::EReady>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EReadyYes, "yes"))
			(data_type::value_type(Domain::Xml::EReadyAbort, "abort"))
			(data_type::value_type(Domain::Xml::EReadyPivot, "pivot"));
}

template<>
Enum<Domain::Xml::EType8>::data_type Enum<Domain::Xml::EType8>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType8Ceph, "ceph"))
			(data_type::value_type(Domain::Xml::EType8Iscsi, "iscsi"));
}

template<>
Enum<Domain::Xml::EBus>::data_type Enum<Domain::Xml::EBus>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EBusIde, "ide"))
			(data_type::value_type(Domain::Xml::EBusFdc, "fdc"))
			(data_type::value_type(Domain::Xml::EBusScsi, "scsi"))
			(data_type::value_type(Domain::Xml::EBusVirtio, "virtio"))
			(data_type::value_type(Domain::Xml::EBusXen, "xen"))
			(data_type::value_type(Domain::Xml::EBusUsb, "usb"))
			(data_type::value_type(Domain::Xml::EBusUml, "uml"))
			(data_type::value_type(Domain::Xml::EBusSata, "sata"))
			(data_type::value_type(Domain::Xml::EBusSd, "sd"));
}

template<>
Enum<Domain::Xml::ETray>::data_type Enum<Domain::Xml::ETray>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ETrayOpen, "open"))
			(data_type::value_type(Domain::Xml::ETrayClosed, "closed"));
}

template<>
Enum<Domain::Xml::ETrans>::data_type Enum<Domain::Xml::ETrans>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ETransAuto, "auto"))
			(data_type::value_type(Domain::Xml::ETransNone, "none"))
			(data_type::value_type(Domain::Xml::ETransLba, "lba"));
}

template<>
Enum<Domain::Xml::EType9>::data_type Enum<Domain::Xml::EType9>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType9Fdc, "fdc"))
			(data_type::value_type(Domain::Xml::EType9Ide, "ide"))
			(data_type::value_type(Domain::Xml::EType9Sata, "sata"))
			(data_type::value_type(Domain::Xml::EType9Ccid, "ccid"));
}

template<>
Enum<Domain::Xml::EModel>::data_type Enum<Domain::Xml::EModel>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModelAuto, "auto"))
			(data_type::value_type(Domain::Xml::EModelBuslogic, "buslogic"))
			(data_type::value_type(Domain::Xml::EModelLsilogic, "lsilogic"))
			(data_type::value_type(Domain::Xml::EModelLsisas1068, "lsisas1068"))
			(data_type::value_type(Domain::Xml::EModelVmpvscsi, "vmpvscsi"))
			(data_type::value_type(Domain::Xml::EModelIbmvscsi, "ibmvscsi"))
			(data_type::value_type(Domain::Xml::EModelVirtioScsi, "virtio-scsi"))
			(data_type::value_type(Domain::Xml::EModelLsisas1078, "lsisas1078"))
			(data_type::value_type(Domain::Xml::EModelVirtioTransitional, "virtio-transitional"))
			(data_type::value_type(Domain::Xml::EModelVirtioNonTransitional, "virtio-non-transitional"))
			(data_type::value_type(Domain::Xml::EModelNcr53c90, "ncr53c90"))
			(data_type::value_type(Domain::Xml::EModelDc390, "dc390"))
			(data_type::value_type(Domain::Xml::EModelAm53c974, "am53c974"))
			(data_type::value_type(Domain::Xml::EModelHvScsi, "hv-scsi"));
}

template<>
Enum<Domain::Xml::EModel1>::data_type Enum<Domain::Xml::EModel1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel1Piix3Uhci, "piix3-uhci"))
			(data_type::value_type(Domain::Xml::EModel1Piix4Uhci, "piix4-uhci"))
			(data_type::value_type(Domain::Xml::EModel1Ehci, "ehci"))
			(data_type::value_type(Domain::Xml::EModel1Ich9Ehci1, "ich9-ehci1"))
			(data_type::value_type(Domain::Xml::EModel1Ich9Uhci1, "ich9-uhci1"))
			(data_type::value_type(Domain::Xml::EModel1Ich9Uhci2, "ich9-uhci2"))
			(data_type::value_type(Domain::Xml::EModel1Ich9Uhci3, "ich9-uhci3"))
			(data_type::value_type(Domain::Xml::EModel1Vt82c686bUhci, "vt82c686b-uhci"))
			(data_type::value_type(Domain::Xml::EModel1PciOhci, "pci-ohci"))
			(data_type::value_type(Domain::Xml::EModel1NecXhci, "nec-xhci"))
			(data_type::value_type(Domain::Xml::EModel1None, "none"))
			(data_type::value_type(Domain::Xml::EModel1Qusb1, "qusb1"))
			(data_type::value_type(Domain::Xml::EModel1Qusb2, "qusb2"))
			(data_type::value_type(Domain::Xml::EModel1QemuXhci, "qemu-xhci"));
}

template<>
Enum<Domain::Xml::EModel2>::data_type Enum<Domain::Xml::EModel2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel2Piix3, "piix3"))
			(data_type::value_type(Domain::Xml::EModel2Piix4, "piix4"))
			(data_type::value_type(Domain::Xml::EModel2Ich6, "ich6"));
}

template<>
Enum<Domain::Xml::EName5>::data_type Enum<Domain::Xml::EName5>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName5SpaprPciHostBridge, "spapr-pci-host-bridge"))
			(data_type::value_type(Domain::Xml::EName5PciBridge, "pci-bridge"))
			(data_type::value_type(Domain::Xml::EName5I82801b11Bridge, "i82801b11-bridge"))
			(data_type::value_type(Domain::Xml::EName5PciePciBridge, "pcie-pci-bridge"))
			(data_type::value_type(Domain::Xml::EName5Ioh3420, "ioh3420"))
			(data_type::value_type(Domain::Xml::EName5PcieRootPort, "pcie-root-port"))
			(data_type::value_type(Domain::Xml::EName5X3130Upstream, "x3130-upstream"))
			(data_type::value_type(Domain::Xml::EName5Xio3130Downstream, "xio3130-downstream"))
			(data_type::value_type(Domain::Xml::EName5Pxb, "pxb"))
			(data_type::value_type(Domain::Xml::EName5PxbPcie, "pxb-pcie"));
}

template<>
Enum<Domain::Xml::EModel3>::data_type Enum<Domain::Xml::EModel3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel3PciRoot, "pci-root"))
			(data_type::value_type(Domain::Xml::EModel3PcieRoot, "pcie-root"));
}

template<>
Enum<Domain::Xml::EModel4>::data_type Enum<Domain::Xml::EModel4>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel4PciBridge, "pci-bridge"))
			(data_type::value_type(Domain::Xml::EModel4DmiToPciBridge, "dmi-to-pci-bridge"))
			(data_type::value_type(Domain::Xml::EModel4PcieToPciBridge, "pcie-to-pci-bridge"))
			(data_type::value_type(Domain::Xml::EModel4PcieRootPort, "pcie-root-port"))
			(data_type::value_type(Domain::Xml::EModel4PcieSwitchUpstreamPort, "pcie-switch-upstream-port"))
			(data_type::value_type(Domain::Xml::EModel4PcieSwitchDownstreamPort, "pcie-switch-downstream-port"))
			(data_type::value_type(Domain::Xml::EModel4PciExpanderBus, "pci-expander-bus"))
			(data_type::value_type(Domain::Xml::EModel4PcieExpanderBus, "pcie-expander-bus"));
}

template<>
Enum<Domain::Xml::EModel5>::data_type Enum<Domain::Xml::EModel5>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel5Virtio, "virtio"))
			(data_type::value_type(Domain::Xml::EModel5VirtioTransitional, "virtio-transitional"))
			(data_type::value_type(Domain::Xml::EModel5VirtioNonTransitional, "virtio-non-transitional"));
}

template<>
Enum<Domain::Xml::EType10>::data_type Enum<Domain::Xml::EType10>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType10Path, "path"))
			(data_type::value_type(Domain::Xml::EType10Handle, "handle"))
			(data_type::value_type(Domain::Xml::EType10Loop, "loop"))
			(data_type::value_type(Domain::Xml::EType10Nbd, "nbd"))
			(data_type::value_type(Domain::Xml::EType10Ploop, "ploop"));
}

template<>
Enum<Domain::Xml::EAccessmode>::data_type Enum<Domain::Xml::EAccessmode>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EAccessmodePassthrough, "passthrough"))
			(data_type::value_type(Domain::Xml::EAccessmodeMapped, "mapped"))
			(data_type::value_type(Domain::Xml::EAccessmodeSquash, "squash"));
}

template<>
Enum<Domain::Xml::EMultidevs>::data_type Enum<Domain::Xml::EMultidevs>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMultidevsDefault, "default"))
			(data_type::value_type(Domain::Xml::EMultidevsRemap, "remap"))
			(data_type::value_type(Domain::Xml::EMultidevsForbid, "forbid"))
			(data_type::value_type(Domain::Xml::EMultidevsWarn, "warn"));
}

template<>
Enum<Domain::Xml::EModel6>::data_type Enum<Domain::Xml::EModel6>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel6Virtio, "virtio"))
			(data_type::value_type(Domain::Xml::EModel6VirtioTransitional, "virtio-transitional"))
			(data_type::value_type(Domain::Xml::EModel6VirtioNonTransitional, "virtio-non-transitional"));
}

template<>
Enum<Domain::Xml::EMacTableManager>::data_type Enum<Domain::Xml::EMacTableManager>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMacTableManagerKernel, "kernel"))
			(data_type::value_type(Domain::Xml::EMacTableManagerLibvirt, "libvirt"));
}

template<>
Enum<Domain::Xml::EState>::data_type Enum<Domain::Xml::EState>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EStateUp, "up"))
			(data_type::value_type(Domain::Xml::EStateDown, "down"));
}

template<>
Enum<Domain::Xml::EName6>::data_type Enum<Domain::Xml::EName6>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName6Kvm, "kvm"))
			(data_type::value_type(Domain::Xml::EName6Vfio, "vfio"));
}

template<>
Enum<Domain::Xml::EName7>::data_type Enum<Domain::Xml::EName7>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName7Qemu, "qemu"))
			(data_type::value_type(Domain::Xml::EName7Vhost, "vhost"));
}

template<>
Enum<Domain::Xml::ETxmode>::data_type Enum<Domain::Xml::ETxmode>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ETxmodeIothread, "iothread"))
			(data_type::value_type(Domain::Xml::ETxmodeTimer, "timer"));
}

template<>
Enum<Domain::Xml::ENativeMode>::data_type Enum<Domain::Xml::ENativeMode>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ENativeModeTagged, "tagged"))
			(data_type::value_type(Domain::Xml::ENativeModeUntagged, "untagged"));
}

template<>
Enum<Domain::Xml::EType11>::data_type Enum<Domain::Xml::EType11>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType11Unix, "unix"));
}

template<>
Enum<Domain::Xml::EMode10>::data_type Enum<Domain::Xml::EMode10>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode10Server, "server"))
			(data_type::value_type(Domain::Xml::EMode10Client, "client"));
}

template<>
Enum<Domain::Xml::EType12>::data_type Enum<Domain::Xml::EType12>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType12Mcast, "mcast"))
			(data_type::value_type(Domain::Xml::EType12Client, "client"));
}

template<>
Enum<Domain::Xml::EType13>::data_type Enum<Domain::Xml::EType13>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType13Tablet, "tablet"))
			(data_type::value_type(Domain::Xml::EType13Mouse, "mouse"))
			(data_type::value_type(Domain::Xml::EType13Keyboard, "keyboard"));
}

template<>
Enum<Domain::Xml::EBus1>::data_type Enum<Domain::Xml::EBus1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EBus1Ps2, "ps2"))
			(data_type::value_type(Domain::Xml::EBus1Usb, "usb"))
			(data_type::value_type(Domain::Xml::EBus1Xen, "xen"));
}

template<>
Enum<Domain::Xml::EModel7>::data_type Enum<Domain::Xml::EModel7>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel7Virtio, "virtio"))
			(data_type::value_type(Domain::Xml::EModel7VirtioTransitional, "virtio-transitional"))
			(data_type::value_type(Domain::Xml::EModel7VirtioNonTransitional, "virtio-non-transitional"));
}

template<>
Enum<Domain::Xml::EModel8>::data_type Enum<Domain::Xml::EModel8>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel8Sb16, "sb16"))
			(data_type::value_type(Domain::Xml::EModel8Es1370, "es1370"))
			(data_type::value_type(Domain::Xml::EModel8Pcspk, "pcspk"))
			(data_type::value_type(Domain::Xml::EModel8Ac97, "ac97"))
			(data_type::value_type(Domain::Xml::EModel8Ich6, "ich6"))
			(data_type::value_type(Domain::Xml::EModel8Ich9, "ich9"))
			(data_type::value_type(Domain::Xml::EModel8Usb, "usb"));
}

template<>
Enum<Domain::Xml::EType14>::data_type Enum<Domain::Xml::EType14>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType14Duplex, "duplex"))
			(data_type::value_type(Domain::Xml::EType14Micro, "micro"));
}

template<>
Enum<Domain::Xml::EName8>::data_type Enum<Domain::Xml::EName8>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName8Kvm, "kvm"))
			(data_type::value_type(Domain::Xml::EName8Vfio, "vfio"))
			(data_type::value_type(Domain::Xml::EName8Xen, "xen"));
}

template<>
Enum<Domain::Xml::ESgio1>::data_type Enum<Domain::Xml::ESgio1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ESgio1Filtered, "filtered"))
			(data_type::value_type(Domain::Xml::ESgio1Unfiltered, "unfiltered"));
}

template<>
Enum<Domain::Xml::EProtocol1>::data_type Enum<Domain::Xml::EProtocol1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EProtocol1Adapter, "adapter"));
}

template<>
Enum<Domain::Xml::EProtocol2>::data_type Enum<Domain::Xml::EProtocol2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EProtocol2Iscsi, "iscsi"));
}

template<>
Enum<Domain::Xml::ESharePolicy>::data_type Enum<Domain::Xml::ESharePolicy>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ESharePolicyAllowExclusive, "allow-exclusive"))
			(data_type::value_type(Domain::Xml::ESharePolicyForceShared, "force-shared"))
			(data_type::value_type(Domain::Xml::ESharePolicyIgnore, "ignore"));
}

template<>
Enum<Domain::Xml::EConnected>::data_type Enum<Domain::Xml::EConnected>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EConnectedKeep, "keep"));
}

template<>
Enum<Domain::Xml::EConnected1>::data_type Enum<Domain::Xml::EConnected1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EConnected1Fail, "fail"))
			(data_type::value_type(Domain::Xml::EConnected1Disconnect, "disconnect"))
			(data_type::value_type(Domain::Xml::EConnected1Keep, "keep"));
}

template<>
Enum<Domain::Xml::EDefaultMode>::data_type Enum<Domain::Xml::EDefaultMode>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EDefaultModeAny, "any"))
			(data_type::value_type(Domain::Xml::EDefaultModeSecure, "secure"))
			(data_type::value_type(Domain::Xml::EDefaultModeInsecure, "insecure"));
}

template<>
Enum<Domain::Xml::EName9>::data_type Enum<Domain::Xml::EName9>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName9Main, "main"))
			(data_type::value_type(Domain::Xml::EName9Display, "display"))
			(data_type::value_type(Domain::Xml::EName9Inputs, "inputs"))
			(data_type::value_type(Domain::Xml::EName9Cursor, "cursor"))
			(data_type::value_type(Domain::Xml::EName9Playback, "playback"))
			(data_type::value_type(Domain::Xml::EName9Record, "record"))
			(data_type::value_type(Domain::Xml::EName9Smartcard, "smartcard"))
			(data_type::value_type(Domain::Xml::EName9Usbredir, "usbredir"));
}

template<>
Enum<Domain::Xml::EMode11>::data_type Enum<Domain::Xml::EMode11>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode11Any, "any"))
			(data_type::value_type(Domain::Xml::EMode11Secure, "secure"))
			(data_type::value_type(Domain::Xml::EMode11Insecure, "insecure"));
}

template<>
Enum<Domain::Xml::ECompression>::data_type Enum<Domain::Xml::ECompression>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ECompressionAutoGlz, "auto_glz"))
			(data_type::value_type(Domain::Xml::ECompressionAutoLz, "auto_lz"))
			(data_type::value_type(Domain::Xml::ECompressionQuic, "quic"))
			(data_type::value_type(Domain::Xml::ECompressionGlz, "glz"))
			(data_type::value_type(Domain::Xml::ECompressionLz, "lz"))
			(data_type::value_type(Domain::Xml::ECompressionOff, "off"));
}

template<>
Enum<Domain::Xml::ECompression1>::data_type Enum<Domain::Xml::ECompression1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ECompression1Auto, "auto"))
			(data_type::value_type(Domain::Xml::ECompression1Never, "never"))
			(data_type::value_type(Domain::Xml::ECompression1Always, "always"));
}

template<>
Enum<Domain::Xml::ECompression2>::data_type Enum<Domain::Xml::ECompression2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ECompression2Auto, "auto"))
			(data_type::value_type(Domain::Xml::ECompression2Never, "never"))
			(data_type::value_type(Domain::Xml::ECompression2Always, "always"));
}

template<>
Enum<Domain::Xml::EMode12>::data_type Enum<Domain::Xml::EMode12>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode12Filter, "filter"))
			(data_type::value_type(Domain::Xml::EMode12All, "all"))
			(data_type::value_type(Domain::Xml::EMode12Off, "off"));
}

template<>
Enum<Domain::Xml::EMode13>::data_type Enum<Domain::Xml::EMode13>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode13Server, "server"))
			(data_type::value_type(Domain::Xml::EMode13Client, "client"));
}

template<>
Enum<Domain::Xml::EName10>::data_type Enum<Domain::Xml::EName10>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName10Qemu, "qemu"))
			(data_type::value_type(Domain::Xml::EName10Vhostuser, "vhostuser"));
}

template<>
Enum<Domain::Xml::EVgaconf>::data_type Enum<Domain::Xml::EVgaconf>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EVgaconfIo, "io"))
			(data_type::value_type(Domain::Xml::EVgaconfOn, "on"))
			(data_type::value_type(Domain::Xml::EVgaconfOff, "off"));
}

template<>
Enum<Domain::Xml::EType15>::data_type Enum<Domain::Xml::EType15>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType15Vga, "vga"))
			(data_type::value_type(Domain::Xml::EType15Cirrus, "cirrus"))
			(data_type::value_type(Domain::Xml::EType15Vmvga, "vmvga"))
			(data_type::value_type(Domain::Xml::EType15Xen, "xen"))
			(data_type::value_type(Domain::Xml::EType15Vbox, "vbox"))
			(data_type::value_type(Domain::Xml::EType15Virtio, "virtio"))
			(data_type::value_type(Domain::Xml::EType15Vzct, "vzct"))
			(data_type::value_type(Domain::Xml::EType15Gop, "gop"))
			(data_type::value_type(Domain::Xml::EType15None, "none"))
			(data_type::value_type(Domain::Xml::EType15Bochs, "bochs"))
			(data_type::value_type(Domain::Xml::EType15Ramfb, "ramfb"));
}

template<>
Enum<Domain::Xml::EQemucdevSrcTypeChoice>::data_type Enum<Domain::Xml::EQemucdevSrcTypeChoice>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceDev, "dev"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceFile, "file"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoicePipe, "pipe"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceUnix, "unix"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceTcp, "tcp"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceUdp, "udp"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceNull, "null"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceStdio, "stdio"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceVc, "vc"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoicePty, "pty"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceSpicevmc, "spicevmc"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceSpiceport, "spiceport"))
			(data_type::value_type(Domain::Xml::EQemucdevSrcTypeChoiceNmdm, "nmdm"));
}

template<>
Enum<Domain::Xml::EType16>::data_type Enum<Domain::Xml::EType16>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType16Raw, "raw"))
			(data_type::value_type(Domain::Xml::EType16Telnet, "telnet"))
			(data_type::value_type(Domain::Xml::EType16Telnets, "telnets"))
			(data_type::value_type(Domain::Xml::EType16Tls, "tls"));
}

template<>
Enum<Domain::Xml::EType17>::data_type Enum<Domain::Xml::EType17>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType17Xen, "xen"))
			(data_type::value_type(Domain::Xml::EType17Serial, "serial"))
			(data_type::value_type(Domain::Xml::EType17Uml, "uml"))
			(data_type::value_type(Domain::Xml::EType17Virtio, "virtio"))
			(data_type::value_type(Domain::Xml::EType17Lxc, "lxc"))
			(data_type::value_type(Domain::Xml::EType17Openvz, "openvz"))
			(data_type::value_type(Domain::Xml::EType17Sclp, "sclp"))
			(data_type::value_type(Domain::Xml::EType17Sclplm, "sclplm"));
}

template<>
Enum<Domain::Xml::EType18>::data_type Enum<Domain::Xml::EType18>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType18IsaSerial, "isa-serial"))
			(data_type::value_type(Domain::Xml::EType18UsbSerial, "usb-serial"));
}

template<>
Enum<Domain::Xml::EName11>::data_type Enum<Domain::Xml::EName11>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName11IsaSerial, "isa-serial"))
			(data_type::value_type(Domain::Xml::EName11UsbSerial, "usb-serial"))
			(data_type::value_type(Domain::Xml::EName11PciSerial, "pci-serial"))
			(data_type::value_type(Domain::Xml::EName11SpaprVty, "spapr-vty"))
			(data_type::value_type(Domain::Xml::EName11Pl011, "pl011"))
			(data_type::value_type(Domain::Xml::EName11Sclpconsole, "sclpconsole"))
			(data_type::value_type(Domain::Xml::EName11Sclplmconsole, "sclplmconsole"));
}

template<>
Enum<Domain::Xml::EState1>::data_type Enum<Domain::Xml::EState1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EState1Connected, "connected"))
			(data_type::value_type(Domain::Xml::EState1Disconnected, "disconnected"));
}

template<>
Enum<Domain::Xml::EModel9>::data_type Enum<Domain::Xml::EModel9>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel9Virtio, "virtio"))
			(data_type::value_type(Domain::Xml::EModel9VirtioTransitional, "virtio-transitional"))
			(data_type::value_type(Domain::Xml::EModel9VirtioNonTransitional, "virtio-non-transitional"));
}

template<>
Enum<Domain::Xml::EModel10>::data_type Enum<Domain::Xml::EModel10>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel10TpmTis, "tpm-tis"))
			(data_type::value_type(Domain::Xml::EModel10TpmCrb, "tpm-crb"));
}

template<>
Enum<Domain::Xml::EVersion>::data_type Enum<Domain::Xml::EVersion>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EVersion12, "1.2"))
			(data_type::value_type(Domain::Xml::EVersion20, "2.0"));
}

template<>
Enum<Domain::Xml::EModel11>::data_type Enum<Domain::Xml::EModel11>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel11Dimm, "dimm"));
}

template<>
Enum<Domain::Xml::EAccess>::data_type Enum<Domain::Xml::EAccess>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EAccessShared, "shared"))
			(data_type::value_type(Domain::Xml::EAccessPrivate, "private"));
}

template<>
Enum<Domain::Xml::EModel12>::data_type Enum<Domain::Xml::EModel12>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel12I6300esb, "i6300esb"))
			(data_type::value_type(Domain::Xml::EModel12Ib700, "ib700"));
}

template<>
Enum<Domain::Xml::EAction>::data_type Enum<Domain::Xml::EAction>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EActionReset, "reset"))
			(data_type::value_type(Domain::Xml::EActionShutdown, "shutdown"))
			(data_type::value_type(Domain::Xml::EActionPoweroff, "poweroff"))
			(data_type::value_type(Domain::Xml::EActionPause, "pause"))
			(data_type::value_type(Domain::Xml::EActionNone, "none"))
			(data_type::value_type(Domain::Xml::EActionDump, "dump"));
}

template<>
Enum<Domain::Xml::EModel13>::data_type Enum<Domain::Xml::EModel13>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel13Virtio, "virtio"))
			(data_type::value_type(Domain::Xml::EModel13VirtioTransitional, "virtio-transitional"))
			(data_type::value_type(Domain::Xml::EModel13VirtioNonTransitional, "virtio-non-transitional"))
			(data_type::value_type(Domain::Xml::EModel13Xen, "xen"))
			(data_type::value_type(Domain::Xml::EModel13None, "none"));
}

template<>
Enum<Domain::Xml::EModel14>::data_type Enum<Domain::Xml::EModel14>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel14Isa, "isa"))
			(data_type::value_type(Domain::Xml::EModel14Pseries, "pseries"))
			(data_type::value_type(Domain::Xml::EModel14Hyperv, "hyperv"));
}

template<>
Enum<Domain::Xml::EName12>::data_type Enum<Domain::Xml::EName12>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName12Aes, "aes"))
			(data_type::value_type(Domain::Xml::EName12Dea, "dea"));
}

} // namespace Libvirt
