#include <stdio.h>
#include <string.h>

#include "easycli.h"
#include "utils/error_handler.h"

/*
    TODO:   Add support for unicode (UTF-8) chars (like ì, ù, §, 你, emoji, etc...)
*/

stat_code callback_on_enter(struct e_string *str, void *ctx, struct e_stack_err *errs);

int main(void) {
    run_easycli_ctx(DEFAULT_PROMPT, DEFAULT_MAX_INPUT_LEN, NULL, callback_on_enter);
    
    return 0;
}

stat_code callback_on_enter(struct e_string *str, void *ctx, struct e_stack_err *errs) {
    (void)ctx;
    (void)errs;
    (void)str;

    if (0 == strcmp(str->content, "exit")) return E_EXIT;
    return E_CONTINUE;
}