#include "../include/hooks.hpp"

bool test_hooks_install_default()
{
  install_default_hooks();

  bool pre_commit_exists = hook_exists(HookType::PRE_COMMIT);
  bool post_commit_exists = hook_exists(HookType::POST_COMMIT);
  bool pre_push_exists = hook_exists(HookType::PRE_PUSH);

  return pre_commit_exists && post_commit_exists && pre_push_exists;
}

bool test_hooks_list()
{
  install_default_hooks();

  list_hooks();

  return true;
}

bool test_hooks_uninstall()
{
  install_default_hooks();

  bool exists_before = hook_exists(HookType::PRE_COMMIT);

  uninstall_hook(HookType::PRE_COMMIT);

  bool exists_after = hook_exists(HookType::PRE_COMMIT);

  return exists_before && !exists_after;
}

bool test_hooks_run()
{
  install_default_hooks();

  HookResult result = run_hook(HookType::PRE_COMMIT);

  return true;
}

bool test_hooks_run_all()
{
  install_default_hooks();

  run_all_hooks("pre-commit");

  return true;
}

bool test_hooks_get_path()
{
  std::string path = get_hook_path(HookType::PRE_COMMIT);

  return !path.empty() && path.find("pre-commit") != std::string::npos;
}

bool test_hooks_get_name()
{
  std::string name = get_hook_name(HookType::PRE_COMMIT);

  return name == "pre-commit";
}

bool test_hooks_is_executable()
{
  install_default_hooks();

  std::string path = get_hook_path(HookType::PRE_COMMIT);
  bool is_executable = is_hook_executable(path);

  return is_executable;
}

bool test_hooks_create_pre_commit()
{
  create_pre_commit_hook();

  bool exists = hook_exists(HookType::PRE_COMMIT);

  return exists;
}

bool test_hooks_create_post_commit()
{
  create_post_commit_hook();

  bool exists = hook_exists(HookType::POST_COMMIT);

  return exists;
}

bool test_hooks_create_pre_push()
{
  create_pre_push_hook();

  bool exists = hook_exists(HookType::PRE_PUSH);

  return exists;
}
