/*
    simulator.h

    Mat Sharff

    Instruction set for sc4

*/

/* States for the FSM */
#define FETCH 0
#define DECODE 1
#define EXECUTE 2

/* Microstates for Fetch */
#define IFETCH1 0
#define IFETCH2 1
#define IFETCH3 2
#define IFETCH4 3

/* Microstates for Decode */
#define DECODE1 0
#define DECODE2 1
#define DECODE3 2
#define DECODE4 3

/* Formats */
#define RITYPE 0
#define RRITYPE 1
#define RRRTYPE 2
#define ITYPE 3

/* Microstates for Execute */
#define NOP 0
#define LDI 1
#define LD 2
#define LD2 3
#define ST 4
#define MOV 5
#define PUSH 6
#define POP 7
#define ADD 8
#define SUB 9
#define AND 10
#define OR 11
#define NOT 12
#define XOR 13
#define SHL 14
#define SHR 15
#define BR 16
#define BRZ 17
#define BRN 18
#define BRC 19
#define BRO 20
#define JSR 21
#define JSRZ 22
#define JSRN 23
#define DI 24
#define EI 25
#define RET 26
#define RETI 27
#define TRAP 28
#define HALT 29
#define ST2 31

#define MINRRRTYPE 7
#define MAXRRRTYPE 15

/* Memory mapped IO */
#define IOBASE 0xB0000000
#define TIMER0 0x0
#define KBD    0x4
#define SCRN   0x8
#define COM1   0xC