#include "9cc.h"

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
    if (token->kind != TK_RESERVED) {
        return false;
    }

    if (strlen(op) != token->len || memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが ND_IDENT の場合は 1 つ読み進めてその ND_IDENT
// のトークンを返す。
Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *ident_token = token;
    token = token->next;
    return ident_token;
}

// 次のトークンが期待している記号のときには true を返す。
// トークンは読み進めない。
bool peek(char *s) {
    if (token->kind != TK_RESERVED || strlen(s) != token->len ||
        memcmp(token->str, s, token->len)) {
        return false;
    }

    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        error_at(token->str, "'%s'ではありません", op);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

// 次のトークンが ND_IDENT の場合は 1 つ読み進めてその ident を返す。
// それ以外の場合はエラーを報告する。
char *expect_ident() {
    Token *ident = consume_ident();
    if (!ident) {
        error_at(token->str, "識別子ではありません");
    }
    return strndup(ident->str, ident->len);
}

bool at_eof() { return token->kind == TK_EOF; }

Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->token = token;
    return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_num(int val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

void global_var();
Function *function();
Type *basetype();
Type *read_type_suffix(Type *base);
Node *declaration();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *func_args_or_null();
Node *primary();

bool is_function() {
    Token *tok = token;
    basetype();
    bool is_func = consume_ident() && consume("(");
    token = tok;

    return is_func;
}

Program *current_program;
Function *current_function;

void push_global_var(Var *var) {
    VarList *head = calloc(1, sizeof(VarList));
    head->head = var;
    head->tail = current_program->global;
    current_program->global = head;
}

// global-bar = basetype ident ("[" num "]")* ";"
void global_var() {
    Var *var = calloc(1, sizeof(Var));
    Type *type = basetype();
    var->name = expect_ident();
    for (VarList *vl = current_program->global; vl; vl = vl->tail) {
        if (!strcmp(vl->head->name, var->name)) {
            error_at(token->str, "定義済みのグローバル変数と名前が重複しています");
        }
    }
    type = read_type_suffix(type);
    var->type = type;
    expect(";");
    push_global_var(var);
}

// program = (function | grobal-var)*
Program *program() {
    current_program = calloc(1, sizeof(Program));
    Function head;
    head.next = NULL;
    Function *cur = &head;

    while (!at_eof()) {
        if (is_function()) {
            cur->next = function();
            cur = cur->next;
        } else {
            global_var();
        }
    }

    // Set offset of params / locals
    for (Function *fun = head.next; fun; fun = fun->next) {
        int offset = 0;
        for (VarList *vl = fun->locals; vl; vl = vl->tail) {
            offset += size_of(vl->head->type);
            vl->head->offset = offset;
        }
        for (VarList *vl = fun->params; vl; vl = vl->tail) {
            offset += size_of(vl->head->type);
            vl->head->offset = offset;
        }
        fun->stack_size = offset;
    }

    current_program->functions = head.next;

    return current_program;
}

VarList *find_local_var(char *ident) {
    for (VarList *list = current_function->locals; list; list = list->tail) {
        if (!strcmp(list->head->name, ident)) {
            return list;
        }
    }
    for (VarList *list = current_function->params; list; list = list->tail) {
        if (!strcmp(list->head->name, ident)) {
            return list;
        }
    }

    return NULL;
}

VarList *find_global_var(char *ident) {
    for (VarList *list = current_program->global; list; list = list->tail) {
        if (!strcmp(list->head->name, ident)) {
            return list;
        }
    }

    return NULL;
}

// basetype = ("int" | "char") "*"*
Type *basetype() {
    TypeKind kind;
    if (consume("char")) {
        kind = TY_CHAR;
    } else {
        expect("int");
        kind = TY_INT;
    }

    Type head;
    Type *cur = calloc(1, sizeof(Type));
    head.ptr_to = cur;
    while (consume("*")) {
        cur->ptr_to = calloc(1, sizeof(Type));
        cur->kind = TY_PTR;
        cur = cur->ptr_to;
    }
    cur->kind = kind;

    return head.ptr_to;
}

// var = basetype ident
Var *var() {
    Var *var = calloc(1, sizeof(Var));
    var->type = basetype();
    var->name = expect_ident();
    if (find_local_var(var->name)) {
        error_at(token->str, "定義済みの変数名と重複しています");
    }

    return var;
}

void *push_local_var(Var *var) {
    VarList *head = calloc(1, sizeof(VarList));
    head->head = var;
    head->tail = current_function->locals;
    current_function->locals = head;
}

// func-params = "(" (var ("," var)*)? ")"
void *func_params() {
    expect("(");
    if (consume(")")) {
        return NULL;
    }

    VarList *head = calloc(1, sizeof(VarList));
    head->head = var();
    VarList *cur = head;

    while (!consume(")")) {
        expect(",");
        VarList *tail = calloc(1, sizeof(VarList));
        cur->tail = tail;
        tail->head = var();
        cur = tail;
    }

    current_function->params = head;
}

// function = basetype ident func-args "{" stmt* "}"
Function *function() {
    Function *fn = calloc(1, sizeof(Function));
    current_function = fn;

    fn->type = basetype();
    fn->name = expect_ident();
    func_params();

    expect("{");

    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    fn->node = head.next;
    return fn;
}

Type *read_type_suffix(Type *base) {
    if (!consume("[")) {
        return base;
    }
    Type *type = calloc(1, sizeof(Type));
    type->kind = TY_ARRAY;
    type->array_size = expect_number();
    expect("]");
    type->ptr_to = read_type_suffix(base);
    return type;
}

// declaration = basetype ident ("[" num "]")* ";"
Node *declaration() {
    Var *var = calloc(1, sizeof(Var));

    Type *base = basetype();
    var->name = expect_ident();

    if (find_local_var(var->name)) {
        error_at(token->str, "定義済みの変数名と重複しています");
    }
    var->type = read_type_suffix(base);

    push_local_var(var);
    expect(";");
    return new_node(ND_DECL);
}

bool is_typename() { return peek("char") || peek("int"); }

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | declaration
//      | expr ";"
Node *stmt() {
    Node *node;
    if (consume("return")) {
        node = new_node(ND_RETURN);
        node->lhs = expr();
        expect(";");
        return node;
    }

    if (consume("if")) {
        node = new_node(ND_IF);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else")) {
            node->els = stmt();
        }
        return node;
    }

    if (consume("while")) {
        node = new_node(ND_WHILE);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }

    if (consume("for")) {
        node = new_node(ND_FOR);
        expect("(");
        if (!consume(";")) {
            node->init = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->inc = expr();
            expect(")");
        }
        node->then = stmt();
        return node;
    }

    if (consume("{")) {
        Node head;
        head.next = NULL;
        Node *cur = &head;
        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }

        node = new_node(ND_BLOCK);
        node->next = head.next;
        return node;
    }

    if (is_typename()) {
        return declaration();
    }

    node = expr();
    expect(";");
    return node;
}

// expr = assign
Node *expr() { return assign(); }

// assign = equality ("=" assign)?
Node *assign() {
    Node *node = equality();
    if (consume("=")) {
        node = new_binary(ND_ASSIGN, node, assign());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==")) {
            node = new_binary(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_binary(ND_NE, node, relational());
        } else {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume("<=")) {
            node = new_binary(ND_LE, node, add());
        } else if (consume(">=")) {
            node = new_binary(ND_LE, add(), node);
        } else if (consume("<")) {
            node = new_binary(ND_LT, node, add());
        } else if (consume(">")) {
            node = new_binary(ND_LT, add(), node);
        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume("+")) {
            node = new_binary(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_binary(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume("*")) {
            node = new_binary(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_binary(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

// unary = ("+" | "-" | "*" | "&")? unary
//       | "sizeof" unary
//       | primary ("[" expr "]")*
//       | primary
Node *unary() {
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_binary(ND_SUB, new_num(0), unary());
    }
    if (consume("*")) {
        return new_binary(ND_DEREF, unary(), NULL);
    }
    if (consume("&")) {
        return new_binary(ND_ADDR, unary(), NULL);
    }
    if (consume("sizeof")) {
        return new_binary(ND_SIZEOF, unary(), NULL);
    }

    Node *node = primary();
    while (consume("[")) {
        Node *exp = new_binary(ND_ADD, node, expr());
        expect("]");
        node = new_binary(ND_DEREF, exp, NULL);
    }
    return node;
}

// func-args = "(" (assign ("," assign)* )? ")"
Node *func_args_or_null() {
    // 最初の "(" を consume した後に呼ぶ
    if (consume(")")) {
        return NULL;
    }

    Node *head = assign();
    Node *cur = head;
    while (consume(",")) {
        cur->next = assign();
        cur = cur->next;
    }
    expect(")");

    return head;
}

static int string_literal_count = 0;
char *string_litera_label() {
    char buf[20];
    sprintf(buf, ".L.data.%d", string_literal_count++);
    return strndup(buf, 20);
}

// primary = "(" expr ")"
//         | ident func-args
//         | ident
//         | str
//         | num
Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok;
    if (tok = consume_ident()) {
        if (consume("(")) {
            Node *node = new_node(ND_FUNCALL);
            Node *func_args = func_args_or_null();
            node->funcname = strndup(tok->str, tok->len);
            node->args = func_args;
            return node;
        }

        Node *node = new_node(ND_VAR);
        char *ident_name = strndup(tok->str, tok->len);

        VarList *local_vl = find_local_var(ident_name);
        VarList *global_vl = find_global_var(ident_name);
        if (!local_vl && !global_vl) {
            error_at(token->str, "未定義の変数です");
        } else if (local_vl) {
            node->var = local_vl->head;
            node->is_local = true;
        } else {
            node->var = global_vl->head;
            node->is_local = false;
        }
        return node;
    }

    tok = token;
    if (tok->kind == TK_STRING) {
        token = token->next;
        Type *type = array_of(char_type(), tok->contents_len);
        Var *var = calloc(1, sizeof(Var));

        var->name = string_litera_label();
        var->type = type;
        var->contents = tok->contents;
        var->contents_len = tok->contents_len;
        push_global_var(var);

        Node *node = new_node(ND_VAR);
        node->var = var;
        node->is_local = false;
        return node;
    }

    assert(tok->kind == TK_NUM);
    return new_num(expect_number());
}
