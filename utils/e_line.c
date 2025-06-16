#include "e_line.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


struct e_line *e_create_line(const size_t max_len) {
    struct e_line *new_line = malloc(sizeof *new_line);
    if (NULL == new_line) return NULL;

    new_line->content = calloc(max_len, sizeof *(new_line->content));
    if (NULL == new_line->content) {
        free(new_line);
        return NULL;
    }

    new_line->cap = max_len;
    new_line->len = 0;
    return new_line;
}

void e_remove_char(struct e_line *line, const size_t target_index) {
    size_t i;

    if (NULL == line) return;
    if (NULL == line->content) return;
    if (target_index > line->len - 1) return;

    for (i = target_index; i < line->len; i ++)
        line->content[i] = line->content[i + 1];
    line->len --;
}

void e_add_char(struct e_line *line, const size_t target_index, const char new_char) {
    size_t i;

    if (target_index > line->cap - 1) return;
    if (line == NULL || line->content == NULL) return;
    if (line->len >= line->cap - 1) return;

    for (i = line->len; i > target_index; i --)
        line->content[i] = line->content[i - 1];

    line->content[target_index] = new_char;
    line->len ++;
    line->content[line->len] = '\0';
}

void e_copy_line(struct e_line *dest, const struct e_line *src) {
    size_t i;

    if (
        NULL == dest || NULL == dest->content ||
        NULL == src || NULL == src->content
    ) return;

    dest->len = src->len;
    dest->cap = src->cap;

    for (i = 0; i < src->len; i ++)
        dest->content[i] = src->content[i];

    dest->content[dest->len] = '\0';
}

int e_line_is_empty(const struct e_line *line) {
    size_t i;

    if (NULL == line) return 1;
    if (NULL == line->content) return 1;

    for (i = 0; i < line->len; i ++) {
        if (!isspace((unsigned char)line->content[i])) return 0;
    }

    return 1;
}

int e_line_equal(const struct e_line *line1, const struct e_line *line2) {
    size_t i;

    if (NULL == line1 || NULL == line2) return 0;
    if (line1->len != line2->len) return 0;

    if (NULL == line1->content && NULL == line2->content) return 1;
    else if (NULL == line1->content || NULL == line2->content) return 0;

    for (i = 0; i < line1->len; i ++)
        if (line1->content[i] != line2->content[i]) return 0;
    
    return 1;
}

void e_clean_line(struct e_line *line) {
    if (NULL == line) return;
    if (NULL == line->content) return;
    
    line->len = 0;
    memset(line->content, 0x0, line->cap);
}

void e_free_line(struct e_line *line) {
    if (NULL == line) return;

    if (NULL == line->content) {
        free(line);
        return;
    }
    free(line->content);
    free(line);
}