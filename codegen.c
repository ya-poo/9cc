#include "9cc.h"

int jump_label_counts = 0;

void gen_lval(Node *node) {
    if (node->kind != ND_LVAR) {
        error("代入の左辺値が変数ではありません");
    }
    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", node->offset);
    printf("    push rax\n");
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_RETURN: {
            gen(node->lhs);
            printf("    pop rax\n");
            printf("    mov rsp, rbp\n");
            printf("    pop rbp\n");
            printf("    ret\n");
            return;
        }
        case ND_NUM: {
            printf("    push %d\n", node->val);
            return;
        }
        case ND_LVAR: {
            gen_lval(node);
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            return;
        }
        case ND_ASSIGN: {
            gen_lval(node->lhs);
            gen(node->rhs);
            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    mov [rax], rdi\n");
            printf("    push rdi\n");
            return;
        }
        case ND_IF: {
            int current_counts = jump_label_counts++;
            if (node->els) {
                gen(node->cond);
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");  // FALSE = 0, TRUE = その他,
                printf("    je  .Lelse%d\n", current_counts);
                gen(node->then);
                printf("    jmp .Lend%d\n", current_counts);
                printf(".Lelse%d:\n", current_counts);
                gen(node->els);
                printf(".Lend%d:\n", current_counts);
            } else {
                gen(node->cond);
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    je  .Lend%d\n", current_counts);
                gen(node->then);
                printf(".Lend%d:\n", current_counts);
            }
            return;
        }
        case ND_WHILE: {
            int current_counts = jump_label_counts++;
            printf(".Lbegin%d:\n", current_counts);
            gen(node->cond);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je  .Lend%d\n", current_counts);
            gen(node->then);
            printf("    jmp .Lbegin%d\n", current_counts);
            printf(".Lend%d:\n", current_counts);
            return;
        }
        case ND_FOR: {
            int current_counts = jump_label_counts++;
            gen(node->init);
            printf(".Lbegin%d:\n", current_counts);
            gen(node->cond);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je  .Lend%d\n", current_counts);
            gen(node->then);
            gen(node->inc);
            printf("    jmp .Lbegin%d\n", current_counts);
            printf(".Lend%d:\n", current_counts);
            return;
        }
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;
        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");
            break;
    }
    printf("    push rax\n");
}

int get_lval_space() {
    if (!locals) {
        return 0;
    }
    return locals->offset;
}

void codegen() {
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // Prologue
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", get_lval_space());

    for (int i = 0; code[i]; i++) {
        gen(code[i]);
        printf("    pop rax\n");
    }

    // Epilogue
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
}
