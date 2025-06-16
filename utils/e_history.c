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

void e_add_entry(struct e_history *history, const struct e_line *new_line) {
    /* Makes a copy of new_line and appends it to history */
    size_t i;
    struct e_line *copy_str;

    if (
        NULL == history ||
        NULL == history->entries ||
        NULL == new_line
    ) return;

    if (e_line_is_empty(new_line)) return;
    if (history->len > 0) {
        if (e_line_equal(new_line, history->entries[history->len - 1])) return;
    }

    copy_str = e_create_line(new_line->cap);
    if (NULL == copy_str) return;

    e_copy_line(copy_str, new_line);

    /* remember: when an entry is added, history->curr always points to the last added element */
    if (history->len < history->cap) {
        history->len ++;
        history->entries[history->len - 1] = copy_str;
        history->curr = history->len - 1;
    }
    else {
        /* deallocate first entry than shift eveything to the left and add entry in the last element at (history->cap - 1) */
        e_free_line(history->entries[0]);
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
        if (NULL != history->entries[i]) e_free_line(history->entries[i]);
    }

    free(history->entries);
    free(history);
}