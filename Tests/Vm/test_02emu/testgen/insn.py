import itertools
import struct



class Byte(object):
	def __init__(self, byte):
		self.byte = byte

	def emit(self):
		return [[self.byte]]



class BytePlusR(Byte):
	def __init__(self, byte):
		Byte.__init__(self, byte)

	def emit(self):
		return [[self.byte + 0]]



class BytePlusC(Byte):
	def __init__(self, byte):
		Byte.__init__(self, byte)

	def emit(self):
		return [[self.byte + 4]]



class Imm(object):
	def __init__(self, size, values = None):
		self.size = size
		if values is not None:
			self.values = values
			self.default = False
		else:
			if size == 1:
				self.values = [0x00, 0x01, 0x80, 0xff]
			elif size == 2:
				self.values = [0x0000, 0x0001, 0x8000, 0xffff]
			elif size == 4:
				self.values = [0x00000000, 0x00000001, 0x80000000, 0xffffffff]
			elif size == 8:
				self.values = [0x0000000000000000, 0x0000000000000001, 0x8000000000000001, 0xffffffffffffffff]
			else:
				raise Exception('Invalid size', size)

			self.default = True

	def emit(self):
		return [map(lambda x: (self.size, x), self.values)]



class ModRM(object):
	def __init__(self, ro_op = None, mod = [0, 3]):
		self.ro = ro_op
		self.mod = mod

	def emit(self):
		rm = 3
		if self.ro is None:
			return [[(mod << 6) | (3 << 3) | rm for mod in self.mod]]
		else:
			return [[(mod << 6) | (self.ro << 3) | rm for mod in self.mod]]



class Prefixes:
	def __init__(self, require66H = False, requireREX = False):
		self.require66H = require66H
		self.requireREX = requireREX

	def emit(self):
		res = [[0xf0, 0xf2, 0xf3, None], [0x67, None]]
		if self.require66H:
			res += [[0x66]]
		if self.requireREX:
			res += [[0x40 | B << 0 | R << 2 | W << 3	for B in [0, 1]
														for R in [0, 1]
														for W in [0, 1]]]
		return res

	def __str__(self):
		pfxs = []
		if self.require66H:
			pfxs += ['require66H = True']
		if self.requireREX:
			pfxs += ['requireREX = True']
		return 'Prefixes(%s)' % (', '.join(pfxs))



class VEX2(object):
	def __init__(self):
		pass

	def emit(self):
		return [[0xC5], [R << 7 | 2 << 3 | L << 2 | pp	for R in [0, 1]
														for L in [0, 1]
														for pp in [0, 1, 2, 3]]]



class VEX3(object):
	def __init__(self, prefix = None, opcode = None):
		self.prefix = prefix
		self.opcode = opcode


	def emit(self):
		return [[0xC4],
				[R << 7 | X << 6 | B << 5 | M	for R in [0, 1]
												for X in [0, 1]
												for B in [0, 1]
												for M in [0, 1, 2, 3, 4]],
				[W << 7 | V << 3 | L << 2 | pp	for W in [0, 1]
												for V in [0, 1, 4]
												for L in [0, 1]
												for pp in [0, 1, 2, 3]]]



class XSTATE:
	def __init__(self, **kwargs):
		self.state = {
			'FCW'   : [0x0000],
			'FSW'   : [0x0000],
			'FTW'   : [0x00],
			'MXCSR' : [0x1F80],
			'MXCSR_MASK' : [0xFFFF],
			'XSTATE_BV'  : [0x0007]
		}

		for i in kwargs.items():
			if i[0] in self.state:
				self.state[i[0]] = i[1]


	def expand(self):
		res = []
		for d in map(dict, itertools.product(*[[(x[0], y) for y in x[1]] for x in self.state.items()])):
			res.append(struct.pack('HHBBHQQII',
									d['FCW'],
									d['FSW'],
									d['FTW'],
									0, 0, 0, 0,
									d['MXCSR'],
									d['MXCSR_MASK']) + \
									"\x00"*(512 - 32) + \
									struct.pack('Q', d['XSTATE_BV'])
					)

		return res

#------------------------------------------------------------------------------

class Op(object):
	def __init__(self, stor, size):
		self.stor = stor
		self.size = size



class insn_desc:
	def __init__(self, mne, ops, opcodes, flags, target_bits, state_desc):
		def filter_bytes(x):
			if isinstance(x, Byte):
				return True
			elif isinstance(x, ModRM):
				return x.ro is not None
			else:
				return False

		def to_bytes(x):
			if isinstance(x, Byte):
				return x.byte
			elif isinstance(x, ModRM):
				return x.ro
			else:
				raise Exception("Can't convert", x)


		self.mne = mne
		self.ops = ops
		self.opcodes = opcodes
		self.flags = flags
		self.target_bits = target_bits
		self.state_desc = state_desc
