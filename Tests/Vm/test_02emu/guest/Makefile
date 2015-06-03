BOOTSECT := bootsect_pm32-64


all: boot-64.iso

ctxmap.inc: ../testgen/ctx.py
	python ../testgen/ctx.py $@

state.inc: ctxmap.inc

boot-64: boot.asm $(BOOTSECT).asm common.inc eltorito.inc config.inc state.inc
	nasm -DTEST_64B_MODE -o bootstrap.bin $(BOOTSECT).asm
	nasm -o $@ boot.asm
	rm bootstrap.bin

boot-%.iso: boot-%
	python ../../vmtest/mkbootiso.py $@ $<

clean:
	rm -f ctxmap.inc
	rm -f boot-*

.PHONY: clean
