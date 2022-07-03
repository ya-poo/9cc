#define _GNU_SOURCE
#include <assert.h>
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
    TK_STRING,    // 文字列
    TK_EOF,       // end-of-file
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    char *str;  // そのトークンとそれ以降の文字列
    int len;    // トークンの長さ

    int val;  // Used if kind == TK_NUM

    // Used if kind == TK_STRING
    char *contents;
    int contents_len;
};

typedef struct Var Var;

//
// parse.c
//

typedef enum { TY_INT, TY_PTR, TY_ARRAY, TY_CHAR } TypeKind;

typedef struct Type Type;

struct Type {
    TypeKind kind;

    struct Type *ptr_to;  // Used if kind == TY_PTR, TY_ARRAY
    size_t array_size;    // Used if kind == TY_ARRAY
};

struct Var {
    // 変数
    char *name;  // 変数名
    Type *type;  // 変数の型

    int offset;  // RBPからのオフセット。ローカル変数でのみ使用。

    // Used for string literal
    char *contents;
    int contents_len;
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
    ND_SIZEOF,   // sizeof
    ND_NUM,
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;

    Type *type;

    Token *token;

    int val;  // Used if kind == ND_NUM

    // Used if kind == ND_VAR
    Var *var;
    bool is_local;

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

typedef struct Program Program;
struct Program {
    VarList *global;  // グローバル変数
    Function *functions;
};

Token *tokenize(char *p);
Program *program();
void error(char *fmt, ...);

//
// codegen.c
//
void codegen(Program *program);

//
// type.c
//
int size_of(Type *type);
void annotate_type(Program *program);
Type *array_of(Type *base, int size);
Type *int_type();
Type *char_type();

//
// global
//
extern char *user_input;
extern Token *token;
