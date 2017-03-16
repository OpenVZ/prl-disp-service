#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:ts=4:sw=4:expandtab
#
# ==================================================================
#
# Copyright (c) 2005-2017, Parallels International GmbH
# All Rights Reserved.
#
# Our contact details: Parallels International GmbH, Vordergasse 59, 8200
# Schaffhausen, Switzerland.
#
# This Module is a part of Parallels unit tests system.
#
# ==================================================================
#

import os, sys, string, random


Target = ''
for arg in sys.argv:
	if arg.lower() == 'debug':
		Target = 'Debug'
		break
	if arg.lower() == 'release':
		Target = 'Release'
		break

for arg in sys.argv:
    if arg.lower() == 'x64':
        Target = Target + '64'
        break

lib_prefix = ''
lib_ext = ''
if sys.platform == 'win32':
	lib_ext = '.dll'
if sys.platform == 'darwin':
	lib_prefix = 'lib'
	lib_ext = '.dylib'
if sys.platform == 'linux2':
	lib_prefix = 'lib'
	lib_ext = '.so'

def init_sdk_in_mode(prlsdkapi):
	try:
		if (sys.argv.count('server')) or (sys.argv.count('ps')):
			prlsdkapi.init_server_sdk()
		elif (sys.argv.count('workstation')) or (sys.argv.count('player')):
			prlsdkapi.init_workstation_sdk()
		else:
			prlsdkapi.init_desktop_sdk()
		return True
	except:
		return False

def init_sdk(prlsdkapi):
	for path in prlsdkapi.sys.path:
		if string.find(path, 'z-Build') != -1:
			prl_sdk_path = path + '/' + lib_prefix + 'prl_sdk' + lib_ext
			break

	prlsdkapi.set_sdk_library_path( prl_sdk_path )
	if init_sdk_in_mode(prlsdkapi) == True:
		return

	prlsdkapi.set_sdk_library_path( '' )
	init_sdk_in_mode(prlsdkapi)

def gen_random_guid():
	"""
	Random guid generator
	"""

	mask = '{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}'
	digits = '0123456789abcdef'
	random.seed()

	while mask.find( 'X' ) != -1:
			i = int( random.random() * len(digits) )
			mask = mask.replace( 'X', digits[i], 1 )

	return mask
