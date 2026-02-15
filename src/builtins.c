#include <stddef.h>
#include <utils.h>
#include <command.h>
#include <shell.h>
#include <builtins.h>
#include <string.h>

typedef struct {
	char *arg;
	int (*func)(ShellState *, SimpleCommand *, bool);
} builtin_t;

static builtin_t builtins[] = {
	{ "cd", NULL },
	{ "exit", NULL },

};

int (*get_builtin(char *arg))(ShellState *, SimpleCommand *, bool)
{
	builtin_t *builtin = builtins;

	for (size_t i = 0; i < ARRAY_SIZE(builtins); i++) {
		builtin += i;
		if (!strcmp(builtin->arg, arg))
			return builtin->func;
	}
	return NULL;
}
