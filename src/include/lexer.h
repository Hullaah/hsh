#ifndef LEXER_H
#define LEXER_H

#include "shell.h"
#include "token.h"

typedef struct Lexer {
	const char *source;
	size_t start;
	size_t cursor;
	Token *tokens;
	Token *last;
	ShellState *shell;
} Lexer;

Token *tokenize(ShellState *shell, const char *input);

#endif
