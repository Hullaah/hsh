#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>
#include <stdio.h>

typedef struct ShellState {
        bool fatal_error;
        bool is_interactive_mode;
        bool had_error;
        char *name;
        int line_number;
} ShellState;

ShellState *shell_init(char *name, bool is_interactive);
void shell_free(ShellState *shell);
void shell_repl(ShellState *shell, FILE *stream);

#endif
