#!/bin/sh
rm -f crt0.o z80-stub.o

as-z80 -gols crt0.o crt0.s && \
sdcc -V -c -D TARGET_QEMU -mz80 --no-std-crt0 --stack-auto z80-stub.c -o z80-stub.o && \
sdcc -V -mz80 --no-peep --no-std-crt0 --data-loc 0x8000 --stack-auto crt0.o z80-stub.o && \
srec_cat crt0.ihx -Intel -output z80stub.bin -Binary && \
cat z80stub.bin /dev/zero | dd bs=1k count=16 > /opt/qemu-z80/share/qemu/zx-rom.bin

# sdcc -V -D TARGET_QEMU -mz80 --no-peep --no-std-crt0 --stack-auto crt0.o z80-stub.c -o z80-stub && \
# srec_cat z80-stub -Intel -output z80-stub.bin -Binary && \
# cat z80-stub.bin /dev/zero | dd bs=1k count=16 > /opt/qemu-z80/share/qemu/zx-rom.bin

# sdcc -V -D TARGET_QEMU -mz80 --no-peep --no-std-crt0 --stack-auto crt0.o z80-stub.c -o z80-stub && \
# srec_cat z80-stub -Intel -output z80-stub.bin -Binary && \
# cat z80-stub.bin /dev/zero | dd bs=1k count=16 > /opt/qemu-z80/share/qemu/zx-rom.bin

# sdcc --no-std-crt0 -D TARGET_Z80 -mz80 --no-peep --stack-auto --code-loc 0x0000 --data-loc 0x9000 z80-stub.c -o z80-stub && \
# srec_cat z80-stub -Intel -output z80-stub.bin -Binary && \
# cat z80-stub.bin /dev/zero | dd bs=1k count=16 > /opt/qemu-z80/share/qemu/zx-rom.bin
