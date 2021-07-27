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

#ifndef __DOMAIN_ENUM_H__
#define __DOMAIN_ENUM_H__
#include "enum.h"

namespace Libvirt
{
namespace Domain
{
namespace Xml
{
enum EType
{
	ETypeQemu,
	ETypeKqemu,
	ETypeKvm,
	ETypeXen,
	ETypeLxc,
	ETypeUml,
	ETypeOpenvz,
	ETypeTest,
	ETypeVmware,
	ETypeHyperv,
	ETypeVbox,
	ETypePhyp,
	ETypeVirtuozzo,
	ETypeBhyve
};

enum EMode
{
	EModeCustom,
	EModeHostModel,
	EModeHostPassthrough
};

enum EMatch
{
	EMatchMinimum,
	EMatchExact,
	EMatchStrict
};

enum ECheck
{
	ECheckNone,
	ECheckPartial,
	ECheckFull
};

enum EFallback
{
	EFallbackAllow,
	EFallbackForbid
};

enum EPolicy
{
	EPolicyForce,
	EPolicyRequire,
	EPolicyOptional,
	EPolicyDisable,
	EPolicyForbid
};

enum EMemAccess
{
	EMemAccessShared,
	EMemAccessPrivate
};

enum ESysinfoBiosName
{
	ESysinfoBiosNameVendor,
	ESysinfoBiosNameVersion,
	ESysinfoBiosNameDate,
	ESysinfoBiosNameRelease
};

enum ESysinfoSystemName
{
	ESysinfoSystemNameManufacturer,
	ESysinfoSystemNameProduct,
	ESysinfoSystemNameVersion,
	ESysinfoSystemNameSerial,
	ESysinfoSystemNameUuid,
	ESysinfoSystemNameSku,
	ESysinfoSystemNameFamily
};

enum ESysinfoBaseBoardName
{
	ESysinfoBaseBoardNameManufacturer,
	ESysinfoBaseBoardNameProduct,
	ESysinfoBaseBoardNameVersion,
	ESysinfoBaseBoardNameSerial,
	ESysinfoBaseBoardNameAsset,
	ESysinfoBaseBoardNameLocation
};

enum ESysinfoChassisName
{
	ESysinfoChassisNameManufacturer,
	ESysinfoChassisNameVersion,
	ESysinfoChassisNameSerial,
	ESysinfoChassisNameAsset,
	ESysinfoChassisNameSku
};

enum EArch
{
	EArchI686,
	EArchX8664,
	EArchIa64
};

enum EMachine
{
	EMachineXenpv,
	EMachineXenner
};

enum EType1
{
	EType1Xen,
	EType1Linux
};

enum EArch1
{
	EArch1I686,
	EArch1X8664
};

enum EMachine1
{
	EMachine1G3beige,
	EMachine1Mac99,
	EMachine1Prep,
	EMachine1Ppce500
};

enum EMachine2
{
	EMachine2Pseries
};

enum EArch2
{
	EArch2S390,
	EArch2S390x
};

enum EMachine3
{
	EMachine3S390,
	EMachine3S390Virtio,
	EMachine3S390Ccw,
	EMachine3S390CcwVirtio
};

enum EArch3
{
	EArch3Armv7l
};

enum EArch4
{
	EArch4Aarch64
};

enum EReadonly
{
	EReadonlyYes,
	EReadonlyNo
};

enum ESecure
{
	ESecureYes,
	ESecureNo
};

enum EType2
{
	EType2Rom,
	EType2Pflash
};

enum EFormat
{
	EFormatRaw,
	EFormatQcow2
};

enum EDev
{
	EDevHd,
	EDevFd,
	EDevCdrom,
	EDevNetwork
};

enum EVirYesNo
{
	EVirYesNoYes,
	EVirYesNoNo
};

enum EMode1
{
	EMode1Emulate,
	EMode1Host,
	EMode1Sysinfo
};

enum EArch5
{
	EArch5I686,
	EArch5X8664,
	EArch5Ppc,
	EArch5Ppc64,
	EArch5Mips,
	EArch5Sparc
};

enum EOffset
{
	EOffsetLocaltime,
	EOffsetUtc
};

enum EAdjustment
{
	EAdjustmentReset
};

enum EBasis
{
	EBasisUtc,
	EBasisLocaltime
};

enum EName
{
	ENamePlatform,
	ENameRtc
};

enum ETrack
{
	ETrackBoot,
	ETrackGuest,
	ETrackWall
};

enum ETickpolicy
{
	ETickpolicyDelay,
	ETickpolicyMerge,
	ETickpolicyDiscard
};

enum EMode2
{
	EMode2Auto,
	EMode2Native,
	EMode2Emulate,
	EMode2Paravirt,
	EMode2Smpsafe
};

enum EName1
{
	EName1Hpet,
	EName1Pit
};

enum EName2
{
	EName2Kvmclock,
	EName2Hypervclock
};

enum EVirOnOff
{
	EVirOnOffOn,
	EVirOnOffOff
};

enum EPlacement
{
	EPlacementStatic,
	EPlacementAuto
};

enum EMode3
{
	EMode3Strict,
	EMode3Preferred,
	EMode3Interleave
};

enum EMode4
{
	EMode4Strict,
	EMode4Preferred,
	EMode4Interleave
};

enum EPolicy1
{
	EPolicy1Default,
	EPolicy1Allow,
	EPolicy1Deny
};

enum EDriver
{
	EDriverQemu,
	EDriverKvm
};

enum EOffOptions
{
	EOffOptionsDestroy,
	EOffOptionsRestart,
	EOffOptionsPreserve,
	EOffOptionsRenameRestart
};

enum ECrashOptions
{
	ECrashOptionsDestroy,
	ECrashOptionsRestart,
	ECrashOptionsPreserve,
	ECrashOptionsRenameRestart,
	ECrashOptionsCoredumpDestroy,
	ECrashOptionsCoredumpRestart
};

enum ELockfailureOptions
{
	ELockfailureOptionsPoweroff,
	ELockfailureOptionsRestart,
	ELockfailureOptionsPause,
	ELockfailureOptionsIgnore
};

enum EName3
{
	EName3Cmt,
	EName3Mbmt,
	EName3Mbml,
	EName3CpuCycles,
	EName3Instructions,
	EName3CacheReferences,
	EName3CacheMisses,
	EName3BranchInstructions,
	EName3BranchMisses,
	EName3BusCycles,
	EName3StalledCyclesFrontend,
	EName3StalledCyclesBackend,
	EName3RefCpuCycles,
	EName3CpuClock,
	EName3TaskClock,
	EName3PageFaults,
	EName3ContextSwitches,
	EName3CpuMigrations,
	EName3PageFaultsMin,
	EName3PageFaultsMaj,
	EName3AlignmentFaults,
	EName3EmulationFaults
};

enum EDevice
{
	EDeviceFloppy,
	EDeviceDisk,
	EDeviceCdrom
};

enum EDevice1
{
	EDevice1Lun
};

enum ESgio
{
	ESgioFiltered,
	ESgioUnfiltered
};

enum ESnapshot
{
	ESnapshotNo,
	ESnapshotInternal,
	ESnapshotExternal
};

enum EStartupPolicy
{
	EStartupPolicyMandatory,
	EStartupPolicyRequisite,
	EStartupPolicyOptional
};

enum EProtocol
{
	EProtocolNbd,
	EProtocolRbd,
	EProtocolSheepdog,
	EProtocolGluster,
	EProtocolIscsi,
	EProtocolHttp,
	EProtocolHttps,
	EProtocolFtp,
	EProtocolFtps,
	EProtocolTftp
};

enum ETransport
{
	ETransportTcp,
	ETransportRdma
};

enum EMode5
{
	EMode5Host,
	EMode5Direct
};

enum EType3
{
	EType3Aio
};

enum EStorageFormat
{
	EStorageFormatRaw,
	EStorageFormatDir,
	EStorageFormatBochs,
	EStorageFormatCloop,
	EStorageFormatDmg,
	EStorageFormatIso,
	EStorageFormatVpc,
	EStorageFormatVdi,
	EStorageFormatFat,
	EStorageFormatVhd
};

enum EStorageFormatBacking
{
	EStorageFormatBackingCow,
	EStorageFormatBackingQcow,
	EStorageFormatBackingQcow2,
	EStorageFormatBackingQed,
	EStorageFormatBackingVmdk
};

enum ECache
{
	ECacheNone,
	ECacheWriteback,
	ECacheWritethrough,
	ECacheDirectsync,
	ECacheUnsafe
};

enum EErrorPolicy
{
	EErrorPolicyStop,
	EErrorPolicyReport,
	EErrorPolicyIgnore,
	EErrorPolicyEnospace
};

enum ERerrorPolicy
{
	ERerrorPolicyStop,
	ERerrorPolicyReport,
	ERerrorPolicyIgnore
};

enum EIo
{
	EIoThreads,
	EIoNative
};

enum EDiscard
{
	EDiscardUnmap,
	EDiscardIgnore
};

enum EJob
{
	EJobCopy
};

enum EJob1
{
	EJob1Copy,
	EJob1ActiveCommit
};

enum EReady
{
	EReadyYes,
	EReadyAbort,
	EReadyPivot
};

enum EType4
{
	EType4Ceph,
	EType4Iscsi
};

enum EBus
{
	EBusIde,
	EBusFdc,
	EBusScsi,
	EBusVirtio,
	EBusXen,
	EBusUsb,
	EBusUml,
	EBusSata,
	EBusSd
};

enum ETray
{
	ETrayOpen,
	ETrayClosed
};

enum EFormat1
{
	EFormat1Default,
	EFormat1Qcow
};

enum EType5
{
	EType5Passphrase
};

enum ETrans
{
	ETransAuto,
	ETransNone,
	ETransLba
};

enum EType6
{
	EType6Fdc,
	EType6Ide,
	EType6Sata,
	EType6Ccid
};

enum EModel
{
	EModelAuto,
	EModelBuslogic,
	EModelLsilogic,
	EModelLsisas1068,
	EModelVmpvscsi,
	EModelIbmvscsi,
	EModelVirtioScsi,
	EModelLsisas1078,
	EModelHvScsi
};

enum EModel1
{
	EModel1Piix3Uhci,
	EModel1Piix4Uhci,
	EModel1Ehci,
	EModel1Ich9Ehci1,
	EModel1Ich9Uhci1,
	EModel1Ich9Uhci2,
	EModel1Ich9Uhci3,
	EModel1Vt82c686bUhci,
	EModel1PciOhci,
	EModel1NecXhci,
	EModel1None,
	EModel1Qusb1,
	EModel1Qusb2,
	EModel1QemuXhci
};

enum EName4
{
	EName4SpaprPciHostBridge,
	EName4PciBridge,
	EName4I82801b11Bridge,
	EName4PciePciBridge,
	EName4Ioh3420,
	EName4PcieRootPort,
	EName4X3130Upstream,
	EName4Xio3130Downstream,
	EName4Pxb,
	EName4PxbPcie
};

enum EModel2
{
	EModel2PciRoot,
	EModel2PcieRoot
};

enum EModel3
{
	EModel3PciBridge,
	EModel3DmiToPciBridge,
	EModel3PcieToPciBridge,
	EModel3PcieRootPort,
	EModel3PcieSwitchUpstreamPort,
	EModel3PcieSwitchDownstreamPort,
	EModel3PciExpanderBus,
	EModel3PcieExpanderBus
};

enum EType7
{
	EType7Path,
	EType7Handle,
	EType7Loop,
	EType7Nbd
};

enum EAccessmode
{
	EAccessmodePassthrough,
	EAccessmodeMapped,
	EAccessmodeSquash
};

enum EMacTableManager
{
	EMacTableManagerKernel,
	EMacTableManagerLibvirt
};

enum EState
{
	EStateUp,
	EStateDown
};

enum EName5
{
	EName5Kvm,
	EName5Vfio
};

enum EName6
{
	EName6Qemu,
	EName6Vhost
};

enum ETxmode
{
	ETxmodeIothread,
	ETxmodeTimer
};

enum ENativeMode
{
	ENativeModeTagged,
	ENativeModeUntagged
};

enum EType8
{
	EType8Unix
};

enum EMode6
{
	EMode6Server,
	EMode6Client
};

enum EType9
{
	EType9Mcast,
	EType9Client
};

enum EType10
{
	EType10Tablet,
	EType10Mouse,
	EType10Keyboard
};

enum EBus1
{
	EBus1Ps2,
	EBus1Usb,
	EBus1Xen
};

enum EModel4
{
	EModel4Sb16,
	EModel4Es1370,
	EModel4Pcspk,
	EModel4Ac97,
	EModel4Ich6,
	EModel4Ich9,
	EModel4Usb
};

enum EType11
{
	EType11Duplex,
	EType11Micro
};

enum EName7
{
	EName7Kvm,
	EName7Vfio,
	EName7Xen
};

enum ESgio1
{
	ESgio1Filtered,
	ESgio1Unfiltered
};

enum EProtocol1
{
	EProtocol1Adapter
};

enum EProtocol2
{
	EProtocol2Iscsi
};

enum ESharePolicy
{
	ESharePolicyAllowExclusive,
	ESharePolicyForceShared,
	ESharePolicyIgnore
};

enum EConnected
{
	EConnectedKeep
};

enum EConnected1
{
	EConnected1Fail,
	EConnected1Disconnect,
	EConnected1Keep
};

enum EDefaultMode
{
	EDefaultModeAny,
	EDefaultModeSecure,
	EDefaultModeInsecure
};

enum EName8
{
	EName8Main,
	EName8Display,
	EName8Inputs,
	EName8Cursor,
	EName8Playback,
	EName8Record,
	EName8Smartcard,
	EName8Usbredir
};

enum EMode7
{
	EMode7Any,
	EMode7Secure,
	EMode7Insecure
};

enum ECompression
{
	ECompressionAutoGlz,
	ECompressionAutoLz,
	ECompressionQuic,
	ECompressionGlz,
	ECompressionLz,
	ECompressionOff
};

enum ECompression1
{
	ECompression1Auto,
	ECompression1Never,
	ECompression1Always
};

enum ECompression2
{
	ECompression2Auto,
	ECompression2Never,
	ECompression2Always
};

enum EMode8
{
	EMode8Filter,
	EMode8All,
	EMode8Off
};

enum EMode9
{
	EMode9Server,
	EMode9Client
};

enum EType12
{
	EType12Vga,
	EType12Cirrus,
	EType12Vmvga,
	EType12Xen,
	EType12Vbox
};

enum EQemucdevSrcTypeChoice
{
	EQemucdevSrcTypeChoiceDev,
	EQemucdevSrcTypeChoiceFile,
	EQemucdevSrcTypeChoicePipe,
	EQemucdevSrcTypeChoiceUnix,
	EQemucdevSrcTypeChoiceTcp,
	EQemucdevSrcTypeChoiceUdp,
	EQemucdevSrcTypeChoiceNull,
	EQemucdevSrcTypeChoiceStdio,
	EQemucdevSrcTypeChoiceVc,
	EQemucdevSrcTypeChoicePty,
	EQemucdevSrcTypeChoiceSpicevmc,
	EQemucdevSrcTypeChoiceSpiceport,
	EQemucdevSrcTypeChoiceNmdm
};

enum EType13
{
	EType13Raw,
	EType13Telnet,
	EType13Telnets,
	EType13Tls
};

enum EType14
{
	EType14Xen,
	EType14Serial,
	EType14Uml,
	EType14Virtio,
	EType14Lxc,
	EType14Openvz,
	EType14Sclp,
	EType14Sclplm
};

enum EType15
{
	EType15IsaSerial,
	EType15UsbSerial
};

enum EName9
{
	EName9IsaSerial,
	EName9UsbSerial,
	EName9PciSerial,
	EName9SpaprVty,
	EName9Pl011,
	EName9Sclpconsole,
	EName9Sclplmconsole
};

enum EType16
{
	EType16Usb
};

enum EBus2
{
	EBus2Usb
};

enum EModel5
{
	EModel5Virtio
};

enum EModel6
{
	EModel6TpmTis,
	EModel6TpmCrb
};

enum EVersion
{
	EVersion12,
	EVersion20
};

enum EModel7
{
	EModel7Dimm
};

enum EModel8
{
	EModel8I6300esb,
	EModel8Ib700
};

enum EAction
{
	EActionReset,
	EActionShutdown,
	EActionPoweroff,
	EActionPause,
	EActionNone,
	EActionDump
};

enum EModel9
{
	EModel9Virtio,
	EModel9Xen,
	EModel9None
};

enum EModel10
{
	EModel10Isa,
	EModel10Pseries,
	EModel10Hyperv
};

enum EName10
{
	EName10Aes,
	EName10Dea
};


} // namespace Xml
} // namespace Domain
} // namespace Libvirt

#endif // __DOMAIN_ENUM_H__
