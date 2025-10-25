#ifndef TOKENIZER_H
#define TOKENIZER_H

/**
 * enum TokenType - Types of tokens in the shell
 * @TOKEN_WORD: a word token
 * @TOKEN_ASSIGNMENT_WORD: an assignment word token
 * @TOKEN_PIPE: pipe operator '|'
 * @TOKEN_AND: logical AND operator '&&'
 * @TOKEN_OR: logical OR operator '||'
 * @TOKEN_SEMICOLON: command separator ';'
 * @TOKEN_BACKGROUND: background operator '&'
 * @TOKEN_REDIRECT_IN: input redirection '<'
 * @TOKEN_REDIRECT_OUT: output redirection '>'
 * @TOKEN_REDIRECT_APPEND: output append redirection '>>'
 * @TOKEN_NEWLINE: newline token
 * @TOKEN_EOF: end of file token
 */
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

Token *tokenize(const char *input);

#endif
