/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "command.hh"
#include "shell.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>


Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _appendOut = false;
    _ambiguousOutput = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile ) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;
    _appendOut = false;
    _ambiguousOutput = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::prompt();
        return;
    }

    if (_ambiguousOutput) {
      printf("Ambiguous output redirect.\n");
      clear();
      Shell::prompt();
      return;
    }

    // Print contents of Command data structure
    //print();

    int defaultin = dup(0);
    int defaultout = dup(1);
    int defaulterr = dup(2);

    // set up error redirection
    if (_errFile) {

      int errfd;
      if (_appendOut) {
        errfd = open(_errFile->c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
      } else {
        errfd = open(_errFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
      }

      if (errfd < 0) {
        perror("open error file");
        exit(1);
      }

      dup2(errfd, 2);
      close(errfd);
    }

    // Add execution here
    // For every simple command fork a new process

    int numCommands = _simpleCommands.size();
    int prevPipeRead = -1;
    int lastPid = -1;

    for (int i = 0; i < numCommands; i++) {

      bool isLast = (i == numCommands-1);
      if (prevPipeRead != -1) {
        dup2(prevPipeRead, 0);
        close(prevPipeRead);
        prevPipeRead=-1;
      } else if (_inFile) {
        int infd = open(_inFile->c_str(), O_RDONLY);
        if (infd < 0) {
          perror("Open input file");
          exit(1);
        }
        dup2(infd,0);
        close(infd);
      } else {
        dup2(defaultin, 0);
      }


      if (!isLast) {

        int fdpipe[2];
        if (pipe(fdpipe) < 0) { 
          perror("pipe"); 
          exit(1);
        }
        dup2(fdpipe[1],1); // make stdout point to write end of pipe
        close(fdpipe[1]);
        prevPipeRead = fdpipe[0]; // save the read end for next iteration
      } else {

        if (_outFile) {

          int outfd;
          if (_appendOut) {
            outfd = open(_outFile->c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
          } else {
            outfd = open(_outFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
          }
          if (outfd < 0) {
            perror("open output file");
            exit(1);
          }

          dup2(outfd, 1);
          close(outfd);
        } else {
          dup2(defaultout, 1);
        }
      }



    SimpleCommand* cmd = _simpleCommands[i];
    int numArgs = cmd->_arguments.size();

    char** args = new char*[numArgs+1];
    for (int i = 0; i < numArgs; i++) {
      args[i] = (char*) cmd->_arguments[i]->c_str();
    }
    args[numArgs]=NULL;
    int pid = fork();

    if (pid < 0) {
      perror("fork");
      exit(1);
    }

    if (pid == 0) {

      close(defaultin);
      close(defaultout);
      close(defaulterr);
      execvp(args[0], args);
      perror("execvp");
      exit(1);
    }

    lastPid = pid;
    delete[] args;
  }

    dup2(defaultin, 0);
    dup2(defaultout, 1);
    dup2(defaulterr, 2);
    close(defaultin);
    close(defaultout);
    close(defaulterr);

    if (!_background) {
      _running=true;
      waitpid(lastPid, NULL, 0);
      _running=false;
    }
    // Setup i/o redirection
    // and call exec

    // Clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
bool Command::_running = false;

