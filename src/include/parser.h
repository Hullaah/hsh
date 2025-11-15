#ifndef PARSER_H
#define PARSER_H

#include <command.h>
#include <shell.h>
#include <token.h>

typedef struct Parser {
	Token *current;
	Token *prev;
	ShellState *shell;
} Parser;

Command *parse(ShellState *shell, Token *tokens);

#endif
