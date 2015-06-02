from insn import *
insn_descs = [
	insn_desc('ADD', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0x0), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x1), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0x1), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x1), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0x2), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0x3), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0x3), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0x3), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('imm', 8)], [Prefixes(), Byte(0x4), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('ADD', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0x5), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('ADD', [Op('imm', 32)], [Prefixes(), Byte(0x5), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('ADD', [Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x5), Imm(4)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0x8), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x9), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0x9), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x9), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0xa), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0xb), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0xb), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xb), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('imm', 8)], [Prefixes(), Byte(0xc), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('OR', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0xd), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('OR', [Op('imm', 32)], [Prefixes(), Byte(0xd), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('OR', [Op('imm', 32)], [Prefixes(requireREX = True), Byte(0xd), Imm(4)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0x10), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x11), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0x11), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x11), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0x12), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0x13), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0x13), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0x13), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('imm', 8)], [Prefixes(), Byte(0x14), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('ADC', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0x15), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('ADC', [Op('imm', 32)], [Prefixes(), Byte(0x15), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('ADC', [Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x15), Imm(4)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0x18), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x19), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0x19), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x19), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0x1a), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0x1b), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0x1b), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0x1b), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('imm', 8)], [Prefixes(), Byte(0x1c), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('SBB', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0x1d), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('SBB', [Op('imm', 32)], [Prefixes(), Byte(0x1d), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('SBB', [Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x1d), Imm(4)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0x20), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x21), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0x21), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x21), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0x22), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0x23), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0x23), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0x23), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('imm', 8)], [Prefixes(), Byte(0x24), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('AND', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0x25), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('AND', [Op('imm', 32)], [Prefixes(), Byte(0x25), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('AND', [Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x25), Imm(4)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0x28), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x29), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0x29), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x29), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0x2a), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0x2b), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0x2b), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0x2b), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('imm', 8)], [Prefixes(), Byte(0x2c), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('SUB', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0x2d), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('SUB', [Op('imm', 32)], [Prefixes(), Byte(0x2d), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('SUB', [Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x2d), Imm(4)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0x30), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x31), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0x31), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x31), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0x32), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0x33), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0x33), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0x33), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('imm', 8)], [Prefixes(), Byte(0x34), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('XOR', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0x35), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('XOR', [Op('imm', 32)], [Prefixes(), Byte(0x35), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('XOR', [Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x35), Imm(4)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0x38), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x39), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0x39), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x39), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0x3a), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0x3b), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0x3b), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0x3b), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('imm', 8)], [Prefixes(), Byte(0x3c), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('CMP', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0x3d), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('CMP', [Op('imm', 32)], [Prefixes(), Byte(0x3d), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('CMP', [Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x3d), Imm(4)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('PUSH', [Op('reg', 16)], [Prefixes(require66H = True), BytePlusR(0x50)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('PUSH', [Op('reg', 64)], [Prefixes(), BytePlusR(0x50)], set(['X64']), 64, [('RSP', [0x2800])]),

	insn_desc('POP', [Op('reg', 16)], [Prefixes(require66H = True), BytePlusR(0x58)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('POP', [Op('reg', 64)], [Prefixes(), BytePlusR(0x58)], set(['X64']), 64, [('RSP', [0x2800])]),

	insn_desc('PUSH', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0x68), Imm(2)], set(['AR0', '186', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('PUSH', [Op('imm', 32)], [Prefixes(), Byte(0x68), Imm(4)], set(['AR0', 'X64']), 32, [('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('reg', 16), Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0x69), ModRM(), Imm(2)], set(['186', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('reg', 32), Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0x69), ModRM(), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('reg', 64), Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x69), ModRM(), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('PUSH', [Op('imm', 8)], [Prefixes(), Byte(0x6a), Imm(1)], set(['186']), 8, [('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('reg', 16), Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0x6b), ModRM(), Imm(1)], set(['186', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('reg', 32), Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0x6b), ModRM(), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('reg', 64), Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0x6b), ModRM(), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('INSW', [], [Prefixes(require66H = True), Byte(0x6d)], set(['186', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('INSD', [], [Prefixes(), Byte(0x6d)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('OUTSW', [], [Prefixes(require66H = True), Byte(0x6f)], set(['186', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('OUTSD', [], [Prefixes(), Byte(0x6f)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('Jcc', [Op('imm', 8)], [Prefixes(), BytePlusC(0x70), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('TEST', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0x84), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('TEST', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0x85), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('TEST', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0x85), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('TEST', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0x85), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('XCHG', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0x86), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('XCHG', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0x87), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('XCHG', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0x87), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('XCHG', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0x87), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 8), Op('rm', 8)], [Prefixes(), Byte(0x8a), ModRM()], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0x8b), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0x8b), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0x8b), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 16), Op('reg_sreg', 16)], [Prefixes(require66H = True), Byte(0x8c), ModRM()], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 32), Op('reg_sreg', 16)], [Prefixes(), Byte(0x8c), ModRM()], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('LEA', [Op('reg', 16), Op('mem', 16)], [Prefixes(require66H = True), Byte(0x8d), ModRM()], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('LEA', [Op('reg', 32), Op('mem', 32)], [Prefixes(), Byte(0x8d), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('LEA', [Op('reg', 64), Op('mem', 64)], [Prefixes(requireREX = True), Byte(0x8d), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('NOP', [], [Prefixes(), Byte(0x90)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('XCHG', [Op('reg', 16)], [Prefixes(require66H = True), BytePlusR(0x90)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('XCHG', [Op('reg', 32)], [Prefixes(), BytePlusR(0x90)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('XCHG', [Op('reg', 64)], [Prefixes(requireREX = True), BytePlusR(0x90)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('CBW', [], [Prefixes(require66H = True), Byte(0x98)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('CWDE', [], [Prefixes(), Byte(0x98)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('CDQE', [], [Prefixes(requireREX = True), Byte(0x98)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('CWD', [], [Prefixes(require66H = True), Byte(0x99)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('CDQ', [], [Prefixes(), Byte(0x99)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('CQO', [], [Prefixes(requireREX = True), Byte(0x99)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('PUSHF', [], [Prefixes(), Byte(0x9c)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('PUSHFW', [], [Prefixes(require66H = True), Byte(0x9c)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('PUSHFQ', [], [Prefixes(), Byte(0x9c)], set(['X64']), 32, [('RSP', [0x2800])]),

	insn_desc('POPF', [], [Prefixes(), Byte(0x9d)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('POPFW', [], [Prefixes(require66H = True), Byte(0x9d)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('POPFQ', [], [Prefixes(), Byte(0x9d)], set(['X64']), 32, [('RSP', [0x2800])]),

	insn_desc('SAHF', [], [Prefixes(), Byte(0x9e)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('LAHF', [], [Prefixes(), Byte(0x9f)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('mem_offs', 64)], [Prefixes(), Byte(0xa0), Imm(8)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('mem_offs', 64)], [Prefixes(require66H = True), Byte(0xa1), Imm(8)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('mem_offs', 64)], [Prefixes(), Byte(0xa1), Imm(8)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('mem_offs', 64)], [Prefixes(requireREX = True), Byte(0xa1), Imm(8)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('mem_offs', 64)], [Prefixes(), Byte(0xa2), Imm(8)], set(['8086', 'NOHLE']), 8, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('mem_offs', 64)], [Prefixes(require66H = True), Byte(0xa3), Imm(8)], set(['8086', 'NOHLE', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('mem_offs', 64)], [Prefixes(), Byte(0xa3), Imm(8)], set(['NOHLE', '386']), 32, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('mem_offs', 64)], [Prefixes(requireREX = True), Byte(0xa3), Imm(8)], set(['NOHLE', 'X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('TEST', [Op('imm', 8)], [Prefixes(), Byte(0xa8), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('TEST', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0xa9), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('TEST', [Op('imm', 32)], [Prefixes(), Byte(0xa9), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('TEST', [Op('imm', 32)], [Prefixes(requireREX = True), Byte(0xa9), Imm(4)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 8), Op('imm', 8)], [Prefixes(), BytePlusR(0xb0), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 16), Op('imm', 16)], [Prefixes(require66H = True), BytePlusR(0xb8), Imm(2)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 32), Op('imm', 32)], [Prefixes(), BytePlusR(0xb8), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('MOV', [Op('reg', 64), Op('imm', 64)], [Prefixes(requireREX = True), BytePlusR(0xb8), Imm(8)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('INT3', [], [Prefixes(), Byte(0xcc)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('INT', [Op('imm', 8)], [Prefixes(), Byte(0xcd), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('IRET', [], [Prefixes(), Byte(0xcf)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('IRETW', [], [Prefixes(require66H = True), Byte(0xcf)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('IRETD', [], [Prefixes(), Byte(0xcf)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('IRETQ', [], [Prefixes(requireREX = True), Byte(0xcf)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('XLATB', [], [Prefixes(), Byte(0xd7)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('LOOPNE', [Op('imm', 8)], [Prefixes(), Byte(0xe0), Imm(1)], set(['8086', 'X64', '386']), 8, [('RSP', [0x2800])]),

	insn_desc('LOOPE', [Op('imm', 8)], [Prefixes(), Byte(0xe1), Imm(1)], set(['8086', 'X64', '386']), 8, [('RSP', [0x2800])]),

	insn_desc('LOOP', [Op('imm', 8)], [Prefixes(), Byte(0xe2), Imm(1)], set(['8086', 'X64', '386']), 8, [('RSP', [0x2800])]),

	insn_desc('JECXZ', [Op('imm', 8)], [Prefixes(), Byte(0xe3), Imm(1)], set(['X64', '386']), 8, [('RSP', [0x2800])]),

	insn_desc('IN', [Op('imm', 8)], [Prefixes(), Byte(0xe4), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('IN', [Op('imm', 8)], [Prefixes(require66H = True), Byte(0xe5), Imm(1)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('IN', [Op('imm', 8)], [Prefixes(), Byte(0xe5), Imm(1)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('OUT', [Op('imm', 8)], [Prefixes(), Byte(0xe6), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('OUT', [Op('imm', 8)], [Prefixes(require66H = True), Byte(0xe7), Imm(1)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('OUT', [Op('imm', 8)], [Prefixes(), Byte(0xe7), Imm(1)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('CALL', [Op('imm', 32)], [Prefixes(), Byte(0xe8), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('JMP', [Op('imm', 32)], [Prefixes(), Byte(0xe9), Imm(4)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('JMP', [Op('imm', 32)], [Prefixes(), Byte(0xe9), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('JMP', [Op('imm', 8)], [Prefixes(), Byte(0xeb), Imm(1)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('IN', [], [Prefixes(), Byte(0xec)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('IN', [], [Prefixes(require66H = True), Byte(0xed)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('IN', [], [Prefixes(), Byte(0xed)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('OUT', [], [Prefixes(), Byte(0xee)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('OUT', [], [Prefixes(require66H = True), Byte(0xef)], set(['8086', 'OSO']), 16, [('RSP', [0x2800])]),

	insn_desc('OUT', [], [Prefixes(), Byte(0xef)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('CMC', [], [Prefixes(), Byte(0xf5)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('CLC', [], [Prefixes(), Byte(0xf8)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('STC', [], [Prefixes(), Byte(0xf9)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('CLI', [], [Prefixes(), Byte(0xfa)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('CLD', [], [Prefixes(), Byte(0xfc)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('STD', [], [Prefixes(), Byte(0xfd)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('LAR', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0x2), ModRM()], set(['OSO', 'PROT', '286']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('LAR', [Op('reg', 32), Op('rm', 16)], [Prefixes(), Byte(0xf), Byte(0x2), ModRM()], set(['PROT', '386']), 32, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('LAR', [Op('reg', 64), Op('rm', 16)], [Prefixes(requireREX = True), Byte(0xf), Byte(0x2), ModRM()], set(['REX.W', 'PROT', 'X64']), 64, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('LSL', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0x3), ModRM()], set(['OSO', 'PROT', '286']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('LSL', [Op('reg', 32), Op('rm', 16)], [Prefixes(), Byte(0xf), Byte(0x3), ModRM()], set(['PROT', '386']), 32, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('LSL', [Op('reg', 64), Op('rm', 16)], [Prefixes(requireREX = True), Byte(0xf), Byte(0x3), ModRM()], set(['REX.W', 'PROT', 'X64']), 64, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('CMOVcc', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf), BytePlusC(0x40), ModRM()], set(['P6', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('CMOVcc', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0xf), BytePlusC(0x40), ModRM()], set(['P6']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('CMOVcc', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf), BytePlusC(0x40), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('Jcc', [Op('imm', 16)], [Prefixes(require66H = True), Byte(0xf), BytePlusC(0x80), Imm(4)], set(['OSO', '386']), 16, [('RSP', [0x2800])]),

	insn_desc('Jcc', [Op('imm', 32)], [Prefixes(), Byte(0xf), BytePlusC(0x80), Imm(4)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('SHLD', [Op('rm', 16), Op('reg', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0xf), Byte(0xa4), ModRM(), Imm(1)], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHLD', [Op('rm', 32), Op('reg', 32), Op('imm', 8)], [Prefixes(), Byte(0xf), Byte(0xa4), ModRM(), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHLD', [Op('rm', 64), Op('reg', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xa4), ModRM(), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SHLD', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0xa5), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHLD', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0xf), Byte(0xa5), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHLD', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xa5), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SHRD', [Op('rm', 16), Op('reg', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0xf), Byte(0xac), ModRM(), Imm(1)], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHRD', [Op('rm', 32), Op('reg', 32), Op('imm', 8)], [Prefixes(), Byte(0xf), Byte(0xac), ModRM(), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHRD', [Op('rm', 64), Op('reg', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xac), ModRM(), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SHRD', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0xad), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHRD', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0xf), Byte(0xad), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHRD', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xad), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0xaf), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0xf), Byte(0xaf), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xaf), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('LSS', [Op('reg', 16), Op('mem', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0xb2), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('LSS', [Op('reg', 32), Op('mem', 32)], [Prefixes(), Byte(0xf), Byte(0xb2), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('LSS', [Op('reg', 64), Op('mem', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xb2), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('LFS', [Op('reg', 16), Op('mem', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0xb4), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('LFS', [Op('reg', 32), Op('mem', 32)], [Prefixes(), Byte(0xf), Byte(0xb4), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('LFS', [Op('reg', 64), Op('mem', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xb4), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('LGS', [Op('reg', 16), Op('mem', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0xb5), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('LGS', [Op('reg', 32), Op('mem', 32)], [Prefixes(), Byte(0xf), Byte(0xb5), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('LGS', [Op('reg', 64), Op('mem', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xb5), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('MOVZX', [Op('reg', 16), Op('rm', 8)], [Prefixes(require66H = True), Byte(0xf), Byte(0xb6), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('MOVZX', [Op('reg', 32), Op('rm', 8)], [Prefixes(), Byte(0xf), Byte(0xb6), ModRM()], set(['386']), 32, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('MOVZX', [Op('reg', 64), Op('rm', 8)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xb6), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('MOVZX', [Op('reg', 32), Op('rm', 16)], [Prefixes(), Byte(0xf), Byte(0xb7), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('MOVZX', [Op('reg', 64), Op('rm', 16)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xb7), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('BSF', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0xbc), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('BSF', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0xf), Byte(0xbc), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('BSF', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xbc), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('BSR', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0xbd), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('BSR', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0xf), Byte(0xbd), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('BSR', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xbd), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('MOVSX', [Op('reg', 16), Op('rm', 8)], [Prefixes(require66H = True), Byte(0xf), Byte(0xbe), ModRM()], set(['OSO', '386']), 16, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('MOVSX', [Op('reg', 32), Op('rm', 8)], [Prefixes(), Byte(0xf), Byte(0xbe), ModRM()], set(['386']), 32, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('MOVSX', [Op('reg', 64), Op('rm', 8)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xbe), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('MOVSX', [Op('reg', 32), Op('rm', 16)], [Prefixes(), Byte(0xf), Byte(0xbf), ModRM()], set(['386']), 32, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('MOVSX', [Op('reg', 64), Op('rm', 16)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xbf), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('XADD', [Op('rm', 8), Op('reg', 8)], [Prefixes(), Byte(0xf), Byte(0xc0), ModRM()], set(['486']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('XADD', [Op('rm', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0xc1), ModRM()], set(['486', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('XADD', [Op('rm', 32), Op('reg', 32)], [Prefixes(), Byte(0xf), Byte(0xc1), ModRM()], set(['486']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('XADD', [Op('rm', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0xc1), ModRM()], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('BSWAP', [Op('reg', 32)], [Prefixes(), Byte(0xf), BytePlusR(0xc8)], set(['486']), 32, [('RSP', [0x2800])]),

	insn_desc('BSWAP', [Op('reg', 64)], [Prefixes(requireREX = True), Byte(0xf), BytePlusR(0xc8)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 8), Op('imm', 8)], [Prefixes(), Byte(0x80), ModRM(0), Imm(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 8), Op('imm', 8)], [Prefixes(), Byte(0x80), ModRM(1), Imm(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 8), Op('imm', 8)], [Prefixes(), Byte(0x80), ModRM(2), Imm(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 8), Op('imm', 8)], [Prefixes(), Byte(0x80), ModRM(3), Imm(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 8), Op('imm', 8)], [Prefixes(), Byte(0x80), ModRM(4), Imm(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 8), Op('imm', 8)], [Prefixes(), Byte(0x80), ModRM(5), Imm(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 8), Op('imm', 8)], [Prefixes(), Byte(0x80), ModRM(6), Imm(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 8), Op('imm', 8)], [Prefixes(), Byte(0x80), ModRM(7), Imm(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0x81), ModRM(0), Imm(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0x81), ModRM(0), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x81), ModRM(0), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0x81), ModRM(1), Imm(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0x81), ModRM(1), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x81), ModRM(1), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0x81), ModRM(2), Imm(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0x81), ModRM(2), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x81), ModRM(2), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0x81), ModRM(3), Imm(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0x81), ModRM(3), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x81), ModRM(3), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0x81), ModRM(4), Imm(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0x81), ModRM(4), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x81), ModRM(4), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0x81), ModRM(5), Imm(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0x81), ModRM(5), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x81), ModRM(5), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0x81), ModRM(6), Imm(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0x81), ModRM(6), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x81), ModRM(6), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0x81), ModRM(7), Imm(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0x81), ModRM(7), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0x81), ModRM(7), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0x83), ModRM(0), Imm(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0x83), ModRM(0), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ADD', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0x83), ModRM(0), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0x83), ModRM(1), Imm(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0x83), ModRM(1), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('OR', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0x83), ModRM(1), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0x83), ModRM(2), Imm(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0x83), ModRM(2), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ADC', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0x83), ModRM(2), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0x83), ModRM(3), Imm(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0x83), ModRM(3), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SBB', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0x83), ModRM(3), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0x83), ModRM(4), Imm(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0x83), ModRM(4), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('AND', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0x83), ModRM(4), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0x83), ModRM(5), Imm(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0x83), ModRM(5), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SUB', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0x83), ModRM(5), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0x83), ModRM(6), Imm(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0x83), ModRM(6), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('XOR', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0x83), ModRM(6), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0x83), ModRM(7), Imm(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0x83), ModRM(7), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('CMP', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0x83), ModRM(7), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('POP', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0x8f), ModRM(0)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('POP', [Op('rm', 64)], [Prefixes(), Byte(0x8f), ModRM(0)], set(['X64']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0xc1), ModRM(4), Imm(1)], set(['186', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0xc1), ModRM(4), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0xc1), ModRM(4), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0xc1), ModRM(5), Imm(1)], set(['186', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0xc1), ModRM(5), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0xc1), ModRM(5), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 16), Op('imm', 8)], [Prefixes(require66H = True), Byte(0xc1), ModRM(7), Imm(1)], set(['186', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 32), Op('imm', 8)], [Prefixes(), Byte(0xc1), ModRM(7), Imm(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 64), Op('imm', 8)], [Prefixes(requireREX = True), Byte(0xc1), ModRM(7), Imm(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ROL', [Op('rm', 8)], [Prefixes(), Byte(0xd0), ModRM(0)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ROR', [Op('rm', 8)], [Prefixes(), Byte(0xd0), ModRM(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('RCL', [Op('rm', 8)], [Prefixes(), Byte(0xd0), ModRM(2)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('RCR', [Op('rm', 8)], [Prefixes(), Byte(0xd0), ModRM(3)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 8)], [Prefixes(), Byte(0xd0), ModRM(4)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 8)], [Prefixes(), Byte(0xd0), ModRM(5)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 8)], [Prefixes(), Byte(0xd0), ModRM(7)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ROL', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd1), ModRM(0)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ROL', [Op('rm', 32)], [Prefixes(), Byte(0xd1), ModRM(0)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ROL', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd1), ModRM(0)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ROR', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd1), ModRM(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ROR', [Op('rm', 32)], [Prefixes(), Byte(0xd1), ModRM(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ROR', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd1), ModRM(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('RCL', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd1), ModRM(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('RCL', [Op('rm', 32)], [Prefixes(), Byte(0xd1), ModRM(2)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('RCL', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd1), ModRM(2)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('RCR', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd1), ModRM(3)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('RCR', [Op('rm', 32)], [Prefixes(), Byte(0xd1), ModRM(3)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('RCR', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd1), ModRM(3)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd1), ModRM(4)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 32)], [Prefixes(), Byte(0xd1), ModRM(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd1), ModRM(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd1), ModRM(5)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 32)], [Prefixes(), Byte(0xd1), ModRM(5)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd1), ModRM(5)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd1), ModRM(7)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 32)], [Prefixes(), Byte(0xd1), ModRM(7)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd1), ModRM(7)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ROL', [Op('rm', 8)], [Prefixes(), Byte(0xd2), ModRM(0)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ROR', [Op('rm', 8)], [Prefixes(), Byte(0xd2), ModRM(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('RCL', [Op('rm', 8)], [Prefixes(), Byte(0xd2), ModRM(2)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('RCR', [Op('rm', 8)], [Prefixes(), Byte(0xd2), ModRM(3)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 8)], [Prefixes(), Byte(0xd2), ModRM(4)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 8)], [Prefixes(), Byte(0xd2), ModRM(5)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 8)], [Prefixes(), Byte(0xd2), ModRM(7)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('ROL', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd3), ModRM(0)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ROL', [Op('rm', 32)], [Prefixes(), Byte(0xd3), ModRM(0)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ROL', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd3), ModRM(0)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('ROR', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd3), ModRM(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('ROR', [Op('rm', 32)], [Prefixes(), Byte(0xd3), ModRM(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('ROR', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd3), ModRM(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('RCL', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd3), ModRM(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('RCL', [Op('rm', 32)], [Prefixes(), Byte(0xd3), ModRM(2)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('RCL', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd3), ModRM(2)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('RCR', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd3), ModRM(3)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('RCR', [Op('rm', 32)], [Prefixes(), Byte(0xd3), ModRM(3)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('RCR', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd3), ModRM(3)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd3), ModRM(4)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 32)], [Prefixes(), Byte(0xd3), ModRM(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHL', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd3), ModRM(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd3), ModRM(5)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 32)], [Prefixes(), Byte(0xd3), ModRM(5)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SHR', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd3), ModRM(5)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xd3), ModRM(7)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 32)], [Prefixes(), Byte(0xd3), ModRM(7)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('SAR', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xd3), ModRM(7)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('PAUSE', [], [Prefixes(), Byte(0xf3), Byte(0x90)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('TEST', [Op('rm', 8), Op('imm', 8)], [Prefixes(), Byte(0xf6), ModRM(0), Imm(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('NOT', [Op('rm', 8)], [Prefixes(), Byte(0xf6), ModRM(2)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('NEG', [Op('rm', 8)], [Prefixes(), Byte(0xf6), ModRM(3)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('MUL', [Op('rm', 8)], [Prefixes(), Byte(0xf6), ModRM(4)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('rm', 8)], [Prefixes(), Byte(0xf6), ModRM(5)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('DIV', [Op('rm', 8)], [Prefixes(), Byte(0xf6), ModRM(6)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('IDIV', [Op('rm', 8)], [Prefixes(), Byte(0xf6), ModRM(7)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('TEST', [Op('rm', 16), Op('imm', 16)], [Prefixes(require66H = True), Byte(0xf7), ModRM(0), Imm(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('TEST', [Op('rm', 32), Op('imm', 32)], [Prefixes(), Byte(0xf7), ModRM(0), Imm(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('TEST', [Op('rm', 64), Op('imm', 32)], [Prefixes(requireREX = True), Byte(0xf7), ModRM(0), Imm(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('NOT', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf7), ModRM(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('NOT', [Op('rm', 32)], [Prefixes(), Byte(0xf7), ModRM(2)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('NOT', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf7), ModRM(2)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('NEG', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf7), ModRM(3)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('NEG', [Op('rm', 32)], [Prefixes(), Byte(0xf7), ModRM(3)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('NEG', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf7), ModRM(3)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('MUL', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf7), ModRM(4)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('MUL', [Op('rm', 32)], [Prefixes(), Byte(0xf7), ModRM(4)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('MUL', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf7), ModRM(4)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf7), ModRM(5)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('rm', 32)], [Prefixes(), Byte(0xf7), ModRM(5)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('IMUL', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf7), ModRM(5)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('DIV', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf7), ModRM(6)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('DIV', [Op('rm', 32)], [Prefixes(), Byte(0xf7), ModRM(6)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('DIV', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf7), ModRM(6)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('IDIV', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf7), ModRM(7)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('IDIV', [Op('rm', 32)], [Prefixes(), Byte(0xf7), ModRM(7)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('IDIV', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf7), ModRM(7)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('INC', [Op('rm', 8)], [Prefixes(), Byte(0xfe), ModRM(0)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('DEC', [Op('rm', 8)], [Prefixes(), Byte(0xfe), ModRM(1)], set(['8086']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('INC', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xff), ModRM(0)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('INC', [Op('rm', 32)], [Prefixes(), Byte(0xff), ModRM(0)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('INC', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xff), ModRM(0)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('DEC', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xff), ModRM(1)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('DEC', [Op('rm', 32)], [Prefixes(), Byte(0xff), ModRM(1)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('DEC', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xff), ModRM(1)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('CALL', [Op('mem', 0)], [Prefixes(), Byte(0xff), ModRM(2)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('CALL', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xff), ModRM(2)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('CALL', [Op('rm', 64)], [Prefixes(), Byte(0xff), ModRM(2)], set(['X64']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('CALL', [Op('mem', 16)], [Prefixes(require66H = True), Byte(0xff), ModRM(3)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('CALL', [Op('mem', 32)], [Prefixes(), Byte(0xff), ModRM(3)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('CALL', [Op('mem', 64)], [Prefixes(requireREX = True), Byte(0xff), ModRM(3)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('JMP', [Op('mem', 0)], [Prefixes(), Byte(0xff), ModRM(4)], set(['8086']), 8, [('RSP', [0x2800])]),

	insn_desc('JMP', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xff), ModRM(4)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('JMP', [Op('rm', 64)], [Prefixes(), Byte(0xff), ModRM(4)], set(['X64']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('JMP', [Op('mem', 16)], [Prefixes(require66H = True), Byte(0xff), ModRM(5)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('JMP', [Op('mem', 32)], [Prefixes(), Byte(0xff), ModRM(5)], set(['386']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('JMP', [Op('mem', 64)], [Prefixes(requireREX = True), Byte(0xff), ModRM(5)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('PUSH', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xff), ModRM(6)], set(['8086', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('PUSH', [Op('rm', 64)], [Prefixes(), Byte(0xff), ModRM(6)], set(['X64']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SLDT', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0x0), ModRM(0)], set(['OSO', '286']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('SLDT', [Op('reg', 32)], [Prefixes(), Byte(0xf), Byte(0x0), ModRM(0)], set(['386']), 32, [('RSP', [0x2800])]),

	insn_desc('SLDT', [Op('reg', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0x0), ModRM(0)], set(['X64', 'REX.W']), 64, [('RSP', [0x2800])]),

	insn_desc('VERR', [Op('rm', 16)], [Prefixes(), Byte(0xf), Byte(0x0), ModRM(4)], set(['PROT', '286']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('VERW', [Op('rm', 16)], [Prefixes(), Byte(0xf), Byte(0x0), ModRM(5)], set(['PROT', '286']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('NOP', [Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0x1f), ModRM(0)], set(['P6', 'OSO']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('NOP', [Op('rm', 32)], [Prefixes(), Byte(0xf), Byte(0x1f), ModRM(0)], set(['P6']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('NOP', [Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0x1f), ModRM(0)], set(['X64', 'REX.W']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('MOVBE', [Op('reg', 16), Op('mem', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0x38), Byte(0xf0), ModRM()], set(['OSO', 'NEHALEM']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('MOVBE', [Op('reg', 32), Op('mem', 32)], [Prefixes(), Byte(0xf), Byte(0x38), Byte(0xf0), ModRM()], set(['NEHALEM']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('MOVBE', [Op('reg', 64), Op('mem', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0x38), Byte(0xf0), ModRM()], set(['REX.W', 'NEHALEM']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('MOVBE', [Op('mem', 16), Op('reg', 16)], [Prefixes(require66H = True), Byte(0xf), Byte(0x38), Byte(0xf1), ModRM()], set(['OSO', 'NEHALEM']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('MOVBE', [Op('mem', 32), Op('reg', 32)], [Prefixes(), Byte(0xf), Byte(0x38), Byte(0xf1), ModRM()], set(['NEHALEM']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('MOVBE', [Op('mem', 64), Op('reg', 64)], [Prefixes(requireREX = True), Byte(0xf), Byte(0x38), Byte(0xf1), ModRM()], set(['REX.W', 'NEHALEM']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])]),

	insn_desc('SETcc', [Op('rm', 8)], [Prefixes(), Byte(0xf), BytePlusC(0x90), ModRM(0)], set(['386']), 8, [('RBX', [0x3000]), (0x0, [0x0, 0x1, 0x80, 0x81]), ('RSP', [0x2800])]),

	insn_desc('POPCNT', [Op('reg', 16), Op('rm', 16)], [Prefixes(require66H = True), Byte(0xf3), Byte(0xf), Byte(0xb8), ModRM()], set(['OSO', 'NEHALEM']), 16, [('RBX', [0x3000, 0x3fff]), (0x0, [0x0, 0x1, 0x8000, 0x8001]), ('RSP', [0x2800])]),

	insn_desc('POPCNT', [Op('reg', 32), Op('rm', 32)], [Prefixes(), Byte(0xf3), Byte(0xf), Byte(0xb8), ModRM()], set(['NEHALEM']), 32, [('RBX', [0x3000, 0x3ffe]), (0x0, [0x0, 0x1, 0x80000000, 0x80000001]), ('RSP', [0x2800])]),

	insn_desc('POPCNT', [Op('reg', 64), Op('rm', 64)], [Prefixes(requireREX = True), Byte(0xf3), Byte(0xf), Byte(0xb8), ModRM()], set(['REX.W', 'X64', 'NEHALEM']), 64, [('RBX', [0x3000, 0x3ffc]), (0x0, [0x0, 0x1, 0x8000000000000000L, 0x8000000000000001L]), ('RSP', [0x2800])])]
