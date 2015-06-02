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
	ETypeParallels,
	ETypeBhyve
};

enum EMode
{
	EModeCustom,
	EModeHostModel,
	EModeHostPassthrough
};

enum EFallback
{
	EFallbackAllow,
	EFallbackForbid
};

enum EMatch
{
	EMatchMinimum,
	EMatchExact,
	EMatchStrict
};

enum EPolicy
{
	EPolicyForce,
	EPolicyRequire,
	EPolicyOptional,
	EPolicyDisable,
	EPolicyForbid
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

enum EDev
{
	EDevHd,
	EDevFd,
	EDevCdrom,
	EDevNetwork
};

enum EEnable
{
	EEnableYes,
	EEnableNo
};

enum EMode1
{
	EMode1Emulate,
	EMode1Host,
	EMode1Sysinfo
};

enum EUseserial
{
	EUseserialYes,
	EUseserialNo
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

enum EPresent
{
	EPresentYes,
	EPresentNo
};

enum EDumpCore
{
	EDumpCoreOn,
	EDumpCoreOff
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

enum EEoi
{
	EEoiOn,
	EEoiOff
};

enum EState
{
	EStateOn,
	EStateOff
};

enum EPolicy1
{
	EPolicy1Default,
	EPolicy1Allow,
	EPolicy1Deny
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

enum EEnabled
{
	EEnabledYes,
	EEnabledNo
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

enum ERawio
{
	ERawioYes,
	ERawioNo
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

enum EType2
{
	EType2Aio
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

enum EIoeventfd
{
	EIoeventfdOn,
	EIoeventfdOff
};

enum EEventIdx
{
	EEventIdxOn,
	EEventIdxOff
};

enum ECopyOnRead
{
	ECopyOnReadOn,
	ECopyOnReadOff
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

enum EType3
{
	EType3Ceph,
	EType3Iscsi
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

enum ERemovable
{
	ERemovableOn,
	ERemovableOff
};

enum EFormat
{
	EFormatDefault,
	EFormatQcow
};

enum EType4
{
	EType4Passphrase
};

enum EMultifunction
{
	EMultifunctionOn,
	EMultifunctionOff
};

enum ETrans
{
	ETransAuto,
	ETransNone,
	ETransLba
};

enum EType5
{
	EType5Fdc,
	EType5Ide,
	EType5Sata,
	EType5Ccid
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
	EModelLsisas1078
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
	EModel1None
};

enum EModel2
{
	EModel2PciRoot,
	EModel2PcieRoot
};

enum EModel3
{
	EModel3PciBridge,
	EModel3DmiToPciBridge
};

enum EType6
{
	EType6Path,
	EType6Handle,
	EType6Loop,
	EType6Nbd
};

enum EAccessmode
{
	EAccessmodePassthrough,
	EAccessmodeMapped,
	EAccessmodeSquash
};

enum EName3
{
	EName3Kvm,
	EName3Vfio
};

enum EName4
{
	EName4Qemu,
	EName4Vhost
};

enum ETxmode
{
	ETxmodeIothread,
	ETxmodeTimer
};

enum EBar
{
	EBarOn,
	EBarOff
};

enum ENativeMode
{
	ENativeModeTagged,
	ENativeModeUntagged
};

enum EType7
{
	EType7Unix
};

enum EMode6
{
	EMode6Server,
	EMode6Client
};

enum EType8
{
	EType8Mcast,
	EType8Client
};

enum EManaged
{
	EManagedYes,
	EManagedNo
};

enum EMissing
{
	EMissingYes,
	EMissingNo
};

enum EType9
{
	EType9Tablet,
	EType9Mouse,
	EType9Keyboard
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
	EModel4Ich9
};

enum EType10
{
	EType10Duplex,
	EType10Micro
};

enum EManaged1
{
	EManaged1Yes,
	EManaged1No
};

enum EName5
{
	EName5Kvm,
	EName5Vfio,
	EName5Xen
};

enum ESgio1
{
	ESgio1Filtered,
	ESgio1Unfiltered
};

enum EFullscreen
{
	EFullscreenYes,
	EFullscreenNo
};

enum EAutoport
{
	EAutoportYes,
	EAutoportNo
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

enum EAutoport1
{
	EAutoport1Yes,
	EAutoport1No
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

enum EName6
{
	EName6Main,
	EName6Display,
	EName6Inputs,
	EName6Cursor,
	EName6Playback,
	EName6Record,
	EName6Smartcard,
	EName6Usbredir
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

enum ECompression3
{
	ECompression3On,
	ECompression3Off
};

enum EMode8
{
	EMode8Filter,
	EMode8All,
	EMode8Off
};

enum ECopypaste
{
	ECopypasteYes,
	ECopypasteNo
};

enum EMode9
{
	EMode9Server,
	EMode9Client
};

enum EEnable1
{
	EEnable1Yes,
	EEnable1No
};

enum EAutoport2
{
	EAutoport2Yes,
	EAutoport2No
};

enum EReplaceUser
{
	EReplaceUserYes,
	EReplaceUserNo
};

enum EMultiUser
{
	EMultiUserYes,
	EMultiUserNo
};

enum EFullscreen1
{
	EFullscreen1Yes,
	EFullscreen1No
};

enum EType11
{
	EType11Vga,
	EType11Cirrus,
	EType11Vmvga,
	EType11Xen,
	EType11Vbox
};

enum EPrimary
{
	EPrimaryYes,
	EPrimaryNo
};

enum EAccel3d
{
	EAccel3dYes,
	EAccel3dNo
};

enum EAccel2d
{
	EAccel2dYes,
	EAccel2dNo
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

enum EType12
{
	EType12Raw,
	EType12Telnet,
	EType12Telnets,
	EType12Tls
};

enum EType13
{
	EType13Xen,
	EType13Serial,
	EType13Uml,
	EType13Virtio,
	EType13Lxc,
	EType13Openvz,
	EType13Sclp,
	EType13Sclplm
};

enum EType14
{
	EType14IsaSerial,
	EType14UsbSerial
};

enum EType15
{
	EType15Usb
};

enum EBus2
{
	EBus2Usb
};

enum EAllow
{
	EAllowYes,
	EAllowNo
};

enum EModel5
{
	EModel5Virtio
};

enum EChoice988
{
	EChoice988DevRandom,
	EChoice988DevHwrng
};

enum EModel6
{
	EModel6TpmTis
};

enum EModel7
{
	EModel7I6300esb,
	EModel7Ib700
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

enum EModel8
{
	EModel8Virtio,
	EModel8Xen,
	EModel8None
};

enum ERelabel
{
	ERelabelYes,
	ERelabelNo
};


} // namespace Xml
} // namespace Domain
} // namespace Libvirt

#endif // __DOMAIN_ENUM_H__
