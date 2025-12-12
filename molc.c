#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include <stdarg.h>

#define MAX_LABELS 100
#define MAX_PATCHES 1024

struct Label {
    char name[32];
    int32_t position;
} labels[MAX_LABELS];
int label_count = 0;

struct Patch {
    char label_name[32];
    int32_t patch_pos;
} patches[MAX_PATCHES];
int patch_count = 0;

void error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}

int is_integer(const char *s) {
    if (!s || !*s) return 0;
    int i = (*s == '-') ? 1 : 0;
    for (; s[i]; i++) if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

int is_floating(const char *s) {
    char *endptr;
    strtof(s, &endptr);
    return *endptr == '\0' && strchr(s, '.') != NULL;
}

int32_t find_label(const char *name) {
    for (int i = 0; i < label_count; i++)
        if (strcmp(labels[i].name, name) == 0) return labels[i].position;
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <arquivo_origem.mol> <arquivo_saida.mb>\n", argv[0]);
        return 1;
    }

    FILE *source = fopen(argv[1], "r");
    if (!source) { perror("Erro ao abrir arquivo de origem"); return 1; }

    char bytecode[16384];
    int32_t ip = 0;
    char token[64];

    while (fscanf(source, "%63s", token) != EOF) {
        // Labels: LOOP:
        if (strchr(token, ':')) {
            token[strlen(token)-1] = '\0';
            if (label_count >= MAX_LABELS) error("Muitas labels\n");
            strcpy(labels[label_count].name, token);
            labels[label_count].position = ip;
            label_count++;
            continue;
        }

        if (strcmp(token, "PUSH") == 0) {
            *(int32_t*)(bytecode + ip) = PUSH; ip += sizeof(int32_t);

            char ch = fgetc(source);
            while (isspace(ch)) ch = fgetc(source);
            ungetc(ch, source);

            if (ch == '"') {
                char str[MAX_STR_LEN + 2];
                if (fscanf(source, "\"%255[^\"]\"", str) != 1)
                    error("String malformada após PUSH\n");
                size_t len = strlen(str);
                if (len > MAX_STR_LEN) error("String muito longa\n");

                *(int32_t*)(bytecode + ip) = TYPE_STRING; ip += sizeof(int32_t);
                bytecode[ip++] = (unsigned char)len;
                memcpy(bytecode + ip, str, len); ip += len;
            } else {
                char num[32];
                if (fscanf(source, "%31s", num) != 1) error("Valor esperado após PUSH\n");
                if (is_integer(num)) {
                    *(int32_t*)(bytecode + ip) = TYPE_INT; ip += sizeof(int32_t);
                    *(int32_t*)(bytecode + ip) = atoi(num); ip += sizeof(int32_t);
                } else if (is_floating(num)) {
                    *(int32_t*)(bytecode + ip) = TYPE_FLOAT; ip += sizeof(int32_t);
                    *(float*)(bytecode + ip) = strtof(num, NULL); ip += sizeof(float);
                } else error("Tipo inválido para PUSH: %s\n", num);
            }
        }
        else if (strcmp(token, "ADD") == 0)     { *(int32_t*)(bytecode + ip) = ADD;     ip += sizeof(int32_t); }
        else if (strcmp(token, "SUB") == 0)     { *(int32_t*)(bytecode + ip) = SUB;     ip += sizeof(int32_t); }
        else if (strcmp(token, "MUL") == 0)     { *(int32_t*)(bytecode + ip) = MUL;     ip += sizeof(int32_t); }
        else if (strcmp(token, "DIV") == 0)     { *(int32_t*)(bytecode + ip) = DIV;     ip += sizeof(int32_t); }
        else if (strcmp(token, "PRINT") == 0)   { *(int32_t*)(bytecode + ip) = PRINT;   ip += sizeof(int32_t); }
        else if (strcmp(token, "PRINTC") == 0)  { *(int32_t*)(bytecode + ip) = PRINTC;  ip += sizeof(int32_t); }
        else if (strcmp(token, "HALT") == 0)    { *(int32_t*)(bytecode + ip) = HALT;    ip += sizeof(int32_t); }
        else if (strcmp(token, "SET") == 0) {
            *(int32_t*)(bytecode + ip) = SET; ip += sizeof(int32_t);
            char var[5];
            if (fscanf(source, "%4s", var) != 1 || !isupper(var[0]) || var[1]) error("Variável inválida\n");
            bytecode[ip++] = toupper(var[0]) - 'A';
        }
        else if (strcmp(token, "GET") == 0) {
            *(int32_t*)(bytecode + ip) = GET; ip += sizeof(int32_t);
            char var[5];
            if (fscanf(source, "%4s", var) != 1 || !isupper(var[0]) || var[1]) error("Variável inválida\n");
            bytecode[ip++] = toupper(var[0]) - 'A';
        }
        else if (strcmp(token, "INPUT") == 0)   { *(int32_t*)(bytecode + ip) = INPUT;   ip += sizeof(int32_t); }
        else if (strcmp(token, "CONCAT") == 0)  { *(int32_t*)(bytecode + ip) = CONCAT;  ip += sizeof(int32_t); }
        else if (strcmp(token, "STRLEN") == 0)  { *(int32_t*)(bytecode + ip) = STRLEN;  ip += sizeof(int32_t); }
        else if (strcmp(token, "EQ") == 0)      { *(int32_t*)(bytecode + ip) = EQ;      ip += sizeof(int32_t); }
        else if (strcmp(token, "GT") == 0)      { *(int32_t*)(bytecode + ip) = GT;      ip += sizeof(int32_t); }
        else if (strcmp(token, "LT") == 0)      { *(int32_t*)(bytecode + ip) = LT;      ip += sizeof(int32_t); }
        else if (strcmp(token, "TOSTRING") == 0){ *(int32_t*)(bytecode + ip) = TOSTRING;ip += sizeof(int32_t); }
        else if (strcmp(token, "DUP") == 0)     { *(int32_t*)(bytecode + ip) = DUP;     ip += sizeof(int32_t); }
        else if (strcmp(token, "SWAP") == 0)    { *(int32_t*)(bytecode + ip) = SWAP;    ip += sizeof(int32_t); }
        else if (strcmp(token, "POP") == 0)     { *(int32_t*)(bytecode + ip) = POP;     ip += sizeof(int32_t); }
        else if (strcmp(token, "JMP") == 0 || strcmp(token, "JZ") == 0) {
            *(int32_t*)(bytecode + ip) = (strcmp(token, "JMP")==0) ? JMP : JZ;
            ip += sizeof(int32_t);
            char label[32];
            if (fscanf(source, "%31s", label) != 1) error("Label esperada após %s\n", token);
            strcpy(patches[patch_count].label_name, label);
            patches[patch_count].patch_pos = ip;
            patch_count++;
            ip += sizeof(int32_t);  // placeholder para offset
        }
        else error("Token desconhecido: %s\n", token);
    }

    // Resolver labels
    for (int i = 0; i < patch_count; i++) {
        int32_t pos = find_label(patches[i].label_name);
        if (pos == -1) error("Label não encontrada: %s\n", patches[i].label_name);
        int32_t offset = pos - (patches[i].patch_pos + sizeof(int32_t));
        *(int32_t*)(bytecode + patches[i].patch_pos) = offset;
    }

    fclose(source);

    FILE *out = fopen(argv[2], "wb");
    if (!out) { perror("Erro ao abrir arquivo de saída"); return 1; }
    ProgramHeader header = { ip };
    fwrite(&header, sizeof(ProgramHeader), 1, out);
    fwrite(bytecode, 1, ip, out);
    fclose(out);

    printf("Compilado com sucesso: %s → %s (%d bytes)\n", argv[1], argv[2], ip);
    return 0;
}