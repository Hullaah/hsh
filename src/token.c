#include "token.h"
#include <stdlib.h>

void token_free_list(Token *head) {
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

Token **token_split_by_semicolon(ShellState *shell, Token *tokens) {
        Token **commands;
        Token *current = tokens, *prev = NULL, *next = NULL, *start = tokens;
        Token *node = malloc(sizeof(Token));
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
