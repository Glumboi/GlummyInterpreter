#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "vm.h"

#define CODE "F:\\CSources\\GlumInterpreter\\demoCode.txt"

int main(int argc, char *argv[])
{
    char *file = NULL;
    if (argc > 1)
    {
        file = argv[1]; // Get file from args
    }

    if (!file)
        file = CODE;

    VM *vm = VM_Create();
    VM_LoadFile(vm, file);
    VM_Run(vm);
    VM_Free(vm);
    return 0;
}
