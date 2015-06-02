#!/usr/bin/python

import itertools

from insn import *

# -----------------------------------------------------------------------------

def gen_arrangements(*argv):
	return itertools.product(*argv)

# -----------------------------------------------------------------------------

regs = {
	'AL' : 0, 'CL' : 1, 'DL' : 2, 'BL' : 3,
	'RAX' : 0, 'RCX' : 1, 'RDX' : 2, 'RBX' : 3, 'RSP' : 4
}

# -----------------------------------------------------------------------------

class test_case:
	def __init__(self, state, insn):
		self.state = state
		self.insn = insn

	def __str__(self):
		return 'Instruction: %s\nState %s:\n' % (' '.join(map(hex, self.insn)), self.state)

# -----------------------------------------------------------------------------

def gen_states(d):
	res = []
	sds = d.state_desc

	for sd in sds:
		if not(isinstance(sd, tuple)):
			raise Exception('Invalid state description', sd)

		vals = sd[1]
		if not isinstance(vals, list):
			vals = sd[1].expand()
			if not isinstance(vals, list):
				vals = [vals]

		res.append(map(lambda x: (sd[0], x), vals))

	return gen_arrangements(*res)



def filter_none(l):
	return filter(lambda x: x is not None, l)



def gen_variants(d):
	res = []

	for opc in d.opcodes:
		res += opc.emit()

	states = gen_states(d)
	insns = gen_arrangements(*res)
	testcases = gen_arrangements(states, insns)
	return itertools.imap(lambda x: test_case(x[0], list(filter_none(x[1]))), testcases)
