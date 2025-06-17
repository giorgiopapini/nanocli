#include <stdio.h>
#include <string.h>

#include "easycli.h"

/*
    TODO:   Evaluate if it's better to use Alternate screen mode in easycli instead of clean_screen function

    TODO:   IT IS BECOMING MORE CLEAR THAT TERM_COLS SHOULD BE CONTAINED INSIDE e_cli_state. MAYBE ADD A SIGWINCH SIGNAL TO UPDATE
            THE TERM_COLS VALUE INSIDE e_cli_state, SO THAT I DONT NEED TO CALL "get_terminal_size" SO OFTEN IN THE CODEBASE.
            IN THE BEST SCENARIO (AND SUPPORTED SCENARIO) TERM_COLS IS NOT GONNA CHANGE RUNTIME
            - MAYBE INSERT "CURSOR_INDEX (the real index of the string)" also inside e_cli_state. In a lot of functions I calculate
            that starting from term_cols, by adding it to e_cli_state I would have even smaller functions --> update real_index

            real_index is used also in _canc() --> but it is calculated as abs_x - prompt_len
            the "real_index" maybe called curr should be store in e_line

    TODO:   Add CTRL_K, CTRL_T, CTRL_U, CTRL_W support
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