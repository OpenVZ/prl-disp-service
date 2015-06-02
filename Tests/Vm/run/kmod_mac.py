#!/usr/bin/env python
"""
Wrapper for kext load/unload utilities circumventing the need for the kext to
be owned by root:wheel to be loadable.

The idea is that a non-privileged user can be allowed to run only this script,
which slightly limits the amount of damage she can make to the system.  To
achieve that, put a line in /etc/sudoers like this:

userjoe ALL= NOPASSWD: /Users/userjoe/some/path/kmod_mac.py
"""

import os, sys, shutil, tempfile, subprocess
from CoreFoundation import *

drv_dir = os.path.abspath(os.path.join(os.path.dirname(__file__),
				"../../../../z-Build/Drivers"))

def get_bundle_id(kext):
	url = CFURLCreateWithFileSystemPath(None, kext, kCFURLPOSIXPathStyle,
			True)
	bundle = CFBundleCreate(None, url)
	return CFBundleGetValueForInfoDictionaryKey(bundle,
			"CFBundleIdentifier")

def kname2path(kext):
	return os.path.join(drv_dir, kext + ".kext")

def list_loaded(kexts):
	cmd = ["kextstat", "-l"]
	for i in map(get_bundle_id, kexts):
		cmd.extend(["-b", i])
	p = subprocess.Popen(cmd, stdout = subprocess.PIPE)
	ret = []
	for l in p.stdout:
		ret.append(l.split()[5])
	if p.wait():
		raise subprocess.CalledProcessError(p.returncode, "kextstat")
	return ret

def do_load(kexts, dryrun = False):
	cmd = ["kextutil", "-v", "0", "--"] + kexts
	if dryrun:
		cmd.insert(1, "-n")
	return not subprocess.call(cmd)

def do_unload(kextids):
	cmd = ["kextunload"]
	for i in reversed(kextids):
		cmd.extend(["-b", i])
	return not subprocess.call(cmd)

def unload(kexts):
	do_unload(map(get_bundle_id, kexts))

def copy_for_load(kext_in, kdir):
	kext = os.path.join(kdir, os.path.basename(kext_in))
	shutil.copytree(kext_in, kext)
	return kext

def load(kexts_in):
	if not do_load(kexts_in, dryrun = True):
		sys.exit(1)

	kdir = tempfile.mkdtemp()
	try:
		kexts = [copy_for_load(ki, kdir) for ki in kexts_in]
		loaded = list_loaded(kexts)
		if loaded:
			if not do_unload(loaded):
				sys.exit(1)
		do_load(kexts)
	finally:
		shutil.rmtree(kdir)

def bail_out(msg = None):
	if msg:
		print >> sys.stderr, msg
	print >> sys.stderr, "Usage: %s load|unload module ..." % sys.argv[0]
	sys.exit(1)

if __name__ == "__main__":
	if len(sys.argv) < 3:
		bail_out("Too few arguments")

	verb = sys.argv[1]
	kexts = map(kname2path, sys.argv[2:])

	func = globals().get(verb)
	if not func:
		bail_out("Unknown verb: %s" % verb)

	func(kexts)
