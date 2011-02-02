#!/bin/sh
sdcc --no-std-crt0 -mz80 --no-peep --stack-auto --code-loc 0x0000 --data-loc 0x9000 z80-stub.c -o z80-stub && \
srec_cat z80-stub -Intel -output z80-stub.bin -Binary && \
cat z80-stub.bin /dev/zero | dd bs=1k count=16 > /opt/qemu-z80/share/qemu/zx-rom.bin
