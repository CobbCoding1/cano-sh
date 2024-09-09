#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "cano_sh.h"
#define ctrl(x) ((x) & 0x1f)

static const char SHELL_PROMPT[] = "[canosh]$ ";
static const Repl REPL_INIT = { .is_running = true };

static
bool shell_repl_initialize(Repl *repl) {
    *repl = REPL_INIT;
	initscr();
	raw();
	noecho();
	keypad(stdscr, TRUE);
    return true;
}

static
void clear_line(size_t line, size_t width) {
	for (size_t i = SSTR_LEN(SHELL_PROMPT); i < width - SSTR_LEN(SHELL_PROMPT); i++)
		mvprintw(line, i, " ");
}

static
bool shell_readline(Repl *repl)
{
	String command = repl->input;
	size_t height __attribute__((unused));
	size_t width;
	size_t line = repl->line;
	size_t position = 0;
	size_t command_max = repl->command_his.count;
	int ch;

	command.count = 0;
	while (true) {
		getmaxyx(stdscr, height, width);
		clear_line(line, width);

		mvprintw(line, 0, "%s%.*s", SHELL_PROMPT, (int)command.count, command.data);
		if (position > command.count)
			position = command.count;

		move(line, SSTR_LEN(SHELL_PROMPT) + position);

		ch = getch();
		switch (ch) {
			case KEY_ENTER:
			case '\n':
				line++;
				if (command.count == 0)
					continue;
				repl->line = line;
				repl->input = command;
				return true;
			case ctrl('d'):
			case ctrl('q'):
				repl->is_running = false;
				return true;
			case ctrl('c'):
				line++;
				command.count = 0;
				break;
			case KEY_BACKSPACE:
				if (command.count > 0)
					command.data[--command.count] = '\0';
				break;
			case KEY_LEFT:
				if (position > 0)
					position--;
				break;
			case KEY_RIGHT:
				position++;
				break;
			case KEY_UP:
				if (repl->command_his.count > 0) {
					repl->command_his.count--;
					command = repl->command_his.data[repl->command_his.count];
					position = command.count;
				}
				break;
			case KEY_DOWN:
				if (repl->command_his.count < command_max) {
					repl->command_his.count++;
					command = repl->command_his.data[repl->command_his.count];
					position = command.count;
				}
				break;
			default:
				DA_APPEND(&command, ch);
				memmove(&command.data[position+1], &command.data[position], command.count - 1 - position);
				command.data[position++] = ch;
				break;
		}
	}
}

static
bool shell_evaluate(Repl *repl)
{
	char **args = parse_command(str_to_cstr(repl->input));

	if (args != NULL) {
		handle_command(args, &repl->line);
		DA_APPEND(&repl->command_his, repl->input);
	}
	repl->input = (String){ 0 };
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
	refresh();
	endwin();
	echo();
    return EXIT_SUCCESS;
}
