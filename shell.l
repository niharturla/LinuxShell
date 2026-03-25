
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include "y.tab.hh"
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shell.hh"

static void yyunput(int c, char *buf_ptr);

void myunputc(int c) {
    unput(c);
}

%}

%option noyywrap
%option stack
%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" {
  return GREATGREAT;
}

"|" {
  return PIPE;
}

"<" {
  return LESS;
}

"&" {
  return AMPERSAND;
}

"2>" {
  return TWOGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"$("[^)]*")" {
    /* subshell: execute command and inject output back into scanner */

    /* extract command between $( and ) */
    std::string cmd(yytext + 2, yyleng - 3);
    cmd += "\n";  /* add newline so shell parses it */

    /* two pipes: one to send command to child, one to read output */
    int inputPipe[2];   /* parent writes command → child reads */
    int outputPipe[2];  /* child writes output → parent reads */

    if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0) {
        perror("pipe");
        exit(1);
    }

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        /* child: redirect stdin to inputPipe, stdout to outputPipe */
        dup2(inputPipe[0], 0);   /* stdin reads from input pipe */
        dup2(outputPipe[1], 1);  /* stdout writes to output pipe */

        /* close unused ends */
        close(inputPipe[1]);
        close(outputPipe[0]);

        /* execute our own shell as the subshell */
        execvp("/proc/self/exe", NULL);
        perror("execvp");
        exit(1);
    }

    /* parent: write command to child's stdin */
    close(inputPipe[0]);   /* close read end - parent only writes */
    close(outputPipe[1]);  /* close write end - parent only reads */

    /* write the command to the child shell */
    write(inputPipe[1], cmd.c_str(), cmd.size());
    close(inputPipe[1]);  /* send EOF to child */

    /* read all output from child */
    std::string output;
    char buf[1024];
    int bytesRead;
    while ((bytesRead = read(outputPipe[0], buf, sizeof(buf))) > 0) {
        output.append(buf, bytesRead);
    }
    close(outputPipe[0]);

    /* wait for child to finish */
    waitpid(pid, NULL, 0);

    sigprocmask(SIG_UNBLOCK, &mask, &oldmask);


    /* remove trailing newline from output */
    while (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }

    for (int i = 0; i < output.size(); i++) {
      if (output[i] == '\n') {
        output[i] = ' ';
      }
    }

    /* inject output back into scanner in reverse order using myunputc */
    for (int i = output.size() - 1; i >= 0; i--) {
        myunputc(output[i]);
    }
}

"<("[^)]*")" {
    /* process substitution <(command) */

    /* extract command between <( and ) */
    std::string cmd(yytext + 2, yyleng - 3);
    cmd += "\n";

    /* create temp directory */
    char tempDir[] = "/tmp/shell_XXXXXX";
    if (mkdtemp(tempDir) == NULL) {
        perror("mkdtemp");
        exit(1);
    }

    /* create named pipe inside temp dir */
    std::string fifoPath = std::string(tempDir) + "/fifo";
    if (mkfifo(fifoPath.c_str(), 0666) < 0) {
        perror("mkfifo");
        exit(1);
    }

    /* block SIGCHLD so handler doesn't reap our child */
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    /* fork child to run the command */
    int inputPipe[2];
    if (pipe(inputPipe) < 0) {
        perror("pipe");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        /* child: read command from inputPipe */
        dup2(inputPipe[0], 0);
        close(inputPipe[1]);

        /* redirect stdout to the named pipe */
        int fifofd = open(fifoPath.c_str(), O_WRONLY);
        if (fifofd < 0) {
            perror("open fifo");
            exit(1);
        }
        dup2(fifofd, 1);
        close(fifofd);

        char* args[] = {(char*)"shell", NULL};
        execvp("/proc/self/exe", args);
        perror("execvp");
        exit(1);
    }

    /* parent: write command to child */
    close(inputPipe[0]);
    write(inputPipe[1], cmd.c_str(), cmd.size());
    close(inputPipe[1]);

    /* restore SIGCHLD */
    sigprocmask(SIG_UNBLOCK, &mask, &oldmask);

    /* store fifo path and pid for cleanup later */
    Shell::_processSubs.push_back({fifoPath, tempDir, pid});

    /* inject fifo path into scanner as a WORD */
    for (int i = fifoPath.size() - 1; i >= 0; i--) {
        myunputc(fifoPath[i]);
    }
}

\"[^\"]*\" {
  /* Remove quotes and return contents as a single WORD */
  yytext[yyleng-1] = '\0';        /* remove closing quote */
  yylval.cpp_string = new std::string(yytext + 1);  /* skip opening quote */
  return WORD;
}

"${"[a-zA-Z_][a-zA-Z0-9_]*"}" {
  /* variable expansion ${VAR} */
  std::string varName(yytext + 2, yyleng - 3);
  char* val = getenv(varName.c_str());
  yylval.cpp_string = new std::string(val ? val : "");
  return WORD;
}

(\\.|\`[^\`]*\`|[^ \t\n\"\`\\>\\|&])+ {
  /* handle escape characters */
  std::string * s = new std::string();
  for (int i = 0; i < yyleng; i++) {
    if (yytext[i] == '\\') {
      i++;  // skip the backslash
      s->push_back(yytext[i]);  // keep the next character literally
    } else {
      s->push_back(yytext[i]);
    }
  }
  yylval.cpp_string = s;
  return WORD;
}

<<EOF>> {
    yypop_buffer_state();
    if (!YY_CURRENT_BUFFER) {
        yyterminate();
    }
}
