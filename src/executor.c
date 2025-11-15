#include <command.h>
#include <executor.h>
#include <utils.h>
#include <environ.h>
#include <stdio.h>
#include <vec.h>
#include <shell.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

static int execute_simple_command(ShellState *shell, SimpleCommand *simple,
				  bool is_background)
{
	pid_t pid;
	int status = 0;

	if (simple->argc == 0)
		return 0;

	pid = fork();

	if (pid < 0) {
		fprintf(stderr, "%s: fork failed: %s\n", shell->name,
			strerror(errno));
		return -1;
	} else if (pid > 0) {
		if (is_background) {
			printf("[1] %d\n", pid);
			return 0;
		} else {
			waitpid(pid, &status, 0);
			return status;
		}
	} else {
		int in, out;

		if (simple->input_file) {
			in = open(simple->input_file, O_RDONLY);
			if (in < 0) {
				fprintf(stderr, "%s: %s: %s\n", shell->name,
					simple->input_file, strerror(errno));
				shell->fatal_error = true;
				return 1;
			}
			dup2(in, STDIN_FILENO);
			close(in);
		}
		if (simple->output_file) {
			if (simple->append_output) {
				out = open(simple->output_file,
					   O_WRONLY | O_CREAT | O_APPEND,
					   S_IRUSR | S_IWUSR | S_IRGRP |
						   S_IROTH);
			} else {
				out = open(simple->output_file,
					   O_WRONLY | O_CREAT | O_TRUNC,
					   S_IRUSR | S_IWUSR | S_IRGRP |
						   S_IROTH);
			}
			if (out < 0) {
				fprintf(stderr, "%s: %s: %s\n", shell->name,
					simple->output_file, strerror(errno));
				shell->fatal_error = true;
				return 1;
			}
			dup2(out, STDOUT_FILENO);
			close(out);
		}
		char **envp = concat(2, simple->envp, environ);
		if (!envp) {
			fprintf(stderr, "Error: malloc failed\n");
			shell->fatal_error = true;
			return 1;
		}
		char *path = build_path(simple->argv[0], getenv("PATH"));
		if (path)
			simple->argv[0] = path;
		execve(simple->argv[0], simple->argv, envp);
		fprintf(stderr, "%s: %d: %s: %s\n", shell->name,
			shell->line_number, simple->argv[0], strerror(errno));
		free(envp);
		free(path);
		shell->fatal_error = true;
		return 1;
	}
	return 0;
}

int execute_command(ShellState *shell [[maybe_unused]],
		    Command *command [[maybe_unused]])
{
	int status;

	if (command == NULL)
		return 0;

	switch (command->type) {
	case CMD_SIMPLE:
		status = execute_simple_command(shell, &command->as.command,
						command->is_background);
		break;
	case CMD_PIPE:
		fprintf(stderr,
			"Executor: Pipe execution not implemented yet.\n");
		status = 0;
		break;
	case CMD_AND:
		fprintf(stderr,
			"Executor: AND execution not implemented yet.\n");
		status = 0;
		break;
	case CMD_OR:
		fprintf(stderr,
			"Executor: OR execution not implemented yet.\n");
		status = 0;
		break;
	case CMD_BACKGROUND:
		fprintf(stderr,
			"Executor: Background execution not implemented yet.\n");
		status = 0;
		break;
	default:
		fprintf(stderr, "Executor: Unknown command type.\n");
		status = -1;
		break;
	}
	return status;
}
