#include "command.h"
#include <stdlib.h>

void command_free(Command *command) {
        if (command == NULL)
                return;

        if (command->type == CMD_SIMPLE) {
                free(command->as.command.argv);
                free(command->as.command.envp);
        } else {
                command_free(command->as.binary.left);
                command_free(command->as.binary.right);
        }
        free(command);
}
