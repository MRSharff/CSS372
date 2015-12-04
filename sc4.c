/*
    sc4.c

    Mat Sharff
    11/5/15

    Simulates a control unit for the LC-2200

*/
#include <stdio.h>
#include "simulator.h"

int main(void) {

    //Memory size constant (this doesn't work for some reason) entered in as size for MEM
    unsigned int MEM_SIZE = 50;

    //Debug monitor headers
    char *header = "SC-4 Debug Monitor by Mat Sharff";
    char *ColHeadings = "Register File                  Memory Dump";

    //Create arrays for simulated memory and registers
    unsigned int MEM[MEM_SIZE]; //MEM_SIZE in place of 50 doesn't work, produces seg fault. Wondering why.
    unsigned int REG[16];

    //Set zero register
    REG[0] = 0;

    //Setup registers
    unsigned int PC = 0;
    int A = 0;
    int B = 0;
    int R = 0;
    unsigned int IR = 0;
    unsigned int MAR = 0;
    unsigned int MDR = 0;
    unsigned int state = FETCH;
    unsigned int microstate = IFETCH1;
    unsigned int opcode = 0;
    unsigned int format = 0;
    unsigned int RegD = 0;
    unsigned int RegS1 = 0;
    unsigned int RegS2 = 0;
    int immed = 0;
    unsigned int SW = 0;
    unsigned int signBit;

    //set register 15 as the stack pointer which will be the last memory location
    REG[15] = (MEM_SIZE - 1);

    //Bitshifting offsets
    int opcodeShift = 27;
    int rdShift = 24;
    int RS1Shift = 20;
    int RS2Shift = 16;

    //Bitmasks
    unsigned int regDmask = 0x0f;
    unsigned int regS1mask = 0x00f;
    unsigned int regS2mask = 0x0000000f;
    unsigned int immed19SBMask = 0x00040000;
    unsigned int immed23SBMask = 0x00400000;
    unsigned int immed27SBMask = 0x04000000;

    //Used for the particular example program
    int *RESULT = &MEM[21];

    //initialize user input to enter the loop
    int userInput = 1;

    int memoryOffest = 0;
    int programSize = 0;

    //Show the debug monitor until the user asks to terminate (option input 9)
    while (userInput != 9) {

        //Debug Monitor printed with buffer
        int bpointer = 0;
        char buffer[1300];

        //Add the headers to the buffer
        bpointer = sprintf(buffer, "%s\n\n", header);
        bpointer += sprintf(buffer + bpointer, "%s\n", ColHeadings);


        //print registers 0-F and memory 0-F
        //MEM(i + counter) for getting memory offset (if program was loaded into mem at 0x10)
        int i;
        for (i = 0; i < 16; i++) {
            bpointer += sprintf(buffer + bpointer, "%X: %08X                %08X: %08X\n", i, REG[i], i + memoryOffest, MEM[i + memoryOffest]);
        }

        //print other registers
        bpointer += sprintf(buffer + bpointer, "\nPC: %08X  IR: %08X SW: %08X\n", PC, IR, SW);
        bpointer += sprintf(buffer + bpointer, "MAR: %08X MDR: %08X ALU.A: %08X ALU.B: %08X ALU.R: %08X\n\n", MAR, MDR, A, B, R);
        bpointer += sprintf(buffer + bpointer, "Commands: 1=Load, 2=Step, 3=Run, 4=Memory, 5=Save, 9=Exit    \n\nEnter: ");


        //print the debug monitor to the console
        printf("%s", buffer);

        //scan for user input (Commands)
        scanf("%d", &userInput);


        //Step vs Run conditions
        //run uses the same case as step, but sets the notstep value to 1
        //so the while loop will run while not halt and while notstep == 1
        int notstep = 1;
        if (userInput == 2) {
            notstep = 0;
        } else if (userInput == 3) {
            notstep = 1;
            userInput = 2;
        }

        //User input switch, what the program will do depends on the user input
        switch (userInput) {
            case 1:
                printf("Enter name of file to load: ");
                char path[100];
                scanf("%s", path);

                //open file from path entered by user
                FILE *programFile = fopen(path, "r");

                //counter used to determine memory location to start
                //putting instructions into (can be set by user later most likely)
                int counter = 0;
                int memLoc;
                int instruction;

                while (fscanf(programFile, "%X:%X", &memLoc, &instruction) == 2) {
                    MEM[memLoc] = instruction;
                    counter++;
                }

                programSize = counter;

                fclose(programFile);

                break;

            case 2:
                // Do until halt executed
                do {
                    switch (state) {
                        case FETCH :
                            //Fetch Microstates
                            while (microstate != IFETCH4) {
                                switch (microstate) {
                                    case IFETCH1 :
                                        MAR = PC;
                                        A = PC;
                                        microstate = IFETCH2;
                                        break;
                                    case IFETCH2 :
                                        //printf("IFTECH2: Get instruction\n");
                                        IR = MEM[MAR];
                                        //scanf("%x", &IR); //get instruction from memory (memory is keyboard in this case)
                                        microstate = IFETCH3;
                                        break;
                                    case IFETCH3 :
                                        PC = A + 1;
                                        microstate = IFETCH4;
                                        break;
                                }
                            }

                            state = DECODE; //after FETCH state is complete
                            microstate = DECODE1; //get opcode
                            break;
                        case DECODE :
                            //Decode Microstates
                            while (microstate != DECODE4) {
                                switch (microstate) {
                                    case DECODE1 : //get opcode
                                        opcode = (IR >> opcodeShift);
                                        microstate = DECODE2;
                                        break;
                                        // Instruction format on page 63
                                    case DECODE2 : //get format
                                        if (opcode == LDI || opcode == PUSH || opcode == POP || opcode == JSR) {
                                            format = RITYPE;
                                        }  else if (opcode == LD || opcode == ST || opcode == NOT) {
                                            format = RRITYPE;
                                        } else if (opcode > MINRRRTYPE && opcode < MAXRRRTYPE) {
                                            format = RRRTYPE;
                                        } else {
                                            format = ITYPE;
                                        }

                                        microstate = DECODE3; // set registers
                                        break;
                                    case DECODE3 :
                                        switch (format) {
                                            case RITYPE :
                                                RegD = ((IR >> rdShift) & regDmask);
                                                //sign extend immediate
                                                signBit = IR & immed23SBMask;
                                                immed = (signBit != 0x0) ? (IR | 0xff800000) : (IR & 0x007fffff);
                                                break;
                                            case RRITYPE :
                                                RegD = ((IR >> rdShift) & regDmask);
                                                RegS1 = ((IR >> RS1Shift) & regS1mask);

                                                signBit = IR & immed19SBMask;
                                                immed = (signBit != 0x0) ? ( IR | 0xfff80000) : (IR & 0x0007ffff);
                                                break;
                                            case RRRTYPE :
                                                RegD = ((IR >> rdShift) & regDmask);
                                                RegS1 = ((IR >> RS1Shift) & regS1mask);
                                                RegS2 = ((IR >> RS2Shift) & regS2mask);
                                                break;
                                            case ITYPE :
                                                signBit = IR & immed27SBMask;
                                                immed = (signBit != 0x0) ? ( IR | 0xf8000000) : (IR & 0x07ffffff);
                                        }
                                        microstate = DECODE4;
                                }
                            }
                            microstate = opcode;
                            state = EXECUTE;
                            break;
                        case EXECUTE :
                            //Execute Microstates
                            switch (microstate) {
                                case LDI :
                                    //printf("In LDI microstate\n");

                                    REG[RegD] = immed; // rd <- immed
                                    state = FETCH;
                                    break;
                                case LD :
                                    //rd <- M[br + ext(immed19)]
                                    //printf("In LD microstate\n");
                                    //RegD = MEM[(int) RegS1 + ((immed << 13) >> 13)]; // (LD rd, br, immed19 load base-relative)
                                    REG[RegD] = MEM[REG[RegS1] + immed];
                                    state = FETCH;
                                    break;
                                case ST :
                                    //printf("In ST microstate\n");
                                    MEM[REG[RegS1] + immed] = REG[RegD];
                                    state = FETCH;
                                    break;
                                case PUSH :
                                    //printf("In PUSH microstate\n");
                                    //MEM[REG[15--]] = RegD; this should work
                                    MEM[REG[15]] = REG[RegD];
                                    REG[15]--;
                                    state = FETCH;
                                    break;
                                case POP :
                                    //printf("In POP microstate\n");
                                    REG[15]++;
                                    //RegD = MEM[REG[++15]]; this should work
                                    REG[RegD] = MEM[REG[15]];
                                    state = FETCH;
                                    break;
                                case ADD :
                                    //printf("In ADD microstate\n");
                                    REG[RegD] = REG[RegS1] + REG[RegS2];
                                    state = FETCH;
                                    break;
                                case SUB :
                                    //printf("In SUB microstate\n");
                                    microstate = 1;
                                    while (microstate != 4) {
                                        switch (microstate) {
                                            case 1:
                                                A = REG[RegS1];
                                                microstate = 2;
                                            case 2:
                                                B = REG[RegS2];
                                                microstate = 3;
                                            case 3:
                                                R = A - B;
                                                REG[RegD] = R;
                                                microstate = 4;

                                        }
                                    }
                                    if (R < 0) {
                                        SW = 0 | 0x80000000;
                                    } else if(REG[RegD] = 0) {
                                        SW = 0 | 0x40000000;
                                    }
                                    //overflow and carry later
                                    state = FETCH;
                                    break;
                                case AND :
                                    //printf("In AND microstate\n");
                                    REG[RegD] = REG[RegS1] & REG[RegS2];
                                    state = FETCH;
                                    break;
                                case OR :
                                    //printf("In OR microstate\n");
                                    REG[RegD] = REG[RegS1] | REG[RegS2];
                                    state = FETCH;
                                    break;
                                case NOT :
                                    //printf("In NOT microstate\n");
                                    REG[RegD] = ~REG[RegS1];
                                    state = FETCH;
                                    break;
                                case SHL :
                                    //printf("In SHL microstate\n");
                                    REG[RegD] = (REG[RegS1] * 2)  * REG[RegS2];
                                    state = FETCH;
                                    break;
                                case SHR :
                                    //printf("In SHR microstate\n");
                                    REG[RegD] = (REG[RegS1] / 2) * REG[RegS2];
                                    state = FETCH;
                                    break;
                                case BR :
                                    //printf("In BR microstate\n");
                                    PC = PC + 1 + immed;
                                    state = FETCH;
                                    break;
                                case BRZ :
                                    //printf("In BRZ microstate\n");
                                    if (SW == 0x40000000) {
                                        PC = PC + 1 + immed;
                                    }
                                    state = FETCH;
                                    break;
                                case BRN :
                                    //printf("In BRN microstate\n");
                                    PC = PC + 1 + immed;
                                    state = FETCH;
                                    break;
                                case BRC :
                                    //printf("In BRC microstate\n");
                                    PC = PC + 1 + immed;
                                    state = FETCH;
                                    break;
                                case BRO :
                                    //printf("In BRO microstate\n");
                                    PC = PC + 1 + immed;
                                    state = FETCH;
                                    break;
                                case JSR :
                                    //printf("In JSR microstate\n");
                                    MEM[REG[15]--] = PC;
                                    PC = REG[RegD];
                                    state = FETCH;
                                    break;
                                case RET :
                                    //printf("In RET microstate\n");
                                    PC = MEM[++REG[15]];
                                    state = FETCH;
                                    break;
                                case RETI :
                                    //printf("In RETI microstate\n");
                                    SW = MEM[++REG[15]];
                                    PC = MEM[++REG[15]];
                                    state = FETCH;
                                    break;
                                case TRAP :
                                    //printf("In TRAP microstate\n");
                                    MEM[REG[15]--] = PC;
                                    MEM[REG[15]--] = SW;
                                    state = FETCH;
                                    break;
                                case HALT :
                                    //printf("In HALT microstate\n");
                                    state = HALT;
                                    //printf("RESULT: R1 = %d, R2 = %d, R3 = %d, R4 = %d\n", MEM[102], MEM[103], MEM[104], MEM[105]);
                                    break;
                            }

                            //printf("PC = %d\nIR = 0x%08x\nMAR = %x\nA = %d\nB = %d\n", PC, IR, MAR, A, B);
                            //printf("RegD = %d\nRegS1 = %d\nRegS2 = %d\n\n", RegD, RegS1, RegS2);

                            microstate = IFETCH1;
                            break;
                    }
                } while (state != HALT && notstep == 1);
                break;

            case 4:
                printf("Starting memory address: ");

                scanf("%X", &memoryOffest);
                break;
            case 5:
                printf("Name of file to save: ");

                scanf("%s", path);

                //open file from path entered by user
                programFile = fopen(path, "w");

                int startAddress;
                int endAddress;

                printf("Starting address: ");
                scanf("%X", &startAddress);

                printf("Ending address: ");
                scanf("%X", &endAddress);

                int j;
                for (j = startAddress; j < endAddress + 1; j++) {
                    fprintf(programFile, "%X:%08X\n", j, MEM[j]);
                }
                break;
            case 9:
                return 0;
        }
    }

    return 0;
}