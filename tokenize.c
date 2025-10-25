#include <utils.h>
#include <stdbool.h>
#include <stddef.h>
#include <tokenizer.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <shell.h>

static size_t start, current;
static const char *source;
static Token *tokens, *tail;

static bool isAtEnd(void)
{
	return source[current] == '\0';
}

static char advance(void)
{
	return source[current++];
}

static char peek(void)
{
	return isAtEnd() ? '\0' : source[current];
}

static void skipBlanks(void)
{
	for (;;) {
		char c = peek();
		switch (c) {
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		default:
			return;
		}
	}
}

bool match(char expected)
{
	if (isAtEnd() || source[current] != expected)
		return (false);
	current++;
	return (true);
}

static void addToken(TokenType type, char *lexeme)
{
	Token *token = malloc(sizeof(Token));
	if (!token) {
		fdprint(STDERR_FILENO, "Error: malloc failed\n");
		shell->fatal_error = true;
	}
	token->type = type;
	token->lexeme = lexeme;
	token->next = NULL;
	if (tail == NULL) {
		tokens = token;
		tail = token;
	} else {
		tail->next = token;
		tail = token;
	}
}

static void handleWord(void)
{
}

static void scanToken(void)
{
	char c = peek();

	switch (c) {
	case ';':
		advance();
		addToken(TOKEN_SEMICOLON, ";");
		break;
	case '<':
		advance();
		addToken(TOKEN_REDIRECT_IN, "<");
		break;
	case '>':
		advance();
		if (match('>'))
			addToken(TOKEN_REDIRECT_APPEND, ">>");
		else
			addToken(TOKEN_REDIRECT_OUT, ">");
		break;
	case '&':
		advance();
		if (match('&'))
			addToken(TOKEN_AND, "&&");
		else
			addToken(TOKEN_BACKGROUND, "&");
		break;
	case '\n':
		advance();
		addToken(TOKEN_EOL, "\n");
		break;
	case '#':
		advance();
		while (peek() != '\n' && !isAtEnd())
			advance();
		break;
	default:
		handleWord();
		break;
	}
}

static Token *scanTokens(void)
{
	while (!isAtEnd()) {
		skipBlanks();
		start = current;
		scanToken();
	}
	return tokens;
}

Token *tokenize(const char *input)
{
	source = input;
	tokens = tail = NULL;
	start = current = 0;
	return scanTokens();
}
