#ifndef shell_hh
#define shell_hh

#include "command.hh"
#include <vector>
#include <string>
#include <set>

struct ProcessSub {
    std::string fifoPath;
    std::string tempDir;
    int pid;
};

struct Shell {
    static void prompt();
    static Command _currentCommand;
    static std::set<int> _backgroundPids;
    static std::vector<ProcessSub> _processSubs;  // add this
};
#endif
