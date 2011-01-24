/* z80-stub.c -- debugging stub for the Z80

   This stub is based on the SH stub by Ben Lee and Steve Chamberlain. 

   Modifications for the Z80 by Leonardo Etcheverry <letcheve@fing.edu.uy>, 2010.
*/

/*
  # compile command
  sdcc  -mz80 --model-large --no-peep --stack-auto --code-loc 0x0000 --stack-loc 0xF000 z80-stub.c -o z80-stub.out

sdcc --no-std-crt0 -mz80 --model-large --no-peep --stack-auto --code-loc 0x0000 --data-loc 0x9000 z80-stub.c -o z80-stub
srec_cat z80-stub.hex -Intel -output z80-stub.bin -Binary


*/


/* Remote communication protocol.

   A debug packet whose contents are <data>
   is encapsulated for transmission in the form:

        $ <data> # CSUM1 CSUM2

        <data> must be ASCII alphanumeric and cannot include characters
        '$' or '#'.  If <data> starts with two characters followed by
        ':', then the existing stubs interpret this as a sequence number.

        CSUM1 and CSUM2 are ascii hex representation of an 8-bit 
        checksum of <data>, the most significant nibble is sent first.
        the hex digits 0-9,a-f are used.

   Receiver responds with:

        +       - if CSUM is correct and ready for next packet
        -       - if CSUM is incorrect

   <data> is as follows:
   All values are encoded in ascii hex digits.

        Request         Packet

        read registers  g
        reply           XX....X         Each byte of register data
                                        is described by two hex digits.
                                        Registers are in the internal order
                                        for GDB, and the bytes in a register
                                        are in the same order the machine uses.
                        or ENN          for an error.

        write regs      GXX..XX         Each byte of register data
                                        is described by two hex digits.
        reply           OK              for success
                        ENN             for an error

        write reg       Pn...=r...      Write register n... with value r...,
                                        which contains two hex digits for each
                                        byte in the register (target byte
                                        order).
        reply           OK              for success
                        ENN             for an error
        (not supported by all stubs).

        read mem        mAA..AA,LLLL    AA..AA is address, LLLL is length.
        reply           XX..XX          XX..XX is mem contents
                                        Can be fewer bytes than requested
                                        if able to read only part of the data.
                        or ENN          NN is errno

        write mem       MAA..AA,LLLL:XX..XX
                                        AA..AA is address,
                                        LLLL is number of bytes,
                                        XX..XX is data
        reply           OK              for success
                        ENN             for an error (this includes the case
                                        where only part of the data was
                                        written).

        cont            cAA..AA         AA..AA is address to resume
                                        If AA..AA is omitted,
                                        resume at same address.

        step            sAA..AA         AA..AA is address to resume
                                        If AA..AA is omitted,
                                        resume at same address.

        last signal     ?               Reply the current reason for stopping.
                                        This is the same reply as is generated
                                        for step or cont : SAA where AA is the
                                        signal number.

        There is no immediate reply to step or cont.
        The reply comes when the machine stops.
        It is           SAA             AA is the "signal number"

        or...           TAAn...:r...;n:r...;n...:r...;
                                        AA = signal number
                                        n... = register number
                                        r... = register contents
        or...           WAA             The process exited, and AA is
                                        the exit status.  This is only
                                        applicable for certains sorts of
                                        targets.
        kill request    k

        toggle debug    d               toggle debug flag (see 386 & 68k stubs)
        reset           r               reset -- see sparc stub.
        reserved        <other>         On other requests, the stub should
                                        ignore the request and send an empty
                                        response ($#<checksum>).  This way
                                        we can extend the protocol and GDB
                                        can tell whether the stub it is
                                        talking to uses the old or the new.
        search          tAA:PP,MM       Search backwards starting at address
                                        AA for a match with pattern PP and
                                        mask MM.  PP and MM are 4 bytes.
                                        Not supported by all stubs.

        general query   qXXXX           Request info about XXXX.
        general set     QXXXX=yyyy      Set value of XXXX to yyyy.
        query sect offs qOffsets        Get section offsets.  Reply is
                                        Text=xxx;Data=yyy;Bss=zzz
        console output  Otext           Send text to stdout.  Only comes from
                                        remote target.

        Responses can be run-length encoded to save space.  A '*' means that
        the next character is an ASCII encoding giving a repeat count which
        stands for that many repititions of the character preceding the '*'.
        The encoding is n+29, yielding a printable character where n >=3 
        (which is where rle starts to win).  Don't use an n > 126. 

        So 
        "0* " means the same as "0000".  */

#include <string.h>

/* LLL comments, de SDCC

   On the Z80 the startup code is inserted by linking with crt0.rel
   which is generated from sdcc/device/lib/z80/crt0.s. If you need a
   different startup code you can use the compiler option --no-std-crt0
   and provide your own crt0.rel.x 

   Then add ''__asm'' and ''__endasm;''3.6 to the beginning and the
   end of the function body:
*/


/* Renesas SH architecture instruction encoding masks */

#define COND_BR_MASK   0xff00
#define UCOND_DBR_MASK 0xe000
#define UCOND_RBR_MASK 0xf0df
#define TRAPA_MASK     0xff00

#define COND_DISP      0x00ff
#define UCOND_DISP     0x0fff
#define UCOND_REG      0x0f00

/* Renesas SH instruction opcodes */

#define BF_INSTR       0x8b00
#define BT_INSTR       0x8900
#define BRA_INSTR      0xa000
#define BSR_INSTR      0xb000
#define JMP_INSTR      0x402b
#define JSR_INSTR      0x400b
#define RTS_INSTR      0x000b
#define RTE_INSTR      0x002b
#define TRAPA_INSTR    0xc300
#define SSTEP_INSTR    0xc3ff

/* Z80 registers (should match the constants used in gdb  */
// TODO!: maybe include someheader.h which is also included from z80-tdep.h?



/* Z80 instruction opcodes */
#define RST08_INST     0xCF
#define BREAK_INST     RST08_INST

/* Renesas SH processor register masks */

#define T_BIT_MASK     0x0001

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound
 * buffers. At least NUMREGBYTES*2 are needed for register packets.
 */
#define BUFMAX 256

/*
 * Number of bytes for registers
 */
#define NUMREGBYTES 26

/*
 * typedef
 */
typedef void (*Function) ();

/*
 * Forward declarations
 */

static int hex (char);
static char *mem2hex (char *, char *, int);
static char *hex2mem (char *, char *, int);
static int hexToInt (char **, int *);
static char *getpacket (void);
static void putpacket (char *);
static int computeSignal (int exceptionVector);
static void handle_exception (int exceptionVector);
void init_serial();

void putDebugChar (char);
char getDebugChar (void);

/* These are in the file but in asm statements so the compiler can't see them */
void catch_exception_4 (void);
void catch_exception_6 (void);
void catch_exception_9 (void);
void catch_exception_10 (void);
void catch_exception_11 (void);
void catch_exception_32 (void);
void catch_exception_33 (void);
void catch_exception_255 (void);



#define catch_exception_random catch_exception_255 /* Treat all odd ones like 255 */

void breakpoint (void);


#define init_stack_size 1024  /* if you change this you should also modify BINIT */
#define stub_stack_size 1024

int init_stack[init_stack_size];//  __attribute__ ((section ("stack"))) = {0};
int stub_stack[stub_stack_size];//  __attribute__ ((section ("stack"))) = {0};

char useless_buffer[30];

void INIT ();
void BINIT ();

#define CPU_BUS_ERROR_VEC  9
#define DMA_BUS_ERROR_VEC 10
#define NMI_VEC           11
#define INVALID_INSN_VEC   4
#define INVALID_SLOT_VEC   6
#define TRAP_VEC          32
#define IO_VEC            33
#define USER_VEC         255


char intcause; /* TODO: initialize */
char in_nmi;   /* Set when handling an NMI, so we don't reenter */
int dofault;  /* Non zero, bus errors will raise exception */

char read_ch; /* TODO byte read from serial port, for now it's a global */

int *stub_sp;

/* debug > 0 prints ill-formed commands in valid packets & checksum errors */
int remote_debug;

enum regnames
  {
    A, F, BC, DE, HL, IX, IY, SP, I, R,
    AX, FX, BCX, DEX, HLX, PC,
    NUM_REGISTERS
  };

typedef struct
  {
    short *memAddr;
    short oldInstr;
  }
stepData;

int registers[NUM_REGISTERS];
stepData instrBuffer;
char stepped;
static const char hexchars[] = "0123456789abcdef";
static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];


void
INIT (void)
{
  __asm
    jp    _RESET
    nop
    nop
    nop
    nop
    nop
    push  af   ;; RST 08, used for breakpoints
    ld    hl,#0 ;; we hit a breakpoint, save this info since we will need it later (handle_exception)
    jp    _sr  ;; TODO maybe change to saveRegisters, semantics
  //        asm ("_BINIT: mov.l  L1,r15");
  //        asm ("bra _INIT");
  //        asm ("nop");
  //        asm ("L1: .long _init_stack + 8*1024*4");
   __endasm;
}

void
RESET (void) __naked
{
  // commented out for qemu debugging
  // init_serial();
  
#ifdef MONITOR
  reset_hook ();
#endif

  in_nmi = 0;
  dofault = 1;
  stepped = 0;

  stub_sp = stub_stack + stub_stack_size;
  breakpoint ();

  while (1)
    ;
}

char highhex(int  x)
{
  return hexchars[(x >> 4) & 0xf];
}

char lowhex(int  x)
{
  return hexchars[x & 0xf];
}

/*
 * Assembly macros
 */

#define BREAKPOINT() { __asm RST 08 __endasm; }

/*
 * Routines to handle hex data
 */

static int
hex (char ch)
{
  if ((ch >= 'a') && (ch <= 'f'))
    return (ch - 'a' + 10);
  if ((ch >= '0') && (ch <= '9'))
    return (ch - '0');
  if ((ch >= 'A') && (ch <= 'F'))
    return (ch - 'A' + 10);
  return (-1);
}

/* convert the memory, pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
static char *
mem2hex (char *mem, char *buf, int count)
{
  int i;
  int ch;
  for (i = 0; i < count; i++)
    {
      ch = *mem++;
      *buf++ = highhex (ch);
      *buf++ = lowhex (ch);
    }
  *buf = 0;
  return (buf);
}

/* convert the hex array pointed to by buf into binary, to be placed in mem */
/* return a pointer to the character after the last byte written */

static char *
hex2mem (char *buf, char *mem, int count)
{
  int i;
  unsigned char ch;
  for (i = 0; i < count; i++)
    {
      ch = hex (*buf++) << 4;
      ch = ch + hex (*buf++);
      *mem++ = ch;
    }
  return (mem);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
static int
hexToInt (char **ptr, int *intValue)
{
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr)
    {
      hexValue = hex (**ptr);
      if (hexValue >= 0)
        {
          *intValue = (*intValue << 4) | hexValue;
          numChars++;
        }
      else
        break;

      (*ptr)++;
    }

  return (numChars);
}

/*
 * Routines to get and put packets
 */

/* scan for the sequence $<data>#<checksum>     */

char *
getpacket (void)
{
  unsigned char *buffer = &remcomInBuffer[0];
  unsigned char checksum;
  unsigned char xmitcsum;
  int count;
  char ch;

  while (1)
    {
      /* wait around for the start character, ignore all other characters */
      while ((ch = getDebugChar ()) != '$')
        ;

retry:
      checksum = 0;
      xmitcsum = -1;
      count = 0;

      /* now, read until a # or end of buffer is found */
      while (count < BUFMAX - 1)
        {
          ch = getDebugChar ();
          if (ch == '$')
            goto retry;
          if (ch == '#')
            break;
          checksum = checksum + ch;
          buffer[count] = ch;
          count = count + 1;
        }
      buffer[count] = 0;

      if (ch == '#')
        {
          ch = getDebugChar ();
          xmitcsum = hex (ch) << 4;
          ch = getDebugChar ();
          xmitcsum += hex (ch);

          if (checksum != xmitcsum)
            {
              putDebugChar ('-');       /* failed checksum */
            }
          else
            {
              putDebugChar ('+');       /* successful transfer */

              /* if a sequence char is present, reply the sequence ID */
              if (buffer[2] == ':')
                {
                  putDebugChar (buffer[0]);
                  putDebugChar (buffer[1]);

                  return &buffer[3];
                }

              return &buffer[0];
            }
        }
    }
}


/* send the packet in buffer. */

static void
putpacket (char *buffer)
{
  int checksum;

  /*  $<packet info>#<checksum>. */
  do
    {
      char *src = buffer;
      putDebugChar ('$');
      checksum = 0;

      while (*src)
        {
          int runlen;

          /* Do run length encoding */
          for (runlen = 0; runlen < 100; runlen ++) 
            {
              if (src[0] != src[runlen]) 
                {
                  if (runlen > 3) 
                    {
                      int encode;
                      /* Got a useful amount */
                      putDebugChar (*src);
                      checksum += *src;
                      putDebugChar ('*');
                      checksum += '*';
                      checksum += (encode = runlen + ' ' - 4);
                      putDebugChar (encode);
                      src += runlen;
                    }
                  else
                    {
                      putDebugChar (*src);
                      checksum += *src;
                      src++;
                    }
                  break;
                }
            }
        }


      putDebugChar ('#');
      putDebugChar (highhex(checksum));
      putDebugChar (lowhex(checksum));
    }
  while  (getDebugChar() != '+');
}


/*
 * this function takes the SH-1 exception number and attempts to
 * translate this number into a unix compatible signal value
 */
static int
computeSignal (int exceptionVector)
{
  int sigval;
  switch (exceptionVector)
    {
    case INVALID_INSN_VEC:
      sigval = 4;
      break;                    
    case INVALID_SLOT_VEC:
      sigval = 4;
      break;                    
    case CPU_BUS_ERROR_VEC:
      sigval = 10;
      break;                    
    case DMA_BUS_ERROR_VEC:
      sigval = 10;
      break;    
    case NMI_VEC:
      sigval = 2;
      break;    

    case TRAP_VEC:
    case USER_VEC:
      sigval = 5;
      break;

    default:
      sigval = 7;               /* "software generated"*/
      break;
    }
  return (sigval);
}

void
doSStep (void)
{
  short *instrMem;
  int displacement;
  int reg;
  unsigned short opcode;

  instrMem = (short *) registers[PC];

  opcode = *instrMem;
  stepped = 1;

  if ((opcode & COND_BR_MASK) == BT_INSTR)
    {
      if (1) // if (registers[SR] & T_BIT_MASK)
        {
          displacement = (opcode & COND_DISP) << 1;
          if (displacement & 0x80)
            displacement |= 0xffffff00;
          /*
                   * Remember PC points to second instr.
                   * after PC of branch ... so add 4
                   */
          instrMem = (short *) (registers[PC] + displacement + 4);
        }
      else
        instrMem += 1;
    }
  else if ((opcode & COND_BR_MASK) == BF_INSTR)
    {
      if (1) // if (registers[SR] & T_BIT_MASK)
        instrMem += 1;
      else
        {
          displacement = (opcode & COND_DISP) << 1;
          if (displacement & 0x80)
            displacement |= 0xffffff00;
          /*
                   * Remember PC points to second instr.
                   * after PC of branch ... so add 4
                   */
          instrMem = (short *) (registers[PC] + displacement + 4);
        }
    }
  else if ((opcode & UCOND_DBR_MASK) == BRA_INSTR)
    {
      displacement = (opcode & UCOND_DISP) << 1;
      if (displacement & 0x0800)
        displacement |= 0xfffff000;

      /*
           * Remember PC points to second instr.
           * after PC of branch ... so add 4
           */
      instrMem = (short *) (registers[PC] + displacement + 4);
    }
  else if ((opcode & UCOND_RBR_MASK) == JSR_INSTR)
    {
      reg = (char) ((opcode & UCOND_REG) >> 8);

      instrMem = (short *) registers[reg];
    }
  else if (opcode == RTS_INSTR)
    ; // instrMem = (short *) registers[PR];
  else if (opcode == RTE_INSTR)
    instrMem = (short *) registers[15];
  else if ((opcode & TRAPA_MASK) == TRAPA_INSTR)
    instrMem = (short *) ((opcode & ~TRAPA_MASK) << 2);
  else
    instrMem += 1;

  instrBuffer.memAddr = instrMem;
  instrBuffer.oldInstr = *instrMem;
  *instrMem = SSTEP_INSTR;
}


/* Undo the effect of a previous doSStep.  If we single stepped,
   restore the old instruction. */

void
undoSStep (void)
{
  if (stepped)
    {  short *instrMem;
      instrMem = instrBuffer.memAddr;
      *instrMem = instrBuffer.oldInstr;
    }
  stepped = 0;
}

/*
This function does all exception handling.  It only does two things -
it figures out why it was called and tells gdb, and then it reacts
to gdb's requests.

When in the monitor mode we talk a human on the serial line rather than gdb.

*/


void
gdb_handle_exception (int exceptionVector)
{
  int sigval, stepping;
  int addr, length;
  char *ptr;

  /* reply to host that an exception has occurred */
  sigval = computeSignal (exceptionVector);
  remcomOutBuffer[0] = 'S';
  remcomOutBuffer[1] = highhex(sigval);
  remcomOutBuffer[2] = lowhex (sigval);
  remcomOutBuffer[3] = 0;

  putpacket (remcomOutBuffer);

  /*
   * exception 255 indicates a software trap
   * inserted in place of code ... so back up
   * PC by one instruction, since this instruction
   * will later be replaced by its original one!
   */
  if (exceptionVector == 0xff
      || exceptionVector == 0x20)
    registers[PC] -= 2;

  /*
   * Do the thangs needed to undo
   * any stepping we may have done!
   */
  undoSStep ();

  stepping = 0;

  while (1)
    {
      remcomOutBuffer[0] = 0;
      ptr = getpacket ();

      switch (*ptr++)
        {
        case '?':
          remcomOutBuffer[0] = 'S';
          remcomOutBuffer[1] = highhex (sigval);
          remcomOutBuffer[2] = lowhex (sigval);
          remcomOutBuffer[3] = 0;
          break;
        case 'd':
          remote_debug = !(remote_debug);       /* toggle debug flag */
          break;
        case 'g':               /* return the value of the CPU registers */
          mem2hex ((char *) registers, remcomOutBuffer, NUMREGBYTES);
          break;
        case 'G':               /* set the value of the CPU registers - return OK */
          hex2mem (ptr, (char *) registers, NUMREGBYTES);
          strcpy (remcomOutBuffer, "OK");
          break;

          /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
        case 'm':
          dofault = 0;
          /* TRY, TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
          if (hexToInt (&ptr, &addr))
            if (*(ptr++) == ',')
              if (hexToInt (&ptr, &length))
                {
                  ptr = 0;
                  mem2hex ((char *) addr, remcomOutBuffer, length);
                }
          if (ptr)
            strcpy (remcomOutBuffer, "E01");
          
          break;

          /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
        case 'M':
          /* TRY, TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
          if (hexToInt (&ptr, &addr))
            if (*(ptr++) == ',')
              if (hexToInt (&ptr, &length))
                if (*(ptr++) == ':')
                  {
                    hex2mem (ptr, (char *) addr, length);
                    ptr = 0;
                    strcpy (remcomOutBuffer, "OK");
                  }
          if (ptr)
            strcpy (remcomOutBuffer, "E02");

          break;

          /* cAA..AA    Continue at address AA..AA(optional) */
          /* sAA..AA   Step one instruction from AA..AA(optional) */
        case 's':
          stepping = 1;
        case 'c':
          {
            /* tRY, to read optional parameter, pc unchanged if no parm */
            if (hexToInt (&ptr, &addr))
              registers[PC] = addr;

            if (stepping)
              doSStep ();
          }
          return;
          break;

          /* kill the program */
        case 'k':               /* do nothing */
          break;
        }                       /* switch */

      /* reply to the request */
      putpacket (remcomOutBuffer);
    }
}


#define GDBCOOKIE 0x5ac 
static int ingdbmode;
void handle_exception(int exceptionVector)
{
  gdb_handle_exception (exceptionVector);
}

void
gdb_mode (void)
{
  ingdbmode = GDBCOOKIE;
  breakpoint();
}
/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */
void
breakpoint (void)
{
  BREAKPOINT ();
}

/* registers constants */
#define R_A     0
#define R_F     1

#define R_BC    2
#define R_DE    3
#define R_HL    4
#define R_IX    5
#define R_IY    6
#define R_SP    7

#define R_I     8
#define R_R     9

#define R_AX    10
#define R_FX    11
#define R_BCX   12
#define R_DEX   13
#define R_HLX   14

// TODO: fix this!
#define R_AF    15
#define R_AFX   16



static void sr() __naked
{
//          /* Calling Reset does the same as pressing the button */
//          asm (".global _Reset
//                .global _WarmReset
//        _Reset:
//        _WarmReset:
//                 mov.l L_sp,r15
//                 bra   _INIT
//                 nop
//                 .align 2
//        L_sp:    .long _init_stack + 8000");

  /* saveRegisters routine */
 saveRegisters:
  __asm
    ld    (#_intcause), hl        ;; routine argument exception cause
    pop   af                      ;; recover original af

    ld    (#_registers + R_HL), hl
    push  af ;; dirty trick to save AF
    pop   hl
    ld    (#_registers + R_AF), hl
                                   
    ld    (#_registers + R_BC), bc
    ld    (#_registers + R_DE), de
    ld    (#_registers + R_IX), ix
    ld    (#_registers + R_IY), iy


    ;; alternate register set
    exx
    ld    (#_registers + R_HLX), hl

    push  af ;; dirty trick to save AF
    pop   hl
    ld    (#_registers + R_AFX), hl
    ld    (#_registers + R_BCX), bc
    ld    (#_registers + R_DEX), de

    ;; switch back to original reg set
    exx             

    ld    hl, (#_intcause)
    push  hl
    call  _handle_exception              
    pop   af      

  __endasm;


//        
//          asm("saveRegisters:
//                mov.l   @(L_reg, pc), r0
//                mov.l   @r15+, r1                               ! pop R0
//                mov.l   r2, @(0x08, r0)                         ! save R2
//                mov.l   r1, @r0                                 ! save R0
//                mov.l   @r15+, r1                               ! pop R1
//                mov.l   r3, @(0x0c, r0)                         ! save R3
//                mov.l   r1, @(0x04, r0)                         ! save R1
//                mov.l   r4, @(0x10, r0)                         ! save R4
//                mov.l   r5, @(0x14, r0)                         ! save R5
//                mov.l   r6, @(0x18, r0)                         ! save R6
//                mov.l   r7, @(0x1c, r0)                         ! save R7
//                mov.l   r8, @(0x20, r0)                         ! save R8
//                mov.l   r9, @(0x24, r0)                         ! save R9
//                mov.l   r10, @(0x28, r0)                        ! save R10
//                mov.l   r11, @(0x2c, r0)                        ! save R11
//                mov.l   r12, @(0x30, r0)                        ! save R12
//                mov.l   r13, @(0x34, r0)                        ! save R13
//                mov.l   r14, @(0x38, r0)                        ! save R14
//                mov.l   @r15+, r4                               ! save arg to handleException
//                add     #8, r15                                 ! hide PC/SR values on stack
//                mov.l   r15, @(0x3c, r0)                        ! save R15
//                add     #-8, r15                                ! save still needs old SP value
//                add     #92, r0                                 ! readjust register pointer
//                mov     r15, r2
//                add     #4, r2
//                mov.l   @r2, r2                                 ! R2 has SR
//                mov.l   @r15, r1                                ! R1 has PC
//                mov.l   r2, @-r0                                ! save SR
//                sts.l   macl, @-r0                              ! save MACL
//                sts.l   mach, @-r0                              ! save MACH
//                stc.l   vbr, @-r0                               ! save VBR
//                stc.l   gbr, @-r0                               ! save GBR
//                sts.l   pr, @-r0                                ! save PR
//                mov.l   @(L_stubstack, pc), r2
//                mov.l   @(L_hdl_except, pc), r3
//                mov.l   @r2, r15
//                jsr     @r3
//                mov.l   r1, @-r0                                ! save PC
//                mov.l   @(L_stubstack, pc), r0
//                mov.l   @(L_reg, pc), r1
//                bra     restoreRegisters
//                mov.l   r15, @r0                                ! save __stub_stack
//                
//                .align 2
//        L_reg:
//                .long   _registers
//        L_stubstack:
//                .long   _stub_sp
//        L_hdl_except:
//                .long   _handle_exception");

}




static void rr() __naked
{
  __asm
    ld    hl, (#_registers+R_AF) ;; restore AF
    push  hl
    pop   af

    ld    hl, (#_registers+R_HL)
    ld    bc, (#_registers+R_BC)      
    ld    de, (#_registers+R_DE)          
    ld    ix, (#_registers+R_IX)      
    ld    iy, (#_registers+R_IY)          


    exx
    ld    hl, (#_registers + R_HLX)

    ld    hl, (#_registers + R_AFX)
    push  hl
    pop   af

    ld    bc, (#_registers+R_BC)      
    ld    de, (#_registers+R_DE)          

    ;; switch back to original reg set
    exx             
                                  
  __endasm;

//        asm("
//                .align 2        
//                .global _resume
//        _resume:
//                mov     r4,r1
//        restoreRegisters:
//                add     #8, r1                                          ! skip to R2
//                mov.l   @r1+, r2                                        ! restore R2
//                mov.l   @r1+, r3                                        ! restore R3
//                mov.l   @r1+, r4                                        ! restore R4
//                mov.l   @r1+, r5                                        ! restore R5
//                mov.l   @r1+, r6                                        ! restore R6
//                mov.l   @r1+, r7                                        ! restore R7
//                mov.l   @r1+, r8                                        ! restore R8
//                mov.l   @r1+, r9                                        ! restore R9
//                mov.l   @r1+, r10                                       ! restore R10
//                mov.l   @r1+, r11                                       ! restore R11
//                mov.l   @r1+, r12                                       ! restore R12
//                mov.l   @r1+, r13                                       ! restore R13
//                mov.l   @r1+, r14                                       ! restore R14
//                mov.l   @r1+, r15                                       ! restore programs stack
//                mov.l   @r1+, r0
//                add     #-8, r15                                        ! uncover PC/SR on stack 
//                mov.l   r0, @r15                                        ! restore PC onto stack
//                lds.l   @r1+, pr                                        ! restore PR
//                ldc.l   @r1+, gbr                                       ! restore GBR           
//                ldc.l   @r1+, vbr                                       ! restore VBR
//                lds.l   @r1+, mach                                      ! restore MACH
//                lds.l   @r1+, macl                                      ! restore MACL
//                mov.l   @r1, r0 
//                add     #-88, r1                                        ! readjust reg pointer to R1
//                mov.l   r0, @(4, r15)                                   ! restore SR onto stack+4
//                mov.l   r2, @-r15
//                mov.l   L_in_nmi, r0
//                mov             #0, r2
//                mov.b   r2, @r0
//                mov.l   @r15+, r2
//                mov.l   @r1+, r0                                        ! restore R0
//                rte
//                mov.l   @r1, r1                                         ! restore R1
//        
//        ");


}


void code_for_catch_exception(int n) __naked
{
  __asm
    nop
    nop
    nop
    nop
    nop
  __endasm;

//          asm("         .globl  _catch_exception_%O0" : : "i" (n)                               ); 
//          asm(" _catch_exception_%O0:" :: "i" (n)                                               );
//        
//          asm("         add     #-4, r15                                ! reserve spot on stack ");
//          asm("         mov.l   r1, @-r15                               ! push R1               ");
//        
//          asm("         mov     #15<<4, r1                                                      ");
//          asm("         ldc     r1, sr                                  ! disable interrupts    ");
//          asm("         mov.l   r0, @-r15                               ! push R0               ");
//        
//          /* Prepare for saving context, we've already pushed r0 and r1, stick exception number
//             into the frame */
//          asm("         mov     r15, r0                                                         ");
//          asm("         add     #8, r0                                                          ");
//          asm("         mov     %0,r1" :: "i" (n)                                               );
//          asm("         extu.b  r1,r1                                                           ");
//          asm("         bra     saveRegisters                           ! save register values  ");
//          asm("         mov.l   r1, @r0                                 ! save exception #      ");
}


static  void
exceptions (void)
{
  // code_for_catch_exception (CPU_BUS_ERROR_VEC);
  // code_for_catch_exception (DMA_BUS_ERROR_VEC);
  // code_for_catch_exception (INVALID_INSN_VEC);
  // code_for_catch_exception (INVALID_SLOT_VEC);
  // code_for_catch_exception (NMI_VEC);
  // code_for_catch_exception (TRAP_VEC);
  code_for_catch_exception (8); // used to be USER_VEC
  // code_for_catch_exception (IO_VEC);
}






/* Support for Serial I/O using on chip uart */

#define SMR0 (*(volatile char *)(0x05FFFEC0)) /* Channel 0  serial mode register */
#define BRR0 (*(volatile char *)(0x05FFFEC1)) /* Channel 0  bit rate register */
#define SCR0 (*(volatile char *)(0x05FFFEC2)) /* Channel 0  serial control register */
#define TDR0 (*(volatile char *)(0x05FFFEC3)) /* Channel 0  transmit data register */
#define SSR0 (*(volatile char *)(0x05FFFEC4)) /* Channel 0  serial status register */
#define RDR0 (*(volatile char *)(0x05FFFEC5)) /* Channel 0  receive data register */

#define SMR1 (*(volatile char *)(0x05FFFEC8)) /* Channel 1  serial mode register */
#define BRR1 (*(volatile char *)(0x05FFFEC9)) /* Channel 1  bit rate register */
#define SCR1 (*(volatile char *)(0x05FFFECA)) /* Channel 1  serial control register */
#define TDR1 (*(volatile char *)(0x05FFFECB)) /* Channel 1  transmit data register */
#define SSR1 (*(volatile char *)(0x05FFFECC)) /* Channel 1  serial status register */
#define RDR1 (*(volatile char *)(0x05FFFECD)) /* Channel 1  receive data register */

/*
 * Serial mode register bits
 */

#define SYNC_MODE               0x80
#define SEVEN_BIT_DATA          0x40
#define PARITY_ON               0x20
#define ODD_PARITY              0x10
#define STOP_BITS_2             0x08
#define ENABLE_MULTIP           0x04
#define PHI_64                  0x03
#define PHI_16                  0x02
#define PHI_4                   0x01

/*
 * Serial control register bits
 */
#define SCI_TIE                         0x80    /* Transmit interrupt enable */
#define SCI_RIE                         0x40    /* Receive interrupt enable */
#define SCI_TE                          0x20    /* Transmit enable */
#define SCI_RE                          0x10    /* Receive enable */
#define SCI_MPIE                        0x08    /* Multiprocessor interrupt enable */
#define SCI_TEIE                        0x04    /* Transmit end interrupt enable */
#define SCI_CKE1                        0x02    /* Clock enable 1 */
#define SCI_CKE0                        0x01    /* Clock enable 0 */

/*
 * Serial status register bits
 */
#define SCI_TDRE                        0x80    /* Transmit data register empty */
#define SCI_RDRF                        0x40    /* Receive data register full */
#define SCI_ORER                        0x20    /* Overrun error */
#define SCI_FER                         0x10    /* Framing error */
#define SCI_PER                         0x08    /* Parity error */
#define SCI_TEND                        0x04    /* Transmit end */
#define SCI_MPB                         0x02    /* Multiprocessor bit */
#define SCI_MPBT                        0x01    /* Multiprocessor bit transfer */


/*
 * Port B IO Register (PBIOR)
 */
#define PBIOR           (*(volatile char *)(0x05FFFFC6))
#define PB15IOR         0x8000
#define PB14IOR         0x4000
#define PB13IOR         0x2000
#define PB12IOR         0x1000
#define PB11IOR         0x0800
#define PB10IOR         0x0400
#define PB9IOR          0x0200
#define PB8IOR          0x0100
#define PB7IOR          0x0080
#define PB6IOR          0x0040
#define PB5IOR          0x0020
#define PB4IOR          0x0010
#define PB3IOR          0x0008
#define PB2IOR          0x0004
#define PB1IOR          0x0002
#define PB0IOR          0x0001

/*
 * Port B Control Register (PBCR1)
 */
#define PBCR1           (*(volatile short *)(0x05FFFFCC))
#define PB15MD1         0x8000
#define PB15MD0         0x4000
#define PB14MD1         0x2000
#define PB14MD0         0x1000
#define PB13MD1         0x0800
#define PB13MD0         0x0400
#define PB12MD1         0x0200
#define PB12MD0         0x0100
#define PB11MD1         0x0080
#define PB11MD0         0x0040
#define PB10MD1         0x0020
#define PB10MD0         0x0010
#define PB9MD1          0x0008
#define PB9MD0          0x0004
#define PB8MD1          0x0002
#define PB8MD0          0x0001

#define PB15MD          PB15MD1|PB14MD0
#define PB14MD          PB14MD1|PB14MD0
#define PB13MD          PB13MD1|PB13MD0
#define PB12MD          PB12MD1|PB12MD0
#define PB11MD          PB11MD1|PB11MD0
#define PB10MD          PB10MD1|PB10MD0
#define PB9MD           PB9MD1|PB9MD0
#define PB8MD           PB8MD1|PB8MD0

#define PB_TXD1         PB11MD1
#define PB_RXD1         PB10MD1
#define PB_TXD0         PB9MD1
#define PB_RXD0         PB8MD1

/*
 * Port B Control Register (PBCR2)
 */
#define PBCR2   0x05FFFFCE
#define PB7MD1  0x8000
#define PB7MD0  0x4000
#define PB6MD1  0x2000
#define PB6MD0  0x1000
#define PB5MD1  0x0800
#define PB5MD0  0x0400
#define PB4MD1  0x0200
#define PB4MD0  0x0100
#define PB3MD1  0x0080
#define PB3MD0  0x0040
#define PB2MD1  0x0020
#define PB2MD0  0x0010
#define PB1MD1  0x0008
#define PB1MD0  0x0004
#define PB0MD1  0x0002
#define PB0MD0  0x0001
        
#define PB7MD   PB7MD1|PB7MD0
#define PB6MD   PB6MD1|PB6MD0
#define PB5MD   PB5MD1|PB5MD0
#define PB4MD   PB4MD1|PB4MD0
#define PB3MD   PB3MD1|PB3MD0
#define PB2MD   PB2MD1|PB2MD0
#define PB1MD   PB1MD1|PB1MD0
#define PB0MD   PB0MD1|PB0MD0


#ifdef MHZ
#define BPS                     32 * 9600 * MHZ / ( BAUD * 10)
#else
#define BPS                     32      /* 9600 for 10 Mhz */
#endif

void handleError (char theSSR);

void
nop (void)
{

}
void 
init_serial (void)
{
  int i;

  /* Clear TE and RE in Channel 1's SCR   */
  SCR1 &= ~(SCI_TE | SCI_RE);

  /* Set communication to be async, 8-bit data, no parity, 1 stop bit and use internal clock */

  SMR1 = 0;
  BRR1 = BPS;

  SCR1 &= ~(SCI_CKE1 | SCI_CKE0);

  /* let the hardware settle */

  for (i = 0; i < 1000; i++)
    nop ();

  /* Turn on in and out */
  SCR1 |= SCI_RE | SCI_TE;

  /* Set the PFC to make RXD1 (pin PB8) an input pin and TXD1 (pin PB9) an output pin */
  PBCR1 &= ~(PB_TXD1 | PB_RXD1);
  PBCR1 |= PB_TXD1 | PB_RXD1;
}


int
getDebugCharReady (void)
{
  return 1;

//   char mySSR;
//   mySSR = SSR1 & ( SCI_PER | SCI_FER | SCI_ORER );
//   if ( mySSR )
//     handleError ( mySSR );
//   return SSR1 & SCI_RDRF ;
}

char 
getDebugChar (void)
{
  char ch;
  char mySSR;

  while ( ! getDebugCharReady())
    ;


  // TODO: make a macro for IO, or see the SFR instructions
  __asm
    in a, (0x00)
    ld (_read_ch), a
  __endasm;

//   ch = RDR1;
//   SSR1 &= ~SCI_RDRF;
// 
//   mySSR = SSR1 & (SCI_PER | SCI_FER | SCI_ORER);
// 
//   if (mySSR)
//     handleError (mySSR);
// 
//    return ch;

  return read_ch;
}

int 
putDebugCharReady (void)
{
  // return (SSR1 & SCI_TDRE);
  return 1;
}

void
putDebugChar (char ch)
{
  while (!putDebugCharReady())
    ;

  /*
   * Write data into TDR and clear TDRE
   */

  // write the char to the output port

  // TODO: make a macro for IO, or see the SFR instructions
  __asm
    ld    a,4(ix) 
    out   (0x00),a
  __endasm;

  //  TDR1 = ch;
  // SSR1 &= ~SCI_TDRE;
 return;
}

void 
handleError (char theSSR)
{
  SSR1 &= ~(SCI_ORER | SCI_PER | SCI_FER);
}

/* pacify the compiler */
void main ()
{
  ;
}
