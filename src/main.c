#include "shell.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
        if (argc > 2) {
                fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
                return 127;
        }

        char *name = argc == 1 ? argv[0] : argv[1];
        bool is_interactive = (argc == 1 && isatty(STDIN_FILENO));

        ShellState *shell = shell_init(name, is_interactive);

        if (!shell) {
                fprintf(stderr, "Error: malloc failed\n");
                return 127;
        }

        if (argc == 2) {
                FILE *stream = fopen(argv[1], "r");
                if (!stream) {
                        fprintf(stderr, "Error: cannot open file %s\n",
                                argv[1]);
                        shell_free(shell);
                        return 127;
                }
                shell_repl(shell, stream);
                fclose(stream);
        } else {
                shell_repl(shell, stdin);
        }

        int exit_code = shell->fatal_error ? 2 : 0;
        shell_free(shell);
        return exit_code;
}
