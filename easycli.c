#include "easycli.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>

#include "utils/e_line.h"
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

struct e_cursor {
    size_t x;
    size_t y;
};

struct e_cli_state {
    const char *prompt;  /* should be null terminated */
    struct e_line **p_line;
    struct e_history *history;
    struct e_cursor *curs;
};

void _clear_terminal_screen(void);
void _enable_raw_mode(struct e_stack_err *errs);
int _is_raw_mode_enabled(void);
void _restore_terminal_mode(void);
void _get_terminal_size(size_t *cols, size_t *rows, struct e_stack_err *errs);
static int _is_cli_state_valid(struct e_cli_state *cli, struct e_stack_err *errs);
static void _reset_cursor(struct e_cursor *curs, struct e_stack_err *errs);
/* double pointer is needed because these functions free *p_dest and loads the e_line retrieved from history */
static void _set_line_to_history_curr(struct e_cli_state *cli, struct e_stack_err *errs);
static void _move_cursor_last_line(
    struct e_cli_state *cli,
    const char pressed_key,
    struct e_stack_err *errs
);
static void _up_arrow(struct e_cli_state *cli, struct e_stack_err *errs);
static void _down_arrow(struct e_cli_state *cli, struct e_stack_err *errs);
static void _right_arrow(struct e_cli_state *cli, struct e_stack_err *errs);
static void _left_arrow(struct e_cli_state *cli, struct e_stack_err *errs);
static void _canc(struct e_cli_state *cli, struct e_stack_err *errs);
static void _backspace(struct e_cli_state *cli, struct e_stack_err *errs);
static void _literal(struct e_cli_state *cli, char *c, struct e_stack_err *errs);
static void _clean_display(struct e_cli_state *cli, struct e_stack_err *errs);
static void _write_display(struct e_cli_state *cli, struct e_stack_err *errs);
static e_stat_code _handle_display(struct e_cli_state *cli, char *c, struct e_stack_err *errs);
e_stat_code easycli(struct e_cli_state *cli, struct e_stack_err *errs);

static struct termios orig_termios;
static int termios_saved = 0;
static int raw_mode_on = 0;


void _clear_terminal_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void _enable_raw_mode(struct e_stack_err *errs) {
    struct termios raw;

    if (-1 == tcgetattr(STDIN_FILENO, &orig_termios))
        e_err_push(errs, e_create_err(TCGETATTR_ERROR, TCGETATTR_ERROR_MSG));
    termios_saved = 1;

    raw = orig_termios;
    raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &raw))
        e_err_push(errs, e_create_err(TCSETATTR_ERROR, TCSETATTR_ERROR_MSG));
    raw_mode_on = 1;
}

int _is_raw_mode_enabled(void) {
    return raw_mode_on;
}

void _restore_terminal_mode(void) {
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        raw_mode_on = 0;
    }
}

void _get_terminal_size(size_t *cols, size_t *rows, struct e_stack_err *errs) {
    struct winsize w;
    if (-1 == ioctl(STDOUT_FILENO, TIOCGWINSZ, &w))
        e_err_push(errs, e_create_err(TERMINAL_SIZE_ERROR, TERMINAL_SIZE_ERROR_MSG));

    if (NULL != cols) *cols = w.ws_col;
    if (NULL != rows) *rows = w.ws_row;
}

static int _is_cli_state_valid(struct e_cli_state *cli, struct e_stack_err *errs) {
    int valid = 1;
    if (NULL == cli) {
        valid = 0;
        e_err_push(errs, e_create_err(NULL_POINTER_ERROR, NULL_CLI_ERROR_MSG));
    }
    else {
        if (NULL == cli->curs) {
            valid = 0;
            e_err_push(errs, e_create_err(NULL_POINTER_ERROR, NULL_CURSOR_ERROR_MSG));
        }
        if (NULL == cli->p_line || NULL == *cli->p_line) {
            valid = 0;
            e_err_push(errs, e_create_err(NULL_POINTER_ERROR, NULL_STR_DEST_ERROR_MSG));
        }
        if (NULL == cli->history) {
            valid = 0;
            e_err_push(errs, e_create_err(NULL_POINTER_ERROR, NULL_HISTORY_ERROR_MSG));
            if (NULL == cli->history->entries)
                e_err_push(errs, e_create_err(NULL_POINTER_ERROR, NULL_HISTORY_ERROR_MSG));
        }
    }
    return valid;
}

static void _reset_cursor(struct e_cursor *curs, struct e_stack_err *errs) {
    if (NULL != curs) {
        curs->x = 0;
        curs->y = 0;
    }
    else e_err_push(errs, e_create_err(NULL_POINTER_ERROR, NULL_CURSOR_ERROR_MSG));
}

static void _set_line_to_history_curr(struct e_cli_state *cli, struct e_stack_err *errs) {
    /* e_free_line(*p_dest) --> than allocate new memory for storing the retrieved entry from history.
    the freshly allocated memory will than be automatically deallocated at the end of everything */
    struct e_line *res;
    size_t used_rows;
    size_t term_cols;
    size_t prompt_len;
    _get_terminal_size(&term_cols, NULL, errs);

    if (0 == cli->history->len) return;
    prompt_len = (NULL != cli->prompt) ? strlen(cli->prompt) : 0;

    res = cli->history->entries[cli->history->curr];
    e_free_line(*cli->p_line);
    if (NULL != res) *cli->p_line = e_create_line(res->cap);
    e_copy_line(*cli->p_line, res);

    /* settings the cursor to be at the end of the new string */
    used_rows = (prompt_len + res->len + term_cols - 1) / term_cols;
    cli->curs->y = (used_rows > 0) ? used_rows - 1 : 0;
    cli->curs->x = (res->len + prompt_len) % term_cols;
}

static void _move_cursor_last_line(  /* complete null check */
    struct e_cli_state *cli,
    const char pressed_key,
    struct e_stack_err *errs
) {
    size_t term_cols;
    size_t prompt_len;
    size_t used_rows;
    size_t move_down;
    _get_terminal_size(&term_cols, NULL, errs);

    prompt_len = (NULL != cli->prompt) ? strlen(cli->prompt) : 0;
    used_rows = (prompt_len + (*cli->p_line)->len + term_cols - 1) / term_cols;
    move_down = (used_rows - 1) - cli->curs->y;

    if (cli->curs->x == term_cols)
        if (ARROW_LEFT_KEY == pressed_key) move_down --;
    
    if (move_down > 0) printf("\033[%ldB\r", move_down);

    if (used_rows > 1) 
        printf("\r\033[%ldC", ((*cli->p_line)->len + prompt_len) % (term_cols * (used_rows - 1)));
    else printf("\r\033[%ldC", (*cli->p_line)->len + prompt_len);
    
    fflush(stdout);
}

static void _up_arrow(struct e_cli_state *cli, struct e_stack_err *errs) {
    if (cli->history->curr == cli->history->len) e_clean_line(*cli->p_line);
    else _set_line_to_history_curr(cli, errs);
    
    if (0 == cli->history->curr) cli->history->curr = cli->history->len;
    else cli->history->curr --;
}

static void _down_arrow(struct e_cli_state *cli, struct e_stack_err *errs) { 
    if (cli->history->curr == cli->history->len) cli->history->curr = 0;
    if (cli->history->curr == cli->history->len - 1) {
        e_clean_line(*cli->p_line);
        return;
    }
    
    cli->history->curr ++;
    if (e_line_equal(*cli->p_line, cli->history->entries[cli->history->curr]))
        cli->history->curr ++;
    
    if (cli->history->curr < cli->history->len)
        _set_line_to_history_curr(cli, errs);
    else {
        if (cli->history->len > 0) cli->history->curr = cli->history->len - 1;
        else cli->history->curr = 0;
        e_clean_line(*cli->p_line);
    }
}

static void _right_arrow(struct e_cli_state *cli, struct e_stack_err *errs) {
    size_t term_cols;
    size_t prompt_len;
    _get_terminal_size(&term_cols, NULL, errs);
    prompt_len = (NULL != cli->prompt) ? strlen(cli->prompt) : 0;

    if (cli->curs->y <= SIZE_MAX / term_cols && prompt_len <= SIZE_MAX - (*cli->p_line)->len) {
        if (cli->curs->y * term_cols + cli->curs->x < (*cli->p_line)->len + prompt_len) {
            /* -1 is for the condition to be valid, adding -1 is because
            curs->x starts from 0 and term_cols starts from 1 --> so it is -2 */
            if (cli->curs->x > term_cols - 2) {
                cli->curs->x = 0;
                cli->curs->y ++;
            }
            else cli->curs->x ++;
        }
    }
    else e_err_push(errs, e_create_err(SIZE_T_OVERFLOW, SIZE_T_OVERFLOW_MSG));
}

static void _left_arrow(struct e_cli_state *cli, struct e_stack_err *errs) {
    size_t term_cols;
    size_t prompt_len;
    _get_terminal_size(&term_cols, NULL, errs);

    if (0 == (*cli->p_line)->len) return;
    prompt_len = (NULL != cli->prompt) ? strlen(cli->prompt) : 0;

    if (cli->curs->y > 0) {
        if (cli->curs->x == 0) {
            cli->curs->x = term_cols - 1;
            cli->curs->y --;
        }
        else cli->curs->x --;
    }
    else if (cli->curs->x > prompt_len) cli->curs->x --;
}

static void _canc(struct e_cli_state *cli, struct e_stack_err *errs) {
    size_t term_cols;
    size_t abs_x = 0;
    size_t prompt_len;
    size_t used_rows;

    prompt_len = (NULL != cli->prompt) ? strlen(cli->prompt) : 0;
    _get_terminal_size(&term_cols, NULL, errs);
    used_rows = (prompt_len + (*cli->p_line)->len + term_cols - 1) / term_cols;

    if ((*cli->p_line)->len > 0 && cli->curs->x < term_cols && used_rows >= 1) {
        if (0 == cli->curs->y) abs_x = cli->curs->x;
        else abs_x = cli->curs->x + (cli->curs->y * term_cols);

        /* this condition prevents canc beyond string end */
        if (abs_x - prompt_len < (*cli->p_line)->len) e_remove_char(*cli->p_line, abs_x - prompt_len);
    }
}

static void _backspace(struct e_cli_state *cli, struct e_stack_err *errs) {
    /* move the cursor_pos than call _canc(...) function */
    size_t term_cols;
    size_t prompt_len;
    int exec_backspace = 1;
    _get_terminal_size(&term_cols, NULL, errs);
    prompt_len = (NULL != cli->prompt) ? strlen(cli->prompt) : 0;

    if (cli->curs->y > 0) {
        if (cli->curs->x == 0) {
            cli->curs->y --;
            cli->curs->x = term_cols - 1;
        }
        else cli->curs->x --;
    }
    else if (cli->curs->y == 0) {
        if (cli->curs->x > prompt_len) cli->curs->x --;
        else exec_backspace = 0;
    }
    if (exec_backspace) _canc(cli, errs);

    if (0 == cli->curs->x && 0 < cli->curs->y) {
        cli->curs->y --;
        cli->curs->x = term_cols;
    }
}

static void _literal(struct e_cli_state *cli, char *c, struct e_stack_err *errs) {
    size_t term_cols;
    size_t prompt_len;
    size_t real_index = 0;
    _get_terminal_size(&term_cols, NULL, errs);
    prompt_len = (NULL != cli->prompt) ? strlen(cli->prompt) : 0;

    if ((*cli->p_line)->len < (*cli->p_line)->cap - 1) {
        if (0 == cli->curs->y) real_index = cli->curs->x - prompt_len;
        else {
            real_index = term_cols - prompt_len;  /* first row (excludes the prompt str length) */
            if (cli->curs->y > 1) real_index += (cli->curs->y - 1) * term_cols;  /* length rows in the middle */
            real_index += cli->curs->x;
        }
        e_add_char(*cli->p_line, real_index, *c);  /* curs->x - prompt_len is valid only for the first line */

        if (cli->curs->x > term_cols - 1) {
            cli->curs->x = 1;
            cli->curs->y ++;
        }
        else cli->curs->x ++;
    }
    else e_err_push(errs, e_create_err(BUFFER_OVERFLOW, BUFFER_OVERFLOW_MSG));
}

static void _clean_display(struct e_cli_state *cli, struct e_stack_err *errs) {
    size_t term_rows;
    size_t term_cols;
    size_t used_rows;
    size_t i;
    size_t prompt_len = (NULL == cli->prompt) ? 0 : strlen(cli->prompt);
    _get_terminal_size(&term_cols, &term_rows, errs);
    used_rows = (prompt_len + (*cli->p_line)->len + term_cols - 1) / term_cols;

    _move_cursor_last_line(cli, 0, errs);

    /* clean output */
    for (i = 0; i < used_rows; i ++) {
        printf("\033[2K");
        if (i < used_rows - 1) printf("\033[A");
    }
    printf("\r");
}

static void _write_display(struct e_cli_state *cli, struct e_stack_err *errs) {
    size_t term_rows;
    size_t term_cols;
    size_t used_rows;
    size_t prompt_len = (NULL == cli->prompt) ? 0 : strlen(cli->prompt);
    _get_terminal_size(&term_cols, &term_rows, errs);
    used_rows = (prompt_len + (*cli->p_line)->len + term_cols - 1) / term_cols;

    printf("%s%s", cli->prompt, (*cli->p_line)->content);
    if (1 < used_rows) {
        printf("\033[%ldA\r", used_rows - 1);
        if (cli->curs->y > 0) printf("\033[%ldB", cli->curs->y);
        if (cli->curs->x > 0) printf("\033[%ldC", cli->curs->x);
    }
    else if (cli->curs->x > 0) printf("\r\033[%ldC", cli->curs->x);
}

static e_stat_code _handle_display(
    struct e_cli_state *cli,
    char *c,
    struct e_stack_err *errs
) {
    e_stat_code status = E_CONTINUE;
    if (!_is_cli_state_valid(cli, errs)) return E_EXIT;

    if (ENTER_KEY == *c) {
        (*cli->p_line)->content[(*cli->p_line)->len] = '\0';
        status = E_SEND_COMMAND;
        _move_cursor_last_line(cli, *c, errs);
        e_add_entry(cli->history, *cli->p_line);
        printf("\r\n");
    }
    else {
        _clean_display(cli, errs);
        if (127 == *c || 8 == *c) {
            if ((*cli->p_line)->len > 0) _backspace(cli, errs);
        }
        else if ('\033' == *c) {  /* catch special keys, add behaviour only to arrow keys (for now) */
            read(STDIN_FILENO, c, 1);
            read(STDIN_FILENO, c, 1);
            switch(*c) {
                case ARROW_UP_KEY:          _up_arrow(cli, errs); break;
                case ARROW_DOWN_KEY:        _down_arrow(cli, errs); break;
                case ARROW_RIGHT_KEY:       _right_arrow(cli, errs); break;
                case ARROW_LEFT_KEY:        _left_arrow(cli, errs); break;
                case CANC_KEY: {
                    /* remove undesired tilde char after canc, otherwise it will affect refresh_display function */
                    read(STDIN_FILENO, c, 1);
                    _canc(cli, errs);
                    break;
                }
                default:                    break;
            }
        }
        else if (IS_LITERAL(*c)) _literal(cli, c, errs);
        _write_display(cli, errs);
    }

    fflush(stdout);
    return status;
}

e_stat_code easycli(struct e_cli_state *cli, struct e_stack_err *errs) {
    fd_set readfds;
    e_stat_code retval = E_CONTINUE;
    int ret;
    char c;

    if (NULL == cli || NULL == cli->p_line || NULL == *(cli->p_line)) {
        retval = E_EXIT;
        e_err_push(errs, e_create_err(NULL_POINTER_ERROR, NULL_STR_DEST_ERROR_MSG));
        goto exit;
    }

    /* print prompt in the first input line */
    if (0 == (*cli->p_line)->len && NULL != cli->prompt) {
        printf("\r%s", cli->prompt);  /* \r overwrites prompt if prompt alredy written */
        cli->curs->x = strlen(cli->prompt);
        fflush(stdout);
    }

    if (!_is_raw_mode_enabled()) {
        _enable_raw_mode(errs);
        atexit(_restore_terminal_mode);
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
    else if (read(STDIN_FILENO, &c, 1) > 0) retval = _handle_display(cli, &c, errs);

exit:
    if (E_EXIT == retval) _restore_terminal_mode();
    return retval;
}

void run_easycli_ctx(
    const char *prompt,
    size_t max_str_len,
    void *ctx,
    e_stat_code (*callback_on_enter)(struct e_line *dest, void *ctx, struct e_stack_err *errs)
) {
    struct e_stack_err *errs = e_create_stack_err(E_STACK_ERR_SIZE);
    struct e_line *line = e_create_line(max_str_len + 1);  /* +1 includes '\0' */
    struct e_history *history = e_create_history(DEFAULT_HISTORY_MAX_SIZE);
    struct e_cursor curs = { .x = 0, .y = 0 };
    e_stat_code code;

    struct e_cli_state cli = {
        .prompt = prompt,
        .p_line = &line,
        .history = history,
        .curs = &curs
    };

    _clear_terminal_screen();
    do {
        code = easycli(&cli, errs);
        if (E_SEND_COMMAND == code) {
            if (callback_on_enter) code = callback_on_enter(line, ctx, errs);
            e_clean_line(line);
            _reset_cursor(&curs, errs);
        }

        /* if an error is found, pop everything from error stack and clean string for next input */
        if (errs->top >= 0) {
            e_stack_flush_errors(errs);
            e_clean_line(line);
        }
    }
    while (E_EXIT != code);

    e_free_line(line);
    e_free_stack_err(errs);
    e_free_history(history);
}