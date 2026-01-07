#include "../include/branch.hpp"
#include "../include/commit.hpp"
#include "../include/config.hpp"
#include "../include/diff.hpp"
#include "../include/error.hpp"
#include "../include/hooks.hpp"
#include "../include/maintenance.hpp"
#include "../include/merge.hpp"
#include "../include/remote.hpp"
#include "../include/stage.hpp"
#include "../include/stash.hpp"
#include "../include/tag.hpp"

void init() {
  try {
    if (ErrorHandler::validateRepository()) {
      throw BitTrackError(ErrorCode::REPOSITORY_ALREADY_EXISTS,
                          "Repository already exists in this directory",
                          ErrorSeverity::WARNING, "init");
    }

    if (!ErrorHandler::safeCreateDirectories(".bittrack/objects")) {
      throw BitTrackError(ErrorCode::DIRECTORY_CREATION_FAILED,
                          "Failed to create objects directory",
                          ErrorSeverity::FATAL, "init");
    }

    if (!ErrorHandler::safeCreateDirectories(".bittrack/commits")) {
      throw BitTrackError(ErrorCode::DIRECTORY_CREATION_FAILED,
                          "Failed to create commits directory",
                          ErrorSeverity::FATAL, "init");
    }

    if (!ErrorHandler::safeCreateDirectories(".bittrack/refs/heads")) {
      throw BitTrackError(ErrorCode::DIRECTORY_CREATION_FAILED,
                          "Failed to create refs/heads directory",
                          ErrorSeverity::FATAL, "init");
    }

    if (!ErrorHandler::safeWriteFile(".bittrack/commits/history", "")) {
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR,
                          "Failed to create history file", ErrorSeverity::FATAL,
                          "init");
    }

    if (!ErrorHandler::safeWriteFile(".bittrack/remote", "")) {
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR,
                          "Failed to create remote file", ErrorSeverity::FATAL,
                          "init");
    }

    if (!ErrorHandler::safeWriteFile(".bittrack/refs/heads/main", "")) {
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR,
                          "Failed to create main branch ref",
                          ErrorSeverity::FATAL, "init");
    }

    if (!ErrorHandler::safeWriteFile(".bittrack/HEAD", "main\n")) {
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR,
                          "Failed to set HEAD to main", ErrorSeverity::FATAL,
                          "init");
    }

    std::cout << "Initialized empty BitTrack repository." << std::endl;
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("init")
}

void status() {
  std::cout << "staged files:" << std::endl;
  for (std::string fileName : getStagedFiles()) {
    std::cout << "\033[32m" << fileName << "\033[0m" << std::endl;
  }

  std::cout << "\n" << std::endl;

  std::cout << "unstaged files:" << std::endl;
  for (std::string fileName : getUnstagedFiles()) {
    std::cout << "\033[31m" << fileName << "\033[0m" << std::endl;
  }
}

void stageFile(int argc, const char *argv[], int &i) {
  try {
    VALIDATE_ARGS(argc, i + 2, "--stage");

    std::string file_to_add = argv[++i];
    VALIDATE_FILE_PATH(file_to_add);

    stage(file_to_add);
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("stage file")
}

void unstageFiles(int argc, const char *argv[], int &i) {
  try {
    VALIDATE_ARGS(argc, i + 2, "--unstage");

    std::string fileToRemove = argv[++i];
    VALIDATE_FILE_PATH(fileToRemove);

    unstage(fileToRemove);
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("unstage file")
}

void showStagedFilesHashes() {
  std::vector<std::string> staged_files = getStagedFiles();

  if (staged_files.empty()) {
    std::cout << "No staged files." << std::endl;
    return;
  }

  std::cout << "Staged files and their hashes:" << std::endl;
  for (const auto &file : staged_files) {
    std::string hash = getFileHash(file);
    std::cout << file << ": " << hash << std::endl;
  }
}

void showCommitHistory() {
  std::cout << "Commit history:" << std::endl;

  std::string history_file_content =
      ErrorHandler::safeReadFile(".bittrack/commits/history");
  std::istringstream history_file(history_file_content);

  std::string line;
  while (std::getline(history_file, line)) {
    if (!line.empty()) {
      std::cout << line << std::endl;
    }
  }
}

void removeCurrentRepo() {
  ErrorHandler::safeRemoveFolder(".bittrack");
  std::cout << "Repository removed." << std::endl;
}

void branchOperations(int argc, const char *argv[], int &i) {
  VALIDATE_ARGS(argc, i + 2, "--branch");

  std::string subFlag = argv[++i];

  if (subFlag == "-l") {
    std::vector<std::string> branches = getBranchesList();
    std::string current_branch = getCurrentBranch();

    for (const auto &branch : branches) {
      if (branch == current_branch) {
        std::cout << "\033[32m" << branch << "\033[0m" << std::endl;
      } else {
        std::cout << branch << std::endl;
      }
    }
  } else if (subFlag == "-c") {
    VALIDATE_ARGS(argc, i + 2, "--branch -c");

    std::string name = argv[++i];
    VALIDATE_BRANCH_NAME(name);

    addBranch(name);
  } else if (subFlag == "-r") {
    VALIDATE_ARGS(argc, i + 2, "--branch -r");

    std::string name = argv[++i];
    VALIDATE_BRANCH_NAME(name);

    removeBranch(name);
  } else if (subFlag == "-m") {
    VALIDATE_ARGS(argc, i + 3, "--branch -m");

    std::string old_name = argv[++i];
    std::string new_name = argv[++i];
    VALIDATE_BRANCH_NAME(old_name);
    VALIDATE_BRANCH_NAME(new_name);

    renameBranch(old_name, new_name);
  } else if (subFlag == "-i") {
    VALIDATE_ARGS(argc, i + 2, "--branch -i");

    std::string name = argv[++i];
    VALIDATE_BRANCH_NAME(name);

    showBranchInfo(name);
  } else if (subFlag == "-h") {
    VALIDATE_ARGS(argc, i + 2, "--branch -h");

    std::string name = argv[++i];
    VALIDATE_BRANCH_NAME(name);

    showBranchHistory(name);
  } else if (subFlag == "-merge") {
    VALIDATE_ARGS(argc, i + 3, "--branch -merge");

    std::string source = argv[++i];
    std::string target = argv[++i];
    VALIDATE_BRANCH_NAME(source);
    VALIDATE_BRANCH_NAME(target);

    MergeResult result = mergeBranches(source, target);
    if (result.success) {
      std::cout << "Merge completed successfully: " << result.message
                << std::endl;
    } else {
      std::cout << "Merge failed: " << result.message << std::endl;
    }
  } else if (subFlag == "-rebase") {
    VALIDATE_ARGS(argc, i + 3, "--branch -rebase");

    std::string source = argv[++i];
    std::string target = argv[++i];
    VALIDATE_BRANCH_NAME(source);
    VALIDATE_BRANCH_NAME(target);

    rebaseBranch(source, target);
  } else {
    throw BitTrackError(ErrorCode::INVALID_ARGUMENTS,
                        "Invalid branch sub-command: " + subFlag +
                            ". Use -l, -c, -r, -m, -i, -h, -merge, or -rebase",
                        ErrorSeverity::ERROR, "--branch");
  }
}

void remoteOperations(int argc, const char *argv[], int &i) {
  try {
    VALIDATE_ARGS(argc, i + 2, "--remote");
    std::string subFlag = argv[++i];

    if (subFlag == "-v") {
      std::string remote = getRemoteOrigin();
      if (remote.empty()) {
        std::cout << "No remote origin set" << std::endl;
      } else {
        std::cout << remote << std::endl;
      }
    } else if (subFlag == "-s") {
      VALIDATE_ARGS(argc, i + 2, "--remote -s");

      std::string url = argv[++i];
      if (!ErrorHandler::validateRemoteUrl(url)) {
        throw BitTrackError(ErrorCode::INVALID_REMOTE_URL,
                            "Invalid remote URL: " + url, ErrorSeverity::ERROR,
                            "--remote -s");
      }

      setRemoteOrigin(url);
    } else if (subFlag == "-l") {
      std::vector<std::string> branches = listRemoteBranches();
      if (branches.empty()) {
        std::cout << "No remote branches found." << std::endl;
      } else {
        std::cout << "Remote branches:" << std::endl;
        for (const auto &branch : branches) {
          std::cout << "  " << branch << std::endl;
        }
      }
    } else if (subFlag == "-d") {
      VALIDATE_ARGS(argc, i + 2, "--remote -d");
      std::string branch_name = argv[++i];
      if (deleteRemoteBranch(branch_name)) {
        std::cout << "Deleted remote branch '" << branch_name << "'"
                  << std::endl;
      } else {
        throw BitTrackError(ErrorCode::REMOTE_CONNECTION_FAILED,
                            "Failed to delete remote branch '" + branch_name +
                                "'",
                            ErrorSeverity::ERROR, "--remote -d");
      }
    } else {
      throw BitTrackError(ErrorCode::INVALID_ARGUMENTS,
                          "Invalid remote sub-command: " + subFlag +
                              ". Use -v, -s, -l, or -d",
                          ErrorSeverity::ERROR, "--remote");
    }
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("remote operations")
}

void checkout(const char *argv[], int &i) {
  try {
    std::string name = argv[++i];
    VALIDATE_BRANCH_NAME(name);
    switchBranch(name);
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("checkout")
}

void diffOperations(int argc, const char *argv[], int &i) {
  try {
    if (i + 1 < argc) {
      std::string subFlag = argv[++i];

      if (subFlag == "--staged") {
        DiffResult result = diffStaged();
        show_diff(result);
      } else if (subFlag == "--unstaged") {
        DiffResult result = diff_unstaged();
        show_diff(result);
      } else if (i + 1 < argc) {
        std::string file1 = subFlag;
        std::string file2 = argv[++i];
        DiffResult result = compareFiles(file1, file2);
        show_diff(result);
      } else {
        DiffResult result = diffWorkingDirectory();
        show_diff(result);
      }
    } else {
      DiffResult result = diffWorkingDirectory();
      show_diff(result);
    }
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("diff operations")
}

void stashOperations(int argc, const char *argv[], int &i) {
  try {
    if (i + 1 < argc) {
      std::string subFlag = argv[++i];

      if (subFlag == "list") {
        stashList();
      } else if (subFlag == "show" && i + 1 < argc) {
        std::string stash_id = argv[++i];
        stashShow(stash_id);
      } else if (subFlag == "apply" && i + 1 < argc) {
        std::string stash_id = argv[++i];
        stashApply(stash_id);
      } else if (subFlag == "pop" && i + 1 < argc) {
        std::string stash_id = argv[++i];
        stashPop(stash_id);
      } else if (subFlag == "drop" && i + 1 < argc) {
        std::string stash_id = argv[++i];
        stashDrop(stash_id);
      } else if (subFlag == "clear") {
        stashClear();
      } else {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS,
                            "Invalid stash sub-command: " + subFlag,
                            ErrorSeverity::ERROR, "--stash");
      }
    } else {
      std::cout << "Enter stash message: ";
      std::string message;
      std::getline(std::cin, message);
      stashChanges(message);
    }
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("stash operations")
}

void configOperations(int argc, const char *argv[], int &i) {
  try {
    if (i + 1 < argc) {
      std::string subFlag = argv[++i];

      if (subFlag == "--list") {
        configList();
      } else if (i + 1 < argc) {
        std::string key = subFlag;
        std::string value = argv[++i];
        configSet(key, value);
        std::cout << "Set " << key << " = " << value << std::endl;
      } else {
        std::string value = configGet(subFlag);
        if (value.empty()) {
          std::cout << "No value set for " << subFlag << std::endl;
        } else {
          std::cout << subFlag << " = " << value << std::endl;
        }
      }
    } else {
      configList();
    }
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("config operations")
}

void tagOperations(int argc, const char *argv[], int &i) {
  try {
    if (i + 1 < argc) {
      std::string subFlag = argv[++i];

      if (subFlag == "-l") {
        tagList();
      } else if (subFlag == "-a" && i + 2 < argc) {
        std::string tag_name = argv[++i];
        std::string message = argv[++i];
        tagCreate(tag_name, "", true);
      } else if (subFlag == "-d" && i + 1 < argc) {
        std::string tag_name = argv[++i];
        tagDelete(tag_name);
      } else if (subFlag == "show" && i + 1 < argc) {
        std::string tag_name = argv[++i];
        tagDetails(tag_name);
      } else {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS,
                            "Invalid tag sub-command: " + subFlag,
                            ErrorSeverity::ERROR, "--tag");
      }
    } else {
      tagList();
    }
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("tag operations")
}

void hooksOperations(int argc, const char *argv[], int &i) {
  try {
    if (i + 1 < argc) {
      std::string subFlag = argv[++i];

      if (subFlag == "list") {
        listHooks();
      } else if (subFlag == "install" && i + 1 < argc) {
        std::string hook_type = argv[++i];
        HookType type = HookType::PRE_COMMIT;
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
        else {
          ErrorHandler::printError(ErrorCode::INVALID_ARGUMENTS,
                                   "Unknown hook type: " + hook_type,
                                   ErrorSeverity::ERROR, "hooks_operations");
          return;
        }

        if (i + 1 < argc) {
          std::string script_path = argv[++i];
          installHook(type, script_path);
        } else {
          installDefaultHooks();
        }
      } else if (subFlag == "uninstall" && i + 1 < argc) {
        std::string hook_type = argv[++i];
        HookType type = HookType::PRE_COMMIT;
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
        else {
          throw BitTrackError(ErrorCode::INVALID_ARGUMENTS,
                              "Unknown hook type: " + hook_type,
                              ErrorSeverity::ERROR, "hooks_operations");
        }

        uninstallHook(type);
      } else {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS,
                            "Invalid hooks sub-command: " + subFlag,
                            ErrorSeverity::ERROR, "--hooks");
      }
    } else {
      listHooks();
    }
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("hooks operations")
}

void maintenanceOperations(int argc, const char *argv[], int &i) {
  try {
    if (i + 1 < argc) {
      std::string subFlag = argv[++i];

      if (subFlag == "gc") {
        garbageCollect();
      } else if (subFlag == "repack") {
        repackRepository();
      } else if (subFlag == "fsck") {
        fsckRepository();
      } else if (subFlag == "stats") {
        showRepositoryInfo();
      } else if (subFlag == "optimize") {
        optimizeRepository();
      } else if (subFlag == "analyze") {
        analyzeRepository();
      } else if (subFlag == "prune") {
        pruneObjects();
      } else {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS,
                            "Invalid maintenance sub-command: " + subFlag,
                            ErrorSeverity::ERROR, "--maintenance");
      }
    } else {
      showRepositoryInfo();
    }
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("maintenance operations")
}

void printHelp() {
  std::cout << "BitTrack - Lightweight Version Control\n\n";
  std::cout << "Usage:\n";
  std::cout << "  bittrack <command> [options]\n\n";
  std::cout << "Commands:\n";
  std::cout
      << "  init                        initialize a new BitTrack repository\n";
  std::cout << "  --status                    show staged and unstaged files\n";
  std::cout << "  --stage <file>              stage a file for commit\n";
  std::cout << "  --unstage <file>            unstage a file\n";
  std::cout
      << "  --commit                    commit staged files with a message\n";
  std::cout
      << "           -m <message>        commit with message (no prompt)\n";
  std::cout << "  --log                       show commit history\n";
  std::cout << "  --current-commit            show the current commit ID\n";
  std::cout << "  --staged-files-hashes       show hashes of staged files\n";
  std::cout << "  --remove-repo               delete the current BitTrack "
               "repository\n";
  std::cout << "  --branch -l                 list all branches\n";
  std::cout << "           -c <name>          create a new branch\n";
  std::cout << "           -r <name>          remove a branch\n";
  std::cout << "           -m <old> <new>     rename a branch\n";
  std::cout << "           -i <name>          show branch information\n";
  std::cout << "           -h <name>          show branch history\n";
  std::cout
      << "           -merge <src> <tgt> merge source into target branch\n";
  std::cout
      << "           -rebase <src> <tgt> rebase source onto target branch\n";
  std::cout << "  --checkout <name>           switch to a different branch\n";
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
  std::cout << "           analyze            analyze repository structure\n";
  std::cout << "           prune              prune unreachable objects\n";
  std::cout << "  --remote -v                 print current remote URL\n";
  std::cout << "           -s <url>           set remote URL\n";
  std::cout << "           -l                 list remote branches\n";
  std::cout << "           -d <branch>        delete remote branch\n";
  std::cout << "  --push [remote] [branch]    push current commit to remote\n";
  std::cout << "  --pull [remote] [branch]    pull changes from remote\n";
  std::cout
      << "  --clone <url> [path]        clone a repository from remote URL\n";
  std::cout
      << "  --fetch [remote]            fetch changes from remote repository\n";
  std::cout << "  --help                      show this help menu\n";
}

int main(int argc, const char *argv[]) {
  try {
    if (argc == 1 || std::string(argv[1]) == "--help") {
      printHelp();
      return 0;
    }

    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];

      if (arg == "init") {
        init();
        break;
      }
      if (!ErrorHandler::validateRepository()) {
        throw BitTrackError(ErrorCode::NOT_IN_REPOSITORY,
                            "Not inside a BitTrack repository",
                            ErrorSeverity::ERROR, "repository check");
      }

      if (arg == "--status") {
        status();
        break;
      } else if (arg == "--stage") {
        stageFile(argc, argv, i);
        break;
      } else if (arg == "--unstage") {
        unstageFiles(argc, argv, i);
        break;
      } else if (arg == "--commit") {
        try {
          std::string message;

          if (i + 1 < argc && std::string(argv[i + 1]) == "-m") {
            if (i + 2 < argc) {
              message = argv[i + 2];
              i += 2;
            } else {
              throw BitTrackError(ErrorCode::MISSING_ARGUMENTS,
                                  "Commit message required after -m flag",
                                  ErrorSeverity::ERROR, "--commit");
            }
          } else {
            std::cout << "message: ";
            getline(std::cin, message);
          }

          VALIDATE_COMMIT_MESSAGE(message);

          commitChanges("almuhidat", message);
        } catch (const BitTrackError &e) {
          ErrorHandler::printError(e);
          throw;
        }
        HANDLE_EXCEPTION("commit")
        break;
      } else if (arg == "--staged-files-hashes") {
        showStagedFilesHashes();
        break;
      } else if (arg == "--current-commit") {
        std::cout << getCurrentCommit() << std::endl;
        break;
      } else if (arg == "--log") {
        showCommitHistory();
        break;
      } else if (arg == "--remove-repo") {
        removeCurrentRepo();
        break;
      } else if (arg == "--branch") {
        branchOperations(argc, argv, i);
        break;
      } else if (arg == "--diff") {
        diffOperations(argc, argv, i);
        break;
      } else if (arg == "--stash") {
        stashOperations(argc, argv, i);
        break;
      } else if (arg == "--config") {
        configOperations(argc, argv, i);
        break;
      } else if (arg == "--tag") {
        tagOperations(argc, argv, i);
        break;
      } else if (arg == "--hooks") {
        hooksOperations(argc, argv, i);
        break;
      } else if (arg == "--maintenance") {
        maintenanceOperations(argc, argv, i);
        break;
      } else if (arg == "--remote") {
        remoteOperations(argc, argv, i);
        break;
      } else if (arg == "--checkout") {
        checkout(argv, i);
        break;
      } else if (arg == "--push") {
        try {
          std::string remote_name = "origin";
          std::string branch_name = getCurrentBranch();

          if (i + 1 < argc && argv[i + 1][0] != '-') {
            remote_name = argv[++i];
          }

          if (i + 1 < argc && argv[i + 1][0] != '-') {
            branch_name = argv[++i];
            VALIDATE_BRANCH_NAME(branch_name);
          }

          pushToRemote(remote_name, branch_name);
        } catch (const BitTrackError &e) {
          ErrorHandler::printError(e);
          throw;
        }
        HANDLE_EXCEPTION("push")
        break;
      } else if (arg == "--pull") {
        try {
          std::string remote_name = "origin";
          std::string branch_name = getCurrentBranch();

          if (i + 1 < argc && argv[i + 1][0] != '-') {
            remote_name = argv[++i];
          }

          if (i + 1 < argc && argv[i + 1][0] != '-') {
            branch_name = argv[++i];
            VALIDATE_BRANCH_NAME(branch_name);
          }

          pullFromRemote(remote_name, branch_name);
        } catch (const BitTrackError &e) {
          ErrorHandler::printError(e);
          throw;
        }
        HANDLE_EXCEPTION("pull")
        break;
      } else if (arg == "--clone") {
        try {
          VALIDATE_ARGS(argc, i + 2, "--clone");

          std::string url = argv[++i];
          std::string local_path = "";

          if (i + 1 < argc && argv[i + 1][0] != '-') {
            local_path = argv[++i];
          }

          cloneRepository(url, local_path);
        } catch (const BitTrackError &e) {
          ErrorHandler::printError(e);
          throw;
        }
        HANDLE_EXCEPTION("clone")
        break;
      } else if (arg == "--fetch") {
        try {
          std::string remote_name = "origin";

          if (i + 1 < argc && argv[i + 1][0] != '-') {
            remote_name = argv[++i];
          }

          fetchFromRemote(remote_name);
        } catch (const BitTrackError &e) {
          ErrorHandler::printError(e);
          throw;
        }
        HANDLE_EXCEPTION("fetch")
        break;
      } else {
        throw BitTrackError(ErrorCode::INVALID_ARGUMENTS,
                            "Unknown command: " + arg, ErrorSeverity::ERROR,
                            "command parsing");
      }
    }

    return 0;
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    return static_cast<int>(e.getCode());
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Unexpected error: " + std::string(e.what()),
                             ErrorSeverity::FATAL, "main");
    return static_cast<int>(ErrorCode::UNEXPECTED_EXCEPTION);
  } catch (...) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Unknown error occurred", ErrorSeverity::FATAL,
                             "main");
    return static_cast<int>(ErrorCode::UNEXPECTED_EXCEPTION);
  }
}
