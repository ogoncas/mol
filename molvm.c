#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"

#define STACK_SIZE 512
#define VAR_COUNT 26

Value stack[STACK_SIZE];
int sp = -1;
Value vars[VAR_COUNT];

void init_value(Value *v) {
    v->type = TYPE_STRING;
    v->data.s_value[0] = '\0';
}

void push(Value v) {
    if (sp >= STACK_SIZE - 1) { printf("Erro: Stack Overflow\n"); exit(1); }
    stack[++sp] = v;
}

Value pop() {
    if (sp < 0) { printf("Erro: Stack Underflow\n"); exit(1); }
    return stack[sp--];
}

int is_integer(const char *s) {
    if (!s || !*s) return 0;
    int i = (*s == '-') ? 1 : 0;
    for (; s[i]; i++) if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

int is_floating(const char *s) {
    char *end;
    strtof(s, &end);
    return *end == '\0';
}

int compare_values(Value a, Value b, OpCode op) {
    if (a.type != b.type) { printf("Erro: Tipos incompatíveis em comparação\n"); exit(1); }
    if (a.type == TYPE_INT) {
        int ai = a.data.i_value, bi = b.data.i_value;
        if (op == EQ) return ai == bi;
        if (op == GT) return ai > bi;
        if (op == LT) return ai < bi;
    } else if (a.type == TYPE_FLOAT) {
        float af = a.data.f_value, bf = b.data.f_value;
        if (op == EQ) return af == bf;
        if (op == GT) return af > bf;
        if (op == LT) return af < bf;
    } else if (a.type == TYPE_STRING) {
        int c = strcmp(a.data.s_value, b.data.s_value);
        if (op == EQ) return c == 0;
        if (op == GT) return c > 0;
        if (op == LT) return c < 0;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) { printf("Uso: %s <arquivo.mb>\n", argv[0]); return 1; }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("Erro ao abrir bytecode"); return 1; }

    ProgramHeader header;
    fread(&header, sizeof(ProgramHeader), 1, f);
    char *program = malloc(header.instruction_count);
    if (!program) { perror("malloc"); fclose(f); return 1; }
    fread(program, 1, header.instruction_count, f);
    fclose(f);

    for (int i = 0; i < VAR_COUNT; i++) init_value(&vars[i]);

    int32_t ip = 0;
    int running = 1;

    while (running && ip < header.instruction_count) {
        int32_t instr = *(int32_t*)(program + ip);
        ip += sizeof(int32_t);

        switch (instr) {
            case HALT: running = 0; break;

            case PUSH: {
                int32_t type = *(int32_t*)(program + ip); ip += sizeof(int32_t);
                Value v; v.type = type;
                if (type == TYPE_INT) { v.data.i_value = *(int32_t*)(program + ip); ip += sizeof(int32_t); }
                else if (type == TYPE_FLOAT) { v.data.f_value = *(float*)(program + ip); ip += sizeof(float); }
                else if (type == TYPE_STRING) {
                    unsigned char len = program[ip++];
                    if (len > MAX_STR_LEN) { printf("Erro: String longa demais\n"); running = 0; break; }
                    memcpy(v.data.s_value, program + ip, len); v.data.s_value[len] = '\0'; ip += len;
                }
                push(v); break;
            }

            case ADD: case SUB: case MUL: case DIV: {
                Value b = pop(), a = pop();
                if (a.type != b.type || (a.type != TYPE_INT && a.type != TYPE_FLOAT)) {
                    printf("Erro: Operação aritmética exige INT ou FLOAT iguais\n"); running = 0; break;
                }
                Value res; res.type = a.type;
                if (a.type == TYPE_INT) {
                    int32_t ai = a.data.i_value, bi = b.data.i_value, r = 0;
                    if (instr == ADD) r = ai + bi;
                    else if (instr == SUB) r = ai - bi;
                    else if (instr == MUL) r = ai * bi;
                    else if (instr == DIV) { if (!bi) { printf("Divisão por zero\n"); running = 0; break; } r = ai / bi; }
                    res.data.i_value = r;
                } else {
                    float af = a.data.f_value, bf = b.data.f_value, r = 0.0f;
                    if (instr == ADD) r = af + bf;
                    else if (instr == SUB) r = af - bf;
                    else if (instr == MUL) r = af * bf;
                    else if (instr == DIV) { if (bf == 0.0f) { printf("Divisão por zero\n"); running = 0; break; } r = af / bf; }
                    res.data.f_value = r;
                }
                push(res); break;
            }

            case PRINT: {
                Value v = pop();
                if (v.type == TYPE_INT) printf("%d\n", v.data.i_value);
                else if (v.type == TYPE_FLOAT) printf("%.6g\n", v.data.f_value);
                else printf("%s\n", v.data.s_value);
                break;
            }

            case PRINTC: {
                Value v = pop();
                if (v.type == TYPE_INT) printf("%d", v.data.i_value);
                else if (v.type == TYPE_FLOAT) printf("%.6g", v.data.f_value);
                else printf("%s", v.data.s_value);
                fflush(stdout);
                break;
            }

            case SET: {
                int idx = program[ip++];
                if (idx < 0 || idx >= VAR_COUNT) { printf("Variável inválida\n"); running = 0; break; }
                vars[idx] = pop();
                break;
            }

            case GET: {
                int idx = program[ip++];
                if (idx < 0 || idx >= VAR_COUNT) { printf("Variável invál, inválida\n"); running = 0; break; }
                push(vars[idx]);
                break;
            }

            case INPUT: {
                char buf[256];
                printf("Input: ");
                if (!fgets(buf, sizeof(buf), stdin)) { printf("Erro de entrada\n"); running = 0; break; }
                buf[strcspn(buf, "\n")] = '\0';
                Value v;
                if (is_integer(buf)) { v.type = TYPE_INT; v.data.i_value = atoi(buf); }
                else if (is_floating(buf)) { v.type = TYPE_FLOAT; v.data.f_value = strtof(buf, NULL); }
                else {
                    v.type = TYPE_STRING;
                    strncpy(v.data.s_value, buf, MAX_STR_LEN);
                    v.data.s_value[MAX_STR_LEN] = '\0';
                }
                push(v);
                break;
            }

            case CONCAT: {
                Value b = pop(), a = pop();
                if (a.type != TYPE_STRING || b.type != TYPE_STRING) { printf("CONCAT exige duas strings\n"); running = 0; break; }
                size_t la = strlen(a.data.s_value), lb = strlen(b.data.s_value);
                if (la + lb > MAX_STR_LEN) { printf("Concatenação excede limite\n"); running = 0; break; }
                Value res; res.type = TYPE_STRING;
                strcpy(res.data.s_value, a.data.s_value);
                strcat(res.data.s_value, b.data.s_value);
                push(res);
                break;
            }

            case STRLEN: {
                Value v = pop();
                if (v.type != TYPE_STRING) { printf("STRLEN exige string\n"); running = 0; break; }
                Value res; res.type = TYPE_INT; res.data.i_value = strlen(v.data.s_value);
                push(res);
                break;
            }

            case EQ: case GT: case LT: {
                Value b = pop(), a = pop();
                Value res; res.type = TYPE_INT;
                res.data.i_value = compare_values(a, b, instr);
                push(res);
                break;
            }

            case TOSTRING: {
                Value v = pop();
                Value res; res.type = TYPE_STRING;
                if (v.type == TYPE_INT) snprintf(res.data.s_value, 256, "%d", v.data.i_value);
                else if (v.type == TYPE_FLOAT) snprintf(res.data.s_value, 256, "%.6g", v.data.f_value);
                else { printf("TOSTRING só aceita INT ou FLOAT\n"); running = 0; break; }
                push(res);
                break;
            }

            case DUP: {
                if (sp < 0) { printf("DUP em pilha vazia\n"); running = 0; break; }
                push(stack[sp]);
                break;
            }

            case SWAP: {
                if (sp < 1) { printf("SWAP precisa de 2 itens\n"); running = 0; break; }
                Value a = pop(), b = pop();
                push(a); push(b);
                break;
            }

            case POP: {
                if (sp < 0) { printf("POP em pilha vazia\n"); running = 0; break; }
                pop();
                break;
            }

            case JMP: {
                int32_t offset = *(int32_t*)(program + ip); ip += sizeof(int32_t);
                ip += offset;
                break;
            }

            case JZ: {
                Value cond = pop();
                int zero = (cond.type == TYPE_INT && cond.data.i_value == 0) ||
                           (cond.type == TYPE_FLOAT && cond.data.f_value == 0.0f);
                int32_t offset = *(int32_t*)(program + ip); ip += sizeof(int32_t);
                if (zero) ip += offset;
                break;
            }

            default:
                printf("Opcode desconhecido: %d\n", instr);
                running = 0;
                break;
        }
    }

    free(program);
    return 0;
}