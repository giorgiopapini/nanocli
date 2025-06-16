#include <stdio.h>
#include <string.h>

#include "easycli.h"

/*
    TODO:   Evaluate if it's better to use Alternate screen mode in easycli instead of clean_screen function

    TODO:   Maybe separate in another module tha cli state struct, add a function to check if members of it arent NULL
            (so in arrow_up, arrow_down, etc... i dont have to do single checks, i can just call something like "is_cli_init");

    TODO:   Try to implement all the linenoise features (especially the support for variour CTRL+key commands)

    TODO:   Reduce the number of file as much as possible, ideally ONE file like linenoise. --> remove error_handler and adopt
            linenoise philosophy, if an error occours, return NULL char *line_content; (DON'T PRINT ERRORS, LET THE DEVELOPER
            MANAGE ERRORS)
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