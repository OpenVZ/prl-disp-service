#!/usr/bin/env python
"""
Wrapper encapsulating privileged operations for module loading/unloading.

The idea is that a non-privileged user can be allowed to run only this script,
which slightly limits the amount of damage she can make to the system.  To
achieve that, put a line in /etc/sudoers like this:

userjoe ALL= NOPASSWD: /Users/userjoe/some/path/kmod_lin.py
"""

import os, sys, subprocess

drv_dir = os.path.abspath(os.path.join(os.path.dirname(__file__),
				"../../../../z-Build/Drivers"))

def list_loaded(mods):
	p = subprocess.Popen(["lsmod"], stdout = subprocess.PIPE)
	ret = []
	for l in p.stdout:
		m = l.split(None, 1)[0]
		if m in mods:
			ret.append(m)
	p.wait()
	return ret

def do_unload(mods):
	cmd = ["modprobe", "-rav", "-d", drv_dir] + list(mods)
	return not subprocess.call(cmd)

def unload(mods):
	return do_unload(reversed(mods))

def load(mods):
	loaded = list_loaded(mods)
	if loaded:
		if not do_unload(loaded):
			sys.exit(1)
	cmd = ["modprobe", "-av", "-d", drv_dir] + list(mods)
	return not subprocess.call(cmd)

def bail_out(msg = None):
	if msg:
		print >> sys.stderr, msg
	print >> sys.stderr, "Usage: %s load|unload module ..." % sys.argv[0]
	sys.exit(1)

if __name__ == "__main__":
	if len(sys.argv) < 3:
		bail_out("Too few arguments")

	verb = sys.argv[1]
	args = sys.argv[2:]

	func = globals().get(verb)
	if not func:
		bail_out("Unknown verb: %s" % verb)

	func(args)
