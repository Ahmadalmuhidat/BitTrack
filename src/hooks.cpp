#include "../include/hooks.hpp"

void installHook(HookType type, const std::string &script_path)
{
  // Check if the script file exists
  if (!std::filesystem::exists(script_path))
  {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND, "Script file '" + script_path + "' does not exist", ErrorSeverity::ERROR, "install_hook");
    return;
  }

  // Copy the script to the hooks directory
  std::string hook_path = getHookPath(type);
  ErrorHandler::safeCreateDirectories(std::filesystem::path(hook_path).parent_path().string());
  ErrorHandler::safeCopyFile(script_path, hook_path); // Copy and overwrite if exists

  // Make the hook executable
  makeHookExecutable(hook_path);
  std::cout << "Installed hook: " << getHookName(type) << std::endl;
}

void uninstallHook(HookType type)
{
  // Remove the hook file if it exists
  std::string hook_path = getHookPath(type);

  // Check if the hook file exists
  if (std::filesystem::exists(hook_path))
  {
    // Remove the hook file
    ErrorHandler::safeRemoveFile(hook_path);
    std::cout << "Uninstalled hook: " << getHookName(type) << std::endl;
  }
  else
  {
    // Hook file does not exist
    std::cout << "Hook not found: " << getHookName(type) << std::endl;
  }
}

void listHooks()
{
  // List all hooks in the hooks directory
  std::string hooks_dir = getHooksDir();

  // Check if the hooks directory exists
  if (!std::filesystem::exists(hooks_dir))
  {
    std::cout << "No hooks directory found." << std::endl;
    return;
  }

  std::cout << "Installed hooks:" << std::endl;
  for (const auto &entry : std::filesystem::directory_iterator(hooks_dir))
  {
    // Only list regular files
    if (entry.is_regular_file())
    {
      std::string hook_name = entry.path().filename().string();
      std::cout << "  " << hook_name;

      // Check if the hook is executable
      if (isHookExecutable(entry.path().string()))
      {
        std::cout << " (executable)";
      }
      else
      {
        std::cout << " (not executable)";
      }

      std::cout << std::endl;
    }
  }
}

HookResult runHook(HookType type, const std::vector<std::string> &args)
{
  // Get the hook path
  std::string hook_path = getHookPath(type);

  // Check if the hook file exists
  if (!std::filesystem::exists(hook_path))
  {
    // If the hook does not exist, return success (no-op)
    HookResult result;
    result.success = true;
    return result;
  }

  // Check if the hook is executable
  if (!isHookExecutable(hook_path))
  {
    // If the hook is not executable, return an error
    HookResult result;
    result.success = false;
    result.error = "Hook is not executable: " + hook_path;
    return result;
  }

  return execute_hook(hook_path, args);
}

void runAllHooks(const std::string &event, const std::vector<std::string> &args)
{
  // Run all hooks matching the event prefix
  std::string hooks_dir = getHooksDir();

  // Check if the hooks directory exists
  if (!std::filesystem::exists(hooks_dir))
  {
    return;
  }

  // Iterate over all files in the hooks directory
  for (const auto &entry : std::filesystem::directory_iterator(hooks_dir))
  {
    if (entry.is_regular_file())
    {
      // Get the hook name
      std::string hook_name = entry.path().filename().string();

      // Check if the hook name starts with the event prefix
      if (hook_name.find(event) == 0)
      {
        // Execute the hook
        HookResult result = execute_hook(entry.path().string(), args);

        // Check for execution errors
        if (!result.success)
        {
          ErrorHandler::printError(ErrorCode::HOOK_ERROR, "Hook failed: " + hook_name, ErrorSeverity::ERROR, "run_all_hooks");
          ErrorHandler::printError(ErrorCode::HOOK_ERROR, "Error: " + result.error, ErrorSeverity::ERROR, "run_all_hooks");
          return;
        }
      }
    }
  }
}

void installDefaultHooks()
{
  createPreCommitHook();
  createPostCommitHook();
  createPrePushHook();

  std::cout << "Installed default hooks." << std::endl;
}

void createPreCommitHook()
{
  std::string hook_path = getHookPath(HookType::PRE_COMMIT);
  ErrorHandler::safeCreateDirectories(std::filesystem::path(hook_path).parent_path().string());
  ErrorHandler::safeWriteFile(
    hook_path,
    "#!/bin/bash\n"
    "# Pre-commit hook\n"
    "echo \"Running pre-commit checks...\"\n"
    "\n"
    "# Check for TODO comments\n"
    "if grep -r \"TODO\\|FIXME\" . --exclude-dir=.bittrack; then\n"
    "  echo \"Warning: Found TODO/FIXME comments\"\n"
    "fi\n"
    "\n"
    "# Check for large files\n"
    "find . -name \"*.txt\" -size +1M -not -path \"./.bittrack/*\" | while read file; do\n"
    "  echo \"Warning: Large file detected: $file\"\n"
    "done\n"
    "\n"
    "echo \"Pre-commit checks completed.\"\n"
    "exit 0\n"
  );

  makeHookExecutable(hook_path);
}

void createPostCommitHook()
{
  std::string hook_path = getHookPath(HookType::POST_COMMIT);
  ErrorHandler::safeCreateDirectories(std::filesystem::path(hook_path).parent_path().string());
  ErrorHandler::safeWriteFile(
    hook_path,
    "#!/bin/bash\n"
    "# Post-commit hook\n"
    "echo \"Post-commit actions...\"\n"
    "\n"
    "# Update commit count\n"
    "if [ -f .bittrack/commit_count ]; then\n"
    "  count=$(cat .bittrack/commit_count)\n"
    "  echo $((count + 1)) > .bittrack/commit_count\n"
    "else\n"
    "  echo \"1\" > .bittrack/commit_count\n"
    "fi\n"
    "\n"
    "echo \"Commit completed successfully.\"\n"
    "exit 0\n"
  );

  makeHookExecutable(hook_path);
}

void createPrePushHook()
{
  std::string hook_path = getHookPath(HookType::PRE_PUSH);
  ErrorHandler::safeCreateDirectories(std::filesystem::path(hook_path).parent_path().string());
  ErrorHandler::safeWriteFile(
    hook_path,
    "#!/bin/bash\n"
    "# Pre-push hook\n"
    "echo \"Running pre-push checks...\"\n"
    "\n"
    "# Check if tests pass\n"
    "if [ -f Makefile ] && grep -q \"test\" Makefile; then\n"
    "  echo \"Running tests...\"\n"
    "  make test\n"
    "  if [ $? -ne 0 ]; then\n"
    "    echo \"Error: Tests failed. Push aborted.\"\n"
    "    exit 1\n"
    "  fi\n"
    "fi\n"
    "\n"
    "echo \"Pre-push checks completed.\"\n"
    "exit 0\n"
  );

  makeHookExecutable(hook_path);
}

HookResult execute_hook(const std::string &hook_path, const std::vector<std::string> &args)
{
  HookResult result;

  // Construct the command with arguments
  std::string command = hook_path;
  for (const auto &arg : args)
  {
    command += " \"" + arg + "\"";
  }

  // Execute the hook and capture output
  FILE *pipe = popen(command.c_str(), "r");
  if (!pipe)
  {
    result.success = false;
    result.error = "Failed to execute hook";
    return result;
  }

  // Read the output
  char buffer[128];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
  {
    result.output += buffer;
  }

  // Get the exit code
  result.exit_code = pclose(pipe);
  result.success = (result.exit_code == 0);

  // Set error message if not successful
  if (!result.success)
  {
    result.error = "Hook exited with code " + std::to_string(result.exit_code);
  }

  return result;
}

bool hookExists(HookType type)
{
  std::string hook_path = getHookPath(type);
  return std::filesystem::exists(hook_path);
}

std::string getHookPath(HookType type)
{
  initializeHookNames();
  std::string hook_name = hook_names[type];
  return getHooksDir() + "/" + hook_name;
}

std::string getHooksDir()
{
  return ".bittrack/hooks";
}

std::string getHookName(HookType type)
{
  initializeHookNames();
  return hook_names[type];
}

bool isHookExecutable(const std::string &hook_path)
{
  // Check if the hook file exists
  if (!std::filesystem::exists(hook_path))
  {
    return false;
  }

  // Check the file permissions
  std::filesystem::perms perms = std::filesystem::status(hook_path).permissions();
  return (perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none;
}

void makeHookExecutable(const std::string &hook_path)
{
  // Check if the hook file exists
  if (std::filesystem::exists(hook_path))
  {
    // Set the executable permission
    std::filesystem::perms perms = std::filesystem::status(hook_path).permissions();
    perms |= std::filesystem::perms::owner_exec;
    std::filesystem::permissions(hook_path, perms);
  }
}

void initializeHookNames()
{
  // Initialize the hook names map if it's empty
  if (hook_names.empty())
  {
    hook_names[HookType::PRE_COMMIT] = "pre-commit";
    hook_names[HookType::POST_COMMIT] = "post-commit";
    hook_names[HookType::PRE_PUSH] = "pre-push";
    hook_names[HookType::POST_PUSH] = "post-push";
    hook_names[HookType::PRE_MERGE] = "pre-merge";
    hook_names[HookType::POST_MERGE] = "post-merge";
    hook_names[HookType::PRE_CHECKOUT] = "pre-checkout";
    hook_names[HookType::POST_CHECKOUT] = "post-checkout";
    hook_names[HookType::PRE_BRANCH] = "pre-branch";
    hook_names[HookType::POST_BRANCH] = "post-branch";
  }
}

std::string getEventName(HookType type)
{
  initializeHookNames();
  std::string hook_name = hook_names[type];

  if (hook_name.find("pre-") == 0)
  {
    return hook_name.substr(4);
  }
  else if (hook_name.find("post-") == 0)
  {
    return hook_name.substr(5);
  }

  return hook_name;
}
