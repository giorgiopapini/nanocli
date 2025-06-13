#include "terminal_handler.h"

#include <stdio.h>
#include <stddef.h>
#include <termios.h>  /* never include just <bits/termios-c_cc.h> (for safety reasons). Even if iwyu says otherwise */
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>


static struct termios orig_termios;
static int termios_saved = 0;
static int raw_mode_on = 0;


void enable_raw_mode(struct e_stack_err *errs) {
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

int is_raw_mode_enabled(void) {
    return raw_mode_on;
}

void restore_terminal_mode(void) {
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        raw_mode_on = 0;
    }
}

void get_terminal_size(size_t *cols, size_t *rows, struct e_stack_err *errs) {
    struct winsize w;
    if (-1 == ioctl(STDOUT_FILENO, TIOCGWINSZ, &w))
        e_err_push(errs, e_create_err(TERMINAL_SIZE_ERROR, TERMINAL_SIZE_ERROR_MSG));

    if (NULL != cols) *cols = w.ws_col;
    if (NULL != rows) *rows = w.ws_row;
}