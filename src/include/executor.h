#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <command.h>
#include <shell.h>

int execute(ShellState *shell, Command *command);

#endif
