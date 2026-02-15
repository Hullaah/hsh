#ifndef BUILTINS_H
#define BUILTINS_H

#include <shell.h>
#include <executor.h>

int (*get_builtin(char *))(ShellState *, SimpleCommand *, bool);

#endif /* BUILTINS_H */
