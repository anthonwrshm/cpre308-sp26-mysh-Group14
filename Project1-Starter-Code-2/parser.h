#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS     64      /* maximum number of tokens in one command */
#define MAX_LINE    512      /* maximum length of one input line        */

/*
 * Parsed representation of one command line.
 *
 * Example:  ls -l > out.txt < in.txt
 *   argc        = 2
 *   argv        = {"ls", "-l", NULL}
 *   input_file  = "in.txt"   (or NULL if no < redirection)
 *   output_file = "out.txt"  (or NULL if no > redirection)
 *   append      = 0          (1 if >> was used)
 */
typedef struct {
    int   argc;
    char *argv[MAX_ARGS];   /* NULL-terminated, suitable for execvp()  */
    char *input_file;       /* NULL if no input redirection            */
    char *output_file;      /* NULL if no output redirection           */
    int   append;           /* 1 = append (>>), 0 = overwrite (>)      */
    int   has_pipe;         /* 1 if a pipe '|' was found (bonus stage) */
    char *pipe_argv[MAX_ARGS]; /* argv for the right-hand side of pipe  */
    int   pipe_argc;
} Command;

/*
 * parse_line()
 *
 * Tokenizes `line` (which may be modified) and fills `cmd`.
 * Returns  0 on success.
 * Returns -1 if the line is empty or contains only whitespace.
 *
 * Recognised tokens:
 *   >    output redirection (overwrite)
 *   >>   output redirection (append)
 *   <    input  redirection
 *   |    pipe separator (bonus stage)
 *
 * Anything else is treated as a command name or argument.
 */
int parse_line(char *line, Command *cmd);

/*
 * free_command()
 *
 * Releases any heap memory allocated by parse_line().
 * Call this after you are done with a Command struct.
 */
void free_command(Command *cmd);

/*
 * print_command()  -- useful for debugging
 *
 * Prints a human-readable summary of `cmd` to stdout.
 */
void print_command(const Command *cmd);

#endif /* PARSER_H */
