#!/bin/python

# Parser for usb, msc events.
# How to use:
# 1) install py-lib for writing to xls-file (http://www.python-excel.org)
# 2) run script, input file is "prl_etrace_dump.txt", output is by default

import xlwt
from optparse import OptionParser

events_rd = {
		"MSC_READ_CSW" : [],
		"MSC_READ_CALLBACK" : [],
		"MSC_START_READ" : [],
		"MSC_READ_CMD" : []
}

events_wr = {
		"MSC_WRITE_CSW" : [],
		"MSC_WRITE_CALLBACK" : [],
		"MSC_START_WRITE" : [],
		"MSC_WRITE_CMD" : []
}

events_aio = {
		"AIO_WORKER_WAKE_UP" : [],
		"AIO_WORKER_STOP_IO" : [],
		"AIO_WORKER_START_IO" : [],
		"AIO_WORKER_SUBMIT" : [],
}

def prepare_dicts(input_filename):
	fd = open(input_filename,'r')
	text = fd.readlines()

	all_events = (events_rd, events_wr, events_aio)

	sort_to_lists = lambda events, line: [word in line and \
		events[word].append(line.split()[0]) \
		for word in events.keys()]

	for line in text:
		if "app" not in line:
			continue
		[sort_to_lists(item, line) for item in all_events]

	fd.close()
	[llist.sort() for llist in item.values() for item in all_events]

def write_to_xls(output_filename):
	wb = xlwt.Workbook()
	ws1 = wb.add_sheet('Read data')
	ws2 = wb.add_sheet('Write data')

	read_seq = ("MSC_READ_CMD",
		"AIO_WORKER_SUBMIT",
		"MSC_START_READ",
		"AIO_WORKER_START_IO",
		"AIO_WORKER_STOP_IO",
		"AIO_WORKER_WAKE_UP",
		"MSC_READ_CALLBACK",
		"MSC_READ_CSW")

	write_seq = ("MSC_WRITE_CMD",
		"AIO_WORKER_SUBMIT",
		"MSC_START_WRITE",
		"AIO_WORKER_START_IO",
		"AIO_WORKER_STOP_IO",
		"AIO_WORKER_WAKE_UP",
		"MSC_WRITE_CALLBACK",
		"MSC_WRITE_CSW")

	for ev in read_seq:
		ws1.write(0, read_seq.index(ev), ev)
	for ev in write_seq:
		ws2.write(0, write_seq.index(ev), ev)

	aio_limit = min(len(events_aio["AIO_WORKER_SUBMIT"]), len(events_aio["AIO_WORKER_WAKE_UP"]))
	rd_limit = min(len(events_rd["MSC_READ_CMD"]), len(events_rd["MSC_READ_CSW"]))
	wr_limit = min(len(events_wr["MSC_WRITE_CMD"]), len(events_wr["MSC_WRITE_CSW"]))

	if events_rd["MSC_READ_CMD"][0] > events_rd["MSC_READ_CSW"][0]:
		events_rd["MSC_READ_CSW"].pop(0)

	for index in range(0, rd_limit):
		read_begin = events_rd["MSC_READ_CMD"][index]
		read_finish = events_rd["MSC_READ_CSW"][index]
		for ev in events_rd.keys():
			for i in range(index, rd_limit):
				if events_rd[ev][i] >= read_begin and \
					events_rd[ev][i] <= read_finish:
					ws1.write(index + 1, read_seq.index(ev), events_rd[ev][i])
					break
		for ev in events_aio.keys():
			for i in range(index, aio_limit):
				if events_aio[ev][i] >= read_begin and \
					events_aio[ev][i] <= read_finish:
					ws1.write(index + 1, read_seq.index(ev), events_aio[ev][i])
					break

	if events_wr["MSC_WRITE_CMD"][0] > events_wr["MSC_WRITE_CSW"][0]:
		events_wr["MSC_WRITE_CSW"].pop(0)

	for index in range(0, wr_limit):
		write_begin = events_wr["MSC_WRITE_CMD"][index]
		write_finish = events_wr["MSC_WRITE_CSW"][index]
		for ev in events_wr.keys():
			for i in range(index, wr_limit):
				if events_wr[ev][i] >= write_begin and \
					events_wr[ev][i] <= write_finish:
					ws2.write(index + 1, write_seq.index(ev), events_wr[ev][i])
					break
		for ev in events_aio.keys():
			for i in range(index, aio_limit):
				if events_aio[ev][i] >= write_begin and \
					events_aio[ev][i] <= write_finish:
					ws2.write(index + 1, write_seq.index(ev), events_aio[ev][i])
					break

	wb.save(output_filename)

def main():
	parser = OptionParser()
	parser.add_option("-i", "--inputfile", dest="input_filename",
		help="file to parse (prl_etrace_dump.txt)", metavar="FILE")
	parser.add_option("-o", "--outputfile", dest="output_filename",
		default="prl_etrace_parse.xls",
		help="output xls file with result (*.xls)", metavar="FILE")
	(options, args) = parser.parse_args()

	print "Parsing data..."
	prepare_dicts(options.input_filename)
	print "Writing to xls..."
	write_to_xls(options.output_filename)

if  __name__ =='__main__':main()
