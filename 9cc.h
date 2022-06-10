#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// parse.c
//
typedef enum {
    TK_RESERVED,  // 記号
    TK_IDENT,     // 識別子
    TK_NUM,       // 整数
    TK_EOF,       // end-of-file
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;    // kindがTK_NUMの場合、その数値
    char *str;  // トークン文字列
    int len;
};

typedef enum {
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_EQ,      // ==
    ND_NE,      // !=
    ND_LT,      // <
    ND_LE,      // <=
    ND_ASSIGN,  // =
    ND_LVAR,    // local variable
    ND_NUM,
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val;     // Used if kind == ND_NUM
    int offset;  // Used if kind == ND_LVAR
};

Token *tokenize(char *p);
void program();
void error(char *fmt, ...);

//
// codegen.c
//
void codegen();

//
// global
//
extern char *user_input;
extern Token *token;
extern Node *code[100];
