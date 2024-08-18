#include "vm.h"

VM *VM_Create()
{
    VM *ret = (VM *)malloc(sizeof(VM));
    ret->ip = 0;
    ret->sp = 0;
    ret->lastPushedLength = 0;
    ret->popReg = 0;
    ret->rCount = 0;
    ret->rIndex = 0;

    memset(ret->stack, 0, VM_STACK_SIZE);
    if (ret)
        return ret;
    return NULL;
}

void VM_LoadFile(VM *self, char *file)
{
    if (!self)
        return;

    FILE *f = fopen(file, "rb");
    if (!f)
    {
        perror("Failed to open file");
        return;
    }

    fseek(f, 0, SEEK_END);
    size_t fileLen = ftell(f);
    rewind(f);

    char lineBuffer[MAX_LINE_LEN];
    size_t isCount = 0;
    while (fgets(lineBuffer, MAX_LINE_LEN, f) != NULL)
    {
        isCount++;
    }

    self->code = (Instruction *)malloc(isCount * sizeof(Instruction));
    if (!self->code)
    {
        fclose(f);
        return;
    }

    rewind(f);
    size_t i = 0;
    while (fgets(lineBuffer, MAX_LINE_LEN, f) != NULL && i < isCount)
    {
        self->code[i++] = VM_SToIs(lineBuffer);
    }

    fclose(f);
}

Instruction VM_SToIs(char *s)
{
    Instruction is = {0, NULL, 0};
    if (s[0] == ';')
    {
        return is;
    }

    // Get the instruction code first
    size_t i = 0;
    while (s[i] != ':' && s[i] != '\0')
    {
        is.iCode += s[i++]; // Create instruction code from characters
    }

    // Get data
    if (s[i] == ':')
        i++; // skip over colon

    bool isStr = FALSE;
    bool readingData = FALSE;
    size_t dataSize = 0;
    size_t dataStart = i;

    while (s[i] != '|' && s[i] != '\0')
    {
        if (readingData)
        {
            i++;
            dataSize++;
            continue;
        }

        if (s[i] == ' ')
        {
            i++;
            dataStart++;
            continue;
        }

        if (s[i] == '&' && !isStr)
        {
            isStr = TRUE;
            readingData = TRUE;
            dataStart = i + 1; // Skip '&' and set dataStart
            continue;
        }

        i++;
        dataSize++;
    }

    is.data = (uint8_t *)malloc(dataSize + 1); // +1 for possible null terminator
    if (!is.data)
    {
        return is;
    }

    for (size_t j = 0; j < dataSize; j++)
    {
        is.data[j] = s[dataStart + j];
        if (!isStr)
        {
            is.data[j] = GETASCIIRAW(is.data[j]); // convert to binary
            DEBUGF("HEX DATA: %#X\n", is.data[j]);
        }
    }

    if (isStr)
    {
        is.data[dataSize - 1] = '\0'; // null-terminate if it's a string
    }

    is.dataLen = dataSize;
    DEBUGF("Loaded instruction: %d\n", is.iCode);
    return is;
}

void VM_Run(VM *self)
{
    // Get start time
    clock_t startTime = clock();
    double elapsedTime = 0;

    while (1)
    {
        Instruction *instruction = VM_GetNextInstruction(self);
        switch (instruction->iCode)
        {
        case PRINT:
            printf("%s\n", instruction->data);
            break;
        case MOVE_REG:
        {
            uint8_t reg = instruction->data[0] * 10 + instruction->data[1];
            if (reg >= VM_REG_COUNT)
                break;

            // Initialize the register value
            self->registers[reg] = 0;

            // Combine the data values into the register
            for (size_t i = 2; i < instruction->dataLen; i++)
            {
                self->registers[reg] = (self->registers[reg] * 10) | instruction->data[i];
            }

            DEBUGF("Pushed %#X", self->registers[reg]);
            DEBUGF(" to reg: %d\n", reg);
            break;
        }
        case END_OF_PROGRAM:
            DEBUG("End of program reached!\n");
            goto endOfProgram;
        case PUSH:
        {
            for (size_t i = 0; i < instruction->dataLen; i++)
            {
                VM_Push(self, instruction->data[i]);
            }
            self->lastPushedLength = instruction->dataLen;
            DEBUGF("Pushed to stack something of size: %d\n", self->lastPushedLength);
            break;
        }
        case DPUSH:
        {
            size_t size = instruction->dataLen * sizeof(uint8_t);
            void *mem = malloc(size);
            if (mem)
            {
                memcpy(mem, instruction->data, size);
                DEBUGF("Dynamically pushed to mem something of size: %zu\n", size);
            }
            break;
        }
        case WAIT:
        {
            DWORD waitTime = 0;
            for (size_t i = 0; i < instruction->dataLen; i++)
            {
                waitTime = (waitTime * 10) + instruction->data[i];
            }
            DEBUGF("Waiting for: %lu ms\n", waitTime);
            Sleep(waitTime);
            break;
        }
        case POP:
        {
            for (size_t i = 0; i < self->lastPushedLength; i++)
            {
                uint8_t pop = VM_Pop(self);
                DEBUGF("Popped: %#X\n", pop);
            }
            break;
        }
        case BJMP:
        {
            self->ip = 0;
            // Combine the data values into the ip
            for (size_t i = 0; i < instruction->dataLen; i++)
            {
                self->ip = (self->ip * 10) + instruction->data[i];
            }
            DEBUGF("Jumped to ip: %#X\n", self->ip);
            break;
        }
        case CAPTURE_REPEAT:
        {
            self->ipCapture = self->ip; // Save current ip
            DEBUGF("Set new captured ip: %d\n", self->ipCapture);
            break;
        }
        case REPEAT:
        {
            if (!self->rCount)
            {
                if (!instruction->dataLen)
                    self->rCount = 1;
                for (size_t i = 0; i < instruction->dataLen; i++)
                {
                    self->rCount = (self->rCount * 10) + instruction->data[i];
                }
                self->rIndex = 0;
            }

            if (self->rIndex <= self->rCount)
            {
                self->rIndex++;
                self->ip = self->ipCapture;
                DEBUGF("Rewinding to captured ip: %d", self->ipCapture);
                DEBUGF(" | rCount: %u\n", self->rCount);
            }
            break;
        }
        case SYS:
        {
            system(instruction->data);
            break;
        }
        default:
            break;
        }
    }

endOfProgram:
    elapsedTime = (double)(clock() - startTime) / CLOCKS_PER_SEC * 1000;
    printf("Program took %.2f ms to execute!\n", elapsedTime);
}

Instruction *VM_GetNextInstruction(VM *self)
{
    return &self->code[self->ip++];
}

void VM_Push(VM *self, uint8_t val)
{

    self->stack[++self->sp] = val;
    return;
}

uint8_t VM_Pop(VM *self)
{
    if (self->sp != -1)
    {
        return self->stack[self->sp--];
    }
    return 0;
}

void VM_Free(VM *self)
{
    if (self)
    {
        free(self->code);
        free(self);
    }
}
