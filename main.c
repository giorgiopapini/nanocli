#include <stdio.h>
#include <string.h>

#include "easycli.h"

/*
    TODO:   Evaluate if it's better to use Alternate screen mode in easycli instead of clean_screen function

    TODO:   real_index is used many times with the same goal and code, it is used also
            in _canc() --> but it is calculated as abs_x - prompt_len,
            the "real_index", maybe called curr, should be store in e_line
*/

e_stat_code callback_on_enter(char *line_content, void *ctx);

int main(void) {
    run_easycli_ctx(DEFAULT_PROMPT, DEFAULT_MAX_INPUT_LEN, NULL, callback_on_enter);
    
    return 0;
}

e_stat_code callback_on_enter(char *line_content, void *ctx) {
    (void)ctx;
    (void)line_content;

    if (0 == strcmp(line_content, "exit")) return E_EXIT;
    return E_CONTINUE;
}