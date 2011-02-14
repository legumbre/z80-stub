	;; gdb monitor crt0.s for a Z80
        .module crt0
       	.globl	_main
        .globl  _sr

	.area	_HEADER (ABS)
	;; Reset vector
	.org 	0
	jp	init

	.org	0x08
        push  hl               ;; RST 08, used for breakpoints
        ld    hl,#08           ;; 08 means a breakpoint was hit, pass it to sr routine
        jp    _sr              ;; TODO maybe change to saveRegisters, semantics

	.org	0x100
init:
	;; Stack at the top of memory.
	ld	sp,#0xffff

        ;; Initialise global variables
        call    gsinit
	call	_main
	jp	_exit

	;; Ordering of segments for the linker.
	.area	_HOME
	.area	_CODE
        .area   _GSINIT
        .area   _GSFINAL

	.area	_DATA
        .area   _BSS
        .area   _HEAP

        .area   _CODE
__clock::
	ld	a,#2
        rst     0x08
	ret

_exit::
	;; Exit - special code to the emulator
	ld	a,#0
        rst     0x08
1$:
	halt
	jr	1$

        .area   _GSINIT
gsinit::
	
        .area   _GSFINAL
        ret
        