#ifndef EASYCLI_LIB_H
#define EASYCLI_LIB_H

#include <stddef.h>

#define DEFAULT_PROMPT              "easycli > "
#define DEFAULT_MASKED_CHAR         '*'
#define DEFAULT_MAX_INPUT_LEN       1024
#define DEFAULT_HISTORY_MAX_SIZE    1024


char *easycli(const char *prompt, size_t max_str_len);
char *easy_ask(const char *question, const size_t max_len, const int masked);
void easy_print(const char *str);

#endif