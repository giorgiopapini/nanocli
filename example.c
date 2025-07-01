#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "easycli.h"

/*
    TODO:   Add custom completion (developer registers a completion callback for autocompletition) Use a TRIE data structure for this

    TODO:   Find a better way to use History, maybe make it initialize by the user and make the user pass it
            in easycli? Otherwise I need to find a way to NOT let it be global. Each easycli call should have its
            own history

    TODO:   Save current history to a file and load history from a file.
*/

static int _login(void);


static int _login(void) {
    char *username = easy_ask("username: ", E_DEFAULT_MAX_INPUT_LEN, 0);
    char *password = easy_ask("password: ", E_DEFAULT_MAX_INPUT_LEN, 1);
    int logged_in = 0;

    if (0 == strcmp(username, "user1") && 0 == strcmp(password, "123"))
        logged_in = 1;
    else logged_in = 0;
    free(username);
    free(password);
    return logged_in;
}

int main(void) {
    char *res;

    /* exit string is needed to deallocate history automatically */
    while (NULL != (res = easycli(E_DEFAULT_PROMPT, E_DEFAULT_MAX_INPUT_LEN))) {
        if (0 == strcmp(res, "login")) {
            if (_login()) easy_print("logged in!");
            else easy_print("login failed!");
        }
        if (0 == strcmp(res, "exit")) {
            free(res);
            break;
        }
        free(res);
    }
    return 0;
}
