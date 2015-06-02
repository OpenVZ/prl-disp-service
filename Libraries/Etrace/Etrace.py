#!/usr/bin/env python

# TCP client example
import socket, os, sys, time, getopt

opt_host = "localhost"
opt_port = 55655

#####################################################

def eprint(msg, eol = True):
	m = time.strftime("%H:%M:%S") + " - " + msg
	if eol:
		print m
	else:
		print m,


def run(host, port, verbose, cmd):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	try:
		s.connect((host, port))
	except socket.error, (value, message):
		print "\n  Error: can't connect to VM @ " + str(host)+ ":" + str(port) + " - " + message
		print "\n  Possible reasons are:"
		print "  1) ETRACE_MASK is not defined in Libraries/Std/Etrace.h"
		print "  2) port number is overridden by the 'etrace.port' system flag\n"
		return

	# connect message
	s.send('@')
	d = s.recv(1)
	if not d == '^':
		print "Error: wrong protocol"
		return
	s.close()

	if "s" in cmd:
		eprint("Dump events to parallels.log file ...")

	if verbose:
		eprint("send command \'" + cmd + "\' ...", eol = False)

	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
		s.connect((host, port))
	except socket.error, (value, message):
		print "Error: connection failed: " + message
		return

	s.send(cmd)
	d = s.recv(1)

	if d == '^':
		if verbose:
			print " completed"
		if "t" in cmd:
			eprint ("Connection OK")
		if "a" in cmd:
			eprint ("Capture ALL events")
		if "i" in cmd:
			eprint ("Capture I/O events")
		if "m" in cmd:
			eprint ("Capture MONITOR events")
		if "u" in cmd:
			eprint ("Capture USB events")
		if "s" in cmd:
			eprint ("Done")
	else:
		eprint ("Error!")

	s.close()

def usage():
	global opt_port
	global opt_host

	print "Usage: " + os.path.basename(sys.argv[0]) + " [OPTIONS] COMMANDs"
	print "Options are:"
	print "  -h|--host HOST - hostname/IP address where VM is started (" + opt_host + " by default)"
	print "  -p|--port PORT - port address (" + str(opt_port) + " by default)"
	print "                   Note: PORT address can be overridden by 'etrace.port' system flag"
	print "  -v|--verbose   - verbose mode"
	print "Commands are:"
	print "   a             - capture all events"
	print "   i             - capture I/O events only"
	print "   m             - capture monitor events only"
	print "   u             - capture usb events only"
	print "   s             - stop capturing and dump events to prl_etrace_dump.bin"
	print "   t             - test connection and exit"
	print ""
	print "Multiple commands can be provided, e.g. \"Etrace.py im\""

def main():
	global opt_port
	global opt_host
	verbose = 0

	cmd = ""
	port = opt_port
	host = opt_host
	cmds = ("a", "i", "m", "u", "s", "t")

	shrt = "p:h:v"
	lng = ["port=", "host=", "verbose"]

	cmd = sys.argv[len(sys.argv) - 1]
	if not all(c in cmds for c in cmd):
		print "Error: unknown command: " + cmd
		usage()
		sys.exit()

	if len(sys.argv) > 1:
		try:
			opts, args = getopt.getopt(sys.argv[1:(len(sys.argv) - 1)], shrt, lng)
		except:
			usage()
			sys.exit(1)

		for o, a in opts:
			if o in ("-p", "--port"):
				port = int(a)
			elif o in ("-h", "--host"):
				host = a
			elif o in ("-v", "--verbose"):
				verbose = 1

	run(host, port, verbose, cmd)

if __name__ == "__main__":
	main()
