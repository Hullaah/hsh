#include "parser.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Parser {
        Token *current;
        Token *prev;
        ShellState *shell;
} Parser;

static Token *parser_peek(Parser *p) { return p->current; }

static Token *parser_previous(Parser *p) { return p->prev; }

static bool parser_is_eol(Parser *p) {
        return parser_peek(p)->type == TOKEN_EOL;
}

static Token *parser_advance(Parser *p) {
        Token *token = parser_peek(p);
        if (!parser_is_eol(p))
                p->current = p->current->next;
        return token;
}

static bool parser_match(Parser *p, int n, ...) {
        if (parser_is_eol(p))
                return false;

        va_list args;
        va_start(args, n);
        for (int i = 0; i < n; i++) {
                TokenType type = va_arg(args, TokenType);
                if (parser_peek(p)->type == type) {
                        p->prev = parser_advance(p);
                        va_end(args);
                        return true;
                }
        }
        va_end(args);
        return false;
}

static Command *parse_simple_command(Parser *p) {
        SimpleCommand *simple = malloc(sizeof(SimpleCommand));
        int envc = 0, capacity = 2;

        if (!simple) {
                p->shell->fatal_error = true;
                return NULL;
        }

        simple->argc = 0;
        simple->argv = malloc(sizeof(char *) * capacity);
        simple->envp = malloc(sizeof(char *) * capacity);
        simple->input_file = NULL;
        simple->output_file = NULL;
        simple->append_output = false;

        if (!simple->argv || !simple->envp) {
                p->shell->fatal_error = true;
                free(simple->argv);
                free(simple->envp);
                free(simple);
                return NULL;
        }

        while (parser_match(p, 1, TOKEN_ASSIGNMENT_WORD)) {
                if (envc + 1 >= capacity) {
                        capacity *= 2;
                        char **new_envp =
                            realloc(simple->envp, capacity * sizeof(char *));
                        if (!new_envp) {
                                p->shell->fatal_error = true;
                                free(simple->envp);
                                free(simple->argv);
                                free(simple);
                                return NULL;
                        }
                        simple->envp = new_envp;
                }
                simple->envp[envc++] = parser_previous(p)->lexeme;
        }
        simple->envp[envc] = NULL;

        capacity = 2;
        if (parser_match(p, 1, TOKEN_WORD)) {
                if (simple->argc + 1 >= capacity) {
                        capacity *= 2;
                        char **new_argv =
                            realloc(simple->argv, sizeof(char *) * capacity);
                        if (!new_argv) {
                                p->shell->fatal_error = true;
                                free(simple->argv);
                                free(simple->envp);
                                free(simple);
                                return NULL;
                        }
                        simple->argv = new_argv;
                }
                simple->argv[simple->argc++] = parser_previous(p)->lexeme;
                simple->argv[simple->argc] = NULL;
        } else if (parser_is_eol(p)) {
                free(simple->argv);
                free(simple->envp);
                free(simple);
                return NULL;
        } else {
                p->shell->had_error = true;
                printf(
                    "%s: %d: Syntax error: \"%s\" unexpected\n", p->shell->name,
                    p->shell->line_number,
                    (parser_previous(p) ? parser_previous(p) : parser_peek(p))
                        ->lexeme);
                free(simple->argv);
                free(simple->envp);
                free(simple);
                return NULL;
        }

        while (true) {
                if (parser_match(p, 2, TOKEN_WORD, TOKEN_ASSIGNMENT_WORD)) {
                        if (simple->argc + 1 >= capacity) {
                                capacity *= 2;
                                char **new_argv = realloc(
                                    simple->argv, sizeof(char *) * capacity);
                                if (!new_argv) {
                                        p->shell->fatal_error = true;
                                        free(simple->argv);
                                        free(simple->envp);
                                        free(simple);
                                        return NULL;
                                }
                                simple->argv = new_argv;
                        }
                        simple->argv[simple->argc++] =
                            parser_previous(p)->lexeme;
                        simple->argv[simple->argc] = NULL;

                } else if (parser_match(p, 3, TOKEN_REDIRECT_IN,
                                        TOKEN_REDIRECT_OUT,
                                        TOKEN_REDIRECT_APPEND)) {
                        Token *op = parser_previous(p);

                        if (!parser_match(p, 1, TOKEN_WORD)) {
                                fprintf(stderr,
                                        "%s: %d: Syntax error: "
                                        "expected filename after '%s'\n",
                                        p->shell->name, p->shell->line_number,
                                        op->lexeme);
                                free(simple->argv);
                                free(simple->envp);
                                free(simple);
                                return NULL;
                        }

                        Token *filename = parser_previous(p);

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
                p->shell->fatal_error = true;
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

static Command *parse_pipeline(Parser *p) {
        Command *cmd = parse_simple_command(p);
        if (cmd)
                cmd->is_background = false;
        else if (p->shell->had_error)
                return NULL;

        while (parser_match(p, 1, TOKEN_PIPE)) {
                if (parser_is_eol(p)) {
                        p->shell->had_error = true;
                        printf("%s: %d: Syntax error: end of line unexpected\n",
                               p->shell->name, p->shell->line_number);
                        command_free(cmd);
                        return NULL;
                }
                Command *right = parse_simple_command(p);
                if (!right) {
                        command_free(cmd);
                        return NULL;
                }
                Command *parent = malloc(sizeof(Command));
                if (!parent) {
                        command_free(cmd);
                        command_free(right);
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

static Command *parse_logical_list(Parser *p) {
        Command *cmd = parse_pipeline(p);
        if (cmd)
                cmd->is_background = false;
        else if (p->shell->had_error)
                return NULL;

        while (parser_match(p, 2, TOKEN_AND, TOKEN_OR)) {
                if (parser_is_eol(p)) {
                        p->shell->had_error = true;
                        printf("%s: %d: Syntax error: end of line unexpected\n",
                               p->shell->name, p->shell->line_number);
                        command_free(cmd);
                        return NULL;
                }
                CommandType parent_type =
                    parser_previous(p)->type == TOKEN_AND ? CMD_AND : CMD_OR;
                Command *right = parse_pipeline(p);
                if (!right) {
                        command_free(cmd);
                        return NULL;
                }
                Command *parent = malloc(sizeof(Command));
                if (!parent) {
                        command_free(cmd);
                        command_free(right);
                        return NULL;
                }
                parent->type = parent_type;
                parent->as.binary.left = cmd;
                parent->as.binary.right = right;
                cmd = parent;
                cmd->is_background = false;
        }
        return cmd;
}

static Command *parse_command(Parser *p) {
        Command *cmd = parse_logical_list(p);
        if (p->shell->had_error) {
                command_free(cmd);
                return NULL;
        }

        while (parser_match(p, 1, TOKEN_BACKGROUND)) {
                cmd->is_background = true;
                if (p->shell->had_error) {
                        command_free(cmd);
                        return NULL;
                }
                if (parser_is_eol(p))
                        return cmd;

                Command *right = parse_logical_list(p);
                if (p->shell->had_error || !right) {
                        command_free(cmd);
                        command_free(right);
                        return NULL;
                }
                Command *parent = malloc(sizeof(Command));
                if (p->shell->had_error || !parent) {
                        command_free(parent);
                        command_free(cmd);
                        command_free(right);
                        return NULL;
                }
                parent->as.binary.left = cmd;
                parent->type = CMD_SEPARATOR;
                parent->as.binary.right = right;
                cmd = parent;
        }
        return cmd;
}

Command *parse(ShellState *shell, Token *tokens) {
        Parser p = {.current = tokens, .prev = NULL, .shell = shell};
        return parse_command(&p);
}
