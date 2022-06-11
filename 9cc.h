#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//
typedef enum {
    TK_RESERVED,  // 記号, 予約語
    TK_IDENT,     // 識別子
    TK_NUM,       // 整数
    TK_EOF,       // end-of-file
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;    // kindがTK_NUMの場合、その数値
    char *str;  // そのトークンとそれ以降の文字列
    int len;
};

typedef struct LVar LVar;

//
// parse.c
//
struct LVar {
    // ローカル変数
    LVar *next;  // 次の変数
    char *name;  // 変数名
    int len;     // 名前の長さ
    int offset;  // RBPからのオフセット
};

void error_at(char *loc, char *fmt, ...);

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
    ND_RETURN,  // return
    ND_IF,      // if
    ND_ELSE,    // else
    ND_WHILE,   // while
    ND_FOR,     // for
    ND_NUM,
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val;     // Used if kind == ND_NUM
    int offset;  // Used if kind == ND_LVAR

    // if (A) B else C
    // while(A) B
    // for (D; A; E) B
    Node *cond;  // A
    Node *then;  // B
    Node *els;   // C
    Node *init;  // D
    Node *inc;   // E
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
extern LVar *locals;
