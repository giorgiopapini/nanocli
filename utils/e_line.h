#ifndef e_line_H
#define e_line_H

#include <stddef.h>

struct e_line {
    char *content;
    size_t len;  /* excluding NULL terminator */
    size_t cap;  /* allocated size (max_len) */
};


struct e_line *e_create_line(const size_t max_len);
void e_remove_char(struct e_line *line, const size_t target_index);
void e_add_char(struct e_line *line, const size_t target_index, const char new_char);
void e_copy_line(struct e_line *dest, const struct e_line *src);
int e_line_is_empty(const struct e_line *line);
int e_line_equal(const struct e_line *line1, const struct e_line *line2);
void e_clean_line(struct e_line *line);
void e_free_line(struct e_line *line);

#endif