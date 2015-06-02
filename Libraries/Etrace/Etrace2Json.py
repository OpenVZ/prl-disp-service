#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:ts=4:sw= 4:noet

import os, re
import json
from optparse import *

class Parser(object):
	def __init__(self):
		self._reg = re.compile("^([\d\.]+)\s+([-\d]+)\s+(IOS|PAX):\s*(\w+):\s*(\w+):([ICBESTF])\s*(.*)")

	def parse(self,l):
		match = self._reg.search(l)
		if not match:
			return False

		time, delta, pref, self._group, self._event, self._phase, aa = match.groups()
		self._time = float(time)
		self._delta = int(delta)
		self._id = 0

		self._args = {}
		if aa != "":
			aa = aa.strip()
			bb = aa.split(",")
			for key_val in bb:
				key, val = key_val.split(":")
				key = key.strip()
				val = val.strip()
				self._args[key] = val

		return True

parser = Parser()

def parse_and_sort(path):
	res = []
	with open(path, 'r') as f:
		for l in f.readlines():
			if (not parser.parse(l)):
				continue
			res.append({'time':parser._time, 'delta':parser._delta, 'group':parser._group, 'event':parser._event, 'phase':parser._phase, 'args':parser._args})
	res.sort(key = lambda x: x['time'])
	return res

def sync_time(master, slave):
	master_time = .0
	slave_time = .0
	ts = 0
	sn = 0
	rtt = 0

	# find master's ts of the first sent rtp packet
	for e in master:
		if (e['group'] != 'RTP' or e['event'] != 'VSendNormal'):
			continue
		master_time = e['time']
		ts = e['args']['ts']
		sn = e['args']['seqnum']
		break

	# find first non-zero rtt
	for e in master:
		if (e['group'] != 'MASTER' or e['event'] != 'VTargRTT' or e['args']['rtt'] == '0'):
			continue
		rtt = int(e['args']['rtt'])
		break

	# find slave's ts of the first seceived rtp packet
	for e in slave:
		if (e['group'] != 'RTP' or e['event'] != 'RTPRecv' or e['args']['seqnum'] != sn or e['args']['ts'] != ts):
			continue
		slave_time = e['time']
		break

	if (master_time == 0 or slave_time == 0 or ts == 0):
		print "!!!Can't find tags for time sync!!!\n"
		return 0

	# return delta in usec
	d = master_time - slave_time + rtt/2/1000.0
	#print "!!! ts:%s sn:%s master:%f slave:%f rtt:%f delta:%f" % (ts, sn, master_time, slave_time, rtt/1000.0, d)
	return d

def webrtc2json(data, mode, delta):
	global vencode
	global vrender
	vcapt_ts = ""
	vrend_ts = ""
	prev_ts = -float('Inf')
	async_f_ts = {}

	# If delta < 0 - it will be counted in master log
	# in other case it will be counted in slave log
	if mode:
		pid = "M"
		comma = 0
		delta = -delta
	else:
		pid = "S"
		comma = 1

	if delta < 0:
		delta = 0

	for e in data:
		if comma:
			print ",\n",
		comma = 1

		json_tid = e['group'] + ':' + e['event']

		# Compensate master/slave clock unsynchronization
		ts = e['time'] + delta

		# If one slice starts at the same ts as privious slice
		# trace_event_importer.js will detect some bug "..a slice of the same id .. was alrady open"
		# So, this is simple workaround - add 1us to ts.
		if ts <= prev_ts:
			if (prev_ts-ts) > 0.000005:
				raise Warning("!!!Too big ts error (prev:%f cur:%f delta:%f)!!!\n%s" % (prev_ts, ts, delta, e));
			ts = prev_ts + 0.0000015
		prev_ts = ts

		str = '{"cat":"%s","pid":"%s","tid":"%s","ts":%d,"ph":"%s","name":"%s"' % \
			(e['group'], pid, json_tid, int(ts*1000000), e['phase'], e['event'])

		if not len(e['args']):
			str += ',"args":{}'
		else:
			str += ',"args":'
			str += json.dumps(e['args'])
		str += ',"id":"0"}'
		print str,

def video_delay(master, slave, delta):
	if delta < 0:
		mdelta = -delta
		sdelta = 0
	else:
		mdelta = 0
		sdelta = delta

	# Store capture start time
	vcapt = {}
	for e in master:
		if e['group'] == 'MASTER' and e['event'][:8] == 'VCapture' and (e['phase'] == 'B' or e['phase'] == 'S') and e['args']['port'] == '0':
			frame_id = e['args']['no']
			vcapt[frame_id] = e['time'] + mdelta

	# Store render end-event time
	vrend = {}
	for e in slave:
		if e['group'] == 'SLAVE' and e['event'][:7] == 'VRender' and (e['phase'] == 'E' or e['phase'] == 'F'):
			frame_id = e['args']["no"]
			vrend[frame_id] = e['time'] + sdelta

	for (frame_id, time) in sorted(vrend.items(), key = lambda x : x[1]):
		if not vcapt.has_key(frame_id):
			#print "Can't find encode start for frame_id:%s\n" % (frame_id)
			continue
		str = ',\n{"cat":"STAT","pid":"A","tid":"A:VDelay","ts":%d,"ph":"C","name":"VDelay","args":{"delay": "%d"}, "id":"0"} ' % \
			(int(time*1000000), int((time - vcapt[frame_id])*1000.0))
		print str,

if __name__ == "__main__":
	conf = OptionParser(usage="uasge: %prog [options] <etrace_master.log> [<etrace_slave.log>]")
	conf.add_option("--no-sync", action="store_true", dest="no_sync", default=False, help = "Do not sync time")
	conf.add_option("--no-video-delay", action="store_true", dest="no_video_delay", default=False, help = "Do not calculate video delay")

	opts, args = conf.parse_args()

	if len(args) < 1 or len(args) > 2:
		conf.error("wrong num of arguments")

	master = parse_and_sort(args[0])
	slave = []
	if len(args) == 2:
		slave = parse_and_sort(args[1])

	delta = 0.0
	if not opts.no_sync and len(args) == 2:
		delta = sync_time(master, slave)

	print "{\"traceEvents\":[",
	webrtc2json(master, True, delta)
	if len(args) == 2:
		webrtc2json(slave, False, delta)
		if not opts.no_video_delay:
			video_delay(master, slave, delta)
	print "]}\n"
