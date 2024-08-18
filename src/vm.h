#ifndef VM_H
#define VM_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>

#define MAX_LINE_LEN 255
#define VM_REG_COUNT 20
#define VM_STACK_SIZE 1024 * 10

// #define NO_PRINT

#define GETASCIIRAW(ascii) ascii - 48
#define MAKEASCIIRAW(raw) raw + 48

#ifndef NO_PRINT
#define DEBUGF(msg, format) printf(msg, format);
#define DEBUG(msg) printf(msg);
#endif
#ifdef NO_PRINT
#define DEBUGF(msg, format) ;
#define DEBUG(msg) ;
#endif

#define TRUE 1
#define FALSE 0

typedef uint8_t bool;

typedef struct instruction
{
    // Example: PRINT: &HELLO, WORLD!|

    int32_t iCode; // Each instruction is 4 bytes minimum
    uint8_t *data; // Data to perform instruction on
    size_t dataLen;
} Instruction;

typedef struct vm
{
    Instruction *code;

    size_t ip;
    size_t ipCapture;

    size_t sp;
    size_t lastPushedLength;

    size_t rCount;
    size_t rIndex;

    int64_t registers[VM_REG_COUNT];
    int64_t popReg;
    bool conditionReg;

    uint8_t stack[VM_STACK_SIZE];
} VM;

typedef enum opcodes
{
    PRINT = 397,
    MOVE_REG = 628,
    END_OF_PROGRAM = 1090,
    PUSH = 320,
    DPUSH = 388,
    POP = 239,
    WAIT = 309,
    BJMP = 297,

    // Basically a do while loop, capture will save the instruction following
    // and repeat at that address the given amount of times

    /*
    e.g: 
    CAPTURE_REPEAT:|
    PRINT: &TEST|
    REPEAT: 99| -> Will print 'TEST' 100 times
    END_OF_PROGRAM
    */
    CAPTURE_REPEAT = 1076,
    REPEAT = 449,

    SYS = 255 // expects a string as data
} OPCODES;

VM *VM_Create();
void VM_LoadFile(VM *self, char *file);
Instruction VM_SToIs(char *s);
void VM_Run(VM *self);
Instruction *VM_GetNextInstruction(VM *self);
void VM_Push(VM *self, uint8_t val);
uint8_t VM_Pop(VM *self);
void VM_Free(VM *self);

#endif