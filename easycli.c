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
#include <ctype.h>
#include <termios.h>

#define ARROW_UP_KEY 'A'
#define ARROW_DOWN_KEY 'B'
#define ARROW_RIGHT_KEY 'C'
#define ARROW_LEFT_KEY 'D'
#define CANC_KEY '3'
#define TILDE_KEY '~'

struct e_cursor {
    size_t x;
    size_t y;
};

struct e_line {
    char *content;
    size_t len;  /* excluding NULL terminator */
    size_t cap;  /* allocated size (max_len) */
};

struct e_history {
    struct e_line **entries;  /* dinamically allocated e_line pointers */
    size_t curr;
    size_t len;
    size_t cap;
};

struct e_cli_state {
    const char *prompt;  /* should be null terminated */
    struct e_line **p_line;
    struct e_history *history;
    struct e_cursor *curs;
};

typedef enum {
	CTRL_A = 1,
	CTRL_B = 2,
	CTRL_C = 3,
	CTRL_D = 4,
	CTRL_E = 5,
	CTRL_F = 6,
	CTRL_H = 8,
	TAB = 9,
    NEWLINE_KEY = 10,
	CTRL_K = 11,
	CTRL_L = 12,
	CARR_RET_KEY = 13,
	CTRL_N = 14,
	CTRL_P = 16,
	CTRL_T = 20,
	CTRL_U = 21,
	CTRL_W = 23,
	ESC_KEY = 27,
	BACKSPACE_KEY = 127
} e_keys;

/* ================= functions related to line management ================== */
struct e_line *e_create_line(const size_t max_len);
void e_remove_char(struct e_line *line, const size_t target_index);
void e_add_char(struct e_line *line, const size_t target_index, const char new_char);
void e_copy_line(struct e_line *dest, const struct e_line *src);
int e_line_is_empty(const struct e_line *line);
int e_line_equal(const struct e_line *line1, const struct e_line *line2);
void e_clean_line(struct e_line *line);
void e_free_line(struct e_line *line);
/* ========================================================================= */
/* ================ functions related to history management ================ */
struct e_history *e_create_history(const size_t max_len);
void e_add_entry(struct e_history *history, const struct e_line *new_line);
void e_free_history(struct e_history *history);
/* ========================================================================= */
/* ========================== terminal management ========================== */
void _clear_terminal_screen(void);
void _enable_raw_mode(void);
int _is_raw_mode_enabled(void);
void _restore_terminal_mode(void);

static struct termios orig_termios;
static int termios_saved = 0;
static int raw_mode_on = 0;
/* ========================================================================= */

void _get_terminal_size(size_t *cols, size_t *rows);
static int _is_cli_state_valid(struct e_cli_state *cli);
static void _reset_cursor(struct e_cursor *curs);
/* double pointer is needed because these functions free *p_dest and loads the e_line retrieved from history */
static void _set_line_to_history_curr(struct e_cli_state *cli);
static void _move_cursor_last_line(struct e_cli_state *cli, const char pressed_key);
static void _enter(struct e_cli_state *cli, const char c);
static void _up_arrow(struct e_cli_state *cli);
static void _down_arrow(struct e_cli_state *cli);
static void _right_arrow(struct e_cli_state *cli);
static void _left_arrow(struct e_cli_state *cli);
static void _canc(struct e_cli_state *cli);
static void _backspace(struct e_cli_state *cli);
static void _literal(struct e_cli_state *cli, char *c);
static void _clean_display(struct e_cli_state *cli);
static void _write_display(struct e_cli_state *cli);
static e_stat_code _handle_display(struct e_cli_state *cli, char *c);
e_stat_code easycli(struct e_cli_state *cli);


/* ================= functions related to line management ================== */
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

    if (NULL == dest || NULL == dest->content || NULL == src || NULL == src->content)
        return;

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

    for (i = 0; i < line->len; i ++)
        if (!isspace((unsigned char)line->content[i])) return 0;
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
/* ========================================================================= */
/* ================ functions related to history management ================ */
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
/* ========================================================================= */
/* ========================== terminal management ========================== */
void _clear_terminal_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void _enable_raw_mode(void) {
    struct termios raw;

    if (-1 == tcgetattr(STDIN_FILENO, &orig_termios)) return;
    termios_saved = 1;

    raw = orig_termios;
    raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &raw)) return;
    raw_mode_on = 1;
}

int _is_raw_mode_enabled(void) { return raw_mode_on; }

void _restore_terminal_mode(void) {
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        raw_mode_on = 0;
    }
}

void _get_terminal_size(size_t *cols, size_t *rows) {
    struct winsize w;
    if (-1 == ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)) return;

    if (NULL != cols) *cols = w.ws_col;
    if (NULL != rows) *rows = w.ws_row;
}
/* ========================================================================= */
/* ============================ CLI management ============================= */
static int _is_cli_state_valid(struct e_cli_state *cli) {
    if (NULL == cli) return 0;
    if (NULL == cli->curs) return 0;
    if (NULL == cli->p_line || NULL == *cli->p_line) return 0;
    if (NULL == cli->history || NULL == cli->history->entries) return 0;
    return 1;
}

static void _reset_cursor(struct e_cursor *curs) {
    if (NULL != curs) {
        curs->x = 0;
        curs->y = 0;
    }
}

static void _set_line_to_history_curr(struct e_cli_state *cli) {
    /* e_free_line(*p_dest) --> than allocate new memory for storing the retrieved entry from history.
    the freshly allocated memory will than be automatically deallocated at the end of everything */
    struct e_line *res;
    size_t used_rows;
    size_t term_cols = 0;
    size_t prompt_len;
    _get_terminal_size(&term_cols, NULL);

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

static void _move_cursor_last_line(struct e_cli_state *cli, const char pressed_key) {
    size_t term_cols = 0;
    size_t prompt_len;
    size_t used_rows;
    size_t move_down;
    _get_terminal_size(&term_cols, NULL);

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

static void _enter(struct e_cli_state *cli, const char c) {
    (*cli->p_line)->content[(*cli->p_line)->len] = '\0';
    _move_cursor_last_line(cli, c);
    e_add_entry(cli->history, *cli->p_line);
    printf("\r\n");
}

static void _up_arrow(struct e_cli_state *cli) {
    if (cli->history->curr == cli->history->len) e_clean_line(*cli->p_line);
    else _set_line_to_history_curr(cli);
    
    if (0 == cli->history->curr) cli->history->curr = cli->history->len;
    else cli->history->curr --;
}

static void _down_arrow(struct e_cli_state *cli) { 
    if (cli->history->curr == cli->history->len) cli->history->curr = 0;
    if (cli->history->curr == cli->history->len - 1) {
        e_clean_line(*cli->p_line);
        return;
    }
    
    cli->history->curr ++;
    if (e_line_equal(*cli->p_line, cli->history->entries[cli->history->curr]))
        cli->history->curr ++;
    
    if (cli->history->curr < cli->history->len)
        _set_line_to_history_curr(cli);
    else {
        if (cli->history->len > 0) cli->history->curr = cli->history->len - 1;
        else cli->history->curr = 0;
        e_clean_line(*cli->p_line);
    }
}

static void _right_arrow(struct e_cli_state *cli) {
    size_t term_cols = 0;
    size_t prompt_len;
    _get_terminal_size(&term_cols, NULL);
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
}

static void _left_arrow(struct e_cli_state *cli) {
    size_t term_cols = 0;
    size_t prompt_len;
    _get_terminal_size(&term_cols, NULL);

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

static void _canc(struct e_cli_state *cli) {
    size_t term_cols = 0;
    size_t abs_x = 0;
    size_t prompt_len;
    size_t used_rows;

    prompt_len = (NULL != cli->prompt) ? strlen(cli->prompt) : 0;
    _get_terminal_size(&term_cols, NULL);
    used_rows = (prompt_len + (*cli->p_line)->len + term_cols - 1) / term_cols;

    if ((*cli->p_line)->len > 0 && cli->curs->x < term_cols && used_rows >= 1) {
        if (0 == cli->curs->y) abs_x = cli->curs->x;
        else abs_x = cli->curs->x + (cli->curs->y * term_cols);

        /* this condition prevents canc beyond string end */
        if (abs_x - prompt_len < (*cli->p_line)->len) e_remove_char(*cli->p_line, abs_x - prompt_len);
    }
}

static void _backspace(struct e_cli_state *cli) {
    /* move the cursor_pos than call _canc(...) function */
    size_t term_cols = 0;
    size_t prompt_len;
    int exec_backspace = 1;
    if (0 >= (*cli->p_line)->len) return;  /* nothing to delete, return */
    
    _get_terminal_size(&term_cols, NULL);
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
    if (exec_backspace) _canc(cli);

    if (0 == cli->curs->x && 0 < cli->curs->y) {
        cli->curs->y --;
        cli->curs->x = term_cols;
    }
}

static void _literal(struct e_cli_state *cli, char *c) {
    size_t term_cols = 0;
    size_t prompt_len;
    size_t real_index = 0;
    _get_terminal_size(&term_cols, NULL);
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
}

static void _clean_display(struct e_cli_state *cli) {
    size_t term_rows;
    size_t term_cols = 0;
    size_t used_rows;
    size_t i;
    size_t prompt_len = (NULL == cli->prompt) ? 0 : strlen(cli->prompt);
    _get_terminal_size(&term_cols, &term_rows);
    used_rows = (prompt_len + (*cli->p_line)->len + term_cols - 1) / term_cols;

    _move_cursor_last_line(cli, 0);

    /* clean output */
    for (i = 0; i < used_rows; i ++) {
        printf("\033[2K");
        if (i < used_rows - 1) printf("\033[A");
    }
    printf("\r");
}

static void _write_display(struct e_cli_state *cli) {
    size_t term_rows;
    size_t term_cols = 0;
    size_t used_rows;
    size_t prompt_len = (NULL == cli->prompt) ? 0 : strlen(cli->prompt);
    _get_terminal_size(&term_cols, &term_rows);
    used_rows = (prompt_len + (*cli->p_line)->len + term_cols - 1) / term_cols;

    printf("%s%s", cli->prompt, (*cli->p_line)->content);
    if (1 < used_rows) {
        printf("\033[%ldA\r", used_rows - 1);
        if (cli->curs->y > 0) printf("\033[%ldB", cli->curs->y);
        if (cli->curs->x > 0) printf("\033[%ldC", cli->curs->x);
    }
    else if (cli->curs->x > 0) printf("\r\033[%ldC", cli->curs->x);
}

static e_stat_code _handle_display(struct e_cli_state *cli, char *c) {
    e_stat_code status = E_CONTINUE;
    size_t term_cols = 1;
    int is_enter = (*c == NEWLINE_KEY || *c == CARR_RET_KEY);
    
    _get_terminal_size(&term_cols, NULL);

    if (!_is_cli_state_valid(cli)) return E_EXIT;
    if (!is_enter) _clean_display(cli);

    switch (*c) {
    case NEWLINE_KEY:
    case CARR_RET_KEY:
        status = E_SEND_COMMAND;
        _enter(cli, *c);
        break;
    case BACKSPACE_KEY:
    case CTRL_H:
        _backspace(cli);
        break;
    case ESC_KEY:
        if (read(STDIN_FILENO, c, 1) <= 0) break;
        if (read(STDIN_FILENO, c, 1) <= 0) break;
        switch(*c) {
        case ARROW_UP_KEY:      _up_arrow(cli); break;
        case ARROW_DOWN_KEY:    _down_arrow(cli); break;
        case ARROW_RIGHT_KEY:   _right_arrow(cli); break;
        case ARROW_LEFT_KEY:    _left_arrow(cli); break;
        case CANC_KEY:
            if (read(STDIN_FILENO, c, 1) <= 0) break;  /* removes undesired tilde */
            _canc(cli);
            break;
        default: break;
        }
        break;
    case CTRL_A:
        cli->curs->y = 0;
        cli->curs->x = strlen(cli->prompt);
        break;
    case CTRL_B:                _left_arrow(cli); break;
    case CTRL_D:                _canc(cli); break;
    case CTRL_E:
        cli->curs->y = ((*cli->p_line)->len + strlen(cli->prompt)) / term_cols;
        cli->curs->x = ((*cli->p_line)->len + strlen(cli->prompt)) % term_cols;
        break;
    case CTRL_F:                _right_arrow(cli); break;
    case CTRL_K:                break;  /* delete to the end of the line */
    case CTRL_L:                _clear_terminal_screen(); break;
    case CTRL_N:                _down_arrow(cli); break;
    case CTRL_P:                _up_arrow(cli); break;
    case CTRL_T:                break;  /* swap the 2 char before the cursor */
    case CTRL_U:                break;  /* delete to the beginning of the line */
    case CTRL_W:                break;  /* delete to the beginning of the current word */
    default:                    _literal(cli, c); break;
    }

    if (!is_enter) _write_display(cli);
    fflush(stdout);
    return status;
}

e_stat_code easycli(struct e_cli_state *cli) {
    fd_set readfds;
    e_stat_code retval = E_CONTINUE;
    int ret;
    char c;

    if (NULL == cli || NULL == cli->p_line || NULL == *(cli->p_line)) {
        retval = E_EXIT;
        goto exit;
    }

    /* print prompt in the first input line */
    if (0 == (*cli->p_line)->len && NULL != cli->prompt) {
        printf("\r%s", cli->prompt);  /* \r overwrites prompt if prompt alredy written */
        cli->curs->x = strlen(cli->prompt);
        fflush(stdout);
    }

    if (!_is_raw_mode_enabled()) {
        _enable_raw_mode();
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
    }
    else if (read(STDIN_FILENO, &c, 1) > 0) retval = _handle_display(cli, &c);

exit:
    if (E_EXIT == retval) _restore_terminal_mode();
    return retval;
}

void run_easycli_ctx(
    const char *prompt,
    size_t max_str_len,
    void *ctx,
    e_stat_code (*callback_on_enter)(char *dest, void *ctx)
) {
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
        code = easycli(&cli);
        if (E_SEND_COMMAND == code) {
            if (NULL != callback_on_enter) code = callback_on_enter(line->content, ctx);
            e_clean_line(line);
            _reset_cursor(&curs);
        }
    }
    while (E_EXIT != code);

    e_free_line(line);
    e_free_history(history);
}