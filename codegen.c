#include "9cc.h"

int jump_label_counts = 0;
char *funarg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen_lval(Node *node) {
    if (node->kind != ND_VAR) {
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
        case ND_VAR: {
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
        case ND_BLOCK: {
            for (Node *n = node->next; n; n = n->next) {
                gen(n);
            }
            return;
        }
        case ND_FUNCALL: {
            int args = 0;
            for (Node *arg = node->args; arg; arg = arg->next) {
                gen(arg);
                args++;
            }
            for (int i = args - 1; i >= 0; i--) {
                printf("    pop %s\n", funarg_regs[i]);
            }

            // RSP を 16 の倍数にする
            int current_counts = jump_label_counts++;
            printf("    mov rax, rsp\n");
            printf("    and rax, 15\n");
            printf("    jnz .Lcall%d\n", current_counts);
            printf("    mov rax, 0\n");
            printf("    call %s\n", node->funcname);
            printf("    jmp .Lend%d\n", current_counts);

            printf(".Lcall%d:\n", current_counts);
            printf("    sub rsp, 8\n");
            printf("    mov rax, 0\n");
            printf("    call %s\n", node->funcname);
            printf("    add rsp, 8\n");

            printf(".Lend%d:\n", current_counts);
            printf("    push rax\n");
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

int get_lval_space(Function *func) {
    if (!func->locals) {
        return 0;
    }
    return func->locals->offset;
}

void gen_func(Function *func) {
    printf(".global %s\n", func->name);
    printf("%s:\n", func->name);

    // Prologue
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", get_lval_space(func));

    // function params
    int args = 0;
    for (Var *var = func->params; var; var = var->next) {
        printf("    mov [rbp-%d], %s\n", var->offset, funarg_regs[args++]);
    }

    for (Node *node = func->node; node; node = node->next) {
        gen(node);
    }

    // Epilogue
    printf(".Lreturn.%s:\n", func->name);
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
}

void codegen(Function *functions) {
    printf(".intel_syntax noprefix\n");
    for (Function *fun = functions; fun; fun = fun->next) {
        gen_func(fun);
    }
}
