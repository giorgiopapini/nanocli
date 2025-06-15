#include "e_string.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


struct e_string *e_create_string(const size_t max_len) {
    struct e_string *new_str = malloc(sizeof *new_str);
    if (NULL == new_str) return NULL;

    new_str->content = calloc(max_len, sizeof *(new_str->content));
    if (NULL == new_str->content) {
        free(new_str);
        return NULL;
    }

    new_str->cap = max_len;
    new_str->len = 0;
    return new_str;
}

void e_remove_char(struct e_string *str, const size_t target_index) {
    size_t i;

    if (NULL == str) return;
    if (NULL == str->content) return;
    if (target_index > str->len - 1) return;

    for (i = target_index; i < str->len; i ++)
        str->content[i] = str->content[i + 1];
    str->len --;
}

void e_add_char(struct e_string *str, const size_t target_index, const char new_char) {
    size_t i;

    if (target_index > str->cap - 1) return;
    if (str == NULL || str->content == NULL) return;
    if (str->len >= str->cap - 1) return;

    for (i = str->len; i > target_index; i --)
        str->content[i] = str->content[i - 1];

    str->content[target_index] = new_char;
    str->len ++;
    str->content[str->len] = '\0';
}

void e_copy_str(struct e_string *dest, const struct e_string *src) {
    size_t i;

    if (
        NULL == dest ||
        NULL == dest->content ||
        NULL == src ||
        NULL == src->content
    ) return;

    dest->len = src->len;
    dest->cap = src->cap;

    for (i = 0; i < src->len; i ++)
        dest->content[i] = src->content[i];

    dest->content[dest->len] = '\0';
}

int e_str_is_empty(const struct e_string *str) {
    size_t i;

    if (NULL == str) return 1;
    if (NULL == str->content) return 1;

    for (i = 0; i < str->len; i ++) {
        if (!isspace((unsigned char)str->content[i])) return 0;
    }

    return 1;
}

int e_str_equal(const struct e_string *str1, const struct e_string *str2) {
    size_t i;
    if (NULL == str1 || NULL == str2) return 0;
    if (str1->len != str2->len) return 0;

    if (NULL == str1->content && NULL == str2->content) return 1;
    else if (NULL == str1->content || NULL == str2->content) return 0;

    for (i = 0; i < str1->len; i ++)
        if (str1->content[i] != str2->content[i]) return 0;
    
    return 1;
}

void e_clean_str(struct e_string *str) {
    if (NULL == str) return;
    if (NULL == str->content) return;
    
    str->len = 0;
    memset(str->content, 0x0, str->cap);
}

void e_free_str(struct e_string *str) {
    if (NULL == str) return;

    if (NULL == str->content) {
        free(str);
        return;
    }
    free(str->content);
    free(str);
}