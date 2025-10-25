#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <tokenizer.h>

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

Command *parse(Token *tokens);

#endif
