from insn import *

def gen_dr7(nbp):
	def blf(rw):
		if rw == 0:
			return [0]
		else:
			return [0, 1, 2, 3]

	def orl(l):
		r = 0
		for e in l:
			r |= e
		return r

	return	[
				orl([(1 << (0 + i*2)) | (rw << (16 + i*2)) | (bl << (18 + i*2)) for i in range(nbp)])
				for rw in [1, 3]
				for bl in blf(rw)
			]


insn_descs = [
	insn_desc('FXSAVE64', [Op('mem', 0)], [Byte(0x48), Byte(0xf), Byte(0xae), ModRM(0)], set(['SSE', 'FPU', 'X64', 'REX.W']), 64,
				[
					('RSP', [0x2800]),
					('RBX', [0x3000, 0x3ff0]),
					('DR0', [0x3000, 0x3001, 0x3ff1, 0x3fef]),
					('DR1', [0x3000, 0x3003, 0x3ff2, 0x3fee]),
					('DR7', gen_dr7(2))
				]
	),

	insn_desc('MOV', [Op('reg', 8), Op('rm', 8)], [Byte(0x8a), ModRM()], set(['8086']), 8,
		[
			('RBX', [0x3000, 0x3001]), ('RSP', [0x2800]),
			('DR0', [0x3000, 0x3001, 0x3003, 0x5000]),
			('DR1', [0x3000, 0x3002, 0x5000]),
			('DR6', [(p1 << 13) | p2 for p1 in range(16) for p2 in range(8)]),
			('DR7', gen_dr7(2))
		]
	),

	insn_desc(
		'MOV', [Op('reg', 16), Op('rm', 16)], [Byte(0x66), Byte(0x8b), ModRM()], set(['8086', 'OSO']), 16,
		[
			('RBX', [0x3000, 0x3001, 0x3ffe]), ('RSP', [0x2800]),
			('DR0', [0x3000, 0x3001, 0x5000, 0x5008]),
			('DR1', [0x3000, 0x3001, 0x5000, 0x5008]),
			('DR7', gen_dr7(2))
		]
	),

	insn_desc('MOV', [Op('reg', 32), Op('rm', 32)], [Byte(0x8b), ModRM()], set(['386']), 32,
		[
			('RBX', [0x3000, 0x3001, 0x3ffd]), ('RSP', [0x2800]),
			('DR0', [0x3000, 0x3002, 0x3ffd, 0x3ffe, 0x3fff, 0x5000, 0x5008]),
			('DR1', [0x3000, 0x3001, 0x3003, 0x3ffd]),
			('DR2', [0x3000, 0x3002, 0x3ffe]),
			('DR7', gen_dr7(3))
		]
	),

	insn_desc('MOV', [Op('reg', 64), Op('rm', 64)], [Byte(0x48), Byte(0x8b), ModRM()], set(['X64', 'REX.W']), 64,
		[
			('RBX', [0x3000, 0x3003, 0x3ffc]), ('RSP', [0x2800]),
			('DR0', [0x3000, 0x3004, 0x3006, 0x3007, 0x5000]),
			('DR1', [0x3000, 0x3001, 0x3003, 0x3ffd]),
			('DR2', [0x3000, 0x3002, 0x3ffe]),
			('DR3', [0x3000, 0x3005, 0x3ffe]),
			('DR7', gen_dr7(4))
		]
	),

	insn_desc('ADD', [Op('rm', 8), Op('reg', 8)], [Byte(0x0), ModRM()], set(['8086']), 8,
		[
			('RBX', [0x3000, 0x3001, 0x3002]), ('RSP', [0x2800]),
			('DR0', [0x3000, 0x3001, 0x3003, 0x5000]),
			('DR1', [0x3000, 0x3002, 0x5000]),
			('DR7', gen_dr7(2))
		]
	),

	insn_desc('ADD', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x1), ModRM()], set(['8086', 'OSO']), 16,
		[
			('RBX', [0x3000, 0x3ffe]), ('RSP', [0x2800]),
			('DR0', [0x3000, 0x3001]),
			('DR1', [0x3000, 0x3001]),
			('DR7', gen_dr7(2))
		]
	),

	insn_desc('ADD', [Op('rm', 32), Op('reg', 32)], [Byte(0x1), ModRM()], set(['386']), 32,
		[
			('RBX', [0x3000, 0x3001, 0x3ffd]), ('RSP', [0x2800]),
			('DR0', [0x3000, 0x3002, 0x3ffd, 0x3ffe, 0x3fff, 0x5000, 0x5008]),
			('DR1', [0x3000, 0x3001, 0x3003, 0x3ffd]),
			('DR2', [0x3000, 0x3002, 0x3ffe]),
			('DR7', gen_dr7(3))
		]
	),

	insn_desc('ADD', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x1), ModRM()], set(['X64', 'REX.W']), 64,
		[
			('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800]),
			('DR0', [0x3000, 0x3001, 0x3002, 0x3003, 0x3004, 0x3005, 0x3006, 0x3007]),
			('DR1', [0x3000, 0x3001, 0x3002, 0x3003, 0x3004, 0x3005, 0x3006, 0x3007]),
			('DR7', gen_dr7(2))
		]
	)
]
