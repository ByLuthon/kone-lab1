/*
*
* Den är programmet är skapat av Luthon Hagvinprice
* För mer information kontakta Email kone@kth.se
*
 *
 * Program which simulates printenv | sort | less
 * If parameters are available it shall printenv | grep [parameterlist] | sort | less
 * 
 * int argv - number of parameters
 * char ** argc - list of paramters
 * 
 * 
*
*
*
*/



#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define MAX_PROCS 4
#define MAX_ARGS 128

#define ERROR(fmt, ...) printf("ERROR: " fmt "\n", ##__VA_ARGS__)


/************ global variable *******************/

/* arguments for execvp system call */
char  *proc_args[MAX_PROCS][MAX_ARGS+1];
/* pipes between the forked processes */
int    proc_pipes[MAX_PROCS-1][2];
/* pids of forked processes */
pid_t  proc_pids[MAX_PROCS];
/* number of processes in use */
int    num_procs = 0;
/* pager to use instead of sort/more */
char  *pager;
/************************************************/

/* initialize global variables and create all required pipes
   which will be used later to redirect output between
   processes
*/
static int initialize_resources()
{
  int i;
  int ret;

  memset(proc_args, 0, sizeof(proc_args));
  memset(proc_pipes, 0, sizeof(proc_pipes));
  memset(proc_pids, 0, sizeof(proc_pids));

  for (i=0; i<MAX_PROCS-1; i++) {
    ret = pipe(proc_pipes[i]);
    if (ret != 0) {
      ERROR("pipe call failed, errno=%s", strerror(errno));
      return ret;
    }
  }
  return 0;
}

/* find the proper pager to use */
int get_pager()
{
  /* when we have environment variable we will use it */
  pager = getenv("PAGER");
  if (pager != NULL)
    return 0;

  /* otherwise check that less exists */
  pager = "/usr/bin/less";
  if (access(pager, F_OK) == 0)
    return 0;

  /* otherwise check that more exists */
  pager = "/bin/more";
  if (access(pager, F_OK) == 0)
    return 0;

  ERROR("Didn't find pager");
  return -1;
}

/* initialize arguments required to execvp process */
void setup_cmds(int argc, char *argv[])
{
  int i;
  num_procs = 0;

  proc_args[num_procs][0] = "printenv";
  num_procs += 1;

  /* if we have command line arguments copy them and use
     them as grep arguments */
  if (argc > 1) {
    proc_args[num_procs][0] = "grep";
    for (i=1; i < argc && i < MAX_ARGS; i++) {
      proc_args[num_procs][i] = argv[i];
    };
    num_procs +=1;
  }

  proc_args[num_procs][0] = "/usr/bin/sort";
  num_procs += 1;

  proc_args[num_procs][0] = pager;
  num_procs += 1;
}

/* close all the pipes inside proc_pipes in the range [start, end) */
void close_pipes(int start, int end)
{
  int i;
  for (i=start; i < end; i++) {
    close(proc_pipes[i][0]);
    close(proc_pipes[i][1]);
  }
}

/* fork and execute processes in the pipe */
int execute_pipe()
{
  int i;
  pid_t pid;
  int ret;
  for (i = 0; i < num_procs; i++) {
    pid = fork();
    if (pid == -1) {
      ERROR("pid failed");
      return -1;
    }
    else if (pid == 0) {
      /* we are in chile process */
      if (i > 0) {
        /* child process reads from pipe */
        close_pipes(0, i-1);
        ret = dup2(proc_pipes[i-1][0], 0);
        if (ret == -1)
          exit(-1);
        close(proc_pipes[i-1][1]);
      }
      if (i < num_procs - 1) {
        /* child process writes to pipe */
        close_pipes(i+1, MAX_PROCS-1);
        ret = dup2(proc_pipes[i][1], 1);
        if (ret == -1)
          exit(-1);
        close(proc_pipes[i][0]);
      }

      /* after redirecting stdin/stdout, execute the real program */
      ret = execvp(proc_args[i][0], proc_args[i]);
      if (ret == -1)
        exit(-1);
    }
    else {
      /* store pid for later waitpid call */
      proc_pids[i] = pid;
    }
  }

  return 0;
}

/* wait for all forked processes to exit */
void wait_pids()
{
  int i;
  for (i = 0; i < num_procs; i++) {
    if (proc_pids[i] != 0) {
      waitpid(proc_pids[i], NULL, 0);
    }
  }
}

/* main: execute the diagenv pipe */
int main(int argc, char *argv[])
{
  int ret = 0;

  ret = initialize_resources();
  if (ret != 0) {
    return ret;
  }

  ret = get_pager();
  if (ret != 0) {
    return ret;
  }

  setup_cmds(argc, argv);
  ret = execute_pipe();
  close_pipes(0, MAX_PROCS-1);
  wait_pids();

  return ret;
}
