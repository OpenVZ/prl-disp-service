"""
Stuff to prepare a bootable Linux ISO (based on the installation image of a
recent Fedora release) containing the tests
"""

import os, sys, time, subprocess, gzip
from urllib2 import urlopen
from logging import debug

import mkcpiogz
from mkbootiso import mkbootiso

__all__ = ["mktestiso"]

distro_url = "http://mir.sw.ru/pub/Linux/mirror/fedora/releases/17/Fedora/x86_64/os/isolinux/"

here = os.path.dirname(__file__)

def fetch_img(filename, local_prefix = ""):
	outfname = os.path.join(local_prefix, filename)
	if os.path.exists(outfname):
		debug("Reusing existing %r", outfname)
		return outfname
	url = distro_url + filename
	debug("Fetching %r", url)

	urlobj = urlopen(url)
	outfile = open(outfname, "wb")

	size = int(urlobj.info().get('content-length', '0'))
	# report progress every this many seconds
	progress_interval = 3
	report_at = time.time() + progress_interval
	while True:
		buf = urlobj.read(1 << 20)	# 1MB buffer
		if not buf:
			break
		outfile.write(buf)

		cur_time = time.time()
		if cur_time > report_at:
			actual = outfile.tell()
			msg = "downloaded %d bytes" % actual
			if size:
				msg += " (%d%%)" % (100 * actual / size)
			debug(msg)
			report_at = cur_time + progress_interval

	actual = outfile.tell()
	outfile.close()
	urlobj.close()
	debug("finished downloading %r (%d bytes)", url, actual)
	return outfile.name

def mktestimg(imgpath, testroot):
	debug("creating archive %r", imgpath)
	imgfp = gzip.open(imgpath, "wb")

	init_sh = os.path.join(here, "init.sh")
	mkcpiogz.add_file(imgfp, init_sh, 0755, os.path.getsize(init_sh),
			"init")
	mkcpiogz.add_dir(imgfp, testroot, 0755, "tests")
	mkcpiogz.add_header(imgfp, "TRAILER!!!", 0, 0)
	imgfp.close()


def mktestiso(isopath, testroot, imgroot = ""):
	isolinux_cfg = os.path.join(here, "isolinux.cfg")

	isolinux_bin = fetch_img("isolinux.bin", imgroot)
	vmlinuz = fetch_img("vmlinuz", imgroot)
	initrd = fetch_img("initrd.img", imgroot)

	testrd = os.path.join(imgroot, "testrd.img")
	mktestimg(testrd, testroot)

	debug("Creating test ISO %r", isopath)
	mkbootiso(isopath, isolinux_bin,
			[isolinux_cfg, vmlinuz, initrd, testrd])

if __name__ == "__main__":
	import logging
	logging.basicConfig(level = logging.DEBUG,
			format = "%(asctime)s %(levelname)s %(message)s")
	mktestiso(sys.argv[1], sys.argv[2])
