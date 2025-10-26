#include <parser.h>
#include <stdbool.h>
#include <tokenizer.h>
#include <shell.h>
#include <utils.h>
#include <unistd.h>
#include <stdio.h>
#include <executor.h>
#include <stdlib.h>

ShellState *shell;

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
	[TOKEN_EOL] = "TOKEN_EOL"
};

static int repl(FILE *stream)
{
	char *line = NULL;
	size_t n = 0;
	ssize_t nread = 0;

	while (true) {
		if (shell->is_interactive_mode)
			fprintf(stdout, "$ ");
		nread = getline(&line, &n, stream);
		if (nread < 0) {
			free(line);
			break;
		}
		Token *tokens = tokenize(line);
		Token *current = tokens;
		while (current != NULL) {
			// For debugging: print token types and lexemes
			fprintf(stdout, "[%s: '%s']\n",
				token_string[current->type],
				current->lexeme);
			current = current->next;
		}
		free(line);
		if (shell->fatal_error) {
			free_tokens(tokens);
			return (127);
		}
		Command *command = parse(tokens);
		free_tokens(tokens);
		execute(command);
		free_command(command);
		if (shell->fatal_error) {
			free_command(command);
			return (127);
		}
		line = NULL;
	}
	putchar('\n');
	return (0);
}

int main(int argc, char **argv)
{
	shell = malloc(sizeof(ShellState));
	if (!shell) {
		fprintf(stderr, "Error: malloc failed\n");
		return (127);
	}

	shell->fatal_error = false;
	shell->is_interactive_mode = true;

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [streamname]", argv[0]);
		return (127);
	} else if (argc == 2) {
		FILE *stream = fopen(argv[1], "r");
		shell->is_interactive_mode = false;
		if (!stream) {
			fprintf(stderr, "Error: cannot open stream %s\n",
				argv[1]);
			return (127);
		}
		return (repl(stream));
	} else if (!isatty(STDIN_FILENO)) {
		shell->is_interactive_mode = false;
		return (repl(stdin));
	} else {
		return (repl(stdin));
	}
	return (0);
}
