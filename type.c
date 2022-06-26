#include "9cc.h"

Type *int_type() {
    Type *type = calloc(1, sizeof(Type));
    type->kind = TY_INT;
    return type;
}

Type *pointer_to(Type *base) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = TY_PTR;
    type->ptr_to = base;
    return type;
}

void visit(Node *node) {
    if (!node) {
        return;
    }

    visit(node->lhs);
    visit(node->rhs);
    visit(node->cond);
    visit(node->then);
    visit(node->els);
    visit(node->init);
    visit(node->inc);
    visit(node->next);
    visit(node->args);

    switch (node->kind) {
        case ND_MUL:
        case ND_DIV:
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_FUNCALL:
        case ND_NUM: {
            node->type = int_type();
            return;
        }
        case ND_VAR: {
            node->type = node->var->type;
            return;
        }
        case ND_ADD: {
            if (node->rhs->type->kind == TY_PTR && node->lhs->type->kind == TY_PTR) {
                error("不正な計算です");
            }
            if (node->rhs->type->kind == TY_PTR) {
                Node *tmp = node->lhs;
                node->lhs = node->rhs;
                node->rhs = tmp;
            }
            node->type = node->lhs->type;
            return;
        }
        case ND_SUB: {
            if (node->rhs->type->kind == TY_PTR) {
                error("不正な計算です");
            }
            node->type = node->lhs->type;
            return;
        }
        case ND_ASSIGN: {
            node->type = node->lhs->type;
            return;
        }
        case ND_ADDR: {
            node->type = pointer_to(node->lhs->type);
            return;
        }
        case ND_DEREF: {
            if (node->lhs->type->kind != TY_PTR) {
                node->type = int_type();
            } else {
                node->type = node->lhs->type->ptr_to;
                return;
            }
        }
    }
}

void annotate_type(Function *functions) {
    for (Function *fun = functions; fun; fun = fun->next) {
        visit(fun->node);
    }
}
