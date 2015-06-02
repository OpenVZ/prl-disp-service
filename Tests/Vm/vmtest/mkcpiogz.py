"""
Generate gzipped cpio archive from a given directory.

The archive format is "new ASCII without CRC".

Such archives are used e.g. for Linux initramfs.
"""
import os, sys, logging, optparse, gzip, stat, shutil, posixpath

_log = logging.getLogger(__name__)

class CpioError(Exception):
	pass

def add_header(arcfp, name, mode, size):
	"""
	New ASCII w/o CRC cpio header
	(see http://people.freebsd.org/~kientzle/libarchive/man/cpio.5.txt)

	Most fields are ignored; only the important ones are non-zero.
	"""
	# name is nul-terminated and nul-padded so that the header (110 bytes)
	# plus name are 4 bytes aligned
	name_len = len(name) + 1
	pad_len = 4 - (len(name) + 110) % 4
	hdr = (
			"070701",	# magic[6]
			"0" * 8,	# ino[8]
			"%08x" % mode,	# mode[8]
			"0" * 32,	# uid[8], gid[8], nlink[8], mtime[8]
			"%08x" % size,	# filesize[8]
			"0" * 32,	# devmajor[8], devminor[8], rdevmajor[8], rdevminor[8]
			"%08x" % name_len,	# namesize[8]
			"0" * 8,	# check[8]
			name,
			"\0" * pad_len
		)
	arcfp.write(''.join(hdr))

def add_file(arcfp, name, mode, size, arc_name):
	_log.debug("adding file %r as %r", name, arc_name)
	add_header(arcfp, arc_name, stat.S_IFREG | mode, size)
	pad_len = 3 - (size - 1) % 4
	fp = open(name, "rb")
	shutil.copyfileobj(fp, arcfp)
	fp.close()
	arcfp.write('\0' * pad_len)

def add_dir(arcfp, name, mode, arc_name):
	_log.debug("adding directory %r as %r", name, arc_name)
	# root dir needs no header
	if arc_name:
		add_header(arcfp, arc_name, stat.S_IFDIR | mode, 0)

	for fname in os.listdir(name):
		add_entry(arcfp,
				os.path.join(name, fname),
				posixpath.join(arc_name, fname))

def add_entry(arcfp, name, arc_name):
	st = os.stat(name)
	mode = st.st_mode
	# ignore all but regular files and dirs
	if stat.S_ISREG(mode):
		add_file(arcfp, name, mode, st.st_size, arc_name)
	elif stat.S_ISDIR(mode):
		add_dir(arcfp, name, mode, arc_name)
	else:
		raise CpioError("%r: unknown file mode %o" % (name, mode))

def add_root(arcfp, root):
	elems = root.split(posixpath.sep)
	# the root itself is created by actual dir
	for i in range(1, len(elems)):
		cur = posixpath.join(*elems[:i])
		add_header(arcfp, cur, stat.S_IFDIR | 0755, 0)

def mkcpiogz(topdir, archive, root):
	_log.debug("creating archive %r", archive)
	arcfp = gzip.open(archive, "wb")

	root = posixpath.normpath(root)
	if root == ".":
		root = ""
	add_root(arcfp, root)
	add_entry(arcfp, topdir, root)
	add_header(arcfp, "TRAILER!!!", 0, 0)
	arcfp.close()

def parse_args():
	usage = "usage: %prog [options] TOPDIR ARCHIVE"
	parser = optparse.OptionParser(usage = usage, description = __doc__)

	parser.add_option("-r", "--root",
			default = "",
			help = "root of the tree in the archive")

	opts, args = parser.parse_args()
	if len(args) != 2:
		parser.error("wrong arguments count")

	return args[0], args[1], opts.root


if __name__ == "__main__":
	logging.basicConfig(level = logging.DEBUG,
			format = "%(asctime)s %(levelname)s %(message)s")

	mkcpiogz(*parse_args())
