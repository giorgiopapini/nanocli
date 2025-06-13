#include <stdio.h>
#include <string.h>

#include "easycli.h"
#include "utils/error_handler.h"

/*
    TODO:   ADD HISTORY NAVIGATION --> BASICALLY ADD ARROW_UP AND ARROW_DOWN BEHAVIOURS

    TODO:   Add support for unicode (UTF-8) chars (like ì, ù, §, 你, emoji, etc...)

    TODO:   CAPIRE PERCHé ARROW UP SALTA L'EFFETTIVA ULTIMA STRINGA, VA ALLA PENULTIMA, MAGARI E' POSSIBILE
            SFRUTTARE QUESTA COSA PER AGGIUNGERE UNA CONDIZIONE TALE PER CUI SI CANCELLA LA STRINGA E SI VA OLTRE
            (perchè nei terminali veri se continuo a fare su e giu alla fine torno alla STRINGA VUOTA)
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