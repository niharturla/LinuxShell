#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <regex.h>
#include <dirent.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include "simpleCommand.hh"

SimpleCommand::SimpleCommand() {
    _arguments = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
    for (auto & arg : _arguments) {
        delete arg;
    }
}

// Converts a shell wildcard pattern (*, ?) into a Regular Expression
static std::string wildcardToRegex(const std::string& pattern) {
    std::string regex = "^";
    for (char c : pattern) {
        switch (c) {
            case '*':  regex += ".*"; break;
            case '?':  regex += ".";  break;
            case '.':  regex += "\\."; break;
            case '+': case '\\': case '(': case ')': case '[': case ']':
            case '{': case '}': case '^': case '$': case '|':
                regex += "\\";
                regex += c;
                break;
            default:   regex += c; break;
        }
    }
    regex += "$";
    return regex;
}

// Recursive function to handle multi-level path expansion
static void expandWildcard(std::string prefix, std::string remaining, std::vector<std::string>& results) {
    // Base Case: Nothing left to expand, add the accumulated prefix to results
    if (remaining.empty()) {
        results.push_back(prefix);
        return;
    }

    // Split remaining path into [current component] / [rest of path]
    size_t slash = remaining.find('/');
    std::string component = (slash == std::string::npos) ? remaining : remaining.substr(0, slash);
    std::string rest = (slash == std::string::npos) ? "" : remaining.substr(slash + 1);

    // If the current component does not contain wildcards, just append and keep going
    if (component.find('*') == std::string::npos && component.find('?') == std::string::npos) {
        std::string nextPrefix;
        if (prefix == "/") nextPrefix = "/" + component;
        else if (prefix.empty()) nextPrefix = component;
        else nextPrefix = prefix + "/" + component;
        
        expandWildcard(nextPrefix, rest, results);
        return;
    }

    // Component contains wildcards: open the directory and match entries
    std::string dirToOpen = prefix.empty() ? "." : prefix;
    DIR* dir = opendir(dirToOpen.c_str());
    if (dir == NULL) return;

    std::string regexStr = wildcardToRegex(component);
    regex_t re;
    if (regcomp(&re, regexStr.c_str(), REG_EXTENDED | REG_NOSUB) != 0) {
        closedir(dir);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Hidden file rule: Don't match entries starting with '.' 
        // unless the wildcard pattern itself starts with '.'
        if (entry->d_name[0] == '.' && component[0] != '.') continue;

        if (regexec(&re, entry->d_name, 0, NULL, 0) == 0) {
            std::string nextPrefix;
            if (prefix == "/") nextPrefix = "/" + std::string(entry->d_name);
            else if (prefix.empty()) nextPrefix = std::string(entry->d_name);
            else nextPrefix = prefix + "/" + std::string(entry->d_name);

            expandWildcard(nextPrefix, rest, results);
        }
    }

    regfree(&re);
    closedir(dir);
}

void SimpleCommand::insertArgument(std::string * argument) {
    // Check if the argument actually contains wildcards
    if (argument->find('*') == std::string::npos && 
        argument->find('?') == std::string::npos) {
        _arguments.push_back(argument);
        return;
    }

    std::vector<std::string> results;

    // Handle Absolute vs Relative paths
    if ((*argument)[0] == '/') {
        // Start recursion from root
        expandWildcard("/", argument->substr(1), results);
    } else {
        // Start recursion from current directory
        expandWildcard("", *argument, results);
    }

    // If no files matched, the shell should treat the wildcard as a literal string
    if (results.empty()) {
        _arguments.push_back(argument);
        return;
    }

    // Wildcard results MUST be sorted alphabetically
    std::sort(results.begin(), results.end());

    // Clean up the original argument and push expanded matches
    for (const auto& s : results) {
        _arguments.push_back(new std::string(s));
    }
    delete argument;
}

void SimpleCommand::print() {
    for (auto & arg : _arguments) {
        std::cout << "\"" << *arg << "\" \t";
    }
    std::cout << std::endl;
}
