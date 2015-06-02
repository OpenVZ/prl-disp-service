"""
Create bootable (El-Torito) CD image.

For simplicity it's very limited and is only capable of creating simple
isolinux-based Linux boot images containing only a few files in the root
directroy.

This module compensates for the lack of genisoimage(mkisofs) on some deficient
but popular OSes.
"""
import os, sys, logging, struct, shutil, array
from datetime import datetime

__all__ = ["mkbootiso"]

_log = logging.getLogger(__name__)
debug = _log.debug

SECTOR_SIZE = 0x800
ISO9660_SIG = "CD001\x01"	# volume desriptor identifier and version

class MkisoError(Exception):
	pass

def le16(v):
	return struct.pack("<H", v)
def be16(v):
	return struct.pack(">H", v)
def le32(v):
	return struct.pack("<I", v)
def be32(v):
	return struct.pack(">I", v)
# ISO9660 also uses both-endian numbers
def bothe16(v):
	return le16(v) + be16(v)
def bothe32(v):
	return le32(v) + be32(v)

# under the assumption of no directories and few files, here's the sector
# layout:
#
#			0:15	# system area (zero-filled)
PRIM_VOLDESC		= 16	# primary volume descriptor
ELTORITO_REC		= 17	# el torito boot record
TERM_VOLDESC		= 18	# volume descriptor terminator
PATH_TABLE_LE		= 19	# little-endian path table
PATH_TABLE_BE		= 20	# big-endian path table
ROOT_DIR		= 21	# root directory
BOOT_CATALOG		= 22	# el torito boot catalog
FILES_DATA		= 23	# files data; starts with the bootloader

# path tables contain only single record for root directory
path_table_le = '\x01\0' + le32(ROOT_DIR) + le16(1) + '\0\0'
path_table_be = '\x01\0' + be32(ROOT_DIR) + be16(1) + '\0\0'

def mkdentry(name, start, size):
	# for simplicity always timestamp to now
	now = datetime.utcnow()

	if name in (".", ".."):
		flag = 2
		name = chr(len(name) - 1)
	else:
		flag = 0
	namelen = len(name)
	pad = int(not (namelen & 1))

	dentry = (
			chr(33 + pad + namelen),	# record length
			'\0',
			bothe32(start),		# 1st sector of data
			bothe32(size),		# size in bytes
			chr(now.year - 1900),
			chr(now.month),
			chr(now.day),
			chr(now.hour),
			chr(now.minute),
			chr(now.second),
			'\0',			# TZ offset
			chr(flag),
			'\0\0',			# unused
			bothe16(1),		# volume sequence number
			chr(namelen),
			name,
			'\0' * pad
			)
	return ''.join(dentry)

def write_primary_voldesc(ofp, nsectors, rootdir):
	now = datetime.utcnow().strftime("%Y%m%d%H%M%S") + "00\0"
	never = '0' * 16 + '\0'
	desc = (
			'\x01',			# primary vol desc
			ISO9660_SIG,
			'\0',			# unused
			"%-32s" % "LINUX",	# system id
			"%-32s" % "BOOTISO",	# volume id
			'\0' * 8,		# unused
			bothe32(nsectors),	# total #sectors
			'\0' * 32,		# unused
			bothe16(1),		# volume set size
			bothe16(1),		# volume sequence number
			bothe16(SECTOR_SIZE),	# sector size
			bothe32(len(path_table_le)),	# path table size
			le32(PATH_TABLE_LE),	# le path table sector
			le32(0),		# 2nd le path table sector
			be32(PATH_TABLE_BE),	# be path table sector
			be32(0),		# 2nd be path table sector
			rootdir,
			' ' * 128,		# volume set id
			' ' * 128,		# publisher id
			' ' * 128,		# data preparer id
			"%-128s" % __file__,	# application id
			' ' * 37,		# copyright file id
			' ' * 37,		# abstract file id
			' ' * 37,		# bibliographical file id
			now,			# volume creation
			now,			# most recent modification
			never,			# volume expires
			now,			# volume effective
			'\x01\0'
			)

	ofp.seek(PRIM_VOLDESC * SECTOR_SIZE)
	ofp.write(''.join(desc))
	debug("wrote primary volume descriptor (sector %d)", PRIM_VOLDESC)

def write_eltorito_rec(ofp):
	etrec = (
			'\x00',			# boot record
			ISO9660_SIG,
			"EL TORITO SPECIFICATION",
			'\0' * 41,		# unused
			le32(BOOT_CATALOG),
			)
	ofp.seek(ELTORITO_REC * SECTOR_SIZE)
	ofp.write(''.join(etrec))
	debug("wrote El Torito boot record (sector %d)", ELTORITO_REC)

def write_voldesc_terminator(ofp):
	ofp.seek(TERM_VOLDESC * SECTOR_SIZE)
	ofp.write('\xff' + ISO9660_SIG)
	debug("wrote volume descriptor set terminator (sector %d)",
			TERM_VOLDESC)

def write_pathtables(ofp):
	ofp.seek(PATH_TABLE_LE * SECTOR_SIZE)
	ofp.write(path_table_le)
	ofp.seek(PATH_TABLE_BE * SECTOR_SIZE)
	ofp.write(path_table_be)
	debug("wrote path tables (sectors %d, %d)",
			PATH_TABLE_LE, PATH_TABLE_BE)

def write_rootdir(ofp, dents):
	dirdata = ''.join(mkdentry(*d) for d in dents)
	dirsize = len(dirdata) + 2 * 34		# leave room for "." and ".."
	dot = mkdentry(".", ROOT_DIR, dirsize)
	dotdot = mkdentry("..", ROOT_DIR, dirsize)

	ofp.seek(ROOT_DIR * SECTOR_SIZE)
	ofp.write(dot + dotdot + dirdata)
	debug("wrote root directory (sector %d, size %d)", ROOT_DIR, dirsize)
	return dot

bootcat_validation_ent = '\x01' + '\0' * 27 + '\xaa\x55\x55\xaa'

def write_bootcatalog(ofp, ldr_name, ldr_offset, ldr_size):
	# loader size in *virtual* sectors (512 bytes)
	ldr_vsects = (ldr_size - 1) / 512 + 1

	bootcat = (
			bootcat_validation_ent,	# validation entry
			'\x88',			# bootable
			'\0',			# no emulation
			'\0' * 2,		# load segment
			'\0',			# system type
			'\0',			# unused
			le16(ldr_vsects),	# sector count
			le32(ldr_offset),	# load RBA
			)			# unused

	ofp.seek(BOOT_CATALOG * SECTOR_SIZE)
	ofp.write(''.join(bootcat))
	debug("wrote boot catalog (sector %d)", BOOT_CATALOG)

def _add_file(ofp, fname, filt):
	name = os.path.basename(fname).upper() + ";1"
	pos = ofp.tell()

	ifp = open(fname, "rb")
	filt(ifp, ofp)
	ifp.close()

	size = ofp.tell() - pos
	padlen = SECTOR_SIZE - 1 - (size - 1) % SECTOR_SIZE
	ofp.write('\0' * padlen)

	start = pos / SECTOR_SIZE
	debug("added %r as %r (sector %d, size %d)", fname, name, start, size)
	return (name, start, size)

def add_file(ofp, fname):
	return _add_file(ofp, fname, shutil.copyfileobj)

def patch_bootldr(ifp, ofp):
	"""
	patch boot information table into loader (see mkisofs(1))
	"""
	bldata = ifp.read()
	bllen = len(bldata)
	bootldr = array.array('I')
	padlen = bootldr.itemsize - 1 - (bllen - 1) % bootldr.itemsize
	bootldr.fromstring(bldata + '\0' * padlen)
	bootldr[2] = PRIM_VOLDESC
	bootldr[3] = ofp.tell() / SECTOR_SIZE
	bootldr[4] = bllen
	bootldr[5] = sum(bootldr[16:]) & 0xffffffff
	bootldr.tofile(ofp)

def add_bootldr(ofp, fname):
	return _add_file(ofp, fname, patch_bootldr)

def mkbootiso(imgpath, bootldr, entries):
	debug("creating El-Torito boot CD %r with %r bootloader", imgpath,
			bootldr)
	ofp = open(imgpath, "wb")

	# write data first but leave room for metadata
	ofp.write('\0' * (FILES_DATA * SECTOR_SIZE))

	bootldr_dent = add_bootldr(ofp, bootldr)
	dents = [add_file(ofp, f) for f in entries]

	nsectors = ofp.tell() / SECTOR_SIZE

	rootdir = write_rootdir(ofp, [bootldr_dent] + dents)
	write_primary_voldesc(ofp, nsectors, rootdir)
	write_eltorito_rec(ofp)
	write_voldesc_terminator(ofp)
	write_pathtables(ofp)
	write_bootcatalog(ofp, *bootldr_dent)

if __name__ == "__main__":
	logging.basicConfig(level = logging.DEBUG,
			format = "%(asctime)s %(levelname)s %(message)s")

	mkbootiso(sys.argv[1], sys.argv[2], sys.argv[3:])
