/* z80-stub.c -- debugging stub for the Z80

   This stub is based on the SH stub by Ben Lee and Steve Chamberlain. 

   Modifications for the Z80 by Leonardo Etcheverry <letcheve@fing.edu.uy>, 2010.
   
   - Building the stub -
   # sdcc --no-std-crt0 -mz80 --model-large --no-peep --stack-auto --code-loc 0x0000 --data-loc 0x9000 z80-stub.c -o z80-stub
   # srec_cat z80-stub -Intel -output z80-stub.bin -Binary

   # (optional, create a 16K rom file for the z80 qemu target)
   # cat z80-stub.bin /dev/zero | dd bs=1k count=16 > /opt/qemu-z80/share/qemu/zx-rom.bin
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

/* Serial link constants */

#ifdef TARGET_Z80 
#define UART_BASE          0x80
#define UART_DATA          UART_BASE+0
#define UART_RX_VALID      UART_BASE+1
#define UART_RX_VALID_MASK 0x80
#else
#define UART_BASE          0x00
#define UART_DATA          UART_BASE+0
#endif


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
/// LLL #define SSTEP_INSTR    0xc3ff

/* Z80 registers (should match the constants used in gdb  */
// TODO: maybe include someheader.h which is also included from z80-tdep.h?


/* Z80 instruction opcodes */
#define RST08_INST     0xCF
#define BREAK_INST     RST08_INST
#define SSTEP_INSTR    BREAK_INST

/* Renesas SH processor register masks */

#define T_BIT_MASK     0x0001

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound
 * buffers. At least NUMREGBYTES*2 are needed for register packets.
 */
#define BUFMAX 256


/* registers constants */
#define R_A     0
#define R_F     1

// 16 bit registers
#define R_BC    2
#define R_DE    4  
#define R_HL    6  
#define R_IX    8  
#define R_IY    10  
#define R_SP    12  

#define R_I     13
#define R_R     14

#define R_AX    16
#define R_FX    17
#define R_BCX   18
#define R_DEX   20
#define R_HLX   22

// TODO: fix this!
#define R_PC    24
// #define R_AF    24
// #define R_AFX   26


/*
 * Number of bytes for registers
 */
#define NUMREGBYTES 26 // 6(1byte) + 10(2bytes)



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


#define catch_exception_random catch_exception_255 /* Treat all odd ones like 255 */

void breakpoint() __naked;


#define init_stack_size 512  /* if you change this you should also modify BINIT */
#define stub_stack_size 512

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

#define Z80_RST08_VEC      8

char intcause; /* TODO: initialize */
char in_nmi;   /* Set when handling an NMI, so we don't reenter */
int dofault;  /* Non zero, bus errors will raise exception */

char read_ch; /* TODO byte read from serial port, for now it's a global */

int *stub_sp;

/* debug > 0 prints ill-formed commands in valid packets & checksum errors */
int remote_debug;

struct {
  char a;
  char f;
  short bc;
  short de;
  short hl;
  short ix;
  short iy;
  short sp;
  char i;
  char r;
  char ax;
  char fx;
  short bcx;
  short dex;
  short hlx;
  short pc; } registers;

typedef struct
  {
    char *memAddr;
    // LLL short oldInstr;
    char oldInstr;
  }
stepData;

// int registers[NUM_REGISTERS];
stepData instrBuffer;
char stepped;
static const char hexchars[] = "0123456789abcdef";
static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

struct buffer
{
  void* base;
  int n_fetch;
  int n_used;
  signed char data[4];
} ;

struct tab_elt
{
  unsigned char val;
  unsigned char mask;
  void * (*fp)(void *pc, struct tab_elt *inst);
  char *        text;
  unsigned char inst_len;
} ;

// typedef void* (*NextPCFunc) (void *pc, tab_elt *inst);

 #define TXTSIZ 24

////  /* Table to disassemble machine codes without prefix.  */
////  static struct tab_elt opc_main[] =
////  {
////    { 0x00, 0xFF, prt       , "nop",            1 },
////    { 0x01, 0xCF, prt_rr_nn , "ld %s,0x%%04x",  3 },
////    { 0x02, 0xFF, prt       , "ld (bc),a",      1 },
////    { 0x03, 0xCF, prt_rr    , "inc " ,          1 },
////    { 0x04, 0xC7, prt_r     , "inc %s",         1 },
////    { 0x05, 0xC7, prt_r     , "dec %s",         1 },
////    { 0x06, 0xC7, ld_r_n    , "ld %s,0x%%02x",  2 },
////    { 0x07, 0xFF, prt       , "rlca",           1 },
////    { 0x08, 0xFF, prt       , "ex af,af'",      1 },
////    { 0x09, 0xCF, prt_rr    , "add hl,",        1 },
////    { 0x0A, 0xFF, prt       , "ld a,(bc)" ,     1 },
////    { 0x0B, 0xCF, prt_rr    , "dec ",           1 },
////    { 0x0F, 0xFF, prt       , "rrca",           1 },
////    { 0x10, 0xFF, pe_djnz   , "djnz ",          2 },
////    { 0x12, 0xFF, prt       , "ld (de),a",      1 },
////    { 0x17, 0xFF, prt       , "rla",            1 },
////    { 0x18, 0xFF, prt_e     , "jr ",            2 },
////    { 0x1A, 0xFF, prt       , "ld a,(de)",      1 },
////    { 0x1F, 0xFF, prt       , "rra",            1 },
////    { 0x20, 0xE7, jr_cc     , "jr %s,",         1 },
////    { 0x22, 0xFF, prt_nn    , "ld (0x%04x),hl", 3 },
////    { 0x27, 0xFF, prt       , "daa",            1 },
////    { 0x2A, 0xFF, prt_nn    , "ld hl,(0x%04x)", 3 },
////    { 0x2F, 0xFF, prt       , "cpl",            1 },
////    { 0x32, 0xFF, prt_nn    , "ld (0x%04x),a",  3 },
////    { 0x37, 0xFF, prt       , "scf",            1 },
////    { 0x3A, 0xFF, prt_nn    , "ld a,(0x%04x)",  3 },
////    { 0x3F, 0xFF, prt       , "ccf",            1 },
//// 
////    { 0x76, 0xFF, prt       , "halt",           1 },
////    { 0x40, 0xC0, ld_r_r    , "ld %s,%s",       1 },
//// 
////    { 0x80, 0xC0, arit_r    , "%s%s",           1 },
//// 
////    { 0xC0, 0xC7, prt_cc    , "ret ",           1 },
////    { 0xC1, 0xCF, pop_rr    , "pop",            1 },
////    { 0xC2, 0xC7, jp_cc_nn  , "jp ",            3 },
////    { 0xC3, 0xFF, prt_nn    , "jp 0x%04x",      3 },
////    { 0xC4, 0xC7, jp_cc_nn  , "call ",          3 },
////    { 0xC5, 0xCF, pop_rr    , "push",           1 }, 
////    { 0xC6, 0xC7, arit_n    , "%s0x%%02x",      2 },
////    { 0xC7, 0xC7, rst       , "rst 0x%02x",     1 },
////    { 0xC9, 0xFF, prt       , "ret",            1 },
////    { 0xCB, 0xFF, pref_cb   , "",               0 },
////    { 0xCD, 0xFF, prt_nn    , "call 0x%04x",    3 },
////    { 0xD3, 0xFF, prt_n     , "out (0x%02x),a", 2 },
////    { 0xD9, 0xFF, prt       , "exx",            1 },
////    { 0xDB, 0xFF, prt_n     , "in a,(0x%02x)",  2 },
////    { 0xDD, 0xFF, pref_ind  , "ix",             0 },
////    { 0xE3, 0xFF, prt       , "ex (sp),hl",     1 },
////    { 0xE9, 0xFF, prt       , "jp (hl)",        1 },
////    { 0xEB, 0xFF, prt       , "ex de,hl",       1 },
////    { 0xED, 0xFF, pref_ed   , "",               0 },
////    { 0xF3, 0xFF, prt       , "di",             1 },
////    { 0xF9, 0xFF, prt       , "ld sp,hl",       1 },
////    { 0xFB, 0xFF, prt       , "ei",             1 },
////    { 0xFD, 0xFF, pref_ind  , "iy",             0 },
////    { 0x00, 0x00, prt       , "????"          , 1 },
////  } ;

 /* Table to disassemble machine codes without prefix.  */
struct tab_elt opc_main[] =
 {
   { 0x00, 0xFF, prt       , "",            1 },
   { 0x01, 0xCF, prt_rr_nn , "",  3 },
   { 0x02, 0xFF, prt       , "",      1 },
   { 0x03, 0xCF, prt_rr    , "" ,          1 },
   { 0x04, 0xC7, prt_r     , "",         1 },
   { 0x05, 0xC7, prt_r     , "",         1 },
   { 0x06, 0xC7, ld_r_n    , "",  2 },
   { 0x07, 0xFF, prt       , "",           1 },
   { 0x08, 0xFF, prt       , "",      1 },
   { 0x09, 0xCF, prt_rr    , "",        1 },
   { 0x0A, 0xFF, prt       , "" ,     1 },
   { 0x0B, 0xCF, prt_rr    , "",           1 },
   { 0x0F, 0xFF, prt       , "",           1 },
   { 0x10, 0xFF, pe_djnz   , "",          2 },
   { 0x12, 0xFF, prt       , "",      1 },
   { 0x17, 0xFF, prt       , "",            1 },
   { 0x18, 0xFF, prt_e     , "",            2 },
   { 0x1A, 0xFF, prt       , "",      1 },
   { 0x1F, 0xFF, prt       , "",            1 },
   { 0x20, 0xE7, jr_cc     , "",         1 },
   { 0x22, 0xFF, prt_nn    , "", 3 },
   { 0x27, 0xFF, prt       , "",            1 },
   { 0x2A, 0xFF, prt_nn    , "", 3 },
   { 0x2F, 0xFF, prt       , "",            1 },
   { 0x32, 0xFF, prt_nn    , "",  3 },
   { 0x37, 0xFF, prt       , "",            1 },
   { 0x3A, 0xFF, prt_nn    , "",  3 },
   { 0x3F, 0xFF, prt       , "",            1 },

   { 0x76, 0xFF, prt       , "",           1 },
   { 0x40, 0xC0, ld_r_r    , "",       1 },

   { 0x80, 0xC0, arit_r    , "",           1 },

   { 0xC0, 0xC7, prt_cc    , "",           1 },
   { 0xC1, 0xCF, pop_rr    , "",            1 },
   { 0xC2, 0xC7, jp_cc_nn  , "",            3 },
   { 0xC3, 0xFF, prt_nn    , "",      3 },
   { 0xC4, 0xC7, jp_cc_nn  , "",          3 },
   { 0xC5, 0xCF, pop_rr    , "",           1 }, 
   { 0xC6, 0xC7, arit_n    , "",      2 },
   { 0xC7, 0xC7, rst       , "",     1 },
   { 0xC9, 0xFF, prt       , "",            1 },
   { 0xCB, 0xFF, pref_cb   , "",               0 },
   { 0xCD, 0xFF, prt_nn    , "",    3 },
   { 0xD3, 0xFF, prt_n     , "", 2 },
   { 0xD9, 0xFF, prt       , "",            1 },
   { 0xDB, 0xFF, prt_n     , "",  2 },
   { 0xDD, 0xFF, pref_ind  , "",             0 },
   { 0xE3, 0xFF, prt       , "",     1 },
   { 0xE9, 0xFF, prt       , "",        1 },
   { 0xEB, 0xFF, prt       , "",       1 },
   { 0xED, 0xFF, pref_ed   , "",               0 },
   { 0xF3, 0xFF, prt       , "",             1 },
   { 0xF9, 0xFF, prt       , "",       1 },
   { 0xFB, 0xFF, prt       , "",             1 },
   { 0xFD, 0xFF, pref_ind  , "",             0 },
   { 0x00, 0x00, prt       , ""          , 1 },
 } ;




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
     push  hl               ;; RST 08, used for breakpoints
     ld    hl,#08           ;; 08 means a breakpoint was hit, pass it to sr routine
     jp    _sr              ;; TODO maybe change to saveRegisters, semantics

    __endasm;
 }

 void
 RESET (void) __naked
 {

 #ifdef MONITOR
   reset_hook ();
 #endif

   in_nmi = 0;
   dofault = 1;
   stepped = 0;

   stub_sp = stub_stack + stub_stack_size;
   breakpoint();

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
     case Z80_RST08_VEC:
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
   char *instrMem;
   char *nextInstrMem;
 ///LLL   int displacement;
 ///LLL   int reg;
   unsigned short opcode;

   struct tab_elt *p;

   //instrMem = (short *) registers[PC];
   instrMem = (char *) registers.pc;

   opcode = *instrMem;
   stepped = 1;

   for (p = opc_main; p->val != (opcode & p->mask); ++p)
     ;

   nextInstrMem = (char *) p->fp(instrMem, p);


 /// LLL   if ((opcode & COND_BR_MASK) == BT_INSTR)
 /// LLL     {
 /// LLL       if (1) // if (registers[SR] & T_BIT_MASK)
 /// LLL         {
 /// LLL           displacement = (opcode & COND_DISP) << 1;
 /// LLL           if (displacement & 0x80)
 /// LLL             displacement |= 0xffffff00;
 /// LLL           /*
 /// LLL                    * Remember PC points to second instr.
 /// LLL                    * after PC of branch ... so add 4
 /// LLL                    */
 /// LLL           instrMem = (short *) (registers[PC] + displacement + 4);
 /// LLL         }
 /// LLL       else
 /// LLL         instrMem += 1;
 /// LLL     }
 /// LLL   else if ((opcode & COND_BR_MASK) == BF_INSTR)
 /// LLL     {
 /// LLL       if (1) // if (registers[SR] & T_BIT_MASK)
 /// LLL         instrMem += 1;
 /// LLL       else
 /// LLL         {
 /// LLL           displacement = (opcode & COND_DISP) << 1;
 /// LLL           if (displacement & 0x80)
 /// LLL             displacement |= 0xffffff00;
 /// LLL           /*
 /// LLL                    * Remember PC points to second instr.
 /// LLL                    * after PC of branch ... so add 4
 /// LLL                    */
 /// LLL           instrMem = (short *) (registers[PC] + displacement + 4);
 /// LLL         }
 /// LLL     }
 /// LLL   else if ((opcode & UCOND_DBR_MASK) == BRA_INSTR)
 /// LLL     {
 /// LLL       displacement = (opcode & UCOND_DISP) << 1;
 /// LLL       if (displacement & 0x0800)
 /// LLL         displacement |= 0xfffff000;
 /// LLL 
 /// LLL       /*
 /// LLL            * Remember PC points to second instr.
 /// LLL            * after PC of branch ... so add 4
 /// LLL            */
 /// LLL       instrMem = (short *) (registers[PC] + displacement + 4);
 /// LLL     }
 /// LLL   else if ((opcode & UCOND_RBR_MASK) == JSR_INSTR)
 /// LLL     {
 /// LLL       reg = (char) ((opcode & UCOND_REG) >> 8);
 /// LLL 
 /// LLL       instrMem = (short *) registers[reg];
 /// LLL     }
 /// LLL   else if (opcode == RTS_INSTR)
 /// LLL     ; // instrMem = (short *) registers[PR];
 /// LLL   else if (opcode == RTE_INSTR)
 /// LLL     instrMem = (short *) registers[15];
 /// LLL   else if ((opcode & TRAPA_MASK) == TRAPA_INSTR)
 /// LLL     instrMem = (short *) ((opcode & ~TRAPA_MASK) << 2);
 /// LLL   else
 /// LLL     instrMem += 1;

   instrMem = nextInstrMem;

   instrBuffer.memAddr = instrMem;
   instrBuffer.oldInstr = *instrMem;
   *instrMem = SSTEP_INSTR;
 }


 /* Undo the effect of a previous doSStep.  If we single stepped,
    restore the old instruction. */

 void
 undoSStep (void)
 {
 /// LLL   if (stepped)
 /// LLL     {  short *instrMem;
 /// LLL       instrMem = instrBuffer.memAddr;
 /// LLL       *instrMem = instrBuffer.oldInstr;
 /// LLL     }
   if (stepped)
     { char *instrMem;
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
    * Exception 0x08 means a RST 08 instruction (breakpoint) inserted in
    * place of code
    * 
    * Backup PC by one instruction, this instruction will later
    * be replaced by the original instruction at the breakpoint
    */
   if (exceptionVector == 0x08)
     registers.pc -= 1;
     // registers[R_PC] -= 1;

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
               registers.pc = addr;
               //registers[R_PC] = addr;
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
 breakpoint (void) __naked
 {
   __asm              
   nop                
   nop                
   nop                
   RST 08             
   nop                
   nop                
   jp  0xb000        
   nop                
   nop                
   __endasm;          
 }


void sr() __naked
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
     ld    (#_intcause), hl        ;; argument passed in hl signals the exception cause
     pop   hl                      ;; recover original hl before hitting the breakpoint

     ld    (#_registers + R_HL), hl
     push  af                      ;; dirty trick to save AF
     pop   hl
     ld    a,h
     ld    (#_registers + R_A), a   
     ld    a,l
     ld    (#_registers + R_F), a    

     ld    (#_registers + R_BC), bc
     ld    (#_registers + R_DE), de
     ld    (#_registers + R_IX), ix
     ld    (#_registers + R_IY), iy

     ;; alternate register set
     exx
     ex   af,af'                     ;;'

     ld    (#_registers + R_HLX), hl

     push  af ;; dirty trick to save AF
     pop   hl
     ld    (#_registers + R_AX),  hl  ;; saves both A and F (because R_FX == R_AX+1)
     ld    (#_registers + R_BCX), bc
     ld    (#_registers + R_DEX), de

     ;; switch back to original reg set
     ex   af,af'                     ;;'
     exx

     pop  hl                         ; get the PC saved as the returned address when the breakpoint hit
     ld   (#_registers + R_PC), hl
     push hl

     ld    hl, (#_intcause)
     push  hl
     call  _handle_exception              
     pop   af      

   __endasm;

 }




 static void rr() __naked
 {
   __asm
     ld    a, (#_registers + R_A) ;; restore AF
     ld    b, a
     ld    a, (#_registers + R_F)
     ld    c, a
     push  bc                              
     pop   af

     ld    hl, (#_registers + R_HL)
     ld    bc, (#_registers + R_BC)      
     ld    de, (#_registers + R_DE)          
     ld    ix, (#_registers + R_IX)      
     ld    iy, (#_registers + R_IY)          


     exx
     ex    af, af'                         ;'
     ld    hl, (#_registers + R_HLX)
     ld    bc, (#_registers + R_BCX)      
     ld    de, (#_registers + R_DEX)          

     ld    hl, (#_registers + R_AX)        ; restores *both* A and F (see register number constants )
     push  hl
     pop   af

     ;; switch back to original reg set
     ex    af, af'                         ;'
     exx             

     ;; put the (new?) PC back in the stack as the return address
     ld    hl, (#_registers + R_PC)
     ex    (sp), hl
         ld    hl, (#_registers + R_HL)

  __endasm;
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
    ;

  /* Turn on in and out */
  SCR1 |= SCI_RE | SCI_TE;

  /* Set the PFC to make RXD1 (pin PB8) an input pin and TXD1 (pin PB9) an output pin */
  PBCR1 &= ~(PB_TXD1 | PB_RXD1);
  PBCR1 |= PB_TXD1 | PB_RXD1;
}


int
getDebugCharReady (void)
{

#ifdef TARGET_Z80
  __asm

  __endasm;
#endif

  return 1;
}

char 
getDebugChar (void)
{
/// LLL  char ch; //TODO: fix this, it's global for now
//  char mySSR;

//   while ( ! getDebugCharReady())
//     ;

#ifdef TARGET_QEMU
  // TODO: make a macro for IO, or see the SFR instructions
  __asm
    in a, (0x00)
    ld (_read_ch), a
  __endasm;
#endif

#ifdef TARGET_Z80

  __asm
0001$: 
    in a, (UART_DATA)
    ld (_read_ch), a

    in a, (UART_RX_VALID)
    and #UART_RX_VALID_MASK
    jr z, 0001$
  __endasm;

#endif

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
  ch = ch; //TODO fix this, pacify the compiler for now
  while (!putDebugCharReady())
    ;

  // write the char to the output port

#ifdef TARGET_QEMU
  // TODO: make a macro for IO, or see the SFR instructions
  __asm
    ld    a,4(ix) 
    out   (0x00),a
  __endasm;
#endif

#ifdef TARGET_Z80
    __asm
    ld    a,4(ix) 
    out   (UART_DATA),a
  __endasm;
#endif

 return;
}

void 
handleError (char theSSR)
{
  theSSR=theSSR;
  // SSR1 &= ~(SCI_ORER | SCI_PER | SCI_FER);
}





// --------------------

static void *
prt (void *pc, struct tab_elt *inst)
{
//   info->fprintf_func (info->stream, "%s", txt);
//   buf->n_used = buf->n_fetch;
  char *cpc = (char *)pc;
  return (cpc + inst->inst_len);
}

static void *
prt_e (void *pc, struct tab_elt *inst)
{
//   char e;
//   int target_addr;
// 
//   if (fetch_data (buf, info, 1))
//     {
//       e = buf->data[1];
//       target_addr = (buf->base + 2 + e) & 0xffff;
//       buf->n_used = buf->n_fetch;
//       info->fprintf_func (info->stream, "%s0x%04x", txt, target_addr);
//     }
//   else
//     buf->n_used = -1;

//  return buf->n_used;
  return pc + inst->inst_len;
}

static void *
jr_cc (void *pc, struct tab_elt *inst)
{
  char mytxt[TXTSIZ];
// 
//   snprintf (mytxt, TXTSIZ, txt, cc_str[(buf->data[0] >> 3) & 3]);
  return prt_e (pc, inst);
}

static void *
prt_nn (void *pc, struct tab_elt *inst)
{
//   int nn;
//   unsigned char *p;
// 
//   p = (unsigned char*) buf->data + buf->n_fetch;
//   if (fetch_data (buf, info, 2))
//     {
//       nn = p[0] + (p[1] << 8);
//       info->fprintf_func (info->stream, txt, nn);
//       buf->n_used = buf->n_fetch;
//     }
//   else
//     buf->n_used = -1;
//  return buf->n_used;
  char *cpc = (char *)pc;
  return (cpc + inst->inst_len);
}

static void *
prt_rr_nn (void *pc, struct tab_elt *inst)
{
   char mytxt[TXTSIZ];
//   int rr;
// 
//   rr = (buf->data[buf->n_fetch - 1] >> 4) & 3; 
//   snprintf (mytxt, TXTSIZ, txt, rr_str[rr]);
//   return prt_nn (buf, info, mytxt);

   return pc + inst->inst_len;
}

static void *
prt_rr (void *pc, struct tab_elt *inst)
{
//   info->fprintf_func (info->stream, "%s%s", txt,
// 		      rr_str[(buf->data[buf->n_fetch - 1] >> 4) & 3]);
//   buf->n_used = buf->n_fetch;
//   return buf->n_used;
  return pc + inst->inst_len;
}

static void *
prt_n  (void *pc, struct tab_elt *inst)
{
//   int n;
//   unsigned char *p;
// 
//   p = (unsigned char*) buf->data + buf->n_fetch;
// 
//   if (fetch_data (buf, info, 1))
//     {
//       n = p[0];
//       info->fprintf_func (info->stream, txt, n);
//       buf->n_used = buf->n_fetch;
//     }
//   else
//     buf->n_used = -1;

//  return buf->n_used;
  char *cpc = (char *)pc;
  return (cpc + inst->inst_len);
}

static void *
ld_r_n  (void *pc, struct tab_elt *inst)
{
   char mytxt[TXTSIZ];
// 
//   snprintf (mytxt, TXTSIZ, txt, r_str[(buf->data[0] >> 3) & 7]);
//   return prt_n (buf, info, mytxt);
   return prt_n(pc, inst);
}

static void *
prt_r (void *pc, struct tab_elt *inst)
{
//   info->fprintf_func (info->stream, txt,
// 		      r_str[(buf->data[buf->n_fetch - 1] >> 3) & 7]);
//   buf->n_used = buf->n_fetch;
//   return buf->n_used;
  char *cpc = (char *)pc;
  return (cpc + inst->inst_len);
}

static void *
ld_r_r (void *pc, struct tab_elt *inst)
{
//   info->fprintf_func (info->stream, txt,
// 		      r_str[(buf->data[buf->n_fetch - 1] >> 3) & 7],
// 		      r_str[buf->data[buf->n_fetch - 1] & 7]);
//   buf->n_used = buf->n_fetch;
//   return buf->n_used;
  char *cpc = (char *)pc;
  return (cpc + inst->inst_len);
}

static void *
arit_r (void *pc, struct tab_elt *inst)
{
//   info->fprintf_func (info->stream, txt,
// 		      arit_str[(buf->data[buf->n_fetch - 1] >> 3) & 7],
// 		      r_str[buf->data[buf->n_fetch - 1] & 7]);
//   buf->n_used = buf->n_fetch;
//   return buf->n_used;
  char *cpc = (char *)pc;
  return (cpc + inst->inst_len);
}

static void *
prt_cc (void *pc, struct tab_elt *inst)
{
//   info->fprintf_func (info->stream, "%s%s", txt,
// 		      cc_str[(buf->data[0] >> 3) & 7]);
//   buf->n_used = buf->n_fetch;
//   return buf->n_used;
  return pc + inst->inst_len;
}

static void *
pop_rr (void *pc, struct tab_elt *inst)
{
//   static char *rr_stack[] = { "bc","de","hl","af"};
// 
//   info->fprintf_func (info->stream, "%s %s", txt,
// 		      rr_stack[(buf->data[0] >> 4) & 3]);
//   buf->n_used = buf->n_fetch;
//   return buf->n_used;
  char *cpc = (char *)pc;
  return (cpc + inst->inst_len);
}


static void *
jp_cc_nn (void *pc, struct tab_elt *inst)
{
  char mytxt[TXTSIZ];
// 
//   snprintf (mytxt,TXTSIZ,
// 	    "%s%s,0x%%04x", txt, cc_str[(buf->data[0] >> 3) & 7]);
  return prt_nn (pc, inst);

}

static void *
arit_n (void *pc, struct tab_elt *inst)
{
  char mytxt[TXTSIZ];
// 
//   snprintf (mytxt,TXTSIZ, txt, arit_str[(buf->data[0] >> 3) & 7]);
  return prt_n (pc, inst);
}

static void *
rst (void *pc, struct tab_elt *inst)
{
//   info->fprintf_func (info->stream, txt, buf->data[0] & 0x38);
//   buf->n_used = buf->n_fetch;
//  return buf->n_used;
  return pc + inst->inst_len;
}

static void *
pref_cb (void *pc, struct tab_elt *inst)
{
//   if (fetch_data (buf, info, 1))
//     {
//       buf->n_used = 2;
//       if ((buf->data[1] & 0xc0) == 0)
// 	info->fprintf_func (info->stream, "%s %s",
// 			    cb2_str[(buf->data[1] >> 3) & 7],
// 			    r_str[buf->data[1] & 7]);
//       else
// 	info->fprintf_func (info->stream, "%s %d,%s",
// 			    cb1_str[(buf->data[1] >> 6) & 3],
// 			    (buf->data[1] >> 3) & 7,
// 			    r_str[buf->data[1] & 7]);
//     }
//   else
//     buf->n_used = -1;

//  return buf->n_used;
  return pc + inst->inst_len;
}

static void *
pref_ind (void *pc, struct tab_elt *inst)
{
//   if (fetch_data (buf, info, 1))
//     {
//       char mytxt[TXTSIZ];
//       struct tab_elt *p;
// 
//       for (p = opc_ind; p->val != (buf->data[1] & p->mask); ++p)
// 	;
//       snprintf (mytxt, TXTSIZ, p->text, txt);
//       p->fp (buf, info, mytxt);
//     }
//   else
//     buf->n_used = -1;

//  return buf->n_used;

  return pc + inst->inst_len;
}

static void *
pref_xd_cb (void *pc, struct tab_elt *inst)
{
//   if (fetch_data (buf, info, 2))
//     {
//       int d;
//       char arg[TXTSIZ];
//       signed char *p;
// 
//       buf->n_used = 4;
//       p = buf->data;
//       d = p[2];
// 
//       if (((p[3] & 0xC0) == 0x40) || ((p[3] & 7) == 0x06))
// 	snprintf (arg, TXTSIZ, "(%s%+d)", txt, d);
//       else
// 	snprintf (arg, TXTSIZ, "(%s%+d),%s", txt, d, r_str[p[3] & 7]);
// 
//       if ((p[3] & 0xc0) == 0)
// 	info->fprintf_func (info->stream, "%s %s",
// 			    cb2_str[(buf->data[3] >> 3) & 7],
// 			    arg);
//       else
// 	info->fprintf_func (info->stream, "%s %d,%s",
// 			    cb1_str[(buf->data[3] >> 6) & 3],
// 			    (buf->data[3] >> 3) & 7,
// 			    arg);
//     }
//   else
//     buf->n_used = -1;

//  return buf->n_used;

  return pc + inst->inst_len;
}

static void *
pref_ed (void *pc, struct tab_elt *inst)
{
//  struct tab_elt *p;
//
//  if (fetch_data(buf, info, 1))
//    {
//      for (p = opc_ed; p->val != (buf->data[1] & p->mask); ++p)
//	;
//      p->fp (buf, info, p->text);
//    }
//  else
//    buf->n_used = -1;

//  return buf->n_used;

  return pc + inst->inst_len;

}
// -------------------- 

//---------- CONTROL JUMP INSTRUCTIONS PSEUDO EVAL FUNCTIONS ----------
void *
pe_djnz (void *pc, struct tab_elt *inst)
{
  char *cpc = (char *)pc;

  if (registers.a - 1 == 0)
    return (cpc + inst->inst_len); 
  else
    {    // result of dec wasn't Z, so we jump e
      int e = cpc[1];
      return (cpc + e + 2);
    }
}

/* pacify the compiler */
void main () {}

/* Local Variables: */
/* compile-command: "./compile_z80stub.sh" */
/* End: */
