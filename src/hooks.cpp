#include "../include/hooks.hpp"

void install_hook(HookType type, const std::string &script_path)
{
  // Check if the script file exists
  if (!std::filesystem::exists(script_path))
  {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND, "Script file '" + script_path + "' does not exist", ErrorSeverity::ERROR, "install_hook");
    return;
  }

  // Copy the script to the hooks directory
  std::string hook_path = get_hook_path(type);
  std::filesystem::create_directories(std::filesystem::path(hook_path).parent_path());                   // Ensure the hooks directory exists
  std::filesystem::copy_file(script_path, hook_path, std::filesystem::copy_options::overwrite_existing); // Copy and overwrite if exists

  // Make the hook executable
  make_hook_executable(hook_path);
  std::cout << "Installed hook: " << get_hook_name(type) << std::endl;
}

void uninstall_hook(HookType type)
{
  // Remove the hook file if it exists
  std::string hook_path = get_hook_path(type);

  // Check if the hook file exists
  if (std::filesystem::exists(hook_path))
  {
    // Remove the hook file
    std::filesystem::remove(hook_path);
    std::cout << "Uninstalled hook: " << get_hook_name(type) << std::endl;
  }
  else
  {
    // Hook file does not exist
    std::cout << "Hook not found: " << get_hook_name(type) << std::endl;
  }
}

void list_hooks()
{
  // List all hooks in the hooks directory
  std::string hooks_dir = get_hooks_dir();

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
      if (is_hook_executable(entry.path().string()))
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

HookResult run_hook(HookType type, const std::vector<std::string> &args)
{
  // Get the hook path
  std::string hook_path = get_hook_path(type);

  // Check if the hook file exists
  if (!std::filesystem::exists(hook_path))
  {
    // If the hook does not exist, return success (no-op)
    HookResult result;
    result.success = true;
    return result;
  }

  // Check if the hook is executable
  if (!is_hook_executable(hook_path))
  {
    // If the hook is not executable, return an error
    HookResult result;
    result.success = false;
    result.error = "Hook is not executable: " + hook_path;
    return result;
  }

  return execute_hook(hook_path, args);
}

void run_all_hooks(const std::string &event, const std::vector<std::string> &args)
{
  // Run all hooks matching the event prefix
  std::string hooks_dir = get_hooks_dir();

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

void install_default_hooks()
{
  create_pre_commit_hook();
  create_post_commit_hook();
  create_pre_push_hook();

  std::cout << "Installed default hooks." << std::endl;
}

void create_pre_commit_hook()
{
  std::string hook_path = get_hook_path(HookType::PRE_COMMIT);
  std::filesystem::create_directories(std::filesystem::path(hook_path).parent_path());
  std::ofstream hook_file(hook_path);

  hook_file << "#!/bin/bash\n";
  hook_file << "# Pre-commit hook\n";
  hook_file << "echo \"Running pre-commit checks...\"\n";
  hook_file << "\n";
  hook_file << "# Check for TODO comments\n";
  hook_file << "if grep -r \"TODO\\|FIXME\" . --exclude-dir=.bittrack; then\n";
  hook_file << "  echo \"Warning: Found TODO/FIXME comments\"\n";
  hook_file << "fi\n";
  hook_file << "\n";
  hook_file << "# Check for large files\n";
  hook_file << "find . -name \"*.txt\" -size +1M -not -path \"./.bittrack/*\" | while read file; do\n";
  hook_file << "  echo \"Warning: Large file detected: $file\"\n";
  hook_file << "done\n";
  hook_file << "\n";
  hook_file << "echo \"Pre-commit checks completed.\"\n";
  hook_file << "exit 0\n";

  hook_file.close();
  make_hook_executable(hook_path);
}

void create_post_commit_hook()
{
  std::string hook_path = get_hook_path(HookType::POST_COMMIT);
  std::filesystem::create_directories(std::filesystem::path(hook_path).parent_path());
  std::ofstream hook_file(hook_path);

  hook_file << "#!/bin/bash\n";
  hook_file << "# Post-commit hook\n";
  hook_file << "echo \"Post-commit actions...\"\n";
  hook_file << "\n";
  hook_file << "# Update commit count\n";
  hook_file << "if [ -f .bittrack/commit_count ]; then\n";
  hook_file << "  count=$(cat .bittrack/commit_count)\n";
  hook_file << "  echo $((count + 1)) > .bittrack/commit_count\n";
  hook_file << "else\n";
  hook_file << "  echo \"1\" > .bittrack/commit_count\n";
  hook_file << "fi\n";
  hook_file << "\n";
  hook_file << "echo \"Commit completed successfully.\"\n";
  hook_file << "exit 0\n";

  hook_file.close();
  make_hook_executable(hook_path);
}

void create_pre_push_hook()
{
  std::string hook_path = get_hook_path(HookType::PRE_PUSH);
  std::filesystem::create_directories(std::filesystem::path(hook_path).parent_path());
  std::ofstream hook_file(hook_path);

  hook_file << "#!/bin/bash\n";
  hook_file << "# Pre-push hook\n";
  hook_file << "echo \"Running pre-push checks...\"\n";
  hook_file << "\n";
  hook_file << "# Check if tests pass\n";
  hook_file << "if [ -f Makefile ] && grep -q \"test\" Makefile; then\n";
  hook_file << "  echo \"Running tests...\"\n";
  hook_file << "  make test\n";
  hook_file << "  if [ $? -ne 0 ]; then\n";
  hook_file << "    echo \"Error: Tests failed. Push aborted.\"\n";
  hook_file << "    exit 1\n";
  hook_file << "  fi\n";
  hook_file << "fi\n";
  hook_file << "\n";
  hook_file << "echo \"Pre-push checks completed.\"\n";
  hook_file << "exit 0\n";

  hook_file.close();
  make_hook_executable(hook_path);
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

bool hook_exists(HookType type)
{
  std::string hook_path = get_hook_path(type);
  return std::filesystem::exists(hook_path);
}

std::string get_hook_path(HookType type)
{
  initialize_hook_names();
  std::string hook_name = hook_names[type];
  return get_hooks_dir() + "/" + hook_name;
}

std::string get_hooks_dir()
{
  return ".bittrack/hooks";
}

std::string get_hook_name(HookType type)
{
  initialize_hook_names();
  return hook_names[type];
}

bool is_hook_executable(const std::string &hook_path)
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

void make_hook_executable(const std::string &hook_path)
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

void initialize_hook_names()
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

std::string get_event_name(HookType type)
{
  initialize_hook_names();
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
