#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


ASTNode* deep_copy_ast(ASTNode* node) {
    if (!node) return NULL;

    ASTNode* copy = create_node(node->type, node->value);
    copy->left = deep_copy_ast(node->left);
    copy->right = deep_copy_ast(node->right);
    copy->next = deep_copy_ast(node->next);
    return copy;
}


ASTNode* fold_constants(ASTNode* node) {
    if (!node) return NULL;

    node->left = fold_constants(node->left);
    node->right = fold_constants(node->right);
    node->next = fold_constants(node->next);

    if (node->type == NODE_BINOP && node->left && node->right &&
        node->left->type == NODE_INT && node->right->type == NODE_INT) {

        int left_val = atoi(node->left->value);
        int right_val = atoi(node->right->value);
        int result = 0;
        char op = node->value[0];

        switch (op) {
            case '+': result = left_val + right_val; break;
            case '-': result = left_val - right_val; break;
            case '*': result = left_val * right_val; break;
            case '/': 
                if (right_val != 0)
                    result = left_val / right_val; 
                else 
                    return node; 
                break;
            case '<': result = left_val < right_val; break;
            default: return node;
        }

        ASTNode* folded = make_int_node(result);
        free_ast(node);
        return folded;
    }

    return node;
}


ASTNode* eliminate_dead_code(ASTNode* node) {
    if (!node) return NULL;

    if (node->type == NODE_SEQ) {
        node->left = eliminate_dead_code(node->left);

        if (node->left && node->left->type == NODE_RETURN) {
            free_ast(node->right);
            node->right = NULL;
        } else {
            node->right = eliminate_dead_code(node->right);
        }
    } else {
        node->left = eliminate_dead_code(node->left);
        node->right = eliminate_dead_code(node->right);
        node->next = eliminate_dead_code(node->next);
    }

    return node;
}


ASTNode* unroll_loops(ASTNode* node) {
    if (!node) return NULL;

    node->left = unroll_loops(node->left);
    node->right = unroll_loops(node->right);
    node->next = unroll_loops(node->next);

    if (node->type == NODE_FOR && node->left && node->right) {
        ASTNode* init = node->left;
        ASTNode* cond = node->right;
        ASTNode* update = cond->next;
        ASTNode* body = update ? update->next : NULL;

        if (!init || !cond || !update || !body) return node;

        if (init->left && init->left->type == NODE_INT &&
            cond->right && cond->right->type == NODE_INT &&
            strcmp(update->value, "++") == 0) {

            int start = atoi(init->left->value);
            int end = atoi(cond->right->value);

            ASTNode* unrolled = NULL;
            for (int i = start; i < end; i++) {
                ASTNode* cloned = deep_copy_ast(body);
                if (unrolled)
                    unrolled = make_seq_node(unrolled, cloned);
                else
                    unrolled = cloned;
            }

            free_ast(node);
            return unrolled;
        }
    }

    return node;
}

ASTNode* optimize_ast(ASTNode* root) {
    root = fold_constants(root);
    root = eliminate_dead_code(root);
    root = unroll_loops(root);
    return root;
}
