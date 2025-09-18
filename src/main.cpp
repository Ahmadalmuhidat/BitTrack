#include "../include/branch.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include "../include/remote.hpp"
#include "../include/error.hpp"
#include "../include/diff.hpp"
#include "../include/stash.hpp"
#include "../include/config.hpp"
#include "../include/tag.hpp"
#include "../include/merge.hpp"
#include "../include/hooks.hpp"
#include "../include/maintenance.hpp"

bool check_repository_exists()
{
  if (!std::filesystem::exists(".bittrack"))
  {
    return false;
  }
  return true;
}

void init()
{
  try
  {
    if (check_repository_exists())
    {
      throw BitTrackError(ErrorCode::REPOSITORY_ALREADY_EXISTS, "Repository already exists in this directory", ErrorSeverity::WARNING, "init");
    }

    // create repository structure with error handling
    if (!ErrorHandler::safeCreateDirectories(".bittrack/objects"))
    {
      throw BitTrackError(ErrorCode::DIRECTORY_CREATION_FAILED, "Failed to create objects directory", ErrorSeverity::FATAL, "init");
    }

    if (!ErrorHandler::safeCreateDirectories(".bittrack/commits"))
    {
      throw BitTrackError(ErrorCode::DIRECTORY_CREATION_FAILED, "Failed to create commits directory", ErrorSeverity::FATAL, "init");
    }

    if (!ErrorHandler::safeCreateDirectories(".bittrack/refs/heads"))
    {
      throw BitTrackError(ErrorCode::DIRECTORY_CREATION_FAILED, "Failed to create refs/heads directory", ErrorSeverity::FATAL, "init");
    }

    // create essential files
    if (!ErrorHandler::safeWriteFile(".bittrack/HEAD", ""))
    {
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR, "Failed to create HEAD file", ErrorSeverity::FATAL, "init");
    }

    if (!ErrorHandler::safeWriteFile(".bittrack/commits/history", ""))
    {
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR, "Failed to create history file", ErrorSeverity::FATAL, "init");
    }

    if (!ErrorHandler::safeWriteFile(".bittrack/remote", ""))
    {
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR, "Failed to create remote file", ErrorSeverity::FATAL, "init");
    }

    add_branch("master");
    switch_branch("master"); // don't check for uncommitted changes during init
    std::cout << "Initialized empty BitTrack repository." << std::endl;
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw; // re-throw to be handled by main
  }
  HANDLE_EXCEPTION("init")
}

void status()
{
  std::cout << "staged files:" << std::endl;
  for (std::string fileName : get_staged_files())
  {
    std::cout << "\033[32m" << fileName << "\033[0m" << std::endl;
  }

  std::cout << "\n"
            << std::endl;

  std::cout << "unstaged files:" << std::endl;
  for (std::string fileName : get_unstaged_files())
  {
    std::cout << "\033[31m" << fileName << "\033[0m" << std::endl;
  }
}

void stage_file(int argc, const char *argv[], int &i)
{
  try
  {
    VALIDATE_ARGS(argc, i + 2, "--stage");

    std::string file_to_add = argv[++i];
    VALIDATE_FILE_PATH(file_to_add);

    stage(file_to_add);
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("stage file")
}

void unstage_files(int argc, const char *argv[], int &i)
{
  try
  {
    VALIDATE_ARGS(argc, i + 2, "--unstage");

    std::string fileToRemove = argv[++i];
    VALIDATE_FILE_PATH(fileToRemove);

    unstage(fileToRemove);
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("unstage file")
}

void commit()
{
  try
  {
    std::cout << "message: ";
    std::string message;
    getline(std::cin, message);

    VALIDATE_COMMIT_MESSAGE(message);

    commit_changes("almuhidat", message);
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("commit")
}

void show_staged_files_hashes()
{
  std::vector<std::string> staged_files = get_staged_files();
  
  if (staged_files.empty())
  {
    std::cout << "No staged files." << std::endl;
    return;
  }
  
  std::cout << "Staged files and their hashes:" << std::endl;
  for (const auto& file : staged_files)
  {
    std::string hash = get_file_hash(file);
    std::cout << file << ": " << hash << std::endl;
  }
}

void show_current_commit()
{
  std::cout << get_current_commit() << std::endl;
}

void show_commit_history()
{
  std::cout << "Commit history:" << std::endl;
  
  std::ifstream history_file(".bittrack/commits/history");
  if (!history_file.is_open())
  {
    std::cout << "No commits found." << std::endl;
    return;
  }
  
  std::string line;
  while (std::getline(history_file, line))
  {
    if (!line.empty())
    {
      std::cout << line << std::endl;
    }
  }
  history_file.close();
}

void remove_current_repo()
{
  std::filesystem::remove_all(".bittrack");
  std::cout << "Repository removed." << std::endl;
}

void branch_operations(int argc, const char *argv[], int &i)
{
  try
  {
    VALIDATE_ARGS(argc, i + 2, "--branch");

    std::string subFlag = argv[++i];

    if (subFlag == "-l")
    {
      // print branches list
      std::vector<std::string> branches = get_branches_list();
      for (const auto &branch : branches)
      {
        std::cout << branch << std::endl;
      }
    }
    else if (subFlag == "-c")
    {
      VALIDATE_ARGS(argc, i + 2, "--branch -c");

      std::string name = argv[++i];
      VALIDATE_BRANCH_NAME(name);

      add_branch(name);
    }
    else if (subFlag == "-r")
    {
      VALIDATE_ARGS(argc, i + 2, "--branch -r");

      std::string name = argv[++i];
      VALIDATE_BRANCH_NAME(name);

      remove_branch(name);
      switch_branch("master");
    }
    else if (subFlag == "-m")
    {
      VALIDATE_ARGS(argc, i + 3, "--branch -m");

      std::string old_name = argv[++i];
      std::string new_name = argv[++i];
      VALIDATE_BRANCH_NAME(old_name);
      VALIDATE_BRANCH_NAME(new_name);

      rename_branch(old_name, new_name);
    }
    else if (subFlag == "-i")
    {
      VALIDATE_ARGS(argc, i + 2, "--branch -i");

      std::string name = argv[++i];
      VALIDATE_BRANCH_NAME(name);

      show_branch_info(name);
    }
    else if (subFlag == "-h")
    {
      VALIDATE_ARGS(argc, i + 2, "--branch -h");

      std::string name = argv[++i];
      VALIDATE_BRANCH_NAME(name);

      show_branch_history(name);
    }
    else if (subFlag == "-merge")
    {
      VALIDATE_ARGS(argc, i + 3, "--branch -merge");

      std::string source = argv[++i];
      std::string target = argv[++i];
      VALIDATE_BRANCH_NAME(source);
      VALIDATE_BRANCH_NAME(target);

      merge_branch(source, target);
    }
    else if (subFlag == "-rebase")
    {
      VALIDATE_ARGS(argc, i + 3, "--branch -rebase");

      std::string source = argv[++i];
      std::string target = argv[++i];
      VALIDATE_BRANCH_NAME(source);
      VALIDATE_BRANCH_NAME(target);

      rebase_branch(source, target);
    }
    else
    {
      throw BitTrackError(ErrorCode::INVALID_ARGUMENTS, "Invalid branch sub-command: " + subFlag + ". Use -l, -c, -r, -m, -i, -h, -merge, or -rebase", ErrorSeverity::ERROR, "--branch");
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("branch operations")
}

void remote_operations(int argc, const char *argv[], int &i)
{
  try
  {
    VALIDATE_ARGS(argc, i + 2, "--remote");
    std::string subFlag = argv[++i];

    if (subFlag == "-v")
    {
      std::string remote = get_remote_origin();
      if (remote.empty())
      {
        std::cout << "No remote origin set" << std::endl;
      }
      else
      {
        std::cout << remote << std::endl;
      }
    }
    else if (subFlag == "-s")
    {
      VALIDATE_ARGS(argc, i + 2, "--remote -s");

      std::string url = argv[++i];
      if (!ErrorHandler::validateRemoteUrl(url))
      {
        throw BitTrackError(ErrorCode::INVALID_REMOTE_URL, "Invalid remote URL: " + url, ErrorSeverity::ERROR, url);
      }

      set_remote_origin(url);
    }
    else
    {
      throw BitTrackError(ErrorCode::INVALID_ARGUMENTS, "Invalid remote sub-command: " + subFlag + ". Use -v or -s", ErrorSeverity::ERROR, "--remote");
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("remote operations")
}

void merge(int argc, const char *argv[], int &i)
{
  try
  {
    VALIDATE_ARGS(argc, i + 3, "--merge");

    std::string first_branch = argv[++i];
    std::string second_branch = argv[++i];

    VALIDATE_BRANCH_NAME(first_branch);
    VALIDATE_BRANCH_NAME(second_branch);

    merge_branches(first_branch, second_branch);
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("merge")
}

void checkout(const char *argv[], int &i)
{
  try
  {
    std::string name = argv[++i];
    VALIDATE_BRANCH_NAME(name);
    switch_branch(name);
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("checkout")
}

void diff_operations(int argc, const char *argv[], int &i)
{
  try
  {
    if (i + 1 < argc)
    {
      std::string subFlag = argv[++i];

      if (subFlag == "--staged")
      {
        DiffResult result = diff_staged();
        show_diff(result);
      }
      else if (subFlag == "--unstaged")
      {
        DiffResult result = diff_unstaged();
        show_diff(result);
      }
      else if (i + 1 < argc)
      {
        // compare two files
        std::string file1 = subFlag;
        std::string file2 = argv[++i];
        DiffResult result = compare_files(file1, file2);
        show_diff(result);
      }
      else
      {
        // show working directory diff
        DiffResult result = diff_working_directory();
        show_diff(result);
      }
    }
    else
    {
      // lShow working directory diff
      DiffResult result = diff_working_directory();
      show_diff(result);
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("diff operations")
}

void stash_operations(int argc, const char *argv[], int &i)
{
  try
  {
    if (i + 1 < argc)
    {
      std::string subFlag = argv[++i];

      if (subFlag == "list")
      {
        stash_list();
      }
      else if (subFlag == "show" && i + 1 < argc)
      {
        std::string stash_id = argv[++i];
        stash_show(stash_id);
      }
      else if (subFlag == "apply" && i + 1 < argc)
      {
        std::string stash_id = argv[++i];
        stash_apply(stash_id);
      }
      else if (subFlag == "pop" && i + 1 < argc)
      {
        std::string stash_id = argv[++i];
        stash_pop(stash_id);
      }
      else if (subFlag == "drop" && i + 1 < argc)
      {
        std::string stash_id = argv[++i];
        stash_drop(stash_id);
      }
      else if (subFlag == "clear")
      {
        stash_clear();
      }
      else
      {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS, "Invalid stash sub-command: " + subFlag, ErrorSeverity::ERROR, "--stash");
      }
    }
    else
    {
      // lDefault: create stash
      stash_changes();
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("stash operations")
}

void config_operations(int argc, const char *argv[], int &i)
{
  try
  {
    if (i + 1 < argc)
    {
      std::string subFlag = argv[++i];

      if (subFlag == "--list")
      {
        config_list();
      }
      else if (i + 1 < argc)
      {
        // lSet config value
        std::string key = subFlag;
        std::string value = argv[++i];
        config_set(key, value);
        std::cout << "Set " << key << " = " << value << std::endl;
      }
      else
      {
        // lGet config value
        std::string value = config_get(subFlag);
        if (value.empty())
        {
          std::cout << "No value set for " << subFlag << std::endl;
        }
        else
        {
          std::cout << subFlag << " = " << value << std::endl;
        }
      }
    }
    else
    {
      config_list();
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("config operations")
}

void tag_operations(int argc, const char *argv[], int &i)
{
  try
  {
    if (i + 1 < argc)
    {
      std::string subFlag = argv[++i];

      if (subFlag == "-l")
      {
        tag_list();
      }
      else if (subFlag == "-a" && i + 2 < argc)
      {
        std::string tag_name = argv[++i];
        std::string message = argv[++i];
        tag_create(tag_name, "", true);
      }
      else if (subFlag == "-d" && i + 1 < argc)
      {
        std::string tag_name = argv[++i];
        tag_delete(tag_name);
      }
      else if (subFlag == "show" && i + 1 < argc)
      {
        std::string tag_name = argv[++i];
        tag_details(tag_name);
      }
      else
      {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS, "Invalid tag sub-command: " + subFlag, ErrorSeverity::ERROR, "--tag");
      }
    }
    else
    {
      tag_list();
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("tag operations")
}

void hooks_operations(int argc, const char *argv[], int &i)
{
  try
  {
    if (i + 1 < argc)
    {
      std::string subFlag = argv[++i];

      if (subFlag == "list")
      {
        list_hooks();
      }
      else if (subFlag == "install" && i + 1 < argc)
      {
        std::string hook_type = argv[++i];
        // lConvert string to HookType enum
        HookType type = HookType::PRE_COMMIT; // lDefault
        if (hook_type == "pre-commit")
          type = HookType::PRE_COMMIT;
        else if (hook_type == "post-commit")
          type = HookType::POST_COMMIT;
        else if (hook_type == "pre-push")
          type = HookType::PRE_PUSH;
        else if (hook_type == "post-push")
          type = HookType::POST_PUSH;
        else if (hook_type == "pre-merge")
          type = HookType::PRE_MERGE;
        else if (hook_type == "post-merge")
          type = HookType::POST_MERGE;
        else if (hook_type == "pre-checkout")
          type = HookType::PRE_CHECKOUT;
        else if (hook_type == "post-checkout")
          type = HookType::POST_CHECKOUT;
        else if (hook_type == "pre-branch")
          type = HookType::PRE_BRANCH;
        else if (hook_type == "post-branch")
          type = HookType::POST_BRANCH;
        else
        {
          std::cerr << "Error: Unknown hook type: " << hook_type << std::endl;
          return;
        }

        if (i + 1 < argc)
        {
          std::string script_path = argv[++i];
          install_hook(type, script_path);
        }
        else
        {
          install_default_hooks();
        }
      }
      else if (subFlag == "uninstall" && i + 1 < argc)
      {
        std::string hook_type = argv[++i];
        // lConvert string to HookType enum
        HookType type = HookType::PRE_COMMIT; // lDefault
        if (hook_type == "pre-commit")
          type = HookType::PRE_COMMIT;
        else if (hook_type == "post-commit")
          type = HookType::POST_COMMIT;
        else if (hook_type == "pre-push")
          type = HookType::PRE_PUSH;
        else if (hook_type == "post-push")
          type = HookType::POST_PUSH;
        else if (hook_type == "pre-merge")
          type = HookType::PRE_MERGE;
        else if (hook_type == "post-merge")
          type = HookType::POST_MERGE;
        else if (hook_type == "pre-checkout")
          type = HookType::PRE_CHECKOUT;
        else if (hook_type == "post-checkout")
          type = HookType::POST_CHECKOUT;
        else if (hook_type == "pre-branch")
          type = HookType::PRE_BRANCH;
        else if (hook_type == "post-branch")
          type = HookType::POST_BRANCH;
        else
        {
          std::cerr << "Error: Unknown hook type: " << hook_type << std::endl;
          return;
        }

        uninstall_hook(type);
      }
      else
      {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS, "Invalid hooks sub-command: " + subFlag, ErrorSeverity::ERROR, "--hooks");
      }
    }
    else
    {
      list_hooks();
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("hooks operations")
}

void maintenance_operations(int argc, const char *argv[], int &i)
{
  try
  {
    if (i + 1 < argc)
    {
      std::string subFlag = argv[++i];

      if (subFlag == "gc")
      {
        garbage_collect();
      }
      else if (subFlag == "repack")
      {
        repack_repository();
      }
      else if (subFlag == "fsck")
      {
        fsck_repository();
      }
      else if (subFlag == "stats")
      {
        show_repository_info();
      }
      else if (subFlag == "optimize")
      {
        optimize_repository();
      }
      else if (subFlag == "analyze")
      {
        analyze_repository();
      }
      else if (subFlag == "clean")
      {
        clean_untracked_files();
      }
      else if (subFlag == "prune")
      {
        prune_objects();
      }
      else
      {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS, "Invalid maintenance sub-command: " + subFlag, ErrorSeverity::ERROR, "--maintenance");
      }
    }
    else
    {
      show_repository_info();
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("maintenance operations")
}

void print_help()
{
  std::cout << "BitTrack - Lightweight Version Control\n\n";
  std::cout << "Usage:\n";
  std::cout << "  bittrack <command> [options]\n\n";
  std::cout << "Commands:\n";
  std::cout << "  init                        initialize a new BitTrack repository\n";
  std::cout << "  --status                    show staged and unstaged files\n";
  std::cout << "  --stage <file>              stage a file for commit\n";
  std::cout << "  --unstage <file>            unstage a file\n";
  std::cout << "  --commit                    commit staged files with a message\n";
  std::cout << "  --log                       show commit history\n";
  std::cout << "  --current-commit            show the current commit ID\n";
  std::cout << "  --staged-files-hashes       show hashes of staged files\n";
  std::cout << "  --remove-repo               delete the current BitTrack repository\n";
  std::cout << "  --branch -l                 list all branches\n";
  std::cout << "           -c <name>          create a new branch\n";
  std::cout << "           -r <name>          remove a branch\n";
  std::cout << "           -m <old> <new>     rename a branch\n";
  std::cout << "           -i <name>          show branch information\n";
  std::cout << "           -h <name>          show branch history\n";
  std::cout << "           -merge <src> <tgt> merge source into target branch\n";
  std::cout << "           -rebase <src> <tgt> rebase source onto target branch\n";
  std::cout << "  --checkout <name>           switch to a different branch\n";
  std::cout << "  --merge <b1> <b2>           merge two branches\n";
  std::cout << "  --diff                      show differences\n";
  std::cout << "           --staged           show staged changes\n";
  std::cout << "           --unstaged         show unstaged changes\n";
  std::cout << "           <file1> <file2>    compare two files\n";
  std::cout << "  --stash                     stash current changes\n";
  std::cout << "           list               list all stashes\n";
  std::cout << "           show <index>       show stash contents\n";
  std::cout << "           apply <index>      apply stash\n";
  std::cout << "           pop <index>        apply and remove stash\n";
  std::cout << "           drop <index>       remove stash\n";
  std::cout << "           clear              remove all stashes\n";
  std::cout << "  --config                    manage configuration\n";
  std::cout << "           --list             list all config\n";
  std::cout << "           <key> <value>      set config value\n";
  std::cout << "  --tag                       manage tags\n";
  std::cout << "           -l                 list all tags\n";
  std::cout << "           -a <name> <msg>    create annotated tag\n";
  std::cout << "           -d <name>          delete tag\n";
  std::cout << "           show <name>        show tag information\n";
  std::cout << "  --hooks                     manage hooks\n";
  std::cout << "           list               list all hooks\n";
  std::cout << "           install <type>     install hook\n";
  std::cout << "           uninstall <type>   uninstall hook\n";
  std::cout << "  --maintenance               repository maintenance\n";
  std::cout << "           gc                 garbage collection\n";
  std::cout << "           repack             repack objects\n";
  std::cout << "           fsck               check repository integrity\n";
  std::cout << "           stats              show repository statistics\n";
  std::cout << "           optimize           optimize repository\n";
  std::cout << "  --remote -v                 print current remote URL\n";
  std::cout << "           -s <url>           set remote URL\n";
  std::cout << "  --push                      push current commit to remote\n";
  std::cout << "  --help                      show this help menu\n";
}

int main(int argc, const char *argv[])
{
  try
  {
    if (argc == 1 || std::string(argv[1]) == "--help")
    {
      print_help();
      return 0;
    }

    for (int i = 1; i < argc; ++i)
    {
      std::string arg = argv[i];

      if (arg == "init")
      {
        init();
        break;
      }

      // validate repository exists for all operations except init
      if (!check_repository_exists())
      {
        throw BitTrackError(ErrorCode::NOT_IN_REPOSITORY, "Not inside a BitTrack repository", ErrorSeverity::ERROR, "repository check");
      }

      if (arg == "--status")
      {
        status();
        break;
      }
      else if (arg == "--stage")
      {
        stage_file(argc, argv, i);
        break;
      }
      else if (arg == "--unstage")
      {
        unstage_files(argc, argv, i);
        break;
      }
      else if (arg == "--commit")
      {
        commit();
        break;
      }
      else if (arg == "--staged-files-hashes")
      {
        show_staged_files_hashes();
        break;
      }
      else if (arg == "--current-commit")
      {
        show_current_commit();
        break;
      }
      else if (arg == "--log")
      {
        show_commit_history();
        break;
      }
      else if (arg == "--remove-repo")
      {
        remove_current_repo();
        break;
      }
      else if (arg == "--branch")
      {
        branch_operations(argc, argv, i);
        break;
      }
      else if (arg == "--diff")
      {
        diff_operations(argc, argv, i);
        break;
      }
      else if (arg == "--stash")
      {
        stash_operations(argc, argv, i);
        break;
      }
      else if (arg == "--config")
      {
        config_operations(argc, argv, i);
        break;
      }
      else if (arg == "--tag")
      {
        tag_operations(argc, argv, i);
        break;
      }
      else if (arg == "--hooks")
      {
        hooks_operations(argc, argv, i);
        break;
      }
      else if (arg == "--maintenance")
      {
        maintenance_operations(argc, argv, i);
        break;
      }
      else if (arg == "--remote")
      {
        remote_operations(argc, argv, i);
        break;
      }
      else if (arg == "--checkout")
      {
        checkout(argv, i);
        break;
      }
      else if (arg == "--merge")
      {
        merge(argc, argv, i);
        break;
      }
      else if (arg == "--push")
      {
        push();
        break;
      }
      else
      {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS, "Unknown command: " + arg, ErrorSeverity::ERROR, "command parsing");
      }
    }

    return 0;
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    return static_cast<int>(e.getCode());
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Unexpected error: " + std::string(e.what()), ErrorSeverity::FATAL, "main");
    return static_cast<int>(ErrorCode::UNEXPECTED_EXCEPTION);
  }
  catch (...)
  {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Unknown error occurred", ErrorSeverity::FATAL, "main");
    return static_cast<int>(ErrorCode::UNEXPECTED_EXCEPTION);
  }
}
