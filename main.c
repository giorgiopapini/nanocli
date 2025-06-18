#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "easycli.h"

/*
    TODO:   Evaluate if it's better to use Alternate screen mode in easycli instead of clean_screen function

    TODO:   Add custom completion (developer registers a completion callback for autocompletition)

    TODO:   Save current history to a file and load history from a file.
*/

e_stat_code callback_on_enter(char *line_content, void *ctx);

int main(void) {
    run_easycli_ctx(DEFAULT_PROMPT, DEFAULT_MAX_INPUT_LEN, NULL, callback_on_enter);
    
    return 0;
}

e_stat_code callback_on_enter(char *line_content, void *ctx) {
    char *prova;
    (void)ctx;
    (void)line_content;

    if (0 == strcmp(line_content, "login")) {
        prova = easy_ask("username: ", 0);
        free(prova);
        prova = easy_ask("password: ", 1);
        if (0 == strcmp(prova, "prova")) printf("SUCCESS");
        else printf("FAILURE");
        free(prova);
        return E_CONTINUE;
    }
    if (0 == strcmp(line_content, "exit")) return E_EXIT;
    return E_NOP;
}