#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

typedef struct ShellState {
        bool fatal_error;
        bool is_interactive_mode;
        bool had_error;
        char *name;
        int line_number;
} ShellState;

typedef enum {
        CMD_SIMPLE,
        CMD_PIPE,
        CMD_AND,
        CMD_OR,
        CMD_SEPARATOR,
} CommandType;

typedef struct SimpleCommand {
        int argc;
        char **argv;
        char **envp;
        char *input_file;
        char *output_file;
        bool append_output;
} SimpleCommand;

typedef struct Command {
        CommandType type;
        bool is_background;
        union {
                SimpleCommand command;
                struct {
                        struct Command *left;
                        struct Command *right;
                } binary;
        } as;
} Command;

static void free_tokens(Token *head);
static void free_command(Command *command);
static int execute_command(Command *command);
static Command *parse(Token *tokens);
static void repl(FILE *stream);

static Token *current_token, *prev_token, *tokens_tree, *tail_token;
static size_t start, cursor;
static const char *source;
ShellState *shell;

int main(int argc, char **argv) {
        shell = malloc(sizeof(ShellState));
        if (!shell) {
                fprintf(stderr, "Error: malloc failed\n");
                return (127);
        }

        shell->fatal_error = false;
        shell->had_error = false;
        shell->is_interactive_mode = true;
        shell->line_number = 0;
        shell->name = argc == 1 ? argv[0] : argv[1];

        if (argc > 2) {
                fprintf(stderr, "Usage: %s [filename]", argv[0]);
                return (127);
        } else if (argc == 2) {
                FILE *stream = fopen(argv[1], "r");
                shell->is_interactive_mode = false;
                if (!stream) {
                        fprintf(stderr, "Error: cannot open file %s\n",
                                argv[1]);
                        return (127);
                }
                repl(stream);
                fclose(stream);
        } else if (!isatty(STDIN_FILENO)) {
                shell->is_interactive_mode = false;
                repl(stdin);
        } else {
                repl(stdin);
        }
        if (shell->fatal_error)
                return (2);
        free(shell);
        return (0);
}

static Token *previous_token(void) { return prev_token; }

static Token *peek_token(void) { return current_token; }

static bool is_eol_token(void) { return peek_token()->type == TOKEN_EOL; }

void free_tokens(Token *head) {
        Token *current = head;
        Token *next;

        while (current != NULL) {
                next = current->next;
                if (current->type == TOKEN_WORD ||
                    current->type == TOKEN_ASSIGNMENT_WORD)
                        free(current->lexeme);
                free(current);
                current = next;
        }
}

void free_command(Command *command) {
        if (command == NULL)
                return;

        if (command->type == CMD_SIMPLE) {
                free(command->as.command.argv);
                free(command->as.command.envp);
        } else {
                free_command(command->as.binary.left);
                free_command(command->as.binary.right);
        }
        free(command);
}

int execute_command(Command *command) {
        (void)command;
        // Execution logic to be implemented
        return 0;
}

static Token *advance_token(void) {
        Token *token = peek_token();
        if (!is_eol_token())
                current_token = current_token->next;
        return token;
}

static bool token_matches(int n, ...) {
        if (is_eol_token())
                return false;
        va_list args;
        va_start(args, n);
        for (int i = 0; i < n; i++) {
                TokenType type = va_arg(args, TokenType);
                if (peek_token()->type == type) {
                        prev_token = advance_token();
                        va_end(args);
                        return true;
                }
        }
        va_end(args);
        return false;
}

static Command *simple_command(void) {
        SimpleCommand *simple = malloc(sizeof(SimpleCommand));
        int envc = 0, capacity = 2;

        if (!simple) {
                shell->fatal_error = true;
                return NULL;
        }
        simple->argc = 0;
        simple->argv = malloc(sizeof(char *) * capacity);
        simple->envp = malloc(sizeof(char *) * capacity);
        simple->input_file = NULL;
        simple->output_file = NULL;
        simple->append_output = false;
        if (!simple->argv || !simple->envp) {
                shell->fatal_error = true;
                free(simple->argv);
                free(simple->envp);
                free(simple);
                return NULL;
        }
        while (token_matches(1, TOKEN_ASSIGNMENT_WORD)) {
                if (envc + 1 >= capacity) {
                        capacity *= 2;
                        char **new_envp =
                            realloc(simple->envp, capacity * sizeof(char *));
                        if (!new_envp) {
                                shell->fatal_error = true;
                                free(simple->envp);
                                free(simple->argv);
                                free(simple);
                                return NULL;
                        }
                        simple->envp = new_envp;
                }
                simple->envp[envc++] = previous_token()->lexeme;
        }
        simple->envp[envc] = NULL;
        capacity = 2;
        if (token_matches(1, TOKEN_WORD)) {
                if (simple->argc + 1 >= capacity) {
                        capacity *= 2;
                        char **new_argv =
                            realloc(simple->argv, sizeof(char *) * capacity);
                        if (!new_argv) {
                                shell->fatal_error = true;
                                free(simple->argv);
                                free(simple->envp);
                                free(simple);
                                return NULL;
                        }
                        simple->argv = new_argv;
                        simple->argv[simple->argc++] = previous_token()->lexeme;
                }
                simple->argv[simple->argc++] = previous_token()->lexeme;
                simple->argv[simple->argc] = NULL;
        } else if (is_eol_token()) {
                free(simple->argv);
                free(simple->envp);
                free(simple);
                return NULL;
        } else {
                shell->had_error = true;
                printf("%s: %d: Syntax error: \"%s\" unexpected\n", shell->name,
                       shell->line_number,
                       (previous_token() ? previous_token() : peek_token())
                           ->lexeme);
                free(simple->argv);
                free(simple->envp);
                free(simple);
                return NULL;
        }
        while (true) {
                if (token_matches(2, TOKEN_WORD, TOKEN_ASSIGNMENT_WORD)) {
                        if (simple->argc + 1 >= capacity) {
                                capacity *= 2;
                                char **new_argv = realloc(
                                    simple->argv, sizeof(char *) * capacity);
                                if (!new_argv) {
                                        shell->fatal_error = true;
                                        free(simple->argv);
                                        free(simple->envp);
                                        free(simple);
                                        return NULL;
                                }
                                simple->argv = new_argv;
                        }
                        simple->argv[simple->argc++] = previous_token()->lexeme;
                        simple->argv[simple->argc] = NULL;

                } else if (token_matches(3, TOKEN_REDIRECT_IN,
                                         TOKEN_REDIRECT_OUT,
                                         TOKEN_REDIRECT_APPEND)) {
                        Token *op = previous_token();

                        if (!token_matches(1, TOKEN_WORD)) {
                                fprintf(stderr,
                                        "%s: %d: Syntax error: "
                                        "expected filename after '%s'\n",
                                        shell->name, shell->line_number,
                                        op->lexeme);
                                free(simple->argv);
                                free(simple->envp);
                                free(simple);
                                return NULL;
                        }

                        Token *filename = previous_token();

                        switch (op->type) {
                        case TOKEN_REDIRECT_IN:
                                simple->input_file = filename->lexeme;
                                break;
                        case TOKEN_REDIRECT_OUT:
                                simple->output_file = filename->lexeme;
                                simple->append_output = false;
                                break;
                        case TOKEN_REDIRECT_APPEND:
                                simple->output_file = filename->lexeme;
                                simple->append_output = true;
                                break;
                        default:
                                break;
                        }
                } else {
                        break;
                }
        }
        Command *cmd = malloc(sizeof(Command));
        if (!cmd) {
                shell->fatal_error = true;
                free(simple->argv);
                free(simple->envp);
                free(simple);
                return NULL;
        }
        cmd->type = CMD_SIMPLE;
        cmd->as.command.argc = simple->argc;
        cmd->as.command.argv = simple->argv;
        cmd->as.command.envp = simple->envp;
        cmd->as.command.input_file = simple->input_file;
        cmd->as.command.output_file = simple->output_file;
        cmd->as.command.append_output = simple->append_output;
        cmd->is_background = false;
        free(simple);
        return cmd;
}

static Command *pipeline(void) {
        Command *cmd = simple_command();
        if (cmd)
                cmd->is_background = false;
        else if (shell->had_error)
                return NULL;
        while (token_matches(1, TOKEN_PIPE)) {
                if (is_eol_token()) {
                        shell->had_error = true;
                        printf("%s: %d: Syntax error: end of line unexpected\n",
                               shell->name, shell->line_number);
                        free_command(cmd);
                        return NULL;
                }
                Command *right = simple_command();
                if (!right) {
                        free_command(cmd);
                        return NULL;
                }
                Command *parent = malloc(sizeof(Command));
                if (!parent) {
                        free_command(cmd);
                        free_command(right);
                        return NULL;
                }
                parent->as.binary.left = cmd;
                parent->type = CMD_PIPE;
                parent->as.binary.right = right;
                cmd = parent;
                cmd->is_background = false;
        }
        return cmd;
}

static Command *logical_list(void) {
        Command *cmd = pipeline();
        if (cmd)
                cmd->is_background = false;
        else if (shell->had_error)
                return NULL;
        while (token_matches(2, TOKEN_AND, TOKEN_OR)) {
                if (is_eol_token()) {
                        shell->had_error = true;
                        printf("%s: %d: Syntax error: end of line unexpected\n",
                               shell->name, shell->line_number);
                        free_command(cmd);
                        return NULL;
                }
                CommandType parent_type =
                    previous_token()->type == TOKEN_AND ? CMD_AND : CMD_OR;
                Command *right = pipeline();
                if (!right) {
                        free_command(cmd);
                        return NULL;
                }
                Command *parent = malloc(sizeof(Command));
                parent->type = parent_type;
                if (!parent) {
                        free_command(cmd);
                        free_command(right);
                        return NULL;
                }
                parent->as.binary.left = cmd;
                parent->as.binary.right = right;
                cmd = parent;
                cmd->is_background = false;
        }
        return cmd;
}

static Command *create_command(void) {
        Command *cmd = logical_list();
        if (shell->had_error) {
                free_command(cmd);
                return NULL;
        }
        while (token_matches(1, TOKEN_BACKGROUND)) {
                cmd->is_background = true;
                if (shell->had_error) {
                        free_command(cmd);
                        return NULL;
                }
                if (is_eol_token())
                        return cmd;

                Command *right = logical_list();
                if (shell->had_error || !right) {
                        free_command(cmd);
                        free_command(right);
                        return NULL;
                }
                Command *parent = malloc(sizeof(Command));
                if (shell->had_error || !parent) {
                        free_command(parent);
                        free_command(cmd);
                        free_command(right);
                        return NULL;
                }
                parent->as.binary.left = cmd;
                parent->type = CMD_SEPARATOR;
                parent->as.binary.right = right;
                cmd = parent;
        }
        return cmd;
}

/**
 * parse - parses the tokens into a command structure
 * @tokens: the list of tokens
 * Return: pointer to the command structure or NULL on failure
 */
Command *parse(Token *tokens) {
        current_token = tokens;
        prev_token = NULL;
        return create_command();
}

/**
 * isAtEnd - checks if we reached the end of the source
 * Return: true if we reached the end, false otherwise
 */
static bool source_at_end(void) { return source[cursor] == '\0'; }

/**
 * advance - advances the current position and returns the current character
 * Return: the current character
 */
static char advance_source(void) { return source[cursor++]; }

/**
 * peek - returns the current character without advancing
 * Return: the current character
 */
static char peek_source(void) {
        return source_at_end() ? '\0' : source[cursor];
}

/**
 * skipBlanks - skips whitespace characters
 */
static void source_skip_blanks(void) {
        for (;;) {
                char c = peek_source();
                switch (c) {
                case ' ':
                case '\r':
                case '\t':
                        advance_source();
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
bool char_matches(char expected) {
        if (source_at_end() || source[cursor] != expected)
                return (false);
        cursor++;
        return (true);
}

/**
 * addToken - adds a token to the token list
 * @type: the type of the token
 * @lexeme: the lexeme of the token
 */
static void append_token(TokenType type, char *lexeme) {
        Token *token = malloc(sizeof(Token));
        if (!token) {
                fprintf(stderr, "Error: malloc failed\n");
                shell->fatal_error = true;
        }
        token->type = type;
        token->lexeme = lexeme;
        token->next = NULL;
        if (tail_token == NULL) {
                tokens_tree = token;
                tail_token = token;
        } else {
                tail_token->next = token;
                tail_token = token;
        }
}

/**
 * isValidIdentifier - checks if a string is a valid identifier
 * @str: the string to check
 * @length: the length of the string
 * Return: true if valid, false otherwise
 */
static bool isValidIdentifier(char *str, size_t length) {
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
static bool word_is_delimeter(char c) {
        return strchr(" \r\t\n;|&<>#", c) != NULL;
}

/**
 * append - appends formatted string to a dynamic string
 * @string: pointer to the dynamic string
 * @length: pointer to the current length of the string
 * @capacity: pointer to the current capacity of the string
 * @format: the format string
 */
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
                        shell->fatal_error = true;
                }
        }
        va_start(args, format);
        vsnprintf(*string + *length, needed + 1, format, args);
        va_end(args);
        *length += needed;
}

/**
 * handle_word- handles a word token
 */
static void handle_word(void) {
        char *string = strdup("");
        size_t length = 0;
        size_t capacity = 0;
        bool has_quotes_before_equal = false, found_equals = false;

        if (!string) {
                fprintf(stderr, "Error: malloc failed\n");
                shell->fatal_error = true;
                return;
        }
        while (!source_at_end() && !word_is_delimeter(peek_source())) {
                if (peek_source() == '\'' || peek_source() == '"') {
                        start = cursor;
                        char quote = advance_source();
                        while (!source_at_end() && peek_source() != quote)
                                advance_source();
                        if (source_at_end()) {
                                fprintf(stderr,
                                        "Error: Unterminated string.\n");
                                shell->had_error = true;
                                return;
                        }
                        if (!found_equals)
                                has_quotes_before_equal = true;
                        advance_source();

                        size_t str_length = cursor - start - 2;
                        char *substr = malloc(str_length + 1);
                        if (!substr) {
                                fprintf(stderr, "Error: malloc failed\n");
                                shell->fatal_error = true;
                                return;
                        }

                        substr =
                            strncpy(substr, &source[start + 1], str_length);
                        substr[str_length] = '\0';
                        append_substr(&string, &length, &capacity, "%s",
                                      substr);
                        free(substr);

                } else {
                        if (peek_source() == '=' && !found_equals)
                                found_equals = true;
                        append_substr(&string, &length, &capacity, "%c",
                                      advance_source());
                }
        }

        if (strcmp(string, "") != 0) {
                size_t equ_pos = strcspn(string, "=");
                if (strchr(string, '=') && !has_quotes_before_equal &&
                    equ_pos > 0 && isValidIdentifier(string, equ_pos)) {
                        append_token(TOKEN_ASSIGNMENT_WORD, string);
                        return;
                }
                append_token(TOKEN_WORD, string);
        } else {
                free(string);
        }
}

/**
 * scanToken - scans a single token from the source
 */
static void scan_token(void) {
        char c = peek_source();

        switch (c) {
        case ';':
                advance_source();
                append_token(TOKEN_SEMICOLON, ";");
                break;
        case '<':
                advance_source();
                append_token(TOKEN_REDIRECT_IN, "<");
                break;
        case '>':
                advance_source();
                if (char_matches('>'))
                        append_token(TOKEN_REDIRECT_APPEND, ">>");
                else
                        append_token(TOKEN_REDIRECT_OUT, ">");
                break;
        case '&':
                advance_source();
                if (char_matches('&'))
                        append_token(TOKEN_AND, "&&");
                else
                        append_token(TOKEN_BACKGROUND, "&");
                break;
        case '|':
                advance_source();
                if (char_matches('|'))
                        append_token(TOKEN_OR, "||");
                else
                        append_token(TOKEN_PIPE, "|");
                break;
        case '\n':
                advance_source();
                append_token(TOKEN_EOL, "\n");
                break;
        case '#':
                advance_source();
                while (peek_source() != '\n' && !is_eol_token())
                        advance_source();
                break;
        default:
                handle_word();
                break;
        }
}

/**
 * scanTokens - scans all tokens from the source
 * Return: the head of the token list
 */
static Token *scan_tokens(void) {
        while (!source_at_end()) {
                source_skip_blanks();
                start = cursor;
                scan_token();
        }
        return tokens_tree;
}

/**
 * tokenize - tokenizes the input string
 * @input: the input string
 * Return: pointer to the head of the token list
 */
Token *tokenize(const char *input) {
        source = input;
        tokens_tree = tail_token = NULL;
        start = cursor = 0;
        return scan_tokens();
}

static Token **rip_off_semicolons(Token *tokens) {
        Token **commands;
        Token *current = tokens, *prev = NULL, *next = NULL, *start = tokens,
              *node = malloc(sizeof(Token));
        size_t count = 0, index = 0;

        if (!node) {
                shell->fatal_error = true;
                return NULL;
        }
        count++;
        while (current) {
                if (current->type == TOKEN_SEMICOLON)
                        count++;
                current = current->next;
        }
        commands = malloc(sizeof(Token *) * (count + 1));
        if (!commands) {
                shell->fatal_error = true;
                free(node);
                return NULL;
        }

        current = tokens;
        start = current;
        while (current) {
                next = current->next;
                if (current->type == TOKEN_SEMICOLON) {
                        node->type = TOKEN_EOL;
                        node->lexeme = "\n";
                        node->next = NULL;
                        if (!prev) {
                                commands[index++] = node;
                        } else {
                                prev->next = node;
                                commands[index++] = start;
                        }
                        start = next;
                        node = malloc(sizeof(Token));
                        if (!node) {
                                shell->fatal_error = true;
                                commands[index] = NULL;
                                return commands;
                        }
                }
                prev = current->type == TOKEN_SEMICOLON ? NULL : current;
                if (current->type == TOKEN_SEMICOLON)
                        free(current);
                current = next;
        }
        node->type = TOKEN_EOL;
        node->lexeme = "\n";
        node->next = NULL;
        if (!prev) {
                commands[index++] = node;
        } else {
                prev->next = node;
                commands[index++] = start;
        }
        commands[index] = NULL;
        return commands;
}

static void repl(FILE *stream) {
        char *line = NULL;
        size_t n = 0;
        ssize_t nread = 0;

        while (true) {
        start:
                shell->line_number++;
                if (shell->is_interactive_mode)
                        fprintf(stdout, "$ ");
                nread = getline(&line, &n, stream);
                if (nread < 0) {
                        free(line);
                        break;
                }
                Token *t = tokenize(line);

#ifdef DEBUG
                static char *token_string[] = {
                    [TOKEN_ASSIGNMENT_WORD] = "TOKEN_ASSIGNMENT_WORD",
                    [TOKEN_WORD] = "TOKEN_WORD",
                    [TOKEN_SEMICOLON] = "TOKEN_SEMICOLON",
                    [TOKEN_REDIRECT_IN] = "TOKEN_REDIRECT_IN",
                    [TOKEN_REDIRECT_OUT] = "TOKEN_REDIRECT_OUT",
                    [TOKEN_REDIRECT_APPEND] = "TOKEN_REDIRECT_APPEND",
                    [TOKEN_BACKGROUND] = "TOKEN_BACKGROUND",
                    [TOKEN_PIPE] = "TOKEN_PIPE",
                    [TOKEN_AND] = "TOKEN_AND",
                    [TOKEN_OR] = "TOKEN_OR",
                    [TOKEN_EOL] = "TOKEN_EOL"};
                Token *current = t;
                while (current != NULL) {
                        // For debugging: print token types and lexemes
                        fprintf(stdout, "[%s: '%s']\n",
                                token_string[current->type], current->lexeme);
                        current = current->next;
                }
#endif
                free(line);
                line = NULL;
                if (shell->fatal_error) {
                        free_tokens(t);
                        return;
                } else if (shell->had_error) {
                        free_tokens(t);
                        shell->had_error = false;
                        continue;
                }
                Token **tokens = rip_off_semicolons(t);
                if (shell->fatal_error) {
                        Token **runner = tokens;
                        while (*runner) {
                                free_tokens(*runner);
                                runner++;
                        }
                        free(tokens);
                        return;
                }
                Token **ptr = tokens;
                for (; *ptr; ptr++) {
                        Command *command = parse(*ptr);
                        execute_command(command);
                        free_command(command);
                        if (shell->fatal_error) {
                                while (*ptr) {
                                        free_tokens(*ptr);
                                        ptr++;
                                }
                                free(tokens);
                                return;
                        } else if (shell->had_error) {
                                shell->had_error = false;
                                while (*ptr) {
                                        free_tokens(*ptr);
                                        ptr++;
                                }
                                free(tokens);
                                if (shell->is_interactive_mode)
                                        goto start;
                                return;
                        }
                        free_tokens(*ptr);
                }
                free(tokens);
        }
        if (shell->is_interactive_mode)
                putchar('\n');
}
