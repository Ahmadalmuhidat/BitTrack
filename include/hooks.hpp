#ifndef HOOKS_HPP
#define HOOKS_HPP

#include <string>
#include <vector>
#include <map>

enum class HookType
{
  PRE_COMMIT,
  POST_COMMIT,
  PRE_PUSH,
  POST_PUSH,
  PRE_MERGE,
  POST_MERGE,
  PRE_CHECKOUT,
  POST_CHECKOUT,
  PRE_BRANCH,
  POST_BRANCH
};

struct HookResult
{
  bool success;
  std::string output;
  std::string error;
  int exit_code;
  
  HookResult(): success(true), exit_code(0) {}
};

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
