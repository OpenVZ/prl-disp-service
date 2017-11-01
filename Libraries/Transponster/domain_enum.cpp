/*
 * Copyright (c) 2015-2017, Parallels International GmbH
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
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
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
			(data_type::value_type(Domain::Xml::ETypeParallels, "parallels"))
			(data_type::value_type(Domain::Xml::ETypeBhyve, "bhyve"));
}

template<>
Enum<Domain::Xml::EMode>::data_type Enum<Domain::Xml::EMode>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModeCustom, "custom"))
			(data_type::value_type(Domain::Xml::EModeHostModel, "host-model"))
			(data_type::value_type(Domain::Xml::EModeHostPassthrough, "host-passthrough"));
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
Enum<Domain::Xml::EType1>::data_type Enum<Domain::Xml::EType1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType1Xen, "xen"))
			(data_type::value_type(Domain::Xml::EType1Linux, "linux"));
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
Enum<Domain::Xml::EReadonly>::data_type Enum<Domain::Xml::EReadonly>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EReadonlyYes, "yes"))
			(data_type::value_type(Domain::Xml::EReadonlyNo, "no"));
}

template<>
Enum<Domain::Xml::EType2>::data_type Enum<Domain::Xml::EType2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType2Rom, "rom"))
			(data_type::value_type(Domain::Xml::EType2Pflash, "pflash"));
}

template<>
Enum<Domain::Xml::EFormat>::data_type Enum<Domain::Xml::EFormat>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EFormatRaw, "raw"))
			(data_type::value_type(Domain::Xml::EFormatQcow2, "qcow2"));
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
Enum<Domain::Xml::EVirYesNo>::data_type Enum<Domain::Xml::EVirYesNo>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EVirYesNoYes, "yes"))
			(data_type::value_type(Domain::Xml::EVirYesNoNo, "no"));
}

template<>
Enum<Domain::Xml::EMode1>::data_type Enum<Domain::Xml::EMode1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode1Emulate, "emulate"))
			(data_type::value_type(Domain::Xml::EMode1Host, "host"))
			(data_type::value_type(Domain::Xml::EMode1Sysinfo, "sysinfo"));
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
Enum<Domain::Xml::EName>::data_type Enum<Domain::Xml::EName>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::ENamePlatform, "platform"))
			(data_type::value_type(Domain::Xml::ENameRtc, "rtc"));
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
Enum<Domain::Xml::EMode2>::data_type Enum<Domain::Xml::EMode2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode2Auto, "auto"))
			(data_type::value_type(Domain::Xml::EMode2Native, "native"))
			(data_type::value_type(Domain::Xml::EMode2Emulate, "emulate"))
			(data_type::value_type(Domain::Xml::EMode2Paravirt, "paravirt"))
			(data_type::value_type(Domain::Xml::EMode2Smpsafe, "smpsafe"));
}

template<>
Enum<Domain::Xml::EName1>::data_type Enum<Domain::Xml::EName1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName1Hpet, "hpet"))
			(data_type::value_type(Domain::Xml::EName1Pit, "pit"));
}

template<>
Enum<Domain::Xml::EName2>::data_type Enum<Domain::Xml::EName2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName2Kvmclock, "kvmclock"))
			(data_type::value_type(Domain::Xml::EName2Hypervclock, "hypervclock"));
}

template<>
Enum<Domain::Xml::EVirOnOff>::data_type Enum<Domain::Xml::EVirOnOff>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EVirOnOffOn, "on"))
			(data_type::value_type(Domain::Xml::EVirOnOffOff, "off"));
}

template<>
Enum<Domain::Xml::EPlacement>::data_type Enum<Domain::Xml::EPlacement>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EPlacementStatic, "static"))
			(data_type::value_type(Domain::Xml::EPlacementAuto, "auto"));
}

template<>
Enum<Domain::Xml::EMode3>::data_type Enum<Domain::Xml::EMode3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode3Strict, "strict"))
			(data_type::value_type(Domain::Xml::EMode3Preferred, "preferred"))
			(data_type::value_type(Domain::Xml::EMode3Interleave, "interleave"));
}

template<>
Enum<Domain::Xml::EMode4>::data_type Enum<Domain::Xml::EMode4>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode4Strict, "strict"))
			(data_type::value_type(Domain::Xml::EMode4Preferred, "preferred"))
			(data_type::value_type(Domain::Xml::EMode4Interleave, "interleave"));
}

template<>
Enum<Domain::Xml::EPolicy1>::data_type Enum<Domain::Xml::EPolicy1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EPolicy1Default, "default"))
			(data_type::value_type(Domain::Xml::EPolicy1Allow, "allow"))
			(data_type::value_type(Domain::Xml::EPolicy1Deny, "deny"));
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
			(data_type::value_type(Domain::Xml::ESgioUnfiltered, "unfiltered"));
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
Enum<Domain::Xml::EStartupPolicy>::data_type Enum<Domain::Xml::EStartupPolicy>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EStartupPolicyMandatory, "mandatory"))
			(data_type::value_type(Domain::Xml::EStartupPolicyRequisite, "requisite"))
			(data_type::value_type(Domain::Xml::EStartupPolicyOptional, "optional"));
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
Enum<Domain::Xml::EMode5>::data_type Enum<Domain::Xml::EMode5>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode5Host, "host"))
			(data_type::value_type(Domain::Xml::EMode5Direct, "direct"));
}

template<>
Enum<Domain::Xml::EType3>::data_type Enum<Domain::Xml::EType3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType3Aio, "aio"));
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
			(data_type::value_type(Domain::Xml::EStorageFormatVhd, "vhd"));
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
Enum<Domain::Xml::EType4>::data_type Enum<Domain::Xml::EType4>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType4Ceph, "ceph"))
			(data_type::value_type(Domain::Xml::EType4Iscsi, "iscsi"));
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
Enum<Domain::Xml::EFormat1>::data_type Enum<Domain::Xml::EFormat1>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EFormat1Default, "default"))
			(data_type::value_type(Domain::Xml::EFormat1Qcow, "qcow"));
}

template<>
Enum<Domain::Xml::EType5>::data_type Enum<Domain::Xml::EType5>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType5Passphrase, "passphrase"));
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
Enum<Domain::Xml::EType6>::data_type Enum<Domain::Xml::EType6>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType6Fdc, "fdc"))
			(data_type::value_type(Domain::Xml::EType6Ide, "ide"))
			(data_type::value_type(Domain::Xml::EType6Sata, "sata"))
			(data_type::value_type(Domain::Xml::EType6Ccid, "ccid"));
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
			(data_type::value_type(Domain::Xml::EModel1None, "none"));
}

template<>
Enum<Domain::Xml::EModel2>::data_type Enum<Domain::Xml::EModel2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel2PciRoot, "pci-root"))
			(data_type::value_type(Domain::Xml::EModel2PcieRoot, "pcie-root"));
}

template<>
Enum<Domain::Xml::EModel3>::data_type Enum<Domain::Xml::EModel3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel3PciBridge, "pci-bridge"))
			(data_type::value_type(Domain::Xml::EModel3DmiToPciBridge, "dmi-to-pci-bridge"));
}

template<>
Enum<Domain::Xml::EType7>::data_type Enum<Domain::Xml::EType7>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType7Path, "path"))
			(data_type::value_type(Domain::Xml::EType7Handle, "handle"))
			(data_type::value_type(Domain::Xml::EType7Loop, "loop"))
			(data_type::value_type(Domain::Xml::EType7Nbd, "nbd"));
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
Enum<Domain::Xml::EName3>::data_type Enum<Domain::Xml::EName3>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName3Kvm, "kvm"))
			(data_type::value_type(Domain::Xml::EName3Vfio, "vfio"));
}

template<>
Enum<Domain::Xml::EName4>::data_type Enum<Domain::Xml::EName4>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName4Qemu, "qemu"))
			(data_type::value_type(Domain::Xml::EName4Vhost, "vhost"));
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
Enum<Domain::Xml::EType8>::data_type Enum<Domain::Xml::EType8>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType8Unix, "unix"));
}

template<>
Enum<Domain::Xml::EMode6>::data_type Enum<Domain::Xml::EMode6>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode6Server, "server"))
			(data_type::value_type(Domain::Xml::EMode6Client, "client"));
}

template<>
Enum<Domain::Xml::EType9>::data_type Enum<Domain::Xml::EType9>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType9Mcast, "mcast"))
			(data_type::value_type(Domain::Xml::EType9Client, "client"));
}

template<>
Enum<Domain::Xml::EType10>::data_type Enum<Domain::Xml::EType10>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType10Tablet, "tablet"))
			(data_type::value_type(Domain::Xml::EType10Mouse, "mouse"))
			(data_type::value_type(Domain::Xml::EType10Keyboard, "keyboard"));
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
Enum<Domain::Xml::EModel4>::data_type Enum<Domain::Xml::EModel4>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel4Sb16, "sb16"))
			(data_type::value_type(Domain::Xml::EModel4Es1370, "es1370"))
			(data_type::value_type(Domain::Xml::EModel4Pcspk, "pcspk"))
			(data_type::value_type(Domain::Xml::EModel4Ac97, "ac97"))
			(data_type::value_type(Domain::Xml::EModel4Ich6, "ich6"))
			(data_type::value_type(Domain::Xml::EModel4Ich9, "ich9"))
			(data_type::value_type(Domain::Xml::EModel4Usb, "usb"));
}

template<>
Enum<Domain::Xml::EType11>::data_type Enum<Domain::Xml::EType11>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType11Duplex, "duplex"))
			(data_type::value_type(Domain::Xml::EType11Micro, "micro"));
}

template<>
Enum<Domain::Xml::EName5>::data_type Enum<Domain::Xml::EName5>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName5Kvm, "kvm"))
			(data_type::value_type(Domain::Xml::EName5Vfio, "vfio"))
			(data_type::value_type(Domain::Xml::EName5Xen, "xen"));
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
Enum<Domain::Xml::EName6>::data_type Enum<Domain::Xml::EName6>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EName6Main, "main"))
			(data_type::value_type(Domain::Xml::EName6Display, "display"))
			(data_type::value_type(Domain::Xml::EName6Inputs, "inputs"))
			(data_type::value_type(Domain::Xml::EName6Cursor, "cursor"))
			(data_type::value_type(Domain::Xml::EName6Playback, "playback"))
			(data_type::value_type(Domain::Xml::EName6Record, "record"))
			(data_type::value_type(Domain::Xml::EName6Smartcard, "smartcard"))
			(data_type::value_type(Domain::Xml::EName6Usbredir, "usbredir"));
}

template<>
Enum<Domain::Xml::EMode7>::data_type Enum<Domain::Xml::EMode7>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode7Any, "any"))
			(data_type::value_type(Domain::Xml::EMode7Secure, "secure"))
			(data_type::value_type(Domain::Xml::EMode7Insecure, "insecure"));
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
Enum<Domain::Xml::EMode8>::data_type Enum<Domain::Xml::EMode8>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode8Filter, "filter"))
			(data_type::value_type(Domain::Xml::EMode8All, "all"))
			(data_type::value_type(Domain::Xml::EMode8Off, "off"));
}

template<>
Enum<Domain::Xml::EMode9>::data_type Enum<Domain::Xml::EMode9>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EMode9Server, "server"))
			(data_type::value_type(Domain::Xml::EMode9Client, "client"));
}

template<>
Enum<Domain::Xml::EType12>::data_type Enum<Domain::Xml::EType12>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType12Vga, "vga"))
			(data_type::value_type(Domain::Xml::EType12Cirrus, "cirrus"))
			(data_type::value_type(Domain::Xml::EType12Vmvga, "vmvga"))
			(data_type::value_type(Domain::Xml::EType12Xen, "xen"))
			(data_type::value_type(Domain::Xml::EType12Vbox, "vbox"));
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
Enum<Domain::Xml::EType13>::data_type Enum<Domain::Xml::EType13>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType13Raw, "raw"))
			(data_type::value_type(Domain::Xml::EType13Telnet, "telnet"))
			(data_type::value_type(Domain::Xml::EType13Telnets, "telnets"))
			(data_type::value_type(Domain::Xml::EType13Tls, "tls"));
}

template<>
Enum<Domain::Xml::EType14>::data_type Enum<Domain::Xml::EType14>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType14Xen, "xen"))
			(data_type::value_type(Domain::Xml::EType14Serial, "serial"))
			(data_type::value_type(Domain::Xml::EType14Uml, "uml"))
			(data_type::value_type(Domain::Xml::EType14Virtio, "virtio"))
			(data_type::value_type(Domain::Xml::EType14Lxc, "lxc"))
			(data_type::value_type(Domain::Xml::EType14Openvz, "openvz"))
			(data_type::value_type(Domain::Xml::EType14Sclp, "sclp"))
			(data_type::value_type(Domain::Xml::EType14Sclplm, "sclplm"));
}

template<>
Enum<Domain::Xml::EType15>::data_type Enum<Domain::Xml::EType15>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType15IsaSerial, "isa-serial"))
			(data_type::value_type(Domain::Xml::EType15UsbSerial, "usb-serial"));
}

template<>
Enum<Domain::Xml::EType16>::data_type Enum<Domain::Xml::EType16>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EType16Usb, "usb"));
}

template<>
Enum<Domain::Xml::EBus2>::data_type Enum<Domain::Xml::EBus2>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EBus2Usb, "usb"));
}

template<>
Enum<Domain::Xml::EModel5>::data_type Enum<Domain::Xml::EModel5>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel5Virtio, "virtio"));
}

template<>
Enum<Domain::Xml::EChoice1043>::data_type Enum<Domain::Xml::EChoice1043>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EChoice1043DevRandom, "/dev/random"))
			(data_type::value_type(Domain::Xml::EChoice1043DevHwrng, "/dev/hwrng"));
}

template<>
Enum<Domain::Xml::EModel6>::data_type Enum<Domain::Xml::EModel6>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel6TpmTis, "tpm-tis"));
}

template<>
Enum<Domain::Xml::EModel7>::data_type Enum<Domain::Xml::EModel7>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel7Dimm, "dimm"));
}

template<>
Enum<Domain::Xml::EModel8>::data_type Enum<Domain::Xml::EModel8>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel8I6300esb, "i6300esb"))
			(data_type::value_type(Domain::Xml::EModel8Ib700, "ib700"));
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
Enum<Domain::Xml::EModel9>::data_type Enum<Domain::Xml::EModel9>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel9Virtio, "virtio"))
			(data_type::value_type(Domain::Xml::EModel9Xen, "xen"))
			(data_type::value_type(Domain::Xml::EModel9None, "none"));
}

template<>
Enum<Domain::Xml::EModel10>::data_type Enum<Domain::Xml::EModel10>::getData()
{
	return ba::list_of<data_type::relation>
			(data_type::value_type(Domain::Xml::EModel10Isa, "isa"))
			(data_type::value_type(Domain::Xml::EModel10Pseries, "pseries"))
			(data_type::value_type(Domain::Xml::EModel10Hyperv, "hyperv"));
}

} // namespace Libvirt
