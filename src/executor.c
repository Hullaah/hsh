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
#include <builtins.h>

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
			mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
			if (simple->append_output) {
				int flags = O_WRONLY | O_CREAT | O_APPEND;
				out = open(simple->output_file, flags, mode);
			} else {
				int flags = O_WRONLY | O_CREAT | O_TRUNC;
				out = open(simple->output_file, flags, mode);
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

static int execute_command(ShellState *shell, SimpleCommand *command,
			   bool is_background)
{
	int (*builtin_func)(ShellState *, SimpleCommand *, bool);
	if (command->argc == 0)
		// modify shell environment for variable assignment
		return 0;
	else if ((builtin_func = get_builtin(command->argv[0])) != NULL)
		return builtin_func(shell, command, is_background);
	else
		return execute_simple_command(shell, command, is_background);
}

int execute_pipeline(ShellState *shell, Command *left, Command *right)
{
	int pipefd[2], status;
	if (pipe(pipefd) == -1) {
		fprintf(stderr, "%s: pipe failed: %s\n", shell->name,
			strerror(errno));
		return -1;
	}
	pid_t left_pid = fork();
	if (left_pid < 0) {
		fprintf(stderr, "%s: fork failed: %s\n", shell->name,
			strerror(errno));
		return -1;
	} else if (left_pid == 0) {
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		close(pipefd[0]);
		status = execute(shell, left);
		exit(status);
	}
	pid_t right_pid = fork();
	if (right_pid < 0) {
		fprintf(stderr, "%s: fork failed: %s\n", shell->name,
			strerror(errno));
		return -1;
	} else if (right_pid == 0) {
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		status = execute(shell, right);
		exit(status);
	} else {
		close(pipefd[0]);
		close(pipefd[1]);
		waitpid(left_pid, NULL, 0);
		waitpid(right_pid, &status, 0);
	}
	return status;
}

int execute(ShellState *shell, Command *command)
{
	int status;

	if (command == NULL)
		return 0;

	switch (command->type) {
	case CMD_SIMPLE:
		status = execute_command(shell, &command->as.command,
					 command->is_background);
		break;
	case CMD_PIPE:
		status = execute_pipeline(shell, command->as.binary.left,
					  command->as.binary.right);
		break;
	case CMD_AND:
		status = execute(shell, command->as.binary.left);
		if (!status)
			status = execute(shell, command->as.binary.right);
		break;
	case CMD_OR:
		status = execute(shell, command->as.binary.left);
		if (status)
			status = execute(shell, command->as.binary.right);
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
