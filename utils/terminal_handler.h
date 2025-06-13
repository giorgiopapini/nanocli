#ifndef TERMINAL_HANDLER_H
#define TERMINAL_HANDLER_H

#include <stddef.h>

#include "error_handler.h"


void enable_raw_mode(struct e_stack_err *errs);
int is_raw_mode_enabled(void);
void restore_terminal_mode(void);
void get_terminal_size(size_t *cols, size_t *rows, struct e_stack_err *errs);

#endif