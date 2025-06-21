## What is easycli?
Easycli is a free, open-source, self-contained and lightweight replacement for GNU Readline, offering the following features:

- Support for multiline input
- Support for input history
- Support for CTRL+KEY shortcuts
- Zero external dependencies
- (~800) lines of code in a single '.c' file

## How to use it
The Easycli API consists of three functions: one to retrieve a line of input when enter key is pressed, one to prompt for specific information, and one to safely print formatted content.
The ```example.c``` file should give you enough info to use the library. The following is an explanation of each function.

```c
char *easycli(const char *prompt, size_t max_str_len);  /* excluding NULL terminator */
```
The ```char *easycli(...)``` function is the entrypoint of the library. It retrieves the inserted line on enter key press.
The following is an usage example for the ```char *easycli(...)``` function.

```c
int main(void) {
    char *line;
    while (NULL == (line = easycli(E_DEFAULT_PROMPT, E_DEFAULT_MAX_INPUT_LEN))) {
        ...
        free(line);
    }
    return 0;
}
```
---
```c
char *easy_ask(const char *question, const size_t max_len, const int masked);  /* excluding NULL terminator */
```
The ```char *easy_ask(...)``` function allows the developer to gather specific information from the user.
You can think of it like how the ```sudo``` command asks for a password. This function is used to replicate that
behaviour.
The following is an usage example for the ```char *easy_ask(...)``` function.

```c
char *username;
char *password;
...
if (0 == strcmp(line, "login")) {
    username = easy_ask("username: ", E_DEFAULT_MAX_INPUT_LEN, 0);
    password = easy_ask("password: ", E_DEFAULT_MAX_INPUT_LEN, 1);
    /* ====== run logic related to username and password ====== */
    free(username);
    free(password);
}
```
---
```c
void easy_print(const char *str);
```
The ```void easy_print(...)``` function is a simple wrapper around the POSIX write syscall. It ensures that the output is properly formatted.
