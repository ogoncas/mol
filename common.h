#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define MAX_STR_LEN 255

typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_FLOAT
} Type;

typedef struct {
    Type type;
    union {
        int32_t i_value;
        char s_value[256];  // +1 para o terminador nulo
        float f_value;
    } data;
} Value;

typedef enum {
    HALT = 0,
    PUSH,
    ADD,
    SUB,
    MUL,
    DIV,
    PRINT,      // com \n
    PRINTC,     // sem \n
    SET,
    GET,
    INPUT,
    CONCAT,
    STRLEN,
    EQ,
    GT,
    LT,
    JMP,
    JZ,
    TOSTRING,
    DUP,
    SWAP,
    POP
} OpCode;

typedef struct {
    int32_t instruction_count;
} ProgramHeader;

#endif