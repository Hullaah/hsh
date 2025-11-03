#include "lexer.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Lexer {
        const char *source;
        size_t start;
        size_t cursor;
        Token *tokens_tree;
        Token *tail_token;
        ShellState *shell;
} Lexer;

static bool lexer_at_end(Lexer *lex) {
        return lex->source[lex->cursor] == '\0';
}

static char lexer_advance(Lexer *lex) { return lex->source[lex->cursor++]; }

static char lexer_peek(Lexer *lex) {
        return lexer_at_end(lex) ? '\0' : lex->source[lex->cursor];
}

static void lexer_skip_blanks(Lexer *lex) {
        for (;;) {
                char c = lexer_peek(lex);
                switch (c) {
                case ' ':
                case '\r':
                case '\t':
                        lexer_advance(lex);
                        break;
                default:
                        return;
                }
        }
}

static bool lexer_match(Lexer *lex, char expected) {
        if (lexer_at_end(lex) || lex->source[lex->cursor] != expected)
                return false;
        lex->cursor++;
        return true;
}

static void lexer_append_token(Lexer *lex, TokenType type, char *lexeme) {
        Token *token = malloc(sizeof(Token));
        if (!token) {
                fprintf(stderr, "Error: malloc failed\n");
                lex->shell->fatal_error = true;
                return;
        }
        token->type = type;
        token->lexeme = lexeme;
        token->next = NULL;

        if (lex->tail_token == NULL) {
                lex->tokens_tree = token;
                lex->tail_token = token;
        } else {
                lex->tail_token->next = token;
                lex->tail_token = token;
        }
}

static bool is_valid_identifier(char *str, size_t length) {
        if (!(str[0] == '_' || (str[0] >= 'a' && str[0] <= 'z') ||
              (str[0] >= 'A' && str[0] <= 'Z')))
                return false;

        for (size_t i = 1; i < length; i++) {
                if (!(str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') ||
                      (str[i] >= 'A' && str[i] <= 'Z') ||
                      (str[i] >= '0' && str[i] <= '9')))
                        return false;
        }
        return true;
}

static bool is_word_delimiter(char c) {
        return strchr(" \r\t\n;|&<>#", c) != NULL;
}

static void append_substr(char **string, size_t *length, size_t *capacity,
                          const char *format, ...) {
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
                        return;
                }
        }

        va_start(args, format);
        vsnprintf(*string + *length, needed + 1, format, args);
        va_end(args);
        *length += needed;
}

static void lexer_handle_word(Lexer *lex) {
        char *string = strdup("");
        size_t length = 0;
        size_t capacity = 0;
        bool has_quotes_before_equal = false, found_equals = false;

        if (!string) {
                fprintf(stderr, "Error: malloc failed\n");
                lex->shell->fatal_error = true;
                return;
        }

        while (!lexer_at_end(lex) && !is_word_delimiter(lexer_peek(lex))) {
                if (lexer_peek(lex) == '\'' || lexer_peek(lex) == '"') {
                        lex->start = lex->cursor;
                        char quote = lexer_advance(lex);
                        while (!lexer_at_end(lex) && lexer_peek(lex) != quote)
                                lexer_advance(lex);

                        if (lexer_at_end(lex)) {
                                fprintf(stderr,
                                        "Error: Unterminated string.\n");
                                lex->shell->had_error = true;
                                free(string);
                                return;
                        }

                        if (!found_equals)
                                has_quotes_before_equal = true;
                        lexer_advance(lex);

                        size_t str_length = lex->cursor - lex->start - 2;
                        char *substr = malloc(str_length + 1);
                        if (!substr) {
                                fprintf(stderr, "Error: malloc failed\n");
                                lex->shell->fatal_error = true;
                                free(string);
                                return;
                        }

                        strncpy(substr, &lex->source[lex->start + 1],
                                str_length);
                        substr[str_length] = '\0';
                        append_substr(&string, &length, &capacity, "%s",
                                      substr);
                        free(substr);
                } else {
                        if (lexer_peek(lex) == '=' && !found_equals)
                                found_equals = true;
                        append_substr(&string, &length, &capacity, "%c",
                                      lexer_advance(lex));
                }
        }

        if (strcmp(string, "") != 0) {
                size_t equ_pos = strcspn(string, "=");
                if (strchr(string, '=') && !has_quotes_before_equal &&
                    equ_pos > 0 && is_valid_identifier(string, equ_pos)) {
                        lexer_append_token(lex, TOKEN_ASSIGNMENT_WORD, string);
                        return;
                }
                lexer_append_token(lex, TOKEN_WORD, string);
        } else {
                free(string);
        }
}

static void lexer_scan_token(Lexer *lex) {
        char c = lexer_peek(lex);

        switch (c) {
        case ';':
                lexer_advance(lex);
                lexer_append_token(lex, TOKEN_SEMICOLON, ";");
                break;
        case '<':
                lexer_advance(lex);
                lexer_append_token(lex, TOKEN_REDIRECT_IN, "<");
                break;
        case '>':
                lexer_advance(lex);
                if (lexer_match(lex, '>'))
                        lexer_append_token(lex, TOKEN_REDIRECT_APPEND, ">>");
                else
                        lexer_append_token(lex, TOKEN_REDIRECT_OUT, ">");
                break;
        case '&':
                lexer_advance(lex);
                if (lexer_match(lex, '&'))
                        lexer_append_token(lex, TOKEN_AND, "&&");
                else
                        lexer_append_token(lex, TOKEN_BACKGROUND, "&");
                break;
        case '|':
                lexer_advance(lex);
                if (lexer_match(lex, '|'))
                        lexer_append_token(lex, TOKEN_OR, "||");
                else
                        lexer_append_token(lex, TOKEN_PIPE, "|");
                break;
        case '\n':
                lexer_advance(lex);
                lexer_append_token(lex, TOKEN_EOL, "\n");
                break;
        case '#':
                lexer_advance(lex);
                while (lexer_peek(lex) != '\n' && !lexer_at_end(lex))
                        lexer_advance(lex);
                break;
        default:
                lexer_handle_word(lex);
                break;
        }
}

Token *tokenize(ShellState *shell, const char *input) {
        Lexer lex = {.source = input,
                     .start = 0,
                     .cursor = 0,
                     .tokens_tree = NULL,
                     .tail_token = NULL,
                     .shell = shell};

        while (!lexer_at_end(&lex)) {
                lexer_skip_blanks(&lex);
                lex.start = lex.cursor;
                lexer_scan_token(&lex);
        }

        return lex.tokens_tree;
}
