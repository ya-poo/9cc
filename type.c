#include "9cc.h"

int size_of(Type *type) {
    switch (type->kind) {
        case TY_INT: {
            return 8;
        }
        case TY_PTR: {
            return 8;
        }
        case TY_ARRAY: {
            return size_of(type->ptr_to) * type->array_size;
        }
    }
}

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
            if (node->rhs->type->ptr_to) {
                Node *tmp = node->lhs;
                node->lhs = node->rhs;
                node->rhs = tmp;
            }
            if (node->rhs->type->ptr_to) {
                error_at(node->token->str, "不正な計算です");
            }
            node->type = node->lhs->type;
            return;
        }
        case ND_SUB: {
            if (node->rhs->type->ptr_to) {
                error_at(node->token->str, "不正な計算です");
            }
            node->type = node->lhs->type;
            return;
        }
        case ND_ASSIGN: {
            node->type = node->lhs->type;
            return;
        }
        case ND_ADDR: {
            if (node->lhs->type->kind == TY_ARRAY) {
                node->type = pointer_to(node->lhs->type->ptr_to);
            } else {
                node->type = pointer_to(node->lhs->type);
            }
            return;
        }
        case ND_DEREF: {
            if (!node->lhs->type->ptr_to) {
                node->type = node->lhs->type;
                return;
            } else {
                node->type = node->lhs->type->ptr_to;
                return;
            }
        }
        case ND_SIZEOF: {
            node->kind = ND_NUM;
            node->type = int_type();
            node->val = size_of(node->lhs->type);
            node->lhs = NULL;
            return;
        }
    }
}

void annotate_type(Function *functions) {
    for (Function *fun = functions; fun; fun = fun->next) {
        visit(fun->node);
    }
}
