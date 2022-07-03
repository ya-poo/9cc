#include "9cc.h"

int jump_label_counts = 0;
char *funcname;
char *argreg1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *argreg8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen(Node *node);

void gen_addr(Node *node) {
    switch (node->kind) {
        case ND_VAR: {
            if (node->is_local) {
                printf("    lea rax, [rbp-%d]\n", node->var->offset);
                printf("    push rax\n");
            } else {
                printf("    push offset %s\n", node->var->name);
            }
            return;
        }
        case ND_DEREF: {
            gen(node->lhs);
            return;
        }
    }
    error("アドレス値ではありません");
}

void gen_lval(Node *node) {
    if (node->type->kind == TY_ARRAY) {
        error("代入の左辺値が変数ではありません");
    }
    gen_addr(node);
}

void load(Type *type) {
    printf("    pop rax\n");
    if (size_of(type) == 1) {
        printf("    movsx rax, byte ptr [rax]\n");
    } else {
        printf("    mov rax, [rax]\n");
    }
    printf("    push rax\n");
}

void store(Type *type) {
    printf("    pop rdi\n");
    printf("    pop rax\n");
    if (size_of(type) == 1) {
        printf("    mov [rax], dil\n");
    } else {
        printf("    mov [rax], rdi\n");
    }
    printf("    push rdi\n");
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_RETURN: {
            gen(node->lhs);
            printf("    pop rax\n");
            printf("    jmp .Lreturn.%s\n", funcname);
            return;
        }
        case ND_NUM: {
            printf("    push %d\n", node->val);
            return;
        }
        case ND_VAR: {
            gen_addr(node);
            if (node->type->kind != TY_ARRAY) {
                load(node->type);
            }
            return;
        }
        case ND_ASSIGN: {
            gen_addr(node->lhs);
            gen(node->rhs);
            store(node->type);
            return;
        }
        case ND_ADDR: {
            gen_addr(node->lhs);
            return;
        }
        case ND_DEREF: {
            gen(node->lhs);
            if (node->type->kind != TY_ARRAY) {
                load(node->type);
            }
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
                printf("    pop %s\n", argreg8[i]);
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
        case ND_DECL: {
            return;
        }
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            if (node->type->ptr_to) {
                printf("    imul rdi, %d\n", size_of(node->type->ptr_to));
            }
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            if (node->type->ptr_to) {
                printf("    imul rdi, %d\n", size_of(node->type->ptr_to));
            }
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

void codegen(Program *program) {
    printf(".intel_syntax noprefix\n");

    // Global Variables
    printf(".data\n");
    for (VarList *vl = program->global; vl; vl = vl->tail) {
        printf("%s:\n", vl->head->name);

        if (!vl->head->contents) {
            printf("    .zero %d\n", size_of(vl->head->type));
        } else {
            for (int i = 0; i < vl->head->contents_len; i++) {
                printf("    .byte %d\n", vl->head->contents[i]);
            }
        }
    }

    printf(".text\n");
    for (Function *fun = program->functions; fun; fun = fun->next) {
        funcname = fun->name;

        printf(".global %s\n", fun->name);
        printf("%s:\n", fun->name);

        // Prologue
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", fun->stack_size);

        // function params
        int args = 0;
        for (VarList *vl = fun->params; vl; vl = vl->tail) {
            int size = size_of(vl->head->type);
            if (size == 1) {
                printf("    mov [rbp-%d], %s\n", vl->head->offset, argreg1[args++]);
            } else {
                assert(size == 8);
                printf("    mov [rbp-%d], %s\n", vl->head->offset, argreg8[args++]);
            }
        }

        for (Node *node = fun->node; node; node = node->next) {
            gen(node);
        }

        // Epilogue
        printf(".Lreturn.%s:\n", fun->name);
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
    }
}
