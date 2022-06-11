#include "9cc.h"

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
    switch (token->kind) {
        case TK_IDENT:
        case TK_NUM:
        case TK_EOF: {
            return false;
        }
        case TK_RESERVED:
        case TK_RETURN:
        case TK_IF:
        case TK_ELSE:
        case TK_WHILE:
        case TK_FOR: {
            break;
        }
        default: {
            error_at(token->str, "Unknown Token");
        }
    }

    if (strlen(op) != token->len || memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが ND_IDENT の場合は 1 つ読み進めてその ND_IDENT
// のトークンを返す。 それ以外の場合は 0 を返す。
Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return 0;
    }
    Token *ident_token = token;
    token = token->next;
    return ident_token;
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

bool at_eof() { return token->kind == TK_EOF; }

Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
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

Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// program = stmt *
void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
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
        consume("(");
        node->cond = expr();
        consume(")");
        node->then = stmt();
        if (consume("else")) {
            node->els = stmt();
        }
        return node;
    }

    if (consume("while")) {
        node = new_node(ND_WHILE);
        consume("(");
        node->cond = expr();
        consume(")");
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

// unary = ("+" | "-")? primary
Node *unary() {
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_binary(ND_SUB, new_num(0), unary());
    }
    return primary();
}

LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }
    return NULL;
}

// primary = num | ident |"(" expr ")"
Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *ident_token = consume_ident();
    if (ident_token) {
        Node *node = new_node(ND_LVAR);

        LVar *lvar = find_lvar(ident_token);
        if (lvar) {
            node->offset = lvar->offset;
        } else {
            lvar = calloc(1, sizeof(LVar));
            if (locals) {
                lvar->next = locals;
                lvar->offset = locals->offset + 8;
            } else {
                lvar->offset = 8;
            }
            lvar->name = ident_token->str;
            lvar->len = ident_token->len;
            node->offset = lvar->offset;
            locals = lvar;
        }
        return node;
    }
    return new_num(expect_number());
}
