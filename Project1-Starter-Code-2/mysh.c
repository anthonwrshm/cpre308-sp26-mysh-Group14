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
 * testing push
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
        // [S1] TODO:
          const char *dir = (cmd->argc > 1) ? cmd->argv[1] : getenv("HOME"); // if cd only has one argument the go to the home directory getenv("HOME")
           if (!dir) { fprintf(stderr, "mysh: cd: HOME not set\n"); return 0; } // if hoem directory is not set then throw an error
           if (chdir(dir) < 0) perror("cd");// use chdir function to change directory if returns successful no error in unseccesfull throw error cd.                             
        return 0;
    }

    /* ---- pwd ---- */
    if (strcmp(cmd->argv[0], "pwd") == 0) {
        // [S1] TODO: use getcwd() -- NOT an external execvp call. pwd must be a true built-in so it reflects the shell's own cwd.
        char buf[PATH_MAX];
        if (getcwd(buf, sizeof(buf)) == NULL) perror("pwd"); // runs getcwd (get path name of current working directory) if getcwd returns anything but null print it with buf
        else printf("%s\n", buf);                                            
        return 0;
    }

    /* ---- pid ---- */
    if (strcmp(cmd->argv[0], "pid") == 0) {
        // [S1] TODO: 
        printf("%d\n", (int)getpid()); // print pid, every shell method must have a pid so no need to check if null
        return 0;
    }

    /* ---- ppid ---- */
    if (strcmp(cmd->argv[0], "ppid") == 0) {
        //[S1] TODO: 
        printf("%d\n", (int)getppid());// print ppid if pid of 1 parent pid will be 0 and still printed
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
    // [S2] TODO:
    pid_t pid = fork(); // creates child proccess that is not shell
     if(cmd->input_file && access(cmd->input_file, F_OK)!=0){ //check to see make sure the input file exist before you create child proccess to run command
        perror("input");
        return;
     }
     
      if (pid == 0){
        apply_redirections(cmd);          //<-- Stage 3 hook (safe no-op until S3)
        execvp(cmd->argv[0], cmd->argv);
        perror(cmd->argv[0]);             //<-- only reached on exec failure
        exit(1);
      }
      if (pid > 0){
        printf("[%d] %s\n", pid, cmd->argv[0]);
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            printf("[%d] %s Exit %d\n", pid, cmd->argv[0], WEXITSTATUS(status));
      }

      if(pid < 0){
        perror("fork");
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
static void apply_redirections(Command *cmd){
    /* [S3] TODO: implement input redirection (cmd->input_file)  */
    if (cmd->input_file != NULL) {
    int fdin = open(cmd->input_file, O_RDONLY);
    if (fdin < 0) { 
        perror("input"); 
        exit(1); 
    }
    dup2(fdin, STDIN_FILENO);
    close(fdin);
}

    /* [S3] TODO: implement output redirection (cmd->output_file) */
    if (cmd->output_file != NULL) {
    int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
    int fdout= open(cmd->output_file, flags, 0644);
    if (fdout < 0) { 
        perror("out"); exit(1); 
    }
    dup2(fdout, STDOUT_FILENO);
    close(fdout);
}
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
static void run_pipe(Command *cmd){
   int pfd[2];// Array to hold file descriptors for the pipe: pfd[0] for reading, pfd[1] for writing

   if (pipe(pfd) < 0) { // Create a pipe; pfd[0] is read end, pfd[1] is write end
        perror("pipe");
        exit(1);
    }

    pid_t left = fork();
    if (left < 0) { //error check for fork
        perror("fork");
        exit(1); // if fork no work exit with error 1
    }

   if (left == 0) { // LEFT CHILD PROCESS: executes the first command
       dup2(pfd[1], STDOUT_FILENO);      // write end -> stdout this is wired to the right hand side child
       close(pfd[0]); close(pfd[1]);     // Close both ends of the pipe in this child; already duplicated write end 
       execvp(cmd->argv[0], cmd->argv); // Execute the left-hand command
       perror(cmd->argv[0]); exit(1); //if fail sendf error 1
   }

   pid_t right = fork(); // Fork the second child (right side of the pipe)

   if (right < 0) { // if fork for right does not work correctly
        perror("fork");
        exit(1);
    }

   if (right == 0) { // RIGHT CHILD PROCESS: executes the command reading from pipe
       dup2(pfd[0], STDIN_FILENO);       // read of child is wired to shared file between both children FileN0
       close(pfd[0]); close(pfd[1]);
       execvp(cmd->pipe_argv[0], cmd->pipe_argv);
       perror(cmd->pipe_argv[0]); exit(1);
   }

   // PARENT: must close BOTH ends before waitpid
   // Viva question: what happens if you forget to close pfd[1] here?
   close(pfd[0]);
   close(pfd[1]);
   waitpid(left,  NULL, 0);
   waitpid(right, NULL, 0);
}


/* Viva qs

Q: Why must cd be built in?
A: You must change directory of the parent process, not the child process, so you cannot fork and run cd in the child

Q: What would happen at the OS level if you forked a child to run cd?
A: The parent would fork, and the child would run cd, changing the directory of the child. The child would then terminate, and the directory of the parent/shell would not change

Q: What happens to the child’s address space after a successful execvp()?
A: After a succsessful execvp(), the entire child's address space is replaced by the new program's memory. i.e, child created, exeecvp() clears code, data, heap and stack but keeps the same pid

Q: What is still inherited from the parent after fork()?
A: Child is a clone of the parent and inherits the code segment, data segment, heap, stack, open file descriptors, current directory and environment variables

Q: What is a zombie process, and when does one appear in your shell?
A: When the parent process does not use wait() to reap the child. This happens in run_external at the waitpid() statement

Q: Why is dup2() called in the child rather than the parent?
A: If you're wiring output to a file, it only runs for that specific command. If you did it in the parent, every subsequent command would also be wired to that same output file

Q: What would break if the parent redirected stdout before forking?
A: See above, but if the parent redirected before forking, everything that would be printed in the terminal would be printied in the file

Q: What does /proc/PID/fd show before and after dup2()?
A: There are three pipes: stdin, stdout and stderror. stdin reads from the terminal, while stdout and stderror write to the terminal.  dup2(fdout, STDOUT_FILENO); takes fdout that we created
   by reading output file and wires stdout to that output file. Before it just goes to the terminal, but after it is wired to the output file. This is the same for stdin and stderror.



*/
