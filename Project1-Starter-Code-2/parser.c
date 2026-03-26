#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */
static char *xstrdup(const char *s)
{
    char *p = strdup(s);
    if (!p) { perror("strdup"); exit(1); }
    return p;
}

/* Free every token in tokens[0..ntok-1] that has not yet been
 * transferred into cmd (i.e. is still non-NULL in the array).        */
static void free_tokens(char **tokens, int ntok)
{
    for (int i = 0; i < ntok; i++) {
        free(tokens[i]);
        tokens[i] = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* parse_line()                                                         */
/* ------------------------------------------------------------------ */
int parse_line(char *line, Command *cmd)
{
    memset(cmd, 0, sizeof(Command));

    /* Strip trailing newline / whitespace */
    int len = (int)strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1]))
        line[--len] = '\0';
    if (len == 0)
        return -1;

    /* ---- Pass 1: tokenise into a flat array ------------------------ */
    char *tokens[MAX_ARGS * 2];
    int   ntok = 0;

    char *p = line;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        if (ntok >= MAX_ARGS * 2 - 1) {
            fprintf(stderr, "mysh: too many tokens\n");
            free_tokens(tokens, ntok);
            return -1;
        }

        if (p[0] == '>' && p[1] == '>') {
            tokens[ntok++] = xstrdup(">>"); p += 2; continue;
        }
        if (*p == '>' || *p == '<' || *p == '|') {
            char op[2] = { *p, '\0' };
            tokens[ntok++] = xstrdup(op); p++; continue;
        }

        char *start = p;
        while (*p && !isspace((unsigned char)*p) &&
               *p != '>' && *p != '<' && *p != '|')
            p++;
        char saved = *p; *p = '\0';
        tokens[ntok++] = xstrdup(start);
        *p = saved;
    }

    if (ntok == 0)
        return -1;

    /* ---- Pass 2: validate and build Command ------------------------ */
    int  in_pipe   = 0;
    int  lhs_argc  = 0;
    int  rhs_argc  = 0;
    int  saw_pipe  = 0;   /* reject duplicate | */

#define SYNTAX_ERR(msg) \
    do { fprintf(stderr, "mysh: syntax error: " msg "\n"); \
         free_tokens(tokens, ntok); \
         memset(cmd, 0, sizeof(Command)); \
         return -1; } while (0)

#define SYNTAX_ERR1(msg, arg) \
    do { fprintf(stderr, "mysh: syntax error: " msg "\n", arg); \
         free_tokens(tokens, ntok); \
         memset(cmd, 0, sizeof(Command)); \
         return -1; } while (0)

    for (int i = 0; i < ntok; i++) {
        char *t = tokens[i];

        /* ---- pipe ---- */
        if (strcmp(t, "|") == 0) {
            if (lhs_argc == 0)
                SYNTAX_ERR("'|' with no command on the left");
            if (saw_pipe)
                SYNTAX_ERR("multiple pipes not supported");
            if (i + 1 >= ntok)
                SYNTAX_ERR("'|' with no command on the right");
            /* peek: next token must not be another operator */
            if (strcmp(tokens[i+1], "|") == 0 ||
                strcmp(tokens[i+1], ">") == 0 ||
                strcmp(tokens[i+1], ">>")== 0 ||
                strcmp(tokens[i+1], "<") == 0)
                SYNTAX_ERR("'|' with no command on the right");
            saw_pipe = 1;
            cmd->has_pipe = 1;
            in_pipe = 1;
            free(t); tokens[i] = NULL;
            continue;
        }

        /* ---- output redirection ---- */
        if (strcmp(t, ">") == 0 || strcmp(t, ">>") == 0) {
            if (cmd->output_file)
                SYNTAX_ERR("multiple output redirections");
            if (i + 1 >= ntok || tokens[i+1][0] == '>' ||
                tokens[i+1][0] == '<' || tokens[i+1][0] == '|')
                SYNTAX_ERR1("expected filename after '%s'", t);
            cmd->append      = (strcmp(t, ">>") == 0) ? 1 : 0;
            free(t); tokens[i] = NULL;
            cmd->output_file = tokens[++i]; tokens[i] = NULL;
            continue;
        }

        /* ---- input redirection ---- */
        if (strcmp(t, "<") == 0) {
            if (cmd->input_file)
                SYNTAX_ERR("multiple input redirections");
            if (i + 1 >= ntok || tokens[i+1][0] == '>' ||
                tokens[i+1][0] == '<' || tokens[i+1][0] == '|')
                SYNTAX_ERR("expected filename after '<'");
            free(t); tokens[i] = NULL;
            cmd->input_file = tokens[++i]; tokens[i] = NULL;
            continue;
        }

        /* ---- regular argument ---- */
        if (!in_pipe) {
            if (lhs_argc >= MAX_ARGS - 1) { free(t); tokens[i]=NULL; continue; }
            cmd->argv[lhs_argc++] = t; tokens[i] = NULL;
        } else {
            if (rhs_argc >= MAX_ARGS - 1) { free(t); tokens[i]=NULL; continue; }
            cmd->pipe_argv[rhs_argc++] = t; tokens[i] = NULL;
        }
    }

    /* Any tokens still non-NULL at this point are unreachable (shouldn't
     * happen after the logic above, but be safe).                     */
    free_tokens(tokens, ntok);

    cmd->argc = lhs_argc;
    cmd->argv[lhs_argc] = NULL;
    cmd->pipe_argc = rhs_argc;
    cmd->pipe_argv[rhs_argc] = NULL;

    if (cmd->argc == 0) {
        fprintf(stderr, "mysh: syntax error: no command\n");
        free_command(cmd);
        return -1;
    }
    if (cmd->has_pipe && cmd->pipe_argc == 0) {
        fprintf(stderr, "mysh: syntax error: no command after '|'\n");
        free_command(cmd);
        return -1;
    }

    return 0;

#undef SYNTAX_ERR
#undef SYNTAX_ERR1
}

/* ------------------------------------------------------------------ */
/* free_command()                                                       */
/* ------------------------------------------------------------------ */
void free_command(Command *cmd)
{
    for (int i = 0; i < cmd->argc; i++)      { free(cmd->argv[i]);      cmd->argv[i]      = NULL; }
    for (int i = 0; i < cmd->pipe_argc; i++) { free(cmd->pipe_argv[i]); cmd->pipe_argv[i] = NULL; }
    if (cmd->input_file)  { free(cmd->input_file);  cmd->input_file  = NULL; }
    if (cmd->output_file) { free(cmd->output_file); cmd->output_file = NULL; }
    cmd->argc = cmd->pipe_argc = cmd->has_pipe = cmd->append = 0;
}

/* ------------------------------------------------------------------ */
/* print_command()  -- debug helper                                     */
/* ------------------------------------------------------------------ */
void print_command(const Command *cmd)
{
    printf("--- Command ---\n");
    for (int i = 0; i < cmd->argc; i++)
        printf("  argv[%d] = \"%s\"\n", i, cmd->argv[i]);
    if (cmd->input_file)
        printf("  input  < \"%s\"\n", cmd->input_file);
    if (cmd->output_file)
        printf("  output %s \"%s\"\n", cmd->append ? ">>" : ">", cmd->output_file);
    if (cmd->has_pipe) {
        printf("  | (pipe)\n");
        for (int i = 0; i < cmd->pipe_argc; i++)
            printf("  pipe_argv[%d] = \"%s\"\n", i, cmd->pipe_argv[i]);
    }
    printf("---------------\n");
}
