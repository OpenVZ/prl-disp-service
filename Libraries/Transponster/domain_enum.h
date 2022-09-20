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
	EModeHostPassthrough,
	EModeMaximum
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

enum EVirOnOff
{
	EVirOnOffOn,
	EVirOnOffOff
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

enum EVirYesNo
{
	EVirYesNoYes,
	EVirYesNoNo
};

enum EAssociativity
{
	EAssociativityNone,
	EAssociativityDirect,
	EAssociativityFull
};

enum EPolicy1
{
	EPolicy1None,
	EPolicy1Writeback,
	EPolicy1Writethrough
};

enum EType1
{
	EType1Access,
	EType1Read,
	EType1Write
};

enum EType2
{
	EType2Access,
	EType2Read,
	EType2Write
};

enum ELevel
{
	ELevel1,
	ELevel2,
	ELevel3
};

enum EMode1
{
	EMode1Emulate,
	EMode1Passthrough,
	EMode1Disable
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

enum EType3
{
	EType3Xen,
	EType3Linux
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

enum EType4
{
	EType4Rom,
	EType4Pflash
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

enum EMode2
{
	EMode2Emulate,
	EMode2Host,
	EMode2Sysinfo
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

enum EMode3
{
	EMode3Auto,
	EMode3Native,
	EMode3Emulate,
	EMode3Paravirt,
	EMode3Smpsafe
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

enum EType5
{
	EType5File,
	EType5Anonymous,
	EType5Memfd
};

enum EMode4
{
	EMode4Shared,
	EMode4Private
};

enum EMode5
{
	EMode5Immediate,
	EMode5Ondemand
};

enum EPlacement
{
	EPlacementStatic,
	EPlacementAuto
};

enum EScheduler
{
	ESchedulerBatch,
	ESchedulerIdle
};

enum EScheduler1
{
	EScheduler1Fifo,
	EScheduler1Rr
};

enum EType6
{
	EType6Both,
	EType6Code,
	EType6Data
};

enum EMode6
{
	EMode6Strict,
	EMode6Preferred,
	EMode6Interleave
};

enum EMode7
{
	EMode7Strict,
	EMode7Preferred,
	EMode7Interleave
};

enum EPolicy2
{
	EPolicy2Default,
	EPolicy2Allow,
	EPolicy2Deny
};

enum EDriver
{
	EDriverQemu,
	EDriverKvm
};

enum EResizing
{
	EResizingEnabled,
	EResizingDisabled,
	EResizingRequired
};

enum EUnknown
{
	EUnknownIgnore,
	EUnknownFault
};

enum EValue
{
	EValueBroken,
	EValueWorkaround,
	EValueFixed
};

enum EValue1
{
	EValue1Broken,
	EValue1Workaround,
	EValue1Fixed
};

enum EValue2
{
	EValue2Broken,
	EValue2Workaround,
	EValue2FixedIbs,
	EValue2FixedCcd,
	EValue2FixedNa
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
	ESgioUnfiltered,
	ESgioVirtioNonTransitional
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

enum EFormat1
{
	EFormat1Default,
	EFormat1Qcow,
	EFormat1Luks
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

enum EMode8
{
	EMode8Host,
	EMode8Direct
};

enum EType7
{
	EType7Aio
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
	EStorageFormatVhd,
	EStorageFormatPloop,
	EStorageFormatLuks
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

enum EDetectZeroes
{
	EDetectZeroesOff,
	EDetectZeroesOn,
	EDetectZeroesUnmap
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

enum EType8
{
	EType8Ceph,
	EType8Iscsi
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

enum ETrans
{
	ETransAuto,
	ETransNone,
	ETransLba
};

enum EType9
{
	EType9Fdc,
	EType9Ide,
	EType9Sata,
	EType9Ccid
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
	EModelVirtioTransitional,
	EModelVirtioNonTransitional,
	EModelNcr53c90,
	EModelDc390,
	EModelAm53c974,
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

enum EModel2
{
	EModel2Piix3,
	EModel2Piix4,
	EModel2Ich6
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

enum EModel3
{
	EModel3PciRoot,
	EModel3PcieRoot
};

enum EModel4
{
	EModel4PciBridge,
	EModel4DmiToPciBridge,
	EModel4PcieToPciBridge,
	EModel4PcieRootPort,
	EModel4PcieSwitchUpstreamPort,
	EModel4PcieSwitchDownstreamPort,
	EModel4PciExpanderBus,
	EModel4PcieExpanderBus
};

enum EModel5
{
	EModel5Virtio,
	EModel5VirtioTransitional,
	EModel5VirtioNonTransitional
};

enum EType10
{
	EType10Path,
	EType10Handle,
	EType10Loop,
	EType10Nbd,
	EType10Ploop
};

enum EAccessmode
{
	EAccessmodePassthrough,
	EAccessmodeMapped,
	EAccessmodeSquash
};

enum EMultidevs
{
	EMultidevsDefault,
	EMultidevsRemap,
	EMultidevsForbid,
	EMultidevsWarn
};

enum EModel6
{
	EModel6Virtio,
	EModel6VirtioTransitional,
	EModel6VirtioNonTransitional
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

enum EType11
{
	EType11Unix
};

enum EMode9
{
	EMode9Server,
	EMode9Client
};

enum EType12
{
	EType12Mcast,
	EType12Client
};

enum EType13
{
	EType13Tablet,
	EType13Mouse,
	EType13Keyboard
};

enum EBus1
{
	EBus1Ps2,
	EBus1Usb,
	EBus1Xen
};

enum EModel7
{
	EModel7Virtio,
	EModel7VirtioTransitional,
	EModel7VirtioNonTransitional
};

enum EModel8
{
	EModel8Sb16,
	EModel8Es1370,
	EModel8Pcspk,
	EModel8Ac97,
	EModel8Ich6,
	EModel8Ich9,
	EModel8Usb
};

enum EType14
{
	EType14Duplex,
	EType14Micro
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

enum EMode10
{
	EMode10Any,
	EMode10Secure,
	EMode10Insecure
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

enum EMode11
{
	EMode11Filter,
	EMode11All,
	EMode11Off
};

enum EMode12
{
	EMode12Server,
	EMode12Client
};

enum EName9
{
	EName9Qemu,
	EName9Vhostuser
};

enum EVgaconf
{
	EVgaconfIo,
	EVgaconfOn,
	EVgaconfOff
};

enum EType15
{
	EType15Vga,
	EType15Cirrus,
	EType15Vmvga,
	EType15Xen,
	EType15Vbox,
	EType15Virtio,
	EType15Vzct,
	EType15Gop,
	EType15None,
	EType15Bochs,
	EType15Ramfb
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

enum EType16
{
	EType16Raw,
	EType16Telnet,
	EType16Telnets,
	EType16Tls
};

enum EType17
{
	EType17Xen,
	EType17Serial,
	EType17Uml,
	EType17Virtio,
	EType17Lxc,
	EType17Openvz,
	EType17Sclp,
	EType17Sclplm
};

enum EType18
{
	EType18IsaSerial,
	EType18UsbSerial
};

enum EName10
{
	EName10IsaSerial,
	EName10UsbSerial,
	EName10PciSerial,
	EName10SpaprVty,
	EName10Pl011,
	EName10Sclpconsole,
	EName10Sclplmconsole
};

enum EState1
{
	EState1Connected,
	EState1Disconnected
};

enum EModel9
{
	EModel9Virtio,
	EModel9VirtioTransitional,
	EModel9VirtioNonTransitional
};

enum EModel10
{
	EModel10TpmTis,
	EModel10TpmCrb
};

enum EVersion
{
	EVersion12,
	EVersion20
};

enum EModel11
{
	EModel11Dimm
};

enum EAccess
{
	EAccessShared,
	EAccessPrivate
};

enum EModel12
{
	EModel12I6300esb,
	EModel12Ib700
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

enum EModel13
{
	EModel13Virtio,
	EModel13VirtioTransitional,
	EModel13VirtioNonTransitional,
	EModel13Xen,
	EModel13None
};

enum EModel14
{
	EModel14Isa,
	EModel14Pseries,
	EModel14Hyperv
};

enum EName11
{
	EName11Aes,
	EName11Dea
};


} // namespace Xml
} // namespace Domain
} // namespace Libvirt

#endif // __DOMAIN_ENUM_H__
