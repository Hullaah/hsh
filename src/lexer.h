#ifndef LEXER_H
#define LEXER_H

#include "shell.h"
#include "token.h"

Token *tokenize(ShellState *shell, const char *input);

#endif
