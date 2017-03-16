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
# Small script to test usecase of bug #1859: Dispatcher crashes in Hdd image creation.
# Note: login user should't be member of Administrator group.
#

import os
import sys
import ConfigParser
import shutil
import imp
import string
# import uuid


sources_root = '../../../'

# Full path of this python file module
__full_file_path__ = os.path.split( os.path.abspath(__file__) )[0]
if __full_file_path__ == '':
    __full_file_path__ = os.path.abspath( os.path.getcwd() )

sys.path.append( os.path.abspath( os.path.join( __full_file_path__, sources_root +  'SDK/Python' ) ) )

import Parallels
Parallels.init_module()


# ---------------------------------------------------------
# Test   IsGuestOs
# ---------------------------------------------------------
def test_IsGuestOs():

    common = Parallels.Common()
    isGuestOs = common.IsGuestOs()

    if isGuestOs:
        print "Is Guest OS !"
    else:
        print "Is Host OS !"

# ---------------------------------------------------------
# Fake handler to redirect to the test()
# ---------------------------------------------------------
if ( __name__ == '__main__' ):

    test_IsGuestOs()

	Parallels.Deinit()

    sys.exit( 0 )
