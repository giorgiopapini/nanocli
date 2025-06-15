#include "easycli.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <errno.h>

#include "utils/terminal_handler.h"
#include "utils/e_string.h"
#include "utils/e_history.h"
#include "utils/error_handler.h"

#define IS_LITERAL(c)       ((unsigned int)(c) >= 32 && (unsigned int)(c) <= 126)

/*
    important! --> fflush(stdout) is not automatically executed inside display management functions
    like "_clean_display()", "_write_display()", and "_handle_display()". It should be executed
    after function execution
    (e.g: )
        18|     _write_display(...);
        19|     fflush(stdout);
*/

static void _reset_cursor(struct e_cursor *curs, struct e_stack_err *errs);
static void _clear_screen(void);
/* double pointer is needed because these functions free *p_dest and loads the e_string retrieved from history */
static void _set_str_to_history_curr(
    struct e_string **p_dest,
    struct e_history *history,
    const size_t prompt_len,
    struct e_cursor *curs,
    struct e_stack_err *errs
);
static void _move_cursor_last_line(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    const char pressed_key,
    struct e_stack_err *errs
);
static void _up_arrow(
    struct e_string **dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_history *history,
    struct e_stack_err *errs
);
static void _down_arrow(
    struct e_string **dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_history *history,
    struct e_stack_err *errs
);
static void _right_arrow(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_stack_err *errs
);
static void _left_arrow(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_stack_err *errs
);
static void _canc(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_stack_err *errs
);
static void _backspace(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_stack_err *errs
);
static void _literal(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    char *c,
    struct e_stack_err *errs
);
static void _clean_display(
    const char *prompt,
    struct e_string *dest,
    struct e_cursor *curs,
    const char pressed_key,
    struct e_stack_err *errs
);
static void _write_display(
    const char *prompt,
    struct e_string *dest,
    struct e_cursor *curs,
    struct e_stack_err *errs
);
static stat_code _handle_display(
    const char *prompt,
    struct e_string **dest,
    char *c,
    struct e_cursor *curs,
    struct e_history *history,
    struct e_stack_err *errs
);


static void _reset_cursor(struct e_cursor *curs, struct e_stack_err *errs) {
    if (NULL != curs) {
        curs->x = 0;
        curs->y = 0;
    }
    else e_err_push(errs, e_create_err(NULL_CURSOR_ERROR, NULL_CURSOR_ERROR_MSG));
}

static void _clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void _set_str_to_history_curr(
    struct e_string **p_dest,
    struct e_history *history,
    const size_t prompt_len,
    struct e_cursor *curs,
    struct e_stack_err *errs
) {
    /* e_free_str(*p_dest) --> than allocate new memory for storing the retrieved entry from history.
    the freshly allocated memory will than be automatically deallocated at the end of everything */
    struct e_string *res;
    size_t used_rows;
    size_t term_cols;

    get_terminal_size(&term_cols, NULL, errs);

    if (NULL == p_dest || NULL == *p_dest) return;
    if (0 == history->len) return;

    res = history->entries[history->curr];
    e_free_str(*p_dest);
    *p_dest = NULL;
    *p_dest = e_create_string(res->cap);
    e_copy_str(*p_dest, res);

    /* settings the cursor to be at the end of the new string */
    used_rows = (prompt_len + res->len + term_cols - 1) / term_cols;
    curs->y = (used_rows > 0) ? used_rows - 1 : 0;
    curs->x = (res->len + prompt_len) % term_cols;
}

static void _move_cursor_last_line(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    const char pressed_key,
    struct e_stack_err *errs
) {
    size_t term_cols;
    size_t used_rows;
    size_t move_down;
    get_terminal_size(&term_cols, NULL, errs);

    if (NULL == dest || NULL == curs) {
        if (NULL == dest) e_err_push(errs, e_create_err(NULL_STR_DEST_ERROR, NULL_STR_DEST_ERROR_MSG));
        if (NULL == curs) e_err_push(errs, e_create_err(NULL_CURSOR_ERROR, NULL_CURSOR_ERROR_MSG));
        return;
    }

    used_rows = (prompt_len + dest->len + term_cols - 1) / term_cols;
    move_down = (used_rows - 1) - curs->y;

    if (curs->x == term_cols)
        if (ARROW_LEFT_KEY == pressed_key) move_down --;
    
    if (move_down > 0) printf("\033[%ldB\r", move_down);

    if (used_rows > 1) 
        printf("\r\033[%ldC", (dest->len + prompt_len) % (term_cols * (used_rows - 1)));
    else printf("\r\033[%ldC", dest->len + prompt_len);
    
    fflush(stdout);
}

static void _up_arrow(
    struct e_string **dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_history *history,
    struct e_stack_err *errs
) {
    if (
        NULL == dest || NULL == *dest ||
        NULL == history || NULL == history->entries ||
        NULL == curs
    ) return;
    
    if (history->curr == history->len) e_clean_str(*dest);
    else _set_str_to_history_curr(dest, history, prompt_len, curs, errs);
    
    if (0 == history->curr) history->curr = history->len;
    else history->curr --;
}

static void _down_arrow(
    struct e_string **dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_history *history,
    struct e_stack_err *errs
) {
    if (
        NULL == dest || NULL == *dest ||
        NULL == history || NULL == history->entries ||
        NULL == curs
    ) return;
    
    if (history->curr == history->len) history->curr = 0;
    if (history->curr == history->len - 1) {
        e_clean_str(*dest);
        return;
    }
    
    history->curr ++;
    if (e_str_equal(*dest, history->entries[history->curr])) history->curr ++;
    
    if (history->curr < history->len)
        _set_str_to_history_curr(dest, history, prompt_len, curs, errs);
    else {
        history->curr = history->len - 1;
        e_clean_str(*dest);
    }
}

static void _right_arrow(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_stack_err *errs
) {
    size_t term_cols;
    get_terminal_size(&term_cols, NULL, errs);

    if (NULL == dest || NULL == curs) {
        if (NULL == dest) e_err_push(errs, e_create_err(NULL_STR_DEST_ERROR, NULL_STR_DEST_ERROR_MSG));
        if (NULL == curs) e_err_push(errs, e_create_err(NULL_CURSOR_ERROR, NULL_CURSOR_ERROR_MSG));
        return;
    }

    if (curs->y <= SIZE_MAX / term_cols && prompt_len <= SIZE_MAX - dest->len) {
        if (curs->y * term_cols + curs->x < dest->len + prompt_len) {
            /* -1 is for the condition to be valid, adding -1 is because
            curs->x starts from 0 and term_cols starts from 1 --> so it is -2 */
            if (curs->x > term_cols - 2) {
                curs->x = 0;
                curs->y ++;
            }
            else curs->x ++;
        }
    }
    else e_err_push(errs, e_create_err(SIZE_T_OVERFLOW, SIZE_T_OVERFLOW_MSG));
}

static void _left_arrow(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_stack_err *errs
) {
    size_t term_cols;
    get_terminal_size(&term_cols, NULL, errs);

    if (NULL == dest || NULL == curs) {
        if (NULL == dest) e_err_push(errs, e_create_err(NULL_STR_DEST_ERROR, NULL_STR_DEST_ERROR_MSG));
        if (NULL == curs) e_err_push(errs, e_create_err(NULL_CURSOR_ERROR, NULL_CURSOR_ERROR_MSG));
        return;
    }
    
    if (0 == dest->len) return;

    if (curs->y > 0) {
        if (curs->x == 0) {
            curs->x = term_cols - 1;
            curs->y --;
        }
        else curs->x --;
    }
    else if (curs->x > prompt_len) curs->x --;
}

static void _canc(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_stack_err *errs
) {
    size_t term_cols;
    size_t abs_x = 0;
    size_t used_rows;
    
    get_terminal_size(&term_cols, NULL, errs);
    used_rows = (prompt_len + dest->len + term_cols - 1) / term_cols;

    if (dest->len > 0 && curs->x < term_cols && used_rows >= 1) {
        if (0 == curs->y) abs_x = curs->x;
        else abs_x = curs->x + (curs->y * term_cols);

        /* this condition prevents canc beyond string end */
        if (abs_x - prompt_len < dest->len) e_remove_char(dest, abs_x - prompt_len);
    }
}

static void _backspace(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    struct e_stack_err *errs
) {
    /* move the cursor_pos than call _canc(...) function */
    size_t term_cols;
    int exec_backspace = 1;
    get_terminal_size(&term_cols, NULL, errs);

    if (curs->y > 0) {
        if (curs->x == 0) {
            curs->y --;
            curs->x = term_cols - 1;
        }
        else curs->x --;
    }
    else if (curs->y == 0) {
        if (curs->x > prompt_len) curs->x --;
        else exec_backspace = 0;
    }
    if (exec_backspace) _canc(dest, curs, prompt_len, errs);

    if (0 == curs->x && 0 < curs->y) {
        curs->y --;
        curs->x = term_cols;
    }
}

static void _literal(
    struct e_string *dest,
    struct e_cursor *curs,
    const size_t prompt_len,
    char *c,
    struct e_stack_err *errs
) {
    size_t term_cols;
    size_t real_index = 0;
    get_terminal_size(&term_cols, NULL, errs);

    if (NULL == dest || NULL == curs) {
        if (NULL == dest) e_err_push(errs, e_create_err(NULL_STR_DEST_ERROR, NULL_STR_DEST_ERROR_MSG));
        if (NULL == curs) e_err_push(errs, e_create_err(NULL_CURSOR_ERROR, NULL_CURSOR_ERROR_MSG));
        return;
    }

    if (dest->len < dest->cap - 1) {
        if (0 == curs->y) real_index = curs->x - prompt_len;
        else {
            real_index = term_cols - prompt_len;  /* first row (excludes the prompt str length) */
            if (curs->y > 1) real_index += (curs->y - 1) * term_cols;  /* length rows in the middle */
            real_index += curs->x;
        }
        e_add_char(dest, real_index, *c);  /* curs->x - prompt_len is valid only for the first line */

        if (curs->x > term_cols - 1) {
            curs->x = 1;
            curs->y ++;
        }
        else curs->x ++;
    }
    else e_err_push(errs, e_create_err(BUFFER_OVERFLOW, BUFFER_OVERFLOW_MSG));
}

static void _clean_display(
    const char *prompt,
    struct e_string *dest,
    struct e_cursor *curs,
    const char pressed_key,
    struct e_stack_err *errs
) {
    size_t term_rows;
    size_t term_cols;
    size_t used_rows;
    size_t i;
    size_t prompt_len = (NULL == prompt) ? 0 : strlen(prompt);


    if (NULL == dest || NULL == curs) {
        if (NULL == dest) e_err_push(errs, e_create_err(NULL_STR_DEST_ERROR, NULL_STR_DEST_ERROR_MSG));
        if (NULL == curs) e_err_push(errs, e_create_err(NULL_CURSOR_ERROR, NULL_CURSOR_ERROR_MSG));
        return;
    }

    get_terminal_size(&term_cols, &term_rows, errs);
    used_rows = (prompt_len + dest->len + term_cols - 1) / term_cols;

    _move_cursor_last_line(dest, curs, prompt_len, pressed_key, errs);

    /* clean output */
    for (i = 0; i < used_rows; i ++) {
        printf("\033[2K");
        if (i < used_rows - 1) printf("\033[A");
    }
    printf("\r");
}

static void _write_display(
    const char *prompt,
    struct e_string *dest,
    struct e_cursor *curs,
    struct e_stack_err *errs
) {
    size_t term_rows;
    size_t term_cols;
    size_t used_rows;
    size_t prompt_len = (NULL == prompt) ? 0 : strlen(prompt);

    if (NULL == dest || NULL == curs) {
        if (NULL == dest) e_err_push(errs, e_create_err(NULL_STR_DEST_ERROR, NULL_STR_DEST_ERROR_MSG));
        if (NULL == curs) e_err_push(errs, e_create_err(NULL_CURSOR_ERROR, NULL_CURSOR_ERROR_MSG));
        return;
    }

    get_terminal_size(&term_cols, &term_rows, errs);
    used_rows = (prompt_len + dest->len + term_cols - 1) / term_cols;

    /* write output */
    printf("%s%s", prompt, dest->content);
    
    /* set cursor position */
    /* back to start, than reach the cursor position */
    if (1 < used_rows) {
        printf("\033[%ldA\r", used_rows - 1);
        if (curs->y > 0) printf("\033[%ldB", curs->y);
        if (curs->x > 0) printf("\033[%ldC", curs->x);
    }
    else if (curs->x > 0) printf("\r\033[%ldC", curs->x);
}

static stat_code _handle_display(
    const char *prompt,
    struct e_string **dest,
    char *c,
    struct e_cursor *curs,
    struct e_history *history,
    struct e_stack_err *errs
) {
    stat_code status = E_CONTINUE;
    size_t prompt_len = 0;
    if (NULL != prompt) prompt_len = strlen(prompt);

    if (NULL == dest || NULL == *dest) {
        e_err_push(errs, e_create_err(NULL_STR_DEST_ERROR, NULL_STR_DEST_ERROR_MSG));
        return E_EXIT;
    }

    if (ENTER_KEY == *c) {
        (*dest)->content[(*dest)->len] = '\0';
        status = E_SEND_COMMAND;
        _move_cursor_last_line(*dest, curs, prompt_len, *c, errs);
        e_add_entry(history, *dest);
        printf("\r\n");
    }
    else {
        _clean_display(prompt, *dest, curs, *c, errs);
        if (127 == *c || 8 == *c) {
            if ((*dest)->len > 0) _backspace(*dest, curs, prompt_len, errs);
        }
        else if ('\033' == *c) {  /* catch special keys, add behaviour only to arrow keys (for now) */
            read(STDIN_FILENO, c, 1);
            read(STDIN_FILENO, c, 1);
            switch(*c) {
                case ARROW_UP_KEY:          _up_arrow(dest, curs, prompt_len, history, errs); break;
                case ARROW_DOWN_KEY:        _down_arrow(dest, curs, prompt_len, history, errs); break;
                case ARROW_RIGHT_KEY:       _right_arrow(*dest, curs, prompt_len, errs); break;
                case ARROW_LEFT_KEY:        _left_arrow(*dest, curs, prompt_len, errs); break;
                case CANC_KEY: {
                    /* remove undesired tilde char after canc, otherwise it will affect refresh_display function */
                    read(STDIN_FILENO, c, 1);
                    _canc(*dest, curs, prompt_len, errs);
                    break;
                }
                default:                    break;
            }
        }
        else if (IS_LITERAL(*c)) _literal(*dest, curs, prompt_len, c, errs);
        _write_display(prompt, *dest, curs, errs);
    }

    fflush(stdout);
    return status;
}

stat_code easycli(
    const char *prompt,
    struct e_string **dest,
    struct e_cursor *curs,
    struct e_history *history,
    struct e_stack_err *errs
) {
    /* return stat_code for the main loop. It also supports e_error pointer to pointer in case the
    developer want to keep track of errors */
    fd_set readfds;
    stat_code retval = E_CONTINUE;
    int ret;
    char c;

    if (NULL == dest || NULL == *dest) {
        retval = E_EXIT;
        e_err_push(errs, e_create_err(NULL_STR_DEST_ERROR, NULL_STR_DEST_ERROR_MSG));
        goto exit;
    }

    /* print prompt in the first input line */
    if (0 == (*dest)->len && NULL != prompt) {
        printf("\r%s", prompt);  /* \r overwrite if prompt alredy written */
        curs->x = strlen(prompt);
        fflush(stdout);
    }

    if (!is_raw_mode_enabled()) {
        enable_raw_mode(errs);
        atexit(restore_terminal_mode);
    }

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, NULL);

    if (-1 == ret) {
        if (EINTR == errno) {
            retval = E_EXIT;
            goto exit;
        }
        else {
            retval = E_EXIT;
            e_err_push(errs, e_create_err(SELECT_ERROR, SELECT_ERROR_MSG));
            goto exit;
        }
    }
    else if (read(STDIN_FILENO, &c, 1) > 0) retval = _handle_display(prompt, dest, &c, curs, history, errs);

exit:
    if (E_EXIT == retval) restore_terminal_mode();
    return retval;
}

void run_easycli_ctx(
    const char *prompt,
    size_t max_str_len,
    void *ctx,
    stat_code (*callback_on_enter)(struct e_string *dest, void *ctx, struct e_stack_err *errs)
) {
    struct e_stack_err *errs = e_create_stack_err(E_STACK_ERR_SIZE);
    struct e_string *str = e_create_string(max_str_len + 1);  /* +1 includes '\0' */
    struct e_history *history = e_create_history(DEFAULT_HISTORY_MAX_SIZE);
    struct e_cursor curs = { .x = 0, .y = 0 };
    stat_code code;

    _clear_screen();
    do {
        code = easycli(prompt, &str, &curs, history, errs);
        if (E_SEND_COMMAND == code) {
            if (callback_on_enter) code = callback_on_enter(str, ctx, errs);
            e_clean_str(str);
            _reset_cursor(&curs, errs);
        }

        /* if an error is found, pop everything from error stack and clean string for next input */
        if (errs->top >= 0) {
            e_stack_flush_errors(errs);
            e_clean_str(str);
        }
    }
    while (E_EXIT != code);

    e_free_str(str);
    e_free_stack_err(errs);
    e_free_history(history);
}