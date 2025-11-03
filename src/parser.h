#ifndef PARSER_H
#define PARSER_H

#include "command.h"
#include "shell.h"
#include "token.h"

Command *parse(ShellState *shell, Token *tokens);

#endif
