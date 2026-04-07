#include <cstdio>
#include <unistd.h>
#include "shell.hh"
#include <signal.h>
#include <sys/wait.h>
#include <set>

int yyparse(void);
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_create_buffer(FILE *file, int size);
extern void yypush_buffer_state(YY_BUFFER_STATE buffer);
extern void yypop_buffer_state();
#define YY_BUF_SIZE 16384

int Shell::_lastReturnCode = 0;
int Shell::_lastBackgroundPid = 0;
std::string Shell::_lastArg = "";
std::string Shell::_shellPath = "";

void Shell::prompt() {
  // adding isatty check
  if (isatty(0)) {
    printf("myshell>");
    fflush(stdout);
  }
}

extern "C" void handler( int sig )
{
    printf("\n");
    fflush(stdout);
    if (!Shell::_currentCommand._running) {
      Shell::prompt();
    }
}

extern "C" void sigChildHandler( int sig) {

  int pid;
  while (( pid = waitpid(-1, NULL, WNOHANG)) > 0) {
    if (Shell::_backgroundPids.count(pid)) {
      printf("[%d] exited.\n", pid);
      fflush(stdout);
      Shell::_backgroundPids.erase(pid);
    }
  }

}

int main() {

  char* resolved = realpath("/proc/self/exe", nullptr);
  Shell::_shellPath = resolved ? resolved : "proc/self/exe";
  free(resolved);

  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  struct sigaction sa_zombie;
  sa_zombie.sa_handler = sigChildHandler;
  sigemptyset(&sa_zombie.sa_mask);
  sa_zombie.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sa_zombie, NULL)) {
    perror("sigaction");
    exit(2);
  }

  if (sigaction(SIGINT, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }

  FILE* shellrc = fopen(".shellrc","r");
  if (shellrc) {
    yypush_buffer_state(yy_create_buffer(stdin, YY_BUF_SIZE));
    yypush_buffer_state(yy_create_buffer(shellrc, YY_BUF_SIZE));
  } else {
    Shell::prompt();
  }
  yyparse();
}

Command Shell::_currentCommand;
std::set<int> Shell::_backgroundPids;
std::vector<ProcessSub> Shell::_processSubs;
