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
# Small script to test usecase of bug #1859: Dispatcher crashes in Hdd image creation.
# Note: login user should't be member of Administrator group.
#

import os
import sys
import ConfigParser
import shutil
import imp
import string
import os.path


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


max_wait_timeout = 70*1000 # 70 sec is max wait timeout

# ---------------------------------------------------------
# Test response of vm requestes
# ---------------------------------------------------------
def test_CreateImage():
	res = 1 # return value

	# First we need to connect to the server
	server = prlsdkapi.Server()
	job = server.login_local()
	job.wait( max_wait_timeout )

	vm = server.create_vm()
	# raw_input ("press any key")

	vm.set_uuid ( CommonTestsUtils.gen_random_guid()  )
	vm.set_name("createImage_" + vm.get_uuid()[:20])

	job = vm.reg('')
	job.wait( max_wait_timeout )

	try:
		try:
			job = vm.begin_edit()
			job.wait( max_wait_timeout )

			# raw_input ("press any key")
			image_path = vm.get_home_path()
			image_path = os.path.dirname( image_path ) + "/image.hdd"

			# create image
			dev = vm.create_vm_dev( prlsdkapi.consts.PDE_HARD_DISK )

			dev.set_image_path ( image_path )
			dev.set_sys_name ( image_path )
			dev.set_disk_size ( 20 )

			job = dev.create_image( 0 )
			job.wait( max_wait_timeout )

			job = vm.commit()
			job.wait( max_wait_timeout )

			# raw_input ("press any key")
		finally:
			# print "vm name = %s" % vm.get_name()
			job = vm.delete()
			job.wait( max_wait_timeout )
	except:
		res = 2

	try:
		# test to server crashes ( if crashes - logoff was failed )
		job = server.logoff()
		job.wait( max_wait_timeout )
	finally:
		res = 0

	return res

# ---------------------------------------------------------
# Fake handler to redirect to the test()
# ---------------------------------------------------------
if ( __name__ == '__main__' ):

	# witching current dir to the folder, where script is
	saved_path = os.getcwd()
	os.chdir( __full_file_path__ )

	ret = 9

	try:
		ret = test_CreateImage()
	finally:
		os.chdir( saved_path )
		print '=============='
		if 0==ret:
			print 'PASSED'
		else:
			print 'FAILED!'
		print '=============='

	sys.exit( ret )
