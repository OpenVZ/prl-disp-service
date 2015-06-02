#!/usr/bin/env python
"""
Perform preparations (load kernel modules, configure PATH) and run all
available tests.

The script assumes that

  - the relevant components (prl_vm_app, monitor, hypervisor and network
    modules) are built
  - the host is configured appropriately, namely
    o the calling user is allowed to run the module loading script with
      elevated privileges without password; can be achieved by e.g.

      sudo run/setup_sudo.py add $USER

    o the calling user is allowed r/w access to the relevant device nodes; can
      be achieved on Linux by e.g.

      sudo run/setup_udev.py add

The calling user doesn't (and normally shouldn't) be root.

The non-zero exit code indicates that either module loading/unloading or some
of the tests failed.
"""

import os, sys, subprocess

here = os.path.dirname(__file__)
src = os.path.join(here, "..", "..")

def sudo_wrap(cmd):
	return ["sudo", "-n"] + cmd

if sys.platform == "linux2":
	vmappdir = "Debug64"
	kmods = ["prl_hypervisor", "prl_netbridge",
			"prl_vtdhook", "prl_usb_connect", "prl_vnic"]
	kmod_script = "kmod_lin.py"
	kmod_wrap = sudo_wrap
elif sys.platform == "darwin":
	vmappdir = "Debug/prl_vm_app.app/Contents/MacOS"
	kmods = ["prl_hypervisor", "prl_netbridge",
			"prl_usb_connect", "prl_vnic"]
	kmod_script = "kmod_mac.py"
	kmod_wrap = sudo_wrap
else:
	raise RuntimeException("%r: unsupported platform" % sys.platform)

def kmod_cmd(verb, mods):
	return kmod_wrap([os.path.join(here, "run", kmod_script), verb] + mods)

path = os.environ["PATH"].split(os.pathsep)
path.append(os.path.join(src, "..", "z-Build", vmappdir))
os.environ["PATH"] = os.pathsep.join(path)

os.environ["PARALLELS_CRASH_HANDLER"] = "0"

subprocess.check_call(kmod_cmd("load", kmods))
subprocess.check_call(os.path.join(here, "run.py"))
subprocess.check_call(kmod_cmd("unload", kmods))
