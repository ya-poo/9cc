#define _GNU_SOURCE
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

typedef struct Var Var;

//
// parse.c
//
struct Var {
    // ローカル変数
    Var *next;   // 次の変数
    char *name;  // 変数名
    int len;     // 名前の長さ
    int offset;  // RBPからのオフセット
};

void error_at(char *loc, char *fmt, ...);

typedef enum {
    ND_ADD,      // +
    ND_SUB,      // -
    ND_MUL,      // *
    ND_DIV,      // /
    ND_EQ,       // ==
    ND_NE,       // !=
    ND_LT,       // <
    ND_LE,       // <=
    ND_ASSIGN,   // =
    ND_VAR,      // variable
    ND_RETURN,   // return
    ND_IF,       // if
    ND_ELSE,     // else
    ND_WHILE,    // while
    ND_FOR,      // for
    ND_BLOCK,    // { ... }
    ND_FUNCALL,  // call function
    ND_FUNC,     // function
    ND_DEREF,    // dereference
    ND_ADDR,     // address
    ND_NUM,
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;

    int val;     // Used if kind == ND_NUM
    int offset;  // Used if kind == ND_VAR

    Node *next;  // Used if kind == ND_BLOCK

    // Used if kind == ND_FUNCALL
    char *funcname;
    Node *args;

    // if (A) B else C
    // while(A) B
    // for (D; A; E) B
    Node *cond;  // A
    Node *then;  // B
    Node *els;   // C
    Node *init;  // D
    Node *inc;   // E
};

typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    Node *node;
    Var *locals;  // ローカル変数
    Var *params;  // 引数
    int stack_size;
};

Token *tokenize(char *p);
Function *program();
void error(char *fmt, ...);

//
// codegen.c
//
void codegen(Function *functions);

//
// global
//
extern char *user_input;
extern Token *token;
