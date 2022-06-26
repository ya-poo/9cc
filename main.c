#include "9cc.h"

//
// global
//
// 入力プログラム
char *user_input;
// 現在着目しているトークン
Token *token;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    user_input = argv[1];
    token = tokenize(user_input);
    Function *functions = program();
    annotate_type(functions);
    codegen(functions);
    return 0;
}
