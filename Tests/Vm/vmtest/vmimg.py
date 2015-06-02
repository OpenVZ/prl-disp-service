"""
Wrapper for VM "image" and XML config
"""
import os, sys, shutil
from logging import debug
try:
	import xml.etree.cElementTree as et
except ImportError:
	import cElementTree as et

__all__ = ["VmImg"]

here = os.path.dirname(__file__)
config_pvs = os.path.join(here, "config.pvs")

class VmImg:
	def __init__(self, path):
		self.home = path
		debug("creating vm in %r", self.home)
		os.makedirs(self.home)

		self.cfgfile = os.path.join(self.home, "config.pvs")
		shutil.copy(config_pvs, self.cfgfile)
		self.cfg = et.ElementTree(file = self.cfgfile)

	def _getDevFile(self, xpath, emulated_type):
		"""
		Return file associated with a device, making sure that it's a
		single device of the kind, is enabled, connected, and is of
		specified emulated type.
		"""
		elems = self.cfg.findall(xpath)
		assert len(elems) == 1
		elem = elems[0]
		assert int(elem.find("EmulatedType").text) == emulated_type
		assert int(elem.find("Enabled").text) == 1
		assert int(elem.find("Connected").text) == 1

		fname = elem.find("SystemName").text.strip()
		return os.path.join(self.home, fname)

	def getIso(self):
		return self._getDevFile("./Hardware/CdRom", 1)

	def getSerialLog(self):
		return self._getDevFile("./Hardware/Serial", 2)

	def setIsoPath(self, path):
		self.cfg.find("./Hardware/CdRom/SystemName").text = path
		self.cfg.find("./Hardware/CdRom/UserFriendlyName").text = path

	def setSystemFlags(self, sf):
		self.cfg.find("./Settings/Runtime/SystemFlags").text = sf

	def setCpuNumber(self, n):
		self.cfg.find("./Hardware/Cpu/Number").text = str(n)

	def setMemorySize(self, mem):
		self.cfg.find("./Hardware/Memory/RAM").text = str(mem)

	def setVideoMemorySize(self, mem):
		self.cfg.find("./Hardware/Video/VideoMemorySize").text = str(mem)

	def enableNetwork(self, enable):
		self.cfg.find("./Hardware/NetworkAdapter/Enabled").text = str(int(enable))
		self.cfg.find("./Hardware/NetworkAdapter/Connected").text = str(int(enable))

	def removeNetwork(self):
		networkNode = self.cfg.find("./Hardware/NetworkAdapter")
		if networkNode is not None:
			self.cfg.find("./Hardware").remove(networkNode)

	def writeCfg(self):
		f = open(self.cfgfile, 'w')
		self.cfg.write(f)
		f.close()
