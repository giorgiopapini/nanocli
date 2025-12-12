#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nanocli.h"

/*
    TODO:   Add custom completion (developer registers a completion callback for autocompletition) Use a TRIE data structure for this

    TODO:   Find a better way to use History, maybe make it initialize by the user and make the user pass it
            in nanocli? Otherwise I need to find a way to NOT let it be global. Each nanocli call should have its
            own history

    TODO:   Save current history to a file and load history from a file.

    TODO:   When the line is full of chars, if left_arrow and than right_arrow is pressed a bug happens
*/

static int _login(void);


static int _login(void) {
    char *username;
    char *password;
    int logged_in = 0;
    
    username = nanocli_ask("username: ", NCLI_DEFAULT_MAX_INPUT_LEN, 0);
    if (!username) return 0;
    password = nanocli_ask("password: ", NCLI_DEFAULT_MAX_INPUT_LEN, 1);
    if (!password) {
        free(username);
        return 0;
    }

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
    while (NULL != (res = nanocli(NCLI_DEFAULT_PROMPT, NCLI_DEFAULT_MAX_INPUT_LEN))) {
        if (0 == strcmp(res, "login")) {
            if (_login()) nanocli_echo("logged in!");
            else nanocli_echo("login failed!");
        }
        if (0 == strcmp(res, "exit")) {
            free(res);
            break;
        }
        free(res);
    }
    return 0;
}
