#ifndef shell_hh
#define shell_hh

#include "command.hh"
#include <set>

struct Shell {

  static void prompt();

  static Command _currentCommand;

  static std::set<int> _backgroundPids;
};

#endif
