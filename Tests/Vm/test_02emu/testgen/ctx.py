import sys

def gen_ctx_map(f):
	fpu_off = 32*8
	seg_off = 20*8
	dr_off = 24*8
	xstate_bv_off = fpu_off + 512
	avx_off = xstate_bv_off + 64

	ctx_map_raw = [ ('rax',  0*8, 8), ('rbx',  1*8, 8), ('rcx',  2*8, 8), ('rdx',  3*8, 8),
					('rsi',  4*8, 8), ('rdi',  5*8, 8), ('rsp',  6*8, 8), ('rbp',  7*8, 8),
					('r8',   8*8, 8), ('r9',   9*8, 8), ('r10', 10*8, 8), ('r11', 11*8, 8),
					('r12', 12*8, 8), ('r13', 13*8, 8), ('r14', 14*8, 8), ('r15', 15*8, 8),

					('vector', 16*8, 8),

					('cr0', 17*8, 8), ('cr2', 17*8, 8),

					('rflags_mask', 18*8, 8),

					('xcr0', 19*8, 8),

					('cs_base', seg_off,      4), ('cs_limit', seg_off + 4,  4),
					('ss_base', seg_off + 8,  4), ('ss_limit', seg_off + 12, 4),
					('ds_base', seg_off + 16, 4), ('ds_limit', seg_off + 20, 4),

					('dr0', dr_off,      8), ('dr1', dr_off +  8, 8), ('dr2', dr_off + 16, 8), ('dr3', dr_off + 24, 8),
					('dr6', dr_off + 32, 8), ('dr7', dr_off + 40, 8),

					('fcw',      fpu_off,       2), ('fsw',        fpu_off + 2,   2), ('ftw', fpu_off + 4, 2),
					('mxcsr',    fpu_off + 24,  4), ('mxcsr_mask', fpu_off + 28,  4),
					('mm0',      fpu_off + 32,  8),
					('xmm0',     fpu_off + 160, 8),
					('xstatebv', xstate_bv_off, 8),
					('ymmh0',    avx_off, 8 )]

	return map(f, ctx_map_raw)



if __name__ == '__main__':
	out = open(sys.argv[1], 'w')
	m = gen_ctx_map(lambda x: x)
	max_field_len = max(map(lambda x: len(x[0]), m)) + 4
	for f in m:
		out.write("%%define %s %s\n" % ((f[0] + '_OFF').upper().ljust(max_field_len), f[1]))
	out.close()
