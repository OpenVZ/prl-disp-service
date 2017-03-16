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
# Small script to tests .



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

# max_wait_timeout = 10*1000 # 70 sec is max wait timeout
max_wait_timeout = 180*1000 # DEBUG

class TestError(Exception):
	def __init__(self, message):
		self.message = message
		Exception.__init__( self, message )


#########################################################################################
class CommonTests:

        # ----------------------------------------------------------
        def __init__ (self):
            i = 0

        # ---------------------------------------------------------
        def main ( self ):

            ret = 0

            methods = [ 'Test_GetLicenseInfo_Basic'
                        , 'Test_bugs3115_2511_CantRecreateImage'
                        # 'Test_GetLicenseInfo_Basic'
                       ]
            for func_name in methods:

                # print 'func name = [%s]' % func_name

                func = getattr ( self, func_name )

                try:
                    server = prlsdkapi.Server()

                    job = server.login_local()
                    job.wait( max_wait_timeout )

                    fret = 1
                    try:
                        fret = func( server )
                    except Exception,e :
                        print 'some error catched: error e=%s' % e

                    if 0==fret:
                        print 'PASSED [ %s ]' % func_name
                    else:
                        print 'FAILED! [ %s ]' % func_name
                        ret += 1

                    # raw_input ("press any key")

                    job = server.logoff()
                    job.wait( max_wait_timeout )
                except Exception,e :
                    print 'some error catched: error e=%s' % e
                    ret +=1

            return ret

        # ---------------------------------------------------------
        def Test_GetLicenseInfo_Basic( self, server ):


            job = server.get_license_info()
            job.wait( max_wait_timeout )
            result = job.get_result()
            lic = result.get_param()

            return 0

        # ---------------------------------------------------------
        def Test_bugs3115_2511_CantRecreateImage( self, server ):

            #
            #   This test try recreate image with different parameters
            #   as defined in
            #   bug 2511: http://bugzilla.parallels.ru/show_bug.cgi?id=2511
            #   bug 3115: http://bugzilla.parallels.ru/show_bug.cgi?id=3115
            #
            result = 0

            vm = server.create_vm()
            # raw_input ("press any key")

            vm.set_uuid ( CommonTestsUtils.gen_random_guid()  )
            vm.set_name ( "createImage_" + vm.get_uuid()[:20] )

            job = vm.reg('')
            job.wait( max_wait_timeout )

            try:
                job = vm.begin_edit()
                job.wait( max_wait_timeout )


                # ============================
                # create HDD device
                # ============================
                devHdd = vm.create_vm_dev( prlsdkapi.consts.PDE_HARD_DISK )

                # raw_input ("press any key")
                image_path = vm.get_home_path()
                image_path = os.path.dirname( image_path ) + "/image.hdd"

                devHdd.set_image_path ( image_path )
                devHdd.set_sys_name ( image_path )
                devHdd.set_disk_size ( 20 )

                # ============================
                # create FDD device
                # ============================
                devFdd = vm.create_vm_dev( prlsdkapi.consts.PDE_FLOPPY_DISK )

                # raw_input ("press any key")
                image_path = vm.get_home_path()
                image_path = os.path.dirname( image_path ) + "/fdd.hdd"

                devFdd.set_image_path ( image_path )
                devFdd.set_sys_name ( image_path )
                devFdd.set_emulated_type ( prlsdkapi.consts.PDT_USE_IMAGE_FILE  )

                job = vm.commit()
                job.wait( max_wait_timeout )

                dictDeviceAndRetCode = {
                    devHdd: prlsdkapi.errors.PRL_ERR_HDD_IMAGE_IS_ALREADY_EXIST,
                    devFdd: prlsdkapi.errors.PRL_ERR_FLOPPY_IMAGE_ALREADY_EXIST
                     }

                for dev, allowRetCode in dictDeviceAndRetCode.iteritems():
                    try:
                       flgAllowRecreate = 1
                       flgDenyRecreate  = 0
                       # print '1: dev=%s, code=%#x' % (dev, allowRetCode)

                       job = dev.create_image( flgDenyRecreate )
                       job.wait( max_wait_timeout )


                       # print 2
                       # try to recreate with success
                       job = dev.create_image( flgAllowRecreate )
                       job.wait( max_wait_timeout )

                       try:
                            # print 3

                            # try to recreate with Error

                            job = dev.create_image( flgDenyRecreate )
                            job.wait( max_wait_timeout )

                            # print 4
                            raise TestError( "error: dispatcher doesn't reject CreateImage transaction to CreateImage(%s)", dev.get_image_path() )
                       except TestError,e:
                           print "error: %s" %e
                           raise
                       except( prlsdkapi.PrlSDKError ):
                           # print 6

                           retCode = job.get_ret_code()

                           # convert to UINT to compare
                           retCode =       ( 0x100000000L + retCode      ) & 0xffffffffL
                           retCodeWaited = ( 0x100000000L + allowRetCode ) & 0xffffffffL
                           # print 7

                           if ( retCode != retCodeWaited ):
                               print 'retcode = %x '  % retCode
                               print 'allow retCode = %x' % retCodeWaited
                               raise prlsdkapi.PrlSDKError( retCode )
                           # print "ok !"
                    except:
                          result += 1
                # end of for

                # raw_input ("press any key")

            except:
                result = 1

            # print "vm name = %s" % vm.get_name()
            job = vm.delete()
            job.wait( max_wait_timeout )

            return result


# ---------------------------------------------------------
# Fake handler to redirect to the test()
# ---------------------------------------------------------
if ( __name__ == '__main__' ):

    # witching current dir to the folder, where script is
    saved_path = os.getcwd()
    os.chdir( __full_file_path__ )

    ret = 9

    try:
        test_obj = CommonTests()
        ret = test_obj.main()
    finally:
        os.chdir( saved_path )
        print '=============='
        if 0==ret:
            print 'PASSED'
        else:
            print 'FAILED! ( %d times) ' % int(ret)
        print '=============='

    sys.exit( ret )

