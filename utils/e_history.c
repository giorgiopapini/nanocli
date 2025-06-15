#include "e_history.h"

#include <stdio.h>
#include <stdlib.h>


struct e_history *e_create_history(const size_t max_len) {
    struct e_history *new_history = malloc(sizeof *new_history);
    if (NULL == new_history) return NULL;

    new_history->entries = calloc(max_len, sizeof *(new_history->entries));
    if (NULL == new_history->entries) {
        free(new_history);
        return NULL;
    }

    new_history->cap = max_len;
    new_history->len = 0;
    new_history->curr = 0;
    return new_history;
}

void e_add_entry(struct e_history *history, const struct e_string *new_str) {
    /* Makes a copy of new_str and appends it to history */
    size_t i;
    struct e_string *copy_str;

    if (
        NULL == history ||
        NULL == history->entries ||
        NULL == new_str
    ) return;

    if (e_str_is_empty(new_str)) return;
    if (history->len > 0) {
        if (e_str_equal(new_str, history->entries[history->len - 1])) return;
    }

    copy_str = e_create_string(new_str->cap);
    if (NULL == copy_str) return;

    e_copy_str(copy_str, new_str);

    /* remember: when an entry is added, history->curr always points to the last added element */
    if (history->len < history->cap) {
        history->len ++;
        history->entries[history->len - 1] = copy_str;
        history->curr = history->len - 1;
    }
    else {
        /* deallocate first entry than shift eveything to the left and add entry in the last element at (history->cap - 1) */
        e_free_str(history->entries[0]);
        for (i = 1; i < history->cap; i ++)
            history->entries[i -1] = history->entries[i];
        history->entries[history->cap - 1] = copy_str;
        history->curr = history->cap - 1;
    }
}

void e_free_history(struct e_history *history) {
    size_t i;

    if (NULL == history || history->len > history->cap) return;
    if (NULL == history->entries) {
        free(history);
        return;
    }

    for (i = 0; i < history->len; i ++) {
        if (NULL != history->entries[i]) e_free_str(history->entries[i]);
    }

    free(history->entries);
    free(history);
}