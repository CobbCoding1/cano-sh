#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "cano_sh.h"

#define ctrl(x) ((x) & 0x1f)

static const char SHELL_PROMPT[] = "[canosh]$ ";
static const Repl REPL_INIT = { .is_running = true };

#define INITIAL_INPUT_CAPACITY 128

#define export __attribute__((visibility("default")))

bool export shell_repl_initialize(Repl *repl) {
	struct termios settings;
    String input = {
		.data = malloc(INITIAL_INPUT_CAPACITY * sizeof(char)),
		.capacity = INITIAL_INPUT_CAPACITY,
	};

	if (input.data == NULL)
		return false;

	*repl = REPL_INIT;
	repl->input = input;

    tcgetattr(STDIN_FILENO, &settings);
    repl->init_settings = settings;
    settings.c_iflag &= ~(IXON);
    settings.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &settings);

	return true;
}


void export shell_cleanup(Repl *repl)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &repl->init_settings);
	free(repl->input.data);
}

static
bool string_ensure_capacity(String *s)
{
	char *transaction;

	if (s->count < s->capacity)
		return true;
	if (s == NULL || s->capacity > (1 << 16)) /* enough for a human */
		return false;
	transaction = realloc(s->data, (s->capacity << 1) * sizeof(char));
	if (transaction == NULL)
		return free(s->data), NULL;
	s->data = transaction;
	s->capacity <<= 1;
	return true;
}

static
bool handle_shortcuts(Repl *repl, char c)
{
	char buf[32]; /* Limit of \e sequences is unkown, this is probably fine */

	switch (c) {
		case '\033': /* alternative keys */
			(void)read(STDIN_FILENO, buf, sizeof buf);
			break;

        case ctrl('d'):
        case ctrl('q'):
            repl->is_running = false;
            write(STDOUT_FILENO, sstr_unpack("exit\n"));
            return true;

		default:
			if (!string_ensure_capacity(&repl->input))
				return false;
			repl->input.data[repl->input.count++] = c;
			__attribute__((fallthrough));
		case '\n':
			write(STDOUT_FILENO, &c, sizeof c);
	}
	return true;
}

bool export shell_readline(Repl *repl)
{
	char c = '\0';

	repl->input.count = 0;
	memset(repl->input.data, '\0', repl->input.capacity);
	write(STDOUT_FILENO, sstr_unpack(SHELL_PROMPT));
	while (c != '\n' && repl->is_running) {
		if (read(STDIN_FILENO, &c, 1) <= 0)
			return false;
		if (!handle_shortcuts(repl, c))
			return false;
	}
	return true;
}

bool export shell_evaluate(Repl *repl)
{
	bool has_cmd = false;
	char **args;

	for (char *p = repl->input.data; !has_cmd && *p != '\0'; p++)
		has_cmd |= !isspace(*p);
    if (strlen(repl->input.data) < 1 || !has_cmd)
		return true;

	args = parse_command(repl->input.data);
	if (args == NULL)
		return false;
	handle_command(args);
	//DA_APPEND(&repl->command_his, repl->input);
	free(args);
	return true;
}

int shell_repl_run(void)
{
    Repl repl = { 0 };

    if (!shell_repl_initialize(&repl))
        return EXIT_FAILURE;
    while (repl.is_running) {
        if (!shell_readline(&repl))
            continue;
        if (!shell_evaluate(&repl))
            break;
    }
	shell_cleanup(&repl);
	// repl should be running at this point unless we reach a memory error
    return repl.is_running ? EXIT_SUCCESS : EXIT_FAILURE;
}
