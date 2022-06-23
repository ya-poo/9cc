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

typedef struct Type Type;

struct Type {
    enum { INT, PTR } kind;
    struct Type *ptr_to;
};

struct Var {
    // 変数
    char *name;  // 変数名
    int offset;  // RBPからのオフセット
    Type *type;  // 変数の型
};

typedef struct VarList VarList;

struct VarList {
    VarList *tail;
    Var *head;
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
    ND_DECL,     // declaration
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
    char *name;  // 関数名
    Node *node;
    VarList *locals;  // ローカル変数
    VarList *params;  // 引数
    int stack_size;
    Type *type;      // 戻り値の型
    Function *next;  // 次の関数
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
