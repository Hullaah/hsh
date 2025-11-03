#ifndef TOKEN_H
#define TOKEN_H

#include "shell.h"

typedef enum TokenType {
        TOKEN_WORD,
        TOKEN_ASSIGNMENT_WORD,
        TOKEN_PIPE,
        TOKEN_AND,
        TOKEN_OR,
        TOKEN_SEMICOLON,
        TOKEN_BACKGROUND,
        TOKEN_REDIRECT_IN,
        TOKEN_REDIRECT_OUT,
        TOKEN_REDIRECT_APPEND,
        TOKEN_EOL,
} TokenType;

typedef struct Token {
        TokenType type;
        char *lexeme;
        struct Token *next;
} Token;

void token_free_list(Token *head);
Token **token_split_by_semicolon(ShellState *shell, Token *tokens);

#endif
