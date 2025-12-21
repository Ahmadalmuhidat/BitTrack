#include "../include/hooks.hpp"

// test installing default hooks
bool test_hooks_install_default()
{
  installDefaultHooks();

  bool pre_commit_exists = hookExists(HookType::PRE_COMMIT);
  bool post_commit_exists = hookExists(HookType::POST_COMMIT);
  bool pre_push_exists = hookExists(HookType::PRE_PUSH);

  return pre_commit_exists && post_commit_exists && pre_push_exists;
}

// test listing hooks
bool test_hooks_list()
{
  installDefaultHooks();

  listHooks();

  return true;
}

// test uninstalling a hook
bool test_hooks_uninstall()
{
  installDefaultHooks();

  bool exists_before = hookExists(HookType::PRE_COMMIT);

  uninstallHook(HookType::PRE_COMMIT);

  bool exists_after = hookExists(HookType::PRE_COMMIT);

  return exists_before && !exists_after;
}

// test running a specific hook
bool test_hooks_run()
{
  installDefaultHooks();

  HookResult result = runHook(HookType::PRE_COMMIT);

  return true;
}

// test running all hooks of a specific type
bool test_hooks_run_all()
{
  installDefaultHooks();

  runAllHooks("pre-commit");

  return true;
}

// test getting the path of a hook
bool test_hooks_get_path()
{
  std::string path = getHookPath(HookType::PRE_COMMIT);
  return !path.empty() && path.find("pre-commit") != std::string::npos;
}

// test getting the name of a hook
bool test_hooks_get_name()
{
  std::string name = getHookName(HookType::PRE_COMMIT);
  return name == "pre-commit";
}

// test checking if a hook is executable
bool test_hooks_is_executable()
{
  installDefaultHooks();

  std::string path = getHookPath(HookType::PRE_COMMIT);
  bool is_executable = isHookExecutable(path);

  return is_executable;
}

// test creating a pre-commit hook
bool test_hooks_create_pre_commit()
{
  createPreCommitHook();

  bool exists = hookExists(HookType::PRE_COMMIT);

  return exists;
}

// test creating a post-commit hook
bool test_hooks_create_post_commit()
{
  createPostCommitHook();

  bool exists = hookExists(HookType::POST_COMMIT);

  return exists;
}

// test creating a pre-push hook
bool test_hooks_create_pre_push()
{
  createPrePushHook();

  bool exists = hookExists(HookType::PRE_PUSH);

  return exists;
}
