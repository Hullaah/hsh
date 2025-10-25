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

static int repl(FILE *stream)
{
	char *line = NULL;
	size_t n = 0;
	ssize_t nread = 0;

	while (true) {
		if (shell->is_interactive_mode)
			fdprint(STDOUT_FILENO, "$ ");
		nread = getline(&line, &n, stream);
		if (nread < 0) {
			free(line);
			break;
		}
		Token *tokens = tokenize(line);
		free(line);
		if (!tokens)
			return (127);
		Command *command = parse(tokens);
		if (!command) {
			free_tokens(tokens);
			return (127);
		}
		execute(command);
		line = NULL;
	}
	return (0);
}

int main(int argc, char **argv)
{
	shell = malloc(sizeof(ShellState));
	if (!shell) {
		fdprint(STDERR_FILENO, "Error: malloc failed\n");
		return (127);
	}

	shell->fatal_error = false;
	shell->is_interactive_mode = true;

	if (argc > 2) {
		fdprint(STDERR_FILENO, "Usage: %s [streamname]", argv[0]);
		return (127);
	} else if (argc == 2) {
		FILE *stream = fopen(argv[1], "r");
		shell->is_interactive_mode = false;
		if (!stream) {
			fdprint(STDERR_FILENO, "Error: cannot open stream %s\n",
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
