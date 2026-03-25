#include <cstdio>
#include <unistd.h>
#include "shell.hh"
#include <signal.h>

int yyparse(void);

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

int main() {

  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }
  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
