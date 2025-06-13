#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#define SELECT_ERROR_MSG            "Error in select(...) function"
#define TCGETATTR_ERROR_MSG         "Unable to retrieve terminal attributes"
#define TCSETATTR_ERROR_MSG         "Unable to set terminal attributes"
#define TERMINAL_SIZE_ERROR_MSG     "Couldn't retrieve the current terminal size"
#define NULL_STR_DEST_ERROR_MSG     "Destination struct e_string is NULL"
#define NULL_CURSOR_ERROR_MSG       "Cursor struct e_cursor is NULL"
#define SIZE_T_OVERFLOW_MSG         "'size_t' overflowed"
#define BUFFER_OVERFLOW_MSG         "Input overflowed buffer"

#define E_STACK_ERR_SIZE            64

#include <stddef.h>

 typedef enum {
    SELECT_ERROR,
    TCGETATTR_ERROR,
    TCSETATTR_ERROR,
    TERMINAL_SIZE_ERROR,
    NULL_STR_DEST_ERROR,
    NULL_CURSOR_ERROR,
    SIZE_T_OVERFLOW,
    BUFFER_OVERFLOW
} err_code;

struct e_error {
    err_code code;
    const char *message;
};

struct e_stack_err {
    struct e_error **data;
    int top;
    size_t cap;  /* data array size */
};


struct e_error *e_create_err(const err_code code, const char *message);
struct e_stack_err *e_create_stack_err(const size_t max_size);
void e_err_push(struct e_stack_err *errs, struct e_error *new_err);
struct e_error *e_err_pop(struct e_stack_err *errs);
void e_stack_flush_errors(struct e_stack_err *errs);  /* this also deallocates everything (stack is a one time read data struct) */
void e_free_error(struct e_error *err);
void e_free_stack_err(struct e_stack_err *errs);

#endif