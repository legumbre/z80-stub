SDCC_FLAGS = -V -c -D TARGET_Z80 -mz80 --no-std-crt0 --stack-auto
SDCC_LD_FLAGS = -V -mz80 --no-peep --no-std-crt0 --data-loc 0x8000 --stack-auto

CRT0_TMPS = crt0.sym crt0.lst crt0.lnk crt0.map
Z80STUB_TMPS = z80-stub.asm z80-stub.sym z80-stub.lst

all: monitor-z80

monitor-z80: crt0.o z80-stub.o
	sdcc ${SDCC_LD_FLAGS} crt0.o z80-stub.o
	cp crt0.ihx monitor.hex

crt0.o: crt0.s
	as-z80 -gols crt0.o crt0.s

z80-stub.o: z80-stub.c
	sdcc ${SDCC_FLAGS} z80-stub.c -o z80-stub.o

monitor-qemu: crt0.o z80-stub.o

clean:
	rm -f crt0.ihx crt0.o z80-stub.o monitor.hex ${CRT0_TMPS} ${Z80STUB_TMPS}


