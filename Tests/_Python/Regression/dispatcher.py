#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# (c) Parallels Software International, Inc. 2005-2008
#
# AUTHOR:
# sergeyt@
#
# dispatcher regression test suite
#

import prlsdkapi
import sys, time, getopt, operator, re
import os, uuid

class VmAutoRemover:
	def __init__( self, vm ):
		self.vm = vm
	def __del__( self ):
		self.vm.refresh_config().wait()

		vm_name = self.vm.name
		vm_state = self.vm.state.state
		print "try to stop and delete vm %s in state %d" %  (vm_name, vm_state )
		if vm_state == prlsdkapi.consts.VMS_RUNNING :
			self.vm.stop().wait()
		self.vm.delete().wait()
		print "vm %s was successfully deleted" % vm_name

class Regress(object):

	def bug115494(self):
		''' bug: https://bugzilla.sw.ru/show_bug.cgi?id=115494
		    After connect/disconnect device device info doesn't store in config.pvs

		'''

		curr_dir = os.path.dirname( os.path.abspath( __file__ ) )
		# print "curr_dir=%s, file=%s" % (curr_dir, __file__ )
		iso_path_1 = os.path.join ( curr_dir, "1.iso" )
		iso_path_2 = os.path.join ( curr_dir, "2.iso" )

		s = prlsdkapi.Server()
		s.login_local().wait()

		# create vm
		vm = s.create_vm()
		vm.set_default_config( s.server_config, prlsdkapi.consts.PVS_GUEST_VER_LIN_REDHAT, False )
		vm.name = str( uuid.uuid1() )
		vm.reg( "", True ).wait()

		# create auto deletter
		auto_rm = VmAutoRemover( vm )

		# create device
		dev_type = prlsdkapi.consts.PDE_OPTICAL_DISK
		vm.begin_edit() # .wait()
		dev = vm.create_device( dev_type )
		dev.image_path = iso_path_1
		dev.sys_name = dev.image_path
		dev.enabled = True
		dev.connected = False
		vm.commit().wait()

		dev_index = dev.index

		# start vm
		vm.start().wait()

		# MAJOR point 1: check that 'connected' state stored
		dev.connect().wait()
		vm.refresh_config().wait()
		## find device
		dev = vm.get_device( dev_type, dev_index );
		if dev.connected != True:
			raise Exception, "Device connected state not stored!"
		# MAJOR point 1 end.


		# MAJOR point 2: check that connect/disconnect store all parameters
		dev.disconnect().wait()
		dev.image_path = iso_path_2
		dev.sys_name = dev.image_path
		dev.connect().wait()

		vm.refresh_config().wait()
		## find device
		dev = vm.get_device( dev_type, dev_index );
		if dev.connected != True :
			raise Exception, "Step2: Device connected state not stored!"
		if dev.sys_name != iso_path_2 :
			raise Exception, "Step2: Device sys_name not stored!"
		# MAJOR point 2 end.

if __name__ == "__main__":
	try:
		test = Regress()
		test.bug115494()
		print "PASSED: Regress suite"
	except Exception, err:
		print "error: %s" %err
		print "FAILED!: Regress suite"
		raise
