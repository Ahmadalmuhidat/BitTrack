#ifndef HOOKS_HPP
#define HOOKS_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <string>

#include "error.hpp"

// Types of hooks supported
enum class HookType
{
  PRE_COMMIT, // Before a commit is made
  POST_COMMIT, // After a commit is made
  PRE_PUSH, // Before pushing to remote
  POST_PUSH, // After pushing to remote
  PRE_MERGE, // Before a merge operation
  POST_MERGE, // After a merge operation
  PRE_CHECKOUT, // Before checking out a branch
  POST_CHECKOUT, // After checking out a branch
  PRE_BRANCH, // Before creating a branch
  POST_BRANCH // After creating a branch
};

// Result of executing a hook
struct HookResult
{
  bool success; // true if hook executed successfully
  std::string output; // output from the hook script
  std::string error; // error output from the hook script
  int exit_code; // exit code of the hook script
  
  HookResult(): success(true), exit_code(0) {}
};

static std::map<HookType, std::string> hook_names;

void install_hook(HookType type, const std::string& script_path);
void uninstall_hook(HookType type);
void list_hooks();
HookResult run_hook(HookType type, const std::vector<std::string>& args = {});
bool hook_exists(HookType type);
HookResult execute_hook(const std::string& hook_path, const std::vector<std::string>& args = {});
void run_all_hooks(const std::string& event, const std::vector<std::string>& args = {});
void install_default_hooks();
void create_pre_commit_hook();
void create_post_commit_hook();
void create_pre_push_hook();
std::string get_hook_path(HookType type);
std::string get_hooks_dir();
std::string get_hook_name(HookType type);
bool is_hook_executable(const std::string& hook_path);
void make_hook_executable(const std::string& hook_path);
std::string get_event_name(HookType type);
void initialize_hook_names();

#endif
