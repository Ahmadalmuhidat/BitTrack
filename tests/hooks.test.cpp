#include "../include/hooks.hpp"

bool test_hooks_install_default()
{
  // Install default hooks
  install_default_hooks();
  
  // Check if hooks were installed
  bool pre_commit_exists = hook_exists(HookType::PRE_COMMIT);
  bool post_commit_exists = hook_exists(HookType::POST_COMMIT);
  bool pre_push_exists = hook_exists(HookType::PRE_PUSH);
  
  return pre_commit_exists && post_commit_exists && pre_push_exists;
}

bool test_hooks_list()
{
  // Install a hook first
  install_default_hooks();
  
  // List hooks (should not throw exception)
  list_hooks();
  
  return true; // If no exception thrown, test passes
}

bool test_hooks_uninstall()
{
  // Install a hook first
  install_default_hooks();
  
  // Verify hook exists
  bool exists_before = hook_exists(HookType::PRE_COMMIT);
  
  // Uninstall hook
  uninstall_hook(HookType::PRE_COMMIT);
  
  // Verify hook is removed
  bool exists_after = hook_exists(HookType::PRE_COMMIT);
  
  return exists_before && !exists_after;
}

bool test_hooks_run()
{
  // Install a hook first
  install_default_hooks();
  
  // Run hook (should not throw exception)
  HookResult result = run_hook(HookType::PRE_COMMIT);
  
  return true; // If no exception thrown, test passes
}

bool test_hooks_run_all()
{
  // Install hooks first
  install_default_hooks();
  
  // Run all hooks for an event (should not throw exception)
  run_all_hooks("pre-commit");
  
  return true; // If no exception thrown, test passes
}

bool test_hooks_get_path()
{
  // Get hook path
  std::string path = get_hook_path(HookType::PRE_COMMIT);
  
  return !path.empty() && path.find("pre-commit") != std::string::npos;
}

bool test_hooks_get_name()
{
  // Get hook name
  std::string name = get_hook_name(HookType::PRE_COMMIT);
  
  return name == "pre-commit";
}

bool test_hooks_is_executable()
{
  // Install a hook first
  install_default_hooks();
  
  // Check if hook is executable
  std::string path = get_hook_path(HookType::PRE_COMMIT);
  bool is_executable = is_hook_executable(path);
  
  return is_executable;
}

bool test_hooks_create_pre_commit()
{
  // Create pre-commit hook
  create_pre_commit_hook();
  
  // Check if hook was created
  bool exists = hook_exists(HookType::PRE_COMMIT);
  
  return exists;
}

bool test_hooks_create_post_commit()
{
  // Create post-commit hook
  create_post_commit_hook();
  
  // Check if hook was created
  bool exists = hook_exists(HookType::POST_COMMIT);
  
  return exists;
}

bool test_hooks_create_pre_push()
{
  // Create pre-push hook
  create_pre_push_hook();
  
  // Check if hook was created
  bool exists = hook_exists(HookType::PRE_PUSH);
  
  return exists;
}
