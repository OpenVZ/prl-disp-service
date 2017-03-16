#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:ts=4:sw=4:noexpandtab
#
# ==================================================================
#
# Copyright (c) 2005-2017, Parallels International GmbH.
# All Rights Reserved.
#
# Our contact details: Parallels International GmbH, Vordergasse 59, 8200
# Schaffhausen, Switzerland.
#
# This Module is a part of Parallels Automated test system.
#
# ==================================================================
#
# Small script to test usecase of bug #2293: Dispatcher crashes on stop.
# Note: login user should't be member of Administrator group.
#

import os
import sys
import ConfigParser
import shutil
import imp
import string
import os.path
# import uuid

import time
import threading

sources_root = '../../../'

if (sys.platform == 'win32'):
	try:
		import win32service
		ws = win32service
	except:
		print( 'win32service module require for test execution under Win platform. Please install Active Python or another package contains this module' )
		sys.exit( -1 )

# Full path of this python file module
__full_file_path__ = os.path.split( os.path.abspath(__file__) )[0]
if __full_file_path__ == '':
    __full_file_path__ = os.path.abspath( os.path.getcwd() )

sys.path.append( os.path.abspath( os.path.join( __full_file_path__, sources_root +  'Build' ) ) )
sys.path.append( os.path.abspath( os.path.join( __full_file_path__, sources_root +  'Tests/_Python' ) ) )
import CommonTestsUtils

sys.path.insert( 0, os.path.abspath( os.path.join( __full_file_path__, sources_root +  'SDK/Python' ) ) )
sys.path.insert( 0, os.path.abspath( os.path.join( __full_file_path__, sources_root +  '../z-Build/' + CommonTestsUtils.Target ) ) )
import prlsdkapi

CommonTestsUtils.init_sdk(prlsdkapi)

max_wait_timeout = 20*1000 # 20 sec is max wait timeout

disp_finish_info_file = "parallels.finish.info"

# ---------------------------------------------------------
# Test response of vm requestes
# ---------------------------------------------------------
def test_CrashOnStop():

		res = 1 # return value

        # First we need to connect to the server
        server = prlsdkapi.Server()
        job = server.login_local()
        job.wait( max_wait_timeout )

        print( 'Stopping Services ...' )
    	if sys.platform == 'linux2' or sys.platform == 'darwin':
        	# Kill services
        	os.system( 'sudo killall -TERM prl_disp_service' )

    	elif sys.platform == 'win32':
			try:
				try:
					scmhandle = ws.OpenSCManager(None, None, ws.SC_MANAGER_ALL_ACCESS)
					svchandle = ws.OpenService(scmhandle, "Parallels Dispatcher Service", ws.SERVICE_ALL_ACCESS)
					ws.ControlService(svchandle, ws.SERVICE_CONTROL_STOP)
				except:
					print( 'Failed to stop service' )
			finally:
				ws.CloseServiceHandle(svchandle)


		# check dispatcher crash
		if ( sys.platform == 'win32' ):
			tmp_path = "%s/Temp/" % os.environ[ 'windir' ]
		else:
			tmp_path = "/tmp/"

		tmp_path = tmp_path + disp_finish_info_file

		sleep_time = 12
		i = 0

		print( 'wait started ...' )
		f = 0
		while ( i < sleep_time ):
			time.sleep( 5 )
			try:
				f = open( tmp_path, 'r' )
			except:
				i = i+1
				continue
			if ( f ):
				break
       	print( 'wait finished ...' )

		res = 1
		if ( f ):
			ts = f.read()

			curr_ts = time.time()

			print "ts = %s, curr_ts = %s " % ( ts, curr_ts )

			delta = abs ( float(ts) - curr_ts)
			print 'delta = %s' % delta

			if ( delta < 90 ):
				res = 0
			else:
				res = 1
		else:
			print "Couldn't to open dispatcher finish file %s" % tmp_path

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
	    ret = test_CrashOnStop()
	finally:
	    os.chdir( saved_path )
		print '=============='
		if 0==ret:
			print 'PASSED'
		else:
			print 'FAILED!'
		print '=============='

    sys.exit( ret )
