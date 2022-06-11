#include "9cc.h"

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");  // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

bool startsWith(char *p, char *q) { return memcmp(p, q, strlen(q)) == 0; }

bool is_lval_initial(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_lval_char(char c) {
    return is_lval_initial(c) || ('0' <= c && c <= '9');
}

bool is_keyword_of(char *p, char *keyword) {
    int length = strlen(keyword);
    return strncmp(p, keyword, length) == 0 && !is_lval_char(p[length]);
}

Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (startsWith(p, "==") || startsWith(p, "!=") || startsWith(p, "<=") ||
            startsWith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (strchr("+-*/()<>;=", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        if (is_keyword_of(p, "return")) {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
            continue;
        }

        if (is_keyword_of(p, "if")) {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }

        if (is_keyword_of(p, "else")) {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
            continue;
        }

        if (is_keyword_of(p, "while")) {
            cur = new_token(TK_WHILE, cur, p, 5);
            p += 5;
            continue;
        }

        if (is_keyword_of(p, "for")) {
            cur = new_token(TK_FOR, cur, p, 3);
            p += 3;
            continue;
        }

        if (is_lval_initial(*p)) {
            int length = 0;
            while (is_lval_char(*(p + length))) {
                length++;
            }
            cur = new_token(TK_IDENT, cur, p, length);
            p += length;
            continue;
        }

        error_at(p, "トークナイズ出来ません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}
