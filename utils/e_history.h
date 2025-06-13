#ifndef E_HISTORY_H
#define E_HISTORY_H

#include <stddef.h>

#include "e_string.h"

#define DEFAULT_HISTORY_MAX_SIZE        1024

/*
    navigate by doing curr ++ and curr --.
    if (curr - 1 < 0) --> curr = len - 1; (access last element (simulate a circular buffer))
    if (curr + 1 > len) --> curr = 0 (access firs element (simulate a circular buffer))

    use a cap relatively large, because when len reaches cap, than I need to start deleting oldest entries
    and move everything one place to left (to add space (last space) to the new entry). This signifies
    that adding a new entry isn't O(1) anymore, it becomes O(n);
*/

struct e_history {
    struct e_string **entries;  /* dinamically allocated e_string pointers */
    size_t curr;
    size_t len;
    size_t cap;
};

struct e_history *e_create_history(const size_t max_len);
void e_add_entry(struct e_history *history, const struct e_string *new_str);
void e_free_history(struct e_history *history);


#endif