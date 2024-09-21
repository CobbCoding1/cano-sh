#include <dlfcn.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cano_sh.h"

typedef bool (*ShellFunc)(Repl *);

#define length_of(arr) (sizeof (arr) / sizeof (arr)[0])

#ifndef HOT_RELOADER_LIB
	#error "Missing definition for HOT_RELOADER_LIB"
#else
	#define cat(x) #x

	#define _HR_LIB_NAME(loc) cat(loc)
	#define HR_LIB_NAME _HR_LIB_NAME(HOT_RELOADER_LIB)
#endif

struct sh {
	ShellFunc init;
	ShellFunc read;
	ShellFunc eval;
	ShellFunc fini;
};

static
const char *FUNC_MAPPINPS[] = {
	"shell_repl_initialize",
	"shell_readline",
	"shell_evaluate",
	"shell_cleanup",
};


typedef int assert_sh_size_is_length_of_func_mappings[
	sizeof *FUNC_MAPPINPS == sizeof ((struct sh *)NULL)->init
	&& sizeof FUNC_MAPPINPS == sizeof (struct sh) ? 1 : -1
] __attribute__((unused));

static
bool setup_shell_lib(struct sh *shell, void **modp)
{
	void **p = (void **)shell;
	void *module = dlopen(HR_LIB_NAME, RTLD_NOW);

	*modp = NULL;
	if (module == NULL)
		return fprintf(stderr, "lib: %s\n", dlerror()),
			false;

	for (size_t i = 0; i < length_of(FUNC_MAPPINPS); i++, p++) {
		*p = dlsym(module, FUNC_MAPPINPS[i]);
		if (*p == NULL) {
			dlclose(module);
			return fprintf(stderr, "[%s]: %s\n", FUNC_MAPPINPS[i], dlerror()),
				false;
		}
	}
	*modp = module;
	return true;
}

static __attribute__((format(printf, 3, 4)))
void run_command(Repl *this, struct sh *shell, char const *fmt, ...)
{
    char buffer[64];
	String cmd = { .data = buffer };
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(this->input.data, sizeof cmd, fmt, ap);
    va_end(ap);
	shell->eval(this);
}


static
int run_shell_module(Repl *this, struct sh *shell, char *binname)
{
	void *module = NULL;
init:
	if (module != NULL)
		dlclose(module);
	if (!setup_shell_lib(shell, &module))
		return EXIT_FAILURE;
	if (!this->is_running)
		shell->init(this);

	while (this->is_running) {
		if (!shell->read(this))
			return EXIT_FAILURE;
		if (!this->is_running)
			break;
		if (!strcmp(this->input.data, "reload")) {
			run_command(this, shell, "make %s", binname);
			goto init;
		}
		if (!shell->eval(this))
			return EXIT_FAILURE;
	}
	shell->fini(this);
	dlclose(module);
	return EXIT_SUCCESS;
}

int main(int argc __attribute__((unused)), char **argv)
{
	struct sh shell = { 0 };
	Repl this = { .is_running = false };

	setlocale(LC_ALL, "");
	return run_shell_module(&this, &shell, *argv);
}
