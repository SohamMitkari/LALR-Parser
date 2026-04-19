/*
 * LALR(1) Parser Implementation in C
 *
 * Grammar (arithmetic expressions):
 *   0: S' -> E          (augmented start)
 *   1: E  -> E + T
 *   2: E  -> T
 *   3: T  -> T * F
 *   4: T  -> F
 *   5: F  -> ( E )
 *   6: F  -> id
 *
 * Terminals:  id, +, *, (, ), $
 * Non-terminals: E, T, F
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_STACK  256
#define MAX_INPUT  256

typedef enum {
    T_ID = 0, T_PLUS, T_STAR, T_LPAREN, T_RPAREN, T_DOLLAR, T_COUNT
} Terminal;

typedef enum {
    NT_E = 0, NT_T, NT_F, NT_COUNT
} NonTerminal;

static const char *nonterm_names[NT_COUNT] = { "E", "T", "F" };

typedef struct { int lhs; int rhs_len; } Rule;

static const Rule rules[] = {
    {-1, 1},  /* 0: S' -> E        */
    { 0, 3},  /* 1: E  -> E + T    */
    { 0, 1},  /* 2: E  -> T        */
    { 1, 3},  /* 3: T  -> T * F    */
    { 1, 1},  /* 4: T  -> F        */
    { 2, 3},  /* 5: F  -> ( E )    */
    { 2, 1},  /* 6: F  -> id       */
};

#define ACCEPT 9999
#define ERR    0

static const int action[12][T_COUNT] = {
/*         id     +      *      (      )      $    */
/* 0 */  {  5,   ERR,   ERR,    4,    ERR,   ERR },
/* 1 */  { ERR,    6,   ERR,   ERR,   ERR,  ACCEPT},
/* 2 */  { ERR,   -2,    7,   ERR,    -2,    -2  },
/* 3 */  { ERR,   -4,   -4,   ERR,    -4,    -4  },
/* 4 */  {  5,   ERR,   ERR,    4,    ERR,   ERR },
/* 5 */  { ERR,   -6,   -6,   ERR,    -6,    -6  },
/* 6 */  {  5,   ERR,   ERR,    4,    ERR,   ERR },
/* 7 */  {  5,   ERR,   ERR,    4,    ERR,   ERR },
/* 8 */  { ERR,    6,   ERR,   ERR,    11,   ERR },
/* 9 */  { ERR,   -1,    7,   ERR,    -1,    -1  },
/*10 */  { ERR,   -3,   -3,   ERR,    -3,    -3  },
/*11 */  { ERR,   -5,   -5,   ERR,    -5,    -5  },
};

static const int goto_tbl[12][NT_COUNT] = {
/*        E   T   F  */
/* 0 */ { 1,  2,  3 },
/* 1 */ { 0,  0,  0 },
/* 2 */ { 0,  0,  0 },
/* 3 */ { 0,  0,  0 },
/* 4 */ { 8,  2,  3 },
/* 5 */ { 0,  0,  0 },
/* 6 */ { 0,  9,  3 },
/* 7 */ { 0,  0, 10 },
/* 8 */ { 0,  0,  0 },
/* 9 */ { 0,  0,  0 },
/*10 */ { 0,  0,  0 },
/*11 */ { 0,  0,  0 },
};

static const char *input_ptr;

typedef struct { Terminal type; char lexeme[64]; } Token;

static Token next_token(void) {
    Token tok;
    while (*input_ptr && isspace((unsigned char)*input_ptr)) input_ptr++;
    char c = *input_ptr;
    if (!c) { tok.type = T_DOLLAR; strcpy(tok.lexeme, "$"); return tok; }
    switch (c) {
        case '+': tok.type = T_PLUS;   strcpy(tok.lexeme, "+"); input_ptr++; break;
        case '*': tok.type = T_STAR;   strcpy(tok.lexeme, "*"); input_ptr++; break;
        case '(': tok.type = T_LPAREN; strcpy(tok.lexeme, "("); input_ptr++; break;
        case ')': tok.type = T_RPAREN; strcpy(tok.lexeme, ")"); input_ptr++; break;
        default:
            if (isalpha((unsigned char)c) || c == '_') {
                int i = 0;
                while (isalnum((unsigned char)*input_ptr) || *input_ptr == '_')
                    tok.lexeme[i++] = *input_ptr++;
                tok.lexeme[i] = '\0';
                tok.type = T_ID;
            } else {
                fprintf(stderr, "Unknown character '%c'\n", c);
                input_ptr++;
                tok.type = T_DOLLAR;
                strcpy(tok.lexeme, "$");
            }
    }
    return tok;
}

static int parse(const char *expr) {
    input_ptr = expr;

    int  state_stack[MAX_STACK];
    char sym_stack[MAX_STACK][32];
    int  top = 0;
    state_stack[0] = 0;
    strcpy(sym_stack[0], "$");

    Token tok = next_token();
    int   step = 1;

    /* Print header that index.html looks for */
    printf("Input: \"%s\"\n", expr);
    printf("\n%-4s %-30s %-20s %-12s %-20s\n",
           "Step", "Stack (states)", "Symbol stack", "Lookahead", "Action");
    printf("--------------------------------------------------------------------------------\n");

    while (1) {
        int s   = state_stack[top];
        int a   = (int)tok.type;
        int act = action[s][a];

        /* Build state-stack string */
        char stack_str[256] = {0};
        for (int i = 0; i <= top; i++) {
            char tmp[16];
            sprintf(tmp, "%d ", state_stack[i]);
            strcat(stack_str, tmp);
        }
        /* Build symbol-stack string */
        char sym_str[256] = {0};
        for (int i = 0; i <= top; i++) {
            strcat(sym_str, sym_stack[i]);
            strcat(sym_str, " ");
        }

        if (act == ACCEPT) {
            printf("%-4d %-30s %-20s %-12s ACCEPT\n",
                   step, stack_str, sym_str, tok.lexeme);
            printf("\nInput accepted! The expression is syntactically valid.\n");
            return 1;
        } else if (act > 0) {
            /* SHIFT */
            printf("%-4d %-30s %-20s %-12s Shift -> state %d\n",
                   step++, stack_str, sym_str, tok.lexeme, act);
            if (top + 1 >= MAX_STACK) { fprintf(stderr, "Stack overflow\n"); return 0; }
            state_stack[++top] = act;
            strncpy(sym_stack[top], tok.lexeme, 31);
            sym_stack[top][31] = '\0';
            tok = next_token();
        } else if (act < 0) {
            /* REDUCE */
            int r    = -act;
            int lhs  = rules[r].lhs;
            int rlen = rules[r].rhs_len;
            printf("%-4d %-30s %-20s %-12s Reduce by rule %d (%s -> ...)\n",
                   step++, stack_str, sym_str, tok.lexeme, r, nonterm_names[lhs]);
            top -= rlen;
            if (top < 0) { fprintf(stderr, "Stack underflow\n"); return 0; }
            int new_state = goto_tbl[state_stack[top]][lhs];
            if (new_state == 0 && !(state_stack[top] == 0 && lhs == 0)) {
                fprintf(stderr, "Error in goto table\n"); return 0;
            }
            state_stack[++top] = new_state;
            strncpy(sym_stack[top], nonterm_names[lhs], 31);
            sym_stack[top][31] = '\0';
        } else {
            /* ERROR */
            printf("%-4d %-30s %-20s %-12s ERROR\n",
                   step, stack_str, sym_str, tok.lexeme);
            printf("\nSyntax error near token '%s'\n", tok.lexeme);
            return 0;
        }
    }
}

int main(void) {
    char buf[MAX_INPUT];
    while (1) {
        printf("\n> ");
        fflush(stdout);
        if (!fgets(buf, sizeof buf, stdin)) break;
        buf[strcspn(buf, "\n")] = '\0';
        if (strcmp(buf, "quit") == 0 || strcmp(buf, "exit") == 0) break;
        if (buf[0] == '\0') continue;
        parse(buf);
    }
    return 0;
}
