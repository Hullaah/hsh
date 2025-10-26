#include <utils.h>
#include <stdbool.h>
#include <stddef.h>
#include <tokenizer.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <shell.h>
#include <stdio.h>
#include <stdarg.h>

static size_t start, current;
static const char *source;
Token *tokens;
static Token *tail;

/**
 * isAtEnd - checks if we reached the end of the source
 * Return: true if we reached the end, false otherwise
 */
static bool isAtEnd(void)
{
	return source[current] == '\0';
}

/**
 * advance - advances the current position and returns the current character
 * Return: the current character
 */
static char advance(void)
{
	return source[current++];
}

/**
 * peek - returns the current character without advancing
 * Return: the current character
 */
static char peek(void)
{
	return isAtEnd() ? '\0' : source[current];
}

/**
 * skipBlanks - skips whitespace characters
 */
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

/**
 * match - matches the current character with the expected character
 * @expected: the expected character
 * Return: true if matched, false otherwise
 */
bool match(char expected)
{
	if (isAtEnd() || source[current] != expected)
		return (false);
	current++;
	return (true);
}

/**
 * addToken - adds a token to the token list
 * @type: the type of the token
 * @lexeme: the lexeme of the token
 */
static void addToken(TokenType type, char *lexeme)
{
	Token *token = malloc(sizeof(Token));
	if (!token) {
		fprintf(stderr, "Error: malloc failed\n");
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

/**
 * isValidIdentifier - checks if a string is a valid identifier
 * @str: the string to check
 * @length: the length of the string
 * Return: true if valid, false otherwise
 */
static bool isValidIdentifier(char *str, size_t length)
{
	if (!(str[0] == '_' || (str[0] >= 'a' && str[0] <= 'z') ||
	      (str[0] >= 'A' && str[0] <= 'Z')))
		return (false);
	for (size_t i = 1; i < length; i++) {
		if (!(str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') ||
		      (str[i] >= 'A' && str[i] <= 'Z') ||
		      (str[i] >= '0' && str[i] <= '9')))
			return (false);
	}
	return (true);
}

/**
 * isWordDelimeter - checks if a character is a word delimiter
 * @c: the character to check
 * Return: true if it is a delimiter, false otherwise
 */
static bool isWordDelimeter(char c)
{
	return strchr(" \r\t\n;|&<>#", c) != NULL;
}

/**
 * append - appends formatted string to a dynamic string
 * @string: pointer to the dynamic string
 * @length: pointer to the current length of the string
 * @capacity: pointer to the current capacity of the string
 * @format: the format string
 */
static void append(char **string, size_t *length, size_t *capacity,
		   const char *format, ...)
{
	va_list args;
	char buffer[2];
	int needed;

	va_start(args, format);
	needed = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (*length + needed + 1 > *capacity) {
		*capacity = (*length + needed + 1) * 2;
		*string = realloc(*string, *capacity);
		if (!*string) {
			fprintf(stderr, "Error: realloc failed\n");
			shell->fatal_error = true;
		}
	}
	va_start(args, format);
	vsnprintf(*string + *length, needed + 1, format, args);
	va_end(args);
	*length += needed;
}

/**
 * handleWord - handles a word token
 */
static void handleWord(void)
{
	char *string = strdup("");
	size_t length = 0;
	size_t capacity = 0;
	bool has_quotes_before_equal = false, found_equals = false;

	if (!string) {
		fprintf(stderr, "Error: malloc failed\n");
		shell->fatal_error = true;
		return;
	}
	while (!isAtEnd() && !isWordDelimeter(peek())) {
		if (peek() == '\'' || peek() == '"') {
			start = current;
			char quote = advance();
			while (!isAtEnd() && peek() != quote)
				advance();
			if (isAtEnd()) {
				fprintf(stderr,
					"Error: Unterminated string.\n");
				shell->had_error = true;
				return;
			}
			if (!found_equals)
				has_quotes_before_equal = true;
			advance();
			size_t str_length = current - start - 2;
			char *substr = malloc(str_length + 1);
			if (!substr) {
				fprintf(stderr, "Error: malloc failed\n");
				shell->fatal_error = true;
			}
			strncpy(substr, &source[start + 1], str_length);
			substr[str_length] = '\0';
			append(&string, &length, &capacity, "%s", substr);
			free(substr);

		} else {
			if (peek() == '=' && !found_equals)
				found_equals = true;
			append(&string, &length, &capacity, "%c", advance());
		}
	}

	if (strcmp(string, "") != 0) {
		size_t equ_pos = strcspn(string, "=");
		if (strchr(string, '=') && !has_quotes_before_equal &&
		    equ_pos > 0 && isValidIdentifier(string, equ_pos)) {
			addToken(TOKEN_ASSIGNMENT_WORD, string);
			return;
		}
		addToken(TOKEN_WORD, string);
	} else {
		free(string);
	}
}

/**
 * scanToken - scans a single token from the source
 */
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
	case '|':
		advance();
		if (match('|'))
			addToken(TOKEN_OR, "||");
		else
			addToken(TOKEN_PIPE, "|");
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

/** 
 * scanTokens - scans all tokens from the source
 * Return: the head of the token list
 */
static Token *scanTokens(void)
{
	while (!isAtEnd()) {
		skipBlanks();
		start = current;
		scanToken();
	}
	return tokens;
}

/**
 * tokenize - tokenizes the input string
 * @input: the input string
 * Return: pointer to the head of the token list
 */
Token *tokenize(const char *input)
{
	source = input;
	tokens = tail = NULL;
	start = current = 0;
	return scanTokens();
}
