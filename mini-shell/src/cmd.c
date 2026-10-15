// SPDX-License-Identifier: BSD-3-Clause

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>
// Prelucrarea string-urilor
#include <string.h>
// free()
#include <stdlib.h>
#include <stdio.h>

#include "cmd.h"
#include "utils.h"

#define READ		0
#define WRITE		1

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir)
{
	/* TODO: Execute cd. */

	if (dir == NULL || dir->next_word != NULL)
		return true;

	char *path_string = get_word(dir);

	if (chdir(path_string) < 0) {
		perror("cd");
		free(path_string);
		return true;
	}
	free(path_string);
	return false;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
	/* TODO: Execute exit/quit. */
	return SHELL_EXIT; /* TODO: Replace with actual exit code. */
}

static void redirections(simple_command_t *s)
{
	int fd;
	char *in = NULL, *out = NULL, *err = NULL;

	if (s->in != NULL)
		in = get_word(s->in);
	if (s->out != NULL)
		out = get_word(s->out);
	if (s->err != NULL)
		err = get_word(s->err);

	if (in != NULL) {
		fd = open(in, O_RDONLY);
		DIE(fd < 0, "open(in)");
		dup2(fd, STDIN_FILENO);
		close(fd);
	}

	int flags;

	if (out != NULL && err != NULL && strcmp(out, err) == 0) {
		if (s->io_flags & IO_OUT_APPEND)
			flags = O_WRONLY | O_CREAT | O_APPEND;
		else
			flags = O_WRONLY | O_CREAT | O_TRUNC;
		fd = open(out, flags, 0644);
		DIE(fd < 0, "open(out-err)");
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		close(fd);
	} else {
		if (out != NULL) {
			if (s->io_flags & IO_OUT_APPEND)
				flags = O_WRONLY | O_CREAT | O_APPEND;
			else
				flags = O_WRONLY | O_CREAT | O_TRUNC;

			fd = open(out, flags, 0644);
			DIE(fd < 0, "open(out)");
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		if (err != NULL) {
			if (s->io_flags & IO_ERR_APPEND)
				flags = O_WRONLY | O_CREAT | O_APPEND;
			else
				flags = O_WRONLY | O_CREAT | O_TRUNC;

			fd = open(err, flags, 0644);
			DIE(fd < 0, "open(err)");
			dup2(fd, STDERR_FILENO);
			close(fd);
		}
	}

	if (in != NULL)
		free(in);
	if (out != NULL)
		free(out);
	if (err != NULL)
		free(err);
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	/* TODO: Sanity checks. */
	if (s == NULL || s->verb == NULL)
		return 1;

	/* TODO: If builtin command, execute the command. */
	char *cmd = get_word(s->verb);
	int res = 0;

	if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
		int saved_stdout = dup(STDOUT_FILENO);
		int saved_stderr = dup(STDERR_FILENO);

		redirections(s);
		if (strcmp(cmd, "cd") == 0)
			res = shell_cd(s->params);
		else
			res = shell_exit();

		dup2(saved_stdout, STDOUT_FILENO);
		dup2(saved_stderr, STDERR_FILENO);
		close(saved_stdout);
		close(saved_stderr);
		free(cmd);

		return res;
	}

	/* TODO: If variable assignment, execute the assignment and return
	 * the exit status.
	 */
	if (strchr(cmd, '=') != NULL) {
		char *VAR = strchr(cmd, '=');
		*VAR = '\0';
		res = setenv(cmd, VAR + 1, 1);

		free(cmd);
		return res;
	}

	/* TODO: If external command:
	 *   1. Fork new process
	 *     2c. Perform redirections in child
	 *     3c. Load executable in child
	 *   2. Wait for child
	 *   3. Return exit status
	 */

	// 1.
	pid_t pid = fork();

	DIE(pid < 0, "fork");

	// 2c.
	// Copilul
	if (pid == 0) {
		redirections(s);

		// 3c.
		int argc;
		char **argv = get_argv(s, &argc);

		res = execvp(argv[0], argv);
		if (res < 0) {
			// Comanda nu exista
			fprintf(stderr, "Execution failed for '%s'\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// 2.
	int status;

	waitpid(pid, &status, 0);
	free(cmd);

	// 3.
	if (WIFEXITED(status))
		return WEXITSTATUS(status);

	return 1; /* TODO: Replace with actual exit status. */
}

/**
 * Process two commands in parallel, by creating two children.
 */
static bool run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO: Execute cmd1 and cmd2 simultaneously. */
	pid_t pid1 = fork();

	DIE(pid1 < 0, "fork");
	if (pid1 == 0)
		exit(parse_command(cmd1, level + 1, father));

	pid_t pid2 = fork();

	DIE(pid2 < 0, "fork");
	if (pid2 == 0)
		exit(parse_command(cmd2, level + 1, father));

	int status2;

	waitpid(pid1, NULL, 0);
	waitpid(pid2, &status2, 0);

	if (WIFEXITED(status2))
		return WEXITSTATUS(status2);

	return true; /* TODO: Replace with actual exit status. */
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static bool run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO: Redirect the output of cmd1 to the input of cmd2. */
	int pipe_fds[2];

	int res = pipe(pipe_fds);

	DIE(res < 0, "pipe");

	pid_t pid1 = fork();

	DIE(pid1 < 0, "fork");
	if (pid1 == 0) {
		close(pipe_fds[0]);
		dup2(pipe_fds[1], STDOUT_FILENO);
		close(pipe_fds[1]);
		exit(parse_command(cmd1, level + 1, father));
	}

	pid_t pid2 = fork();

	DIE(pid2 < 0, "fork");
	if (pid2 == 0) {
		close(pipe_fds[1]);
		dup2(pipe_fds[0], STDIN_FILENO);
		close(pipe_fds[0]);
		exit(parse_command(cmd2, level + 1, father));
	}

	close(pipe_fds[0]);
	close(pipe_fds[1]);

	int status;

	waitpid(pid1, NULL, 0);
	waitpid(pid2, &status, 0);

	if (WIFEXITED(status))
		return WEXITSTATUS(status);

	return true; /* TODO: Replace with actual exit status. */
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	/* TODO: sanity checks */
	if (c == NULL)
		return 0;

	if (c->op == OP_NONE) {
		/* TODO: Execute a simple command. */
		return parse_simple(c->scmd, level, c); /* TODO: Replace with actual exit code of command. */
	}

	int res1;

	switch (c->op) {
	case OP_SEQUENTIAL:
		/* TODO: Execute the commands one after the other. */
		parse_command(c->cmd1, level + 1, c);
		return parse_command(c->cmd2, level + 1, c);

		break;

	case OP_PARALLEL:
		/* TODO: Execute the commands simultaneously. */
		return run_in_parallel(c->cmd1, c->cmd2, level, c);

		break;

	case OP_CONDITIONAL_NZERO:
		/* TODO: Execute the second command only if the first one
		 * returns non zero.
		 */
		res1 = parse_command(c->cmd1, level + 1, c);
		if (res1 != 0)
			return parse_command(c->cmd2, level + 1, c);
		return res1;

		break;

	case OP_CONDITIONAL_ZERO:
		/* TODO: Execute the second command only if the first one
		 * returns zero.
		 */
		res1 = parse_command(c->cmd1, level + 1, c);
		if (res1 == 0)
			return parse_command(c->cmd2, level + 1, c);
		return res1;

		break;

	case OP_PIPE:
		/* TODO: Redirect the output of the first command to the
		 * input of the second.
		 */
		return run_on_pipe(c->cmd1, c->cmd2, level, c);
		//break;

	default:
		return SHELL_EXIT;
	}

	return 0; /* TODO: Replace with actual exit code of command. */
}
