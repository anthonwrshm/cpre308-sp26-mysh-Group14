/*
 * mysh.c  --  CprE 308 Project 1: UNIX Shell Skeleton
 *
 * Build:   make
 * Run:     ./mysh
 *
 * Fill in every section marked TODO.
 * [S1] = Stage 1  REPL + built-in commands
 * [S2] = Stage 2  External command execution
 * [S3] = Stage 3  I/O redirection
 * [BP] = Bonus    Pipes (optional, +10 pts extra credit)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include "parser.h"

#define PROMPT "mysh> "

static int  run_builtin(Command *cmd);
static void run_external(Command *cmd);
static void run_pipe(Command *cmd);
static void apply_redirections(Command *cmd);

/* ================================================================== */
/* main()                                                               */
/* ================================================================== */
int main(void)
{
    char    line[MAX_LINE];
    Command cmd;

    while (1) {
        /* [S1] Print the prompt */
        printf("%s", PROMPT);
        fflush(stdout);

        /* [S1] Read one line of input */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        /* Parse the line into a Command struct */
        if (parse_line(line, &cmd) < 0)
            continue;

        /* [S1] If it is a built-in, run_builtin() handles it and returns 0.
         *      If not a built-in, it returns -1 and we fall through.  */
        if (run_builtin(&cmd) == 0) {
            free_command(&cmd);
            continue;
        }

        /* [BP] Bonus: pipe command */
        if (cmd.has_pipe) {
            run_pipe(&cmd);
            free_command(&cmd);
            continue;
        }

        /* [S2] External command */
        run_external(&cmd);
        free_command(&cmd);
    }

    return 0;
}

/* ================================================================== */
/* Stage 1 -- Built-in commands                                        */
/* ================================================================== */

/*
 * run_builtin()
 *
 * Returns  0 if cmd->argv[0] matches a built-in and it was executed.
 * Returns -1 if the command is not a built-in (caller handles it).
 *
 * Built-ins to implement:
 *   exit [n]  -- exit with status n (default 0); validate n is numeric
 *   cd [dir]  -- chdir(); no argument => $HOME
 *   pwd       -- print working directory (use getcwd(), not execvp)
 *   pid       -- print this shell's PID
 *   ppid      -- print this shell's parent PID
 */
static int run_builtin(Command *cmd)
{
    /* Guard: parse_line() guarantees argc >= 1, but be defensive */
    if (cmd->argc == 0 || cmd->argv[0] == NULL)
        return -1;

    /* ---- exit ---- */
    if (strcmp(cmd->argv[0], "exit") == 0) {
        int status = 0;
        if (cmd->argc > 1) {
            /* [S1] TODO: validate that argv[1] is a valid integer.
             * Hint: use strtol() and check errno + endptr instead of atoi(),
             * which silently ignores trailing garbage like "42abc".          */
            char *endptr;
            errno = 0;
            long val = strtol(cmd->argv[1], &endptr, 10);
            if (errno != 0 || *endptr != '\0') {
                fprintf(stderr, "mysh: exit: invalid status '%s'\n", cmd->argv[1]);
                return 0;   /* stay in shell; do not exit on bad argument */
            }
            status = (int)val;
        }
        exit(status);
    }

    /* ---- cd ---- */
    if (strcmp(cmd->argv[0], "cd") == 0) {
        /* [S1] TODO:
         *   const char *dir = (cmd->argc > 1) ? cmd->argv[1] : getenv("HOME");
         *   if (!dir) { fprintf(stderr, "mysh: cd: HOME not set\n"); return 0; }
         *   if (chdir(dir) < 0) perror("cd");                                    */
        return 0;
    }

    /* ---- pwd ---- */
    if (strcmp(cmd->argv[0], "pwd") == 0) {
        /* [S1] TODO: use getcwd() -- NOT an external execvp call.
         * pwd must be a true built-in so it reflects the shell's own cwd.
         *   char buf[PATH_MAX];
         *   if (getcwd(buf, sizeof(buf)) == NULL) perror("pwd");
         *   else printf("%s\n", buf);                                            */
        return 0;
    }

    /* ---- pid ---- */
    if (strcmp(cmd->argv[0], "pid") == 0) {
        /* [S1] TODO: printf("%d\n", (int)getpid()); */
        return 0;
    }

    /* ---- ppid ---- */
    if (strcmp(cmd->argv[0], "ppid") == 0) {
        /* [S1] TODO: printf("%d\n", (int)getppid()); */
        return 0;
    }

    return -1;  /* not a built-in */
}

/* ================================================================== */
/* Stage 2 -- External command execution                               */
/* ================================================================== */

/*
 * run_external()
 *
 * Fork a child, exec the command, wait for it, print PID and status.
 *
 * Required output format:
 *   [<pid>] <cmdname>            -- printed BEFORE child executes
 *   [<pid>] <cmdname> Exit N     -- printed AFTER child exits normally
 */
static void run_external(Command *cmd)
{
    /* [S2] TODO:
     *
     * pid_t pid = fork();
     *
     * CHILD  (pid == 0):
     *   apply_redirections(cmd);          <-- Stage 3 hook (safe no-op until S3)
     *   execvp(cmd->argv[0], cmd->argv);
     *   perror(cmd->argv[0]);             <-- only reached on exec failure
     *   exit(1);
     *
     * PARENT (pid > 0):
     *   printf("[%d] %s\n", pid, cmd->argv[0]);
     *   int status;
     *   waitpid(pid, &status, 0);
     *   if (WIFEXITED(status))
     *       printf("[%d] %s Exit %d\n", pid, cmd->argv[0], WEXITSTATUS(status));
     *
     * ERROR  (pid < 0):
     *   perror("fork");
     */

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        /* --- CHILD --- */
        apply_redirections(cmd);   /* Stage 3: set up redirections  */

        /* [S2] TODO: call execvp here */

        perror(cmd->argv[0]);
        exit(1);

    } else {
        /* --- PARENT --- */
        /* [S2] TODO: print PID, waitpid, print exit status */
        int status;
        waitpid(pid, &status, 0);
        (void)status;  /* remove this line once you use status */
    }
}

/* ================================================================== */
/* Stage 3 -- I/O Redirection  (called inside the child)              */
/* ================================================================== */

/*
 * apply_redirections()
 *
 * Called in the child between fork() and execvp().
 * Opens files named in cmd->input_file and cmd->output_file,
 * wires them to stdin/stdout using dup2(), then closes the raw fd.
 *
 * Exit with exit(1) on any error.
 *
 * Pattern for output redirection:
 *
 *   int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
 *   int fd = open(cmd->output_file, flags, 0644);
 *   if (fd < 0) { perror("open"); exit(1); }
 *   dup2(fd, STDOUT_FILENO);
 *   close(fd);   // <-- always close after dup2 to avoid fd leak
 *
 * Viva questions this function will generate:
 *   - Why is this called in the child, not the parent?
 *   - What would break if the parent redirected before forking?
 *   - Why must close(fd) follow every dup2(fd, ...)?
 */
static void apply_redirections(Command *cmd)
{
    /* [S3] TODO: implement input redirection (cmd->input_file)  */
    /* [S3] TODO: implement output redirection (cmd->output_file) */
    (void)cmd;  /* remove this line once you start implementing */
}

/* ================================================================== */
/* Bonus Stage -- Pipes  (+10 pts extra credit)                        */
/* ================================================================== */

/*
 * run_pipe()
 *
 * NOTE: This function is intentionally left as a stub.
 * Pipes are a bonus stage (+10 pts extra credit).
 * Do not attempt this until Stages 1-3 are fully working.
 *
 * When you are ready, implement the following steps:
 *
 *   int pfd[2];
 *   pipe(pfd);                            // create pipe
 *
 *   pid_t left = fork();                  // child 1: left side of pipe
 *   if (left == 0) {
 *       dup2(pfd[1], STDOUT_FILENO);      // write end -> stdout
 *       close(pfd[0]); close(pfd[1]);
 *       execvp(cmd->argv[0], cmd->argv);
 *       perror(cmd->argv[0]); exit(1);
 *   }
 *
 *   pid_t right = fork();                 // child 2: right side of pipe
 *   if (right == 0) {
 *       dup2(pfd[0], STDIN_FILENO);       // read end -> stdin
 *       close(pfd[0]); close(pfd[1]);
 *       execvp(cmd->pipe_argv[0], cmd->pipe_argv);
 *       perror(cmd->pipe_argv[0]); exit(1);
 *   }
 *
 *   // PARENT: must close BOTH ends before waitpid
 *   // Viva question: what happens if you forget to close pfd[1] here?
 *   close(pfd[0]);
 *   close(pfd[1]);
 *
 *   waitpid(left,  NULL, 0);
 *   waitpid(right, NULL, 0);
 */
static void run_pipe(Command *cmd)
{
    (void)cmd;
    fprintf(stderr, "mysh: pipes not yet implemented (bonus stage)\n");
}
