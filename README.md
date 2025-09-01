## What is nanocli?
nanocli is a free, open-source, self-contained and lightweight replacement for GNU Readline, offering the following features:

- Support for multiline input
- Support for input history
- Support for CTRL+KEY shortcuts
- Zero external dependencies
- (~800) lines of code in a single '.c' file

nanocli is currently available only for Unix-like operating systems conforming to the POSIX standard.

## How to use it
The nanocli API consists of three functions: one to retrieve a line of input when enter key is pressed, one to prompt for specific information, and one to safely print formatted content.
The ```example.c``` file should give you enough info to use the library. The following is an explanation of each function.

```c
char *nanocli(const char *prompt, size_t max_str_len);  /* excluding NULL terminator */
```
The ```char *nanocli(...)``` function is the entrypoint of the library. It retrieves the inserted line on enter key press.
The following is an usage example for the ```char *nanocli(...)``` function.

```c
int main(void) {
    char *line;
    while (NULL == (line = nanocli(ncli_DEFAULT_PROMPT, ncli_DEFAULT_MAX_INPUT_LEN))) {
        ...
        free(line);
    }
    return 0;
}
```
---
```c
char *nanocli_ask(const char *question, const size_t max_len, const int masked);  /* excluding NULL terminator */
```
The ```char *nanocli_ask(...)``` function allows the developer to gather specific information from the user.
You can think of it like how the ```sudo``` command asks for a password. This function is used to replicate that
behaviour.
The following is an usage example for the ```char *nanocli_ask(...)``` function.

```c
char *username;
char *password;
...
if (0 == strcmp(line, "login")) {
    username = nanocli_ask("username: ", ncli_DEFAULT_MAX_INPUT_LEN, 0);
    password = nanocli_ask("password: ", ncli_DEFAULT_MAX_INPUT_LEN, 1);
    /* ====== run logic related to username and password ====== */
    free(username);
    free(password);
}
```
---
```c
void nanocli_echo(const char *str);
```
The ```void nanocli_echo(...)``` function is a simple wrapper around the POSIX write syscall. It ensures that the output is properly formatted.
