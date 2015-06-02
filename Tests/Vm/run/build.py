#!/usr/bin/env python
"""
Ugly and unreliable builder which builds only the necessary stuff: prl_vm_app,
monitor, and hypervisor and netdevice modules.
"""

import os, sys, subprocess

src_dir = os.path.abspath(os.path.join(
	os.path.dirname(__file__), "..", "..", ".."))

if sys.platform in ("linux2", "darwin"):
	ncpus = os.sysconf("SC_NPROCESSORS_ONLN")
	make_cmd = ["make", "-C", "Vm", "-j", str(ncpus), "debug"]
else:
	raise RuntimeException("unsupported platform: " + sys.platform)

if sys.platform == "linux2":
	qthome = "/usr/lib64/qt4"
	os.environ["QMAKESPEC"] = "linux-g++-64"

	def drv_build():
		from Drv import action_lin
		action_lin("build")

elif sys.platform == "darwin":
	qthome = "/usr/local/Trolltech/Qt-4.8.0-shared"

	def drv_build():
		from Drv import action_mac
		action_mac("build")

else:
	raise RuntimeException("%r: unsupported platform" % sys.platform)

path = os.environ["PATH"].split(os.pathsep)
path.append(os.path.join(qthome, "bin"))
os.environ["PATH"] = os.pathsep.join(path)

os.chdir(src_dir)
sys.path.append(".")

subprocess.check_call([sys.executable, "Gen.py",
		"noall", "nocheck", "nogui", "nopython", "nodrv", "noqmake"])

subprocess.check_call(["qmake", "Vm"])

subprocess.check_call(make_cmd)

subprocess.check_call([sys.executable, "Mon.py", "x64", "local"])

drv_build()
