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

enum EMode2
{
	EMode2Emulate,
	EMode2Passthrough
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

enum EFirmware
{
	EFirmwareBios,
	EFirmwareEfi
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

enum EName
{
	ENameEnrolledKeys,
	ENameSecureBoot
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

enum EMode3
{
	EMode3Host,
	EMode3Direct
};

enum EDev
{
	EDevHd,
	EDevFd,
	EDevCdrom,
	EDevNetwork
};

enum EMode4
{
	EMode4Emulate,
	EMode4Host,
	EMode4Sysinfo
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

enum EName1
{
	EName1Platform,
	EName1Rtc
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

enum EMode5
{
	EMode5Auto,
	EMode5Native,
	EMode5Emulate,
	EMode5Paravirt,
	EMode5Smpsafe
};

enum EName2
{
	EName2Hpet,
	EName2Pit
};

enum EName3
{
	EName3Kvmclock,
	EName3Hypervclock
};

enum EType5
{
	EType5File,
	EType5Anonymous,
	EType5Memfd
};

enum EMode6
{
	EMode6Shared,
	EMode6Private
};

enum EMode7
{
	EMode7Immediate,
	EMode7Ondemand
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

enum EMode8
{
	EMode8Strict,
	EMode8Preferred,
	EMode8Interleave
};

enum EMode9
{
	EMode9Strict,
	EMode9Preferred,
	EMode9Interleave
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

enum EName4
{
	EName4Cmt,
	EName4Mbmt,
	EName4Mbml,
	EName4CpuCycles,
	EName4Instructions,
	EName4CacheReferences,
	EName4CacheMisses,
	EName4BranchInstructions,
	EName4BranchMisses,
	EName4BusCycles,
	EName4StalledCyclesFrontend,
	EName4StalledCyclesBackend,
	EName4RefCpuCycles,
	EName4CpuClock,
	EName4TaskClock,
	EName4PageFaults,
	EName4ContextSwitches,
	EName4CpuMigrations,
	EName4PageFaultsMin,
	EName4PageFaultsMaj,
	EName4AlignmentFaults,
	EName4EmulationFaults
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

enum EName5
{
	EName5SpaprPciHostBridge,
	EName5PciBridge,
	EName5I82801b11Bridge,
	EName5PciePciBridge,
	EName5Ioh3420,
	EName5PcieRootPort,
	EName5X3130Upstream,
	EName5Xio3130Downstream,
	EName5Pxb,
	EName5PxbPcie
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

enum EName6
{
	EName6Kvm,
	EName6Vfio
};

enum EName7
{
	EName7Qemu,
	EName7Vhost
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

enum EMode10
{
	EMode10Server,
	EMode10Client
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

enum EName8
{
	EName8Kvm,
	EName8Vfio,
	EName8Xen
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

enum EName9
{
	EName9Main,
	EName9Display,
	EName9Inputs,
	EName9Cursor,
	EName9Playback,
	EName9Record,
	EName9Smartcard,
	EName9Usbredir
};

enum EMode11
{
	EMode11Any,
	EMode11Secure,
	EMode11Insecure
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

enum EMode12
{
	EMode12Filter,
	EMode12All,
	EMode12Off
};

enum EMode13
{
	EMode13Server,
	EMode13Client
};

enum EName10
{
	EName10Qemu,
	EName10Vhostuser
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

enum EName11
{
	EName11IsaSerial,
	EName11UsbSerial,
	EName11PciSerial,
	EName11SpaprVty,
	EName11Pl011,
	EName11Sclpconsole,
	EName11Sclplmconsole
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
	EModel12Ib700,
	EModel12Diag288,
	EModel12Itco
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

enum EName12
{
	EName12Aes,
	EName12Dea
};


} // namespace Xml
} // namespace Domain
} // namespace Libvirt

#endif // __DOMAIN_ENUM_H__
