//////////////////////////////////////////////////////////////////////////
///
/// @file ApiDevNums.h
///
/// @brief Description of the structures and constants
///
/// @author myakhin
///
/// Description of the structures and constants required
/// for the API request exchange between Application and Monitor
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH. All rights reserved.
///
/// This file is licensed for use in the Linux Kernel under the
/// terms of the GNU General Public License version 2.
///
///
//////////////////////////////////////////////////////////////////////////

#ifndef __API_DEV_NUMS_H__
#define __API_DEV_NUMS_H__

/**
 * Memory accounting constants and macro
 */

/**
 * Booting info schema
 */

/**
 * Boot devices mask looks like specified below:
 *
 * 0000 0000
 * |||| ||||
 * |||| |||+-------- Floppy Disk (on/off: XOR 0x1)
 * |||| ||+--------- CD/DVD-ROM (on/off XOR 0x2)
 * |||| |+---------- Hard Disk (on/off XOR 0x4)
 * |||| +----------- Network (on/off XOR 0x8)
 * |||+------------- USB-device (on/off XOR 0x10)
 * ||+-------------- reserved for future use
 * |+--------------- reserved for future use
 * +---------------- reserved for future use
 *
 * In order to check the state of device in the boot mask,
 * or toggle the state of the device, use macro definitions
 * stated in this header below.
 *
 * Boot sequense is a string, right justified with zeroes,
 * which contains the corresponding sequential number in the bit position
 * of the bootable device (if device is enabled) or zero (if device
 * is disabled in the boot sequence), for example:
 *
 * For the boot mask like "0000 0111", which stands for:
 *
 * Floppy - enabled,
 * CD/DVD-ROM - enabled,
 * HDD - enabled,
 * Network - disabled,
 * USB - disabled
 *
 * The boot sequence mask may look like this:
 *
 * "0000 0123", which denotes the following boot order:
 * HDD, CD/DVD-ROM, Floppy
 *
 * or
 * "0000 0321", which denotes the following boot order:
 * Floppy, CD/DVD-ROM, HDD
 *
 * or
 * "0000 0213", which denotes the following boot order:
 * CD/DVD-ROM, HDD, Floppy
 */

#define MAX_FLOPPY_COUNT			2
#define MAX_IDE_COUNT				4
#define MAX_SERIAL_COUNT			4


#define FLOPPY_BOOT_MASK				0x1
#define CDROM_BOOT_MASK					0x2
#define HARDDISK_BOOT_MASK				0x4
#define NETWORK_BOOT_MASK				0x8
#define USBDEVICE_BOOT_MASK				0x10

#define IS_FLOPPY_BOOT_ENABLED(x)		( (x) & FLOPPY_BOOT_MASK ?1:0 )
#define IS_CDROM_BOOT_ENABLED(x)		( (x) & CDROM_BOOT_MASK ?1:0 )
#define IS_HARDDISK_BOOT_ENABLED(x)		( (x) & HARDDISK_BOOT_MASK ?1:0 )
#define IS_NETWORK_BOOT_ENABLED(x)		( (x) & NETWORK_BOOT_MASK ?1:0 )
#define IS_USBDEVICE_BOOT_ENABLED(x)	( (x) & USBDEVICE_BOOT_MASK ?1:0 )

#define SET_FLOPPY_BOOT_ENABLED(x)		(x) | FLOPPY_BOOT_MASK;
#define SET_CDROM_BOOT_ENABLED(x)		(x) | CDROM_BOOT_MASK;
#define SET_HARDDISK_BOOT_ENABLED(x)	(x) | HARDDISK_BOOT_MASK;
#define SET_NETWORK_BOOT_ENABLED(x)		(x) | NETWORK_BOOT_MASK;
#define SET_USBDEVICE_BOOT_ENABLED(x)	(x) | USBDEVICE_BOOT_MASK;

#define SET_FLOPPY_BOOT_DISABLED(x)		(x) & ~FLOPPY_BOOT_MASK;
#define SET_CDROM_BOOT_DISABLED(x)		(x) & ~CDROM_BOOT_MASK;
#define SET_HARDDISK_BOOT_DISABLED(x)	(x) & ~HARDDISK_BOOT_MASK;
#define SET_NETWORK_BOOT_DISABLED(x)	(x) & ~NETWORK_BOOT_MASK;
#define SET_USBDEVICE_BOOT_DISABLED(x)	(x) & ~USBDEVICE_BOOT_MASK;



#define PRL_MAX_IDE_DEVICES_NUM IDE_DEVICES
#define PRL_MAX_SCSI_DEVICES_NUM BUSLOGIC_SCSI_MAXIMUM_TARGETS
#define PRL_MAX_SATA_DEVICES_NUM SATA_DEVICES
#define PRL_MAX_GENERIC_PCI_DEVICES VTD_MAX_DEVICE_COUNT

// This is a number of cpu allowed to be in vm config
// (used in GUI validation routines and in vm config validator)
#define PRL_MAX_CPU_COUNT				256

#define PRL_MAX_DEVICE_INDEX				1024


// Types of IDE drives (CD, HDD)
enum
{
	IDE_DEVICE_0 = 0, // should be zero, very strictly
	IDE_DEVICE_1,
	IDE_DEVICE_2,
	IDE_DEVICE_3,
	IDE_DEVICE_LAST
};

// IDE devices count
#define IDE_DEVICES IDE_DEVICE_LAST


/**
* Define number of target devices supported by emulated scsi controller
*/
#define BUSLOGIC_SCSI_MAXIMUM_TARGETS (16)


/*
* maximum SATA port per HBA
*/
#define AHCI_PORT_COUNT	(32)

/*
* Maximum numbers of SATA devices can be connected to VM,
* this value is less or equal to AHCI_PORT_COUNT.
* While AHCI_PORT_COUNT is describing a maximum device count can be connected
* to single AHCI controller, this value contain maximum number of SATA devices
* used at emulation. At initial version of AHCI controller, this value is 6,
* as it defined for ICH8 built-in AHCI controller. But it can be expanded
* if needed
*/
#define SATA_DEVICES (6)

// Parallel ports count
#define GUEST_LPT_MAX_COUNT (3)

// FDD devices count
#define FDD_DEVICES (2)

// Number of Lpt ports
#define LPT_DEVICES				GUEST_LPT_MAX_COUNT

// Number of usb printers
#define USB_PRINTER_DEVICES (8)

// Total number of printers
#define PRINTER_DEVICES			(LPT_DEVICES+USB_PRINTER_DEVICES)

// Number of Serial-ports
#define SERIAL_DEVICES              0x04

/*
* Maximum number of network devices
* Note: this constant can't just be changed since some flags is
* binded to this value
*/
#define MAX_NET_DEVICES (16)

/**
 * Maximum number of hooked devices
 */
#define VTD_MAX_DEVICE_COUNT (10)

#endif	// __API_DEV_NUMS_H__
