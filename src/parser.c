#include <token.h>
#include <parser.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * parser_peek - Returns the current token.
 * @p: Pointer to the Parser structure.
 * Return: Pointer to the current token.
 */
static Token *parser_peek(Parser *p)
{
	return p->current;
}

/**
 * parser_previous - Returns the previous token.
 * @p: Pointer to the Parser structure.
 * Return: Pointer to the previous token.
 */

static Token *parser_previous(Parser *p)
{
	return p->prev;
}
/**
 * parser_is_eol - Checks if the current token is an end-of-line token.
 * @p: Pointer to the Parser structure.
 * Return: true if the current token is TOKEN_EOL, false otherwise.
 */
static bool parser_is_eol(Parser *p)
{
	return parser_peek(p)->type == TOKEN_EOL;
}
/**
 * parser_advance - Advances the parser to the next token.
 * @p: Pointer to the Parser structure.
 * Return: Pointer to the current token before advancing.
 */
static Token *parser_advance(Parser *p)
{
	Token *token = parser_peek(p);
	if (!parser_is_eol(p))
		p->current = p->current->next;
	return token;
}
/**
 * parser_match - Checks if the current token matches any of the given types.
 * @p: Pointer to the Parser structure.
 * @n: Number of token types to check.
 * @...: Token types to match against.
 * Return: true if a match is found, false otherwise.
 */
static bool parser_match(Parser *p, int n, ...)
{
	if (parser_is_eol(p))
		return false;

	va_list args;
	va_start(args, n);
	for (int i = 0; i < n; i++) {
		TokenType type = va_arg(args, TokenType);
		if (parser_peek(p)->type == type) {
			p->prev = parser_advance(p);
			va_end(args);
			return true;
		}
	}
	va_end(args);
	return false;
}
/**
 * parse_simple_command - Parses a simple command.
 * @p: Pointer to the Parser structure.
 *
 * Return: Pointer to the parsed Command structure, or NULL on failure.
 */
static Command *parse_simple_command(Parser *p)
{
	SimpleCommand *simple = malloc(sizeof(SimpleCommand));
	int envc = 0, capacity = 2;

	if (!simple) {
		p->shell->fatal_error = true;
		return NULL;
	}

	simple->argc = 0;
	simple->argv = malloc(sizeof(char *) * capacity);
	simple->envp = malloc(sizeof(char *) * capacity);
	simple->input_file = NULL;
	simple->output_file = NULL;
	simple->append_output = false;

	if (!simple->argv || !simple->envp) {
		p->shell->fatal_error = true;
		free(simple->argv);
		free(simple->envp);
		free(simple);
		return NULL;
	}

	while (parser_match(p, 1, TOKEN_ASSIGNMENT_WORD)) {
		if (envc + 1 >= capacity) {
			capacity *= 2;
			char **new_envp = realloc(simple->envp,
						  capacity * sizeof(char *));
			if (!new_envp) {
				p->shell->fatal_error = true;
				free(simple->envp);
				free(simple->argv);
				free(simple);
				return NULL;
			}
			simple->envp = new_envp;
		}
		simple->envp[envc++] = parser_previous(p)->lexeme;
	}
	simple->envp[envc] = NULL;

	capacity = 2;
	if (parser_match(p, 1, TOKEN_WORD)) {
		if (simple->argc + 1 >= capacity) {
			capacity *= 2;
			char **new_argv = realloc(simple->argv,
						  sizeof(char *) * capacity);
			if (!new_argv) {
				p->shell->fatal_error = true;
				free(simple->argv);
				free(simple->envp);
				free(simple);
				return NULL;
			}
			simple->argv = new_argv;
		}
		simple->argv[simple->argc++] = parser_previous(p)->lexeme;
		simple->argv[simple->argc] = NULL;
	} else if (parser_is_eol(p)) {
		free(simple->argv);
		free(simple->envp);
		free(simple);
		return NULL;
	} else if ((parser_peek(p)->type == TOKEN_AND ||
		    parser_peek(p)->type == TOKEN_OR ||
		    parser_peek(p)->type == TOKEN_PIPE) &&
		   parser_previous(p) != NULL &&
		   parser_previous(p)->type == TOKEN_ASSIGNMENT_WORD) {
	} else {
		p->shell->had_error = true;
		printf("%s: %d: Syntax error: \"%s\" unexpected\n",
		       p->shell->name, p->shell->line_number,
		       (parser_previous(p) ? parser_previous(p) :
					     parser_peek(p))
			       ->lexeme);
		free(simple->argv);
		free(simple->envp);
		free(simple);
		return NULL;
	}

	while (true) {
		if (parser_match(p, 2, TOKEN_WORD, TOKEN_ASSIGNMENT_WORD)) {
			if (simple->argc + 1 >= capacity) {
				capacity *= 2;
				char **new_argv =
					realloc(simple->argv,
						sizeof(char *) * capacity);
				if (!new_argv) {
					p->shell->fatal_error = true;
					free(simple->argv);
					free(simple->envp);
					free(simple);
					return NULL;
				}
				simple->argv = new_argv;
			}
			simple->argv[simple->argc++] =
				parser_previous(p)->lexeme;
			simple->argv[simple->argc] = NULL;

		} else if (parser_match(p, 3, TOKEN_REDIRECT_IN,
					TOKEN_REDIRECT_OUT,
					TOKEN_REDIRECT_APPEND)) {
			Token *op = parser_previous(p);

			if (!parser_match(p, 1, TOKEN_WORD)) {
				fprintf(stderr,
					"%s: %d: Syntax error: "
					"expected filename after '%s'\n",
					p->shell->name, p->shell->line_number,
					op->lexeme);
				free(simple->argv);
				free(simple->envp);
				free(simple);
				return NULL;
			}

			Token *filename = parser_previous(p);

			switch (op->type) {
			case TOKEN_REDIRECT_IN:
				simple->input_file = filename->lexeme;
				break;
			case TOKEN_REDIRECT_OUT:
				simple->output_file = filename->lexeme;
				simple->append_output = false;
				break;
			case TOKEN_REDIRECT_APPEND:
				simple->output_file = filename->lexeme;
				simple->append_output = true;
				break;
			default:
				break;
			}
		} else {
			break;
		}
	}

	Command *cmd = malloc(sizeof(Command));
	if (!cmd) {
		p->shell->fatal_error = true;
		free(simple->argv);
		free(simple->envp);
		free(simple);
		return NULL;
	}

	cmd->type = CMD_SIMPLE;
	cmd->as.command = *simple;
	cmd->is_background = false;
	free(simple);
	return cmd;
}
/**
 * parse_pipeline - Parses a pipeline of commands connected by pipe operators.
 * @p: Pointer to the Parser structure.
 *
 * Return: Pointer to the parsed Command structure, or NULL on failure.
 */
static Command *parse_pipeline(Parser *p)
{
	Command *cmd = parse_simple_command(p);
	if (cmd)
		cmd->is_background = false;
	else if (p->shell->had_error)
		return NULL;

	while (parser_match(p, 1, TOKEN_PIPE)) {
		if (parser_is_eol(p)) {
			p->shell->had_error = true;
			printf("%s: %d: Syntax error: end of line unexpected\n",
			       p->shell->name, p->shell->line_number);
			command_free(cmd);
			return NULL;
		}
		Command *right = parse_simple_command(p);
		if (!right) {
			command_free(cmd);
			return NULL;
		}
		Command *parent = malloc(sizeof(Command));
		if (!parent) {
			command_free(cmd);
			command_free(right);
			return NULL;
		}
		parent->as.binary.left = cmd;
		parent->type = CMD_PIPE;
		parent->as.binary.right = right;
		cmd = parent;
		cmd->is_background = false;
	}
	return cmd;
}
/**
 * parse_logical_list - Parses a logical list of commands connected by
 *                      AND/OR operators.
 * @p: Pointer to the Parser structure.
 *
 * Return: Pointer to the parsed Command structure, or NULL on failure.
 */
static Command *parse_logical_list(Parser *p)
{
	Command *cmd = parse_pipeline(p);
	if (cmd)
		cmd->is_background = false;
	else if (p->shell->had_error)
		return NULL;

	while (parser_match(p, 2, TOKEN_AND, TOKEN_OR)) {
		if (parser_is_eol(p)) {
			p->shell->had_error = true;
			printf("%s: %d: Syntax error: end of line unexpected\n",
			       p->shell->name, p->shell->line_number);
			command_free(cmd);
			return NULL;
		}
		CommandType parent_type =
			parser_previous(p)->type == TOKEN_AND ? CMD_AND :
								CMD_OR;
		Command *right = parse_pipeline(p);
		if (!right) {
			command_free(cmd);
			return NULL;
		}
		Command *parent = malloc(sizeof(Command));
		if (!parent) {
			command_free(cmd);
			command_free(right);
			return NULL;
		}
		parent->type = parent_type;
		parent->as.binary.left = cmd;
		parent->as.binary.right = right;
		cmd = parent;
		cmd->is_background = false;
	}
	return cmd;
}
/**
 * parse_command - Parses a command, handling background execution.
 * @p: Pointer to the Parser structure.
 *
 * Return: Pointer to the parsed Command structure, or NULL on failure.
 */
static Command *parse_command(Parser *p)
{
	Command *cmd = parse_logical_list(p);
	if (p->shell->had_error) {
		command_free(cmd);
		return NULL;
	}

	while (parser_match(p, 1, TOKEN_BACKGROUND)) {
		cmd->is_background = true;
		if (p->shell->had_error) {
			command_free(cmd);
			return NULL;
		}
		if (parser_is_eol(p))
			return cmd;

		Command *right = parse_logical_list(p);
		if (p->shell->had_error || !right) {
			command_free(cmd);
			command_free(right);
			return NULL;
		}
		Command *parent = malloc(sizeof(Command));
		if (p->shell->had_error || !parent) {
			command_free(parent);
			command_free(cmd);
			command_free(right);
			return NULL;
		}
		parent->as.binary.left = cmd;
		parent->type = CMD_BACKGROUND;
		parent->as.binary.right = right;
		cmd = parent;
	}
	return cmd;
}
/**
 * parse - Parses a list of tokens into a command structure.
 * @shell: Pointer to the shell state.
 * @tokens: Pointer to the head of the token list.
 * Return: Pointer to the parsed Command structure, or NULL on failure.
 */
Command *parse(ShellState *shell, Token *tokens)
{
	Parser p = { .current = tokens, .prev = NULL, .shell = shell };
	return parse_command(&p);
}
