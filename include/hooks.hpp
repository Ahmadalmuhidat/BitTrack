#ifndef HOOKS_HPP
#define HOOKS_HPP

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "error.hpp"

// Types of hooks supported
enum class HookType
{
  PRE_COMMIT,    // Before a commit is made
  POST_COMMIT,   // After a commit is made
  PRE_PUSH,      // Before pushing to remote
  POST_PUSH,     // After pushing to remote
  PRE_PULL,      // Before pull from remote
  POST_PULL,     // After pull from remote
  PRE_MERGE,     // Before a merge operation
  POST_MERGE,    // After a merge operation
  PRE_CHECKOUT,  // Before checking out a branch
  POST_CHECKOUT, // After checking out a branch
  PRE_BRANCH,    // Before creating a branch
  POST_BRANCH    // After creating a branch
};

// Result of executing a hook
struct HookResult
{
  bool success;       // true if hook executed successfully
  std::string output; // output from the hook script
  std::string error;  // error output from the hook script
  int exit_code;      // exit code of the hook script

  HookResult() : success(true), exit_code(0) {}
};

static std::map<HookType, std::string> hook_names;

void installHook(
    HookType type,
    const std::string &script_path);
void uninstallHook(HookType type);
void printHooks();
HookResult runHook(
    HookType type,
    const std::vector<std::string> &args = {});
bool hookExists(HookType type);
HookResult executeHook(
    const std::string &hook_path,
    const std::vector<std::string> &args = {});
void installDefaultHooks();
void createPreCommitHook();
void createPostCommitHook();
void createPrePushHook();
std::string getHookPath(HookType type);
std::string getHooksDir();
std::string getHookName(HookType type);
bool isHookExecutable(const std::string &hook_path);
void makeHookExecutable(const std::string &hook_path);
std::string getHookName(HookType type);
void initializeHookNames();

#endif
