#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:ts=4:sw=4:noexpandtab
#
# ==================================================================
#
# Copyright (c) 2005-2015 Parallels IP Holdings GmbH.
# All Rights Reserved.
#
# This Module is a part of Parallels Automated test system.
#
# ==================================================================
#
# Small script to test register existing VM .
#
#       Algorithm:
#
#        # First we need to connect to the server
#        # 1. Create VM
#        # raw_input ("press any key")
#            # 2. Get Vm Path
#            # 3. Unreg
#        # raw_input ("press any key")
#        # 4. Test to register Vm
#		 # 5. Clean test space - remove temorally VM
#		 # Logoff

import os
import sys
import ConfigParser
import shutil
import imp
import string
import os.path
# import uuid


sources_root = '../../../'

# Full path of this python file module
__full_file_path__ = os.path.split( os.path.abspath(__file__) )[0]
if __full_file_path__ == '':
    __full_file_path__ = os.path.abspath( os.path.getcwd() )

sys.path.append( os.path.abspath( os.path.join( __full_file_path__, sources_root +  'Tests/_Python' ) ) )
import CommonTestsUtils

sys.path.insert( 0, os.path.abspath( os.path.join( __full_file_path__, sources_root +  'SDK/Python' ) ) )
sys.path.insert( 0, os.path.abspath( os.path.join( __full_file_path__, sources_root +  '../z-Build/' + CommonTestsUtils.Target ) ) )
import prlsdkapi

CommonTestsUtils.init_sdk(prlsdkapi)


max_wait_timeout = 3600*1000

vm_type_str = {prlsdkapi.consts.PVT_CT: "CT", prlsdkapi.consts.PVT_VM:"VM"}

# ---------------------------------------------------------
# Test response of vm requestes
# ---------------------------------------------------------
def test(vm_type):
        res = 1 # return value

        new_vm_uuid = CommonTestsUtils.gen_random_guid()


        # First we need to connect to the server
        server = prlsdkapi.Server()
        job = server.login_local()
        job.wait( max_wait_timeout )

        # 1. Create VM
        vm = server.create_vm()
        vm.refresh_config()
        vm.set_vm_type( vm_type )
        vm.set_uuid ( new_vm_uuid )
		vm.set_name ( "reg" + vm_type_str[vm_type] + "_" + vm.get_uuid().strip('{}')[:20] )
		print "Create " + vm_type_str[vm_type] + " done."

        # raw_input ("press any key")

        job = vm.reg('')
        job.wait( max_wait_timeout )
		print "Reg " + vm_type_str[vm_type] + " done."

        try:
            # 2. Get Vm Path
            vm.refresh_config().wait( max_wait_timeout )
			print "GetConfig done."

            # path_to_config_pvs = vm.get_home_path()
            # vm_dir = os.path.dirname( path_to_config_pvs )
            vm_dir = vm.get_home_path()

            print "vm_dir = [%s]" % vm_dir

            # 3. Unreg
            job = vm.unreg()
            job.wait( max_wait_timeout )
			print "Unreg done"

        except:

            job = vm.delete()
            job.wait( max_wait_timeout )
			print "Delete done"

		# print "vm_dir = [%s]" % vm_dir

        # raw_input ("press any key")

        # 4. Test to register Vm
        job = server.register_vm( vm_dir, True )
        job.wait( max_wait_timeout )
        result = job.get_result()
		vm = result.get_param()
		print "Register " + vm_type_str[vm_type] + " done."

		# 5. Clean test space - remove temorally VM
        job = vm.delete()
        job.wait( max_wait_timeout )
		print vm_type_str[vm_type] + " Delete2 done."

        try:
            job = server.logoff()
            job.wait( max_wait_timeout )
            res = 0
 			print "Logoff done"
        except:
            res = 2

        return res

# ---------------------------------------------------------
# Fake handler to redirect to the test()
# ---------------------------------------------------------
if ( __name__ == '__main__' ):

    # witching current dir to the folder, where script is
    saved_path = os.getcwd()
    os.chdir( __full_file_path__ )

    ret = 9

	for vm_type in (prlsdkapi.consts.PVT_CT, prlsdkapi.consts.PVT_VM):
		try:
			ret = test(vm_type)
		finally:
			os.chdir( saved_path )
			print '=============='
			if 0==ret:
				print 'PASSED'
				print '=============='
			else:
				print 'FAILED!'
				print '=============='
				sys.exit( ret )

    sys.exit( 0 )
