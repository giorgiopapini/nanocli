#ifndef EASYCLI_LIB_H
#define EASYCLI_LIB_H

#include <stddef.h>

#include "utils/e_line.h"
#include "utils/error_handler.h"

#define DEFAULT_PROMPT              "easycli > "
#define DEFAULT_MAX_INPUT_LEN       1024

#define ARROW_UP_KEY 'A'
#define ARROW_DOWN_KEY 'B'
#define ARROW_RIGHT_KEY 'C'
#define ARROW_LEFT_KEY 'D'
#define CANC_KEY '3'
#define TILDE_KEY '~'
#define ENTER_KEY '\n'
#define BACKSPACE_KEY 127

typedef enum {
    E_EXIT = -1,
    E_CONTINUE = 0,  /* rename something like "keep running, or keep listening, or continue" since EXIT means exiting from CLI loop, and Send command means literally send command */
    E_SEND_COMMAND
} e_stat_code;


void run_easycli_ctx(
    const char *prompt,
    size_t max_str_len,  /* excluding '\0' */
    void *ctx,
    e_stat_code (*callback_on_enter)(struct e_line *dest, void *ctx, struct e_stack_err *errs)
);

#endif