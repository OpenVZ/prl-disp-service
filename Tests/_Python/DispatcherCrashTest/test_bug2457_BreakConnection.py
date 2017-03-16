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
# simple script to test bugs:
#   bug #2298     cri     nor     Unexplained dispatcher crush under Mac OS
#   bug #2457     cri     nor     Dispatcher crushes during remote connection
#
#

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


max_wait_timeout = 10*1000 # 10 sec is max wait timeout


# ---------------------------------------------------------
# Test response of vm requestes
# ---------------------------------------------------------
def test():
                res = 1 # return value

                i = 0;
                try:

                    while i< 1000:
                       if i%20 == 0:
                          print "%d " % i
                       if i%200 == 0:
                          print "\n"

                       i += 1
                       srv = prlsdkapi.Server()

                       job = srv.login_local( )
                       job.wait( max_wait_timeout )

                       job = srv.get_vm_list()
                       job.wait( max_wait_timeout )

                except prlsdkapi.PrlSDKError, e:
                    print 'error caught! count = %d' % i
					print e
                    return 1

                return 0


# ---------------------------------------------------------
# Fake handler to redirect to the test()
# ---------------------------------------------------------
if ( __name__ == '__main__' ):

    # witching current dir to the folder, where script is
    saved_path = os.getcwd()
    os.chdir( __full_file_path__ )

    ret = 9

    try:
        ret = test()
    finally:
        os.chdir( saved_path )
        print '=============='
        if 0==ret:
            print 'PASSED'
        else:
            print 'FAILED!'
        print '=============='

    sys.exit( ret )
