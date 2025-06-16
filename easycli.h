#ifndef EASYCLI_LIB_H
#define EASYCLI_LIB_H

#include <stddef.h>

#define DEFAULT_PROMPT              "easycli > "
#define DEFAULT_MAX_INPUT_LEN       1024
#define DEFAULT_HISTORY_MAX_SIZE    1024

typedef enum {
    E_EXIT = -1,
    E_CONTINUE = 0,  /* rename something like "keep running, or keep listening, or continue" since EXIT means exiting from CLI loop, and Send command means literally send command */
    E_SEND_COMMAND
} e_stat_code;


void run_easycli_ctx(
    const char *prompt,
    size_t max_str_len,  /* excluding '\0' */
    void *ctx,
    e_stat_code (*callback_on_enter)(char *dest, void *ctx)
);

#endif