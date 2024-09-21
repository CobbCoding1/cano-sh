#ifndef CANO_SH_H
    #define CANO_SH_H

    #include <stdbool.h>
    #include <stddef.h>

    #define DATA_START_CAPACITY 128

    #define sstr_len(sstr) (sizeof(sstr) - 1)
    #define sstr_unpack(sstr) (sstr), sstr_len((sstr))

    #define ASSERT(cond, ...) \
        do { \
            if (!(cond)) { \
                fprintf(stderr, "%s:%d: ASSERTION FAILED: ", __FILE__, __LINE__); \
                fprintf(stderr, __VA_ARGS__); \
                fprintf(stderr, "\n"); \
                exit(1); \
            } \
        } while (0)

    #define DA_APPEND(da, item) do {                                                       \
        if ((da)->count >= (da)->capacity) {                                               \
            (da)->capacity = (da)->capacity == 0 ? DATA_START_CAPACITY : (da)->capacity*2; \
            void *new = calloc(((da)->capacity+1), sizeof(*(da)->data));                   \
            ASSERT(new,"outta ram");                                                       \
            if ((da)->data != NULL)                                                        \
                memcpy(new, (da)->data, (da)->count);                                      \
            free((da)->data);                                                              \
            (da)->data = new;                                                              \
        }                                                                                  \
        (da)->data[(da)->count++] = (item);                                                \
    } while (0)

    #define DA_CHECK_BOUNDS(da, bounds, new_s) do { \
        if((bounds) >= (da)->capacity) { \
            (da)->capacity = (new_s); \
            (da)->data = realloc((da)->data, sizeof(char)*(da)->capacity); \
            if((da)->data == NULL) { \
                repl->is_running = false; \
                fprintf(stderr, "Could not allocate space for clipboard\n"); \
                return 0; \
            } \
        } \
    } while (0)


typedef struct {
	char *data;
	size_t count;
	size_t capacity;
} String;

typedef struct {
	String *data;
	size_t count;
	size_t capacity;
} Strings;

typedef struct shell_repl_s {
	String input;
	String clipboard;
    bool is_running;
} Repl;

char *str_to_cstr(String str);

int shell_repl_run(void);

char **parse_command(char *command);
void handle_command(char **args);

bool shell_repl_initialize(Repl *repl);
bool shell_readline(Repl *repl);
bool shell_evaluate(Repl *repl);
void shell_cleanup(Repl *repl);

#endif
