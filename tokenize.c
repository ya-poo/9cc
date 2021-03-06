#include "9cc.h"

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void verror_at(char *loc, char *fmt, va_list ap) {
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (tok) {
        verror_at(tok->str, fmt, ap);
    }
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

bool is_lval_char(char c) { return is_lval_initial(c) || ('0' <= c && c <= '9'); }

static char *keywords[] = {"return", "if", "else", "while", "for", "sizeof", "int", "char"};
static char *operators[] = {"==", "!=", "<=", ">="};
static char *symbols[] = {"+", "-", "*", "/", "(", ")", "<", ">",
                          ";", "=", "{", "}", ",", "&", "[", "]"};
char *get_reserved(char *p) {
    for (int i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
        int length = strlen(keywords[i]);
        if (startsWith(p, keywords[i]) && !is_lval_char(p[length])) {
            return keywords[i];
        }
    }
    for (int i = 0; i < sizeof(operators) / sizeof(*operators); i++) {
        if (startsWith(p, operators[i])) {
            return operators[i];
        }
    }
    for (int i = 0; i < sizeof(symbols) / sizeof(*symbols); i++) {
        if (startsWith(p, symbols[i])) {
            return symbols[i];
        }
    }

    return NULL;
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

        char *reserved = get_reserved(p);
        if (reserved) {
            int length = strlen(reserved);
            cur = new_token(TK_RESERVED, cur, p, length);
            p += length;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        if (*p == '"') {
            char *q = p++;
            while (*p && *p != '"') {
                p++;
            }
            if (!*p) {
                error_at(q, "不正な文字列です");
            }
            p++;

            cur = new_token(TK_STRING, cur, q, q - p);
            cur->contents = strndup(q + 1, p - q - 2);
            cur->contents_len = p - q - 1;
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
