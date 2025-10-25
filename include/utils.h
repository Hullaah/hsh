#ifndef UTILS_H
#define UTILS_H

#include <tokenizer.h>
#include <parser.h>

void fdprint(int fd, const char *format, ...);

void free_tokens(Token *head);

void free_command(Command *command);

#endif
