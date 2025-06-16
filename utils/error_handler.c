#include "error_handler.h"

#include <stdio.h>
#include <stdlib.h>

int _e_stack_is_empty(struct e_stack_err *errs);
int _e_stack_is_full(struct e_stack_err *errs);


int _e_stack_is_empty(struct e_stack_err *errs) {
    return errs->top == -1;
}

int _e_stack_is_full(struct e_stack_err *errs) {
    return (size_t)errs->top == errs->cap - 1;
}

struct e_error *e_create_err(const e_err_code code, const char *message) {
    struct e_error *new_err = malloc(sizeof(*new_err));
    if (!new_err) return NULL;

    new_err->code = code;
    new_err->message = message;
    return new_err;
}

struct e_stack_err *e_create_stack_err(const size_t max_size) {
    struct e_stack_err *new_stack = malloc(sizeof *new_stack);
    if (NULL == new_stack) return NULL;

    new_stack->data = malloc(max_size * sizeof *new_stack->data);
    if (NULL == new_stack->data) return NULL;

    new_stack->cap = max_size;
    new_stack->top = -1;
    return new_stack;
}

void e_err_push(struct e_stack_err *errs, struct e_error *new_err) {
    if (NULL == errs || _e_stack_is_full(errs)) return;
    errs->data[++errs->top] = new_err;
}

struct e_error *e_err_pop(struct e_stack_err *errs) {
    if (NULL == errs || _e_stack_is_empty(errs)) return NULL;
    return errs->data[errs->top--];
}

void e_stack_flush_errors(struct e_stack_err *errs) {
    struct e_error *curr_err;

    printf("\r\n");
    while (NULL != (curr_err = e_err_pop(errs))) {
        printf("\033[31m[ERROR] (%d) --> %s\033[0m\r\n", curr_err->code, curr_err->message);
        e_free_error(curr_err);
    }
}

void e_free_error(struct e_error *err) {
    if (NULL != err) free(err);
}

void e_free_stack_err(struct e_stack_err *errs) {
    struct e_error *curr_err;

    while (NULL != (curr_err = e_err_pop(errs))) 
        e_free_error(curr_err);

    if (NULL != errs) {
        if (NULL != errs->data) free(errs->data);
        free(errs);
    }
}