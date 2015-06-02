#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:ts=4:sw=4:noexpandtab
#
# ==================================================================
#
# Copyright (c) 2005-2015 Parallels IP Holdings GmbH.
# All Rights Reserved.
#
# This Module is a part of Parallels test system.
#
# This test implemented Task #1508 ( http://bugzilla/show_bug.cgi?id=1508 )
# This test check receiving response of request sent to vm through dispatcher.
#
# ==================================================================
#

import os
import sys
import ConfigParser
import shutil
import imp
import string

sources_root = '../../../'

# Full path of this python file module
__full_file_path__ = os.path.split( os.path.abspath(__file__) )[0]
if __full_file_path__ == '':
    __full_file_path__ = os.path.abspath( os.path.getcwd() )

sys.path.append( os.path.abspath( os.path.join( __full_file_path__, sources_root +  'SDK/Python' ) ) )
sys.path.append( os.path.abspath( os.path.join( __full_file_path__, sources_root +  'Build' ) ) )

# Importing build utilites - allowing to call TRACE without prefix
import BuildUtil
from BuildUtil import TRACE

import Parallels


max_wait_timeout = 20*1000 # 20 sec is max wait timeout
vmconfig_file_name = "TestResponseOfVmRequest.pvs"

# ---------------------------------------------------------
# Test response of vm requestes
# ---------------------------------------------------------
def test_ResponseOfVmRequest():

		vm_config  = BuildUtil.file_to_string( vmconfig_file_name )

        # First we need to connect to the server
        server = Parallels.Server()
        job = server.LoginLocal()
        job.Wait( max_wait_timeout )
        job.Free()

		vm = server.CreateVm()
		vm.FromString( vm_config )

		job = vm.Reg('')
        job.Wait( max_wait_timeout )
	    job.Free()

		try:
			test_usecase_StartStop( vm )
			# test_usecase_StartRestartStop( vm )
			# test_usecase_StartPauseStartStop( vm )
			# test_usecase_StartSuspendResumeStop( vm )
		finally:
			job = vm.Delete()
    	    job.Wait( max_wait_timeout )
	    	job.Free()

        return 0


def test_usecase_StartStop( vm ):

		job = vm.Start()
        job.Wait( max_wait_timeout )
	    job.Free()

		job = vm.Stop()
        job.Wait( max_wait_timeout )
	    job.Free()

		return 0;

# def test_usecase_StartRestartStop( vm ):
# def	test_usecase_StartPauseStartStop( vm ):
# def	test_usecase_StartSuspendResumeStop( vm ):




# ---------------------------------------------------------
# Fake handler to redirect to the test()
# ---------------------------------------------------------
if ( __name__ == '__main__' ):

    # witching current dir to the folder, where script is
    saved_path = os.getcwd()
    os.chdir( __full_file_path__ )

	ret=0

	try:
	    ret = test_ResponseOfVmRequest()
	finally:
	    os.chdir( saved_path )
		print '=============='
		if 0==ret:
			print 'PASSED'
		else:
			print 'FAILED!'
		print '=============='

    sys.exit( ret )
