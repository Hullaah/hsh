#include "shell.h"
#include "command.h"
#include "executor.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include <stdlib.h>
#include <string.h>

ShellState *shell_init(char *name, bool is_interactive) {
        ShellState *shell = malloc(sizeof(ShellState));
        if (!shell)
                return NULL;

        shell->fatal_error = false;
        shell->had_error = false;
        shell->is_interactive_mode = is_interactive;
        shell->line_number = 0;
        shell->name = name;
        return shell;
}

void shell_free(ShellState *shell) { free(shell); }

void shell_repl(ShellState *shell, FILE *stream) {
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

                Token *tokens = tokenize(shell, line);
                free(line);
                line = NULL;

                if (shell->fatal_error) {
                        token_free_list(tokens);
                        return;
                }
                if (shell->had_error) {
                        token_free_list(tokens);
                        shell->had_error = false;
                        continue;
                }

                Token **commands = token_split_by_semicolon(shell, tokens);
                if (shell->fatal_error) {
                        Token **runner = commands;
                        while (*runner) {
                                token_free_list(*runner);
                                runner++;
                        }
                        free(commands);
                        return;
                }

                Token **ptr = commands;
                for (; *ptr; ptr++) {
                        Command *command = parse(shell, *ptr);
                        execute_command(command);
                        command_free(command);

                        if (shell->fatal_error) {
                                while (*ptr) {
                                        token_free_list(*ptr);
                                        ptr++;
                                }
                                free(commands);
                                return;
                        }
                        if (shell->had_error) {
                                shell->had_error = false;
                                while (*ptr) {
                                        token_free_list(*ptr);
                                        ptr++;
                                }
                                free(commands);
                                if (shell->is_interactive_mode)
                                        goto start;
                                return;
                        }
                        token_free_list(*ptr);
                }
                free(commands);
        }

        if (shell->is_interactive_mode)
                putchar('\n');
}
