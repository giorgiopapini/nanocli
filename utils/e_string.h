#ifndef E_STRING_H
#define E_STRING_H

#include <stddef.h>

struct e_string {
    char *content;
    size_t len;  /* excluding NULL terminator */
    size_t cap;  /* allocated size (max_len) */
};


struct e_string *e_create_string(const size_t max_len);
void e_remove_char(struct e_string *str, const size_t target_index);
void e_add_char(struct e_string *str, const size_t target_index, const char new_char);
void e_copy_str(struct e_string *dest, const struct e_string *src);
int e_str_is_empty(const struct e_string *str);
int e_str_equal(const struct e_string *str1, const struct e_string *str2);
void e_clean_str(struct e_string *str);
void e_free_str(struct e_string *str);

#endif