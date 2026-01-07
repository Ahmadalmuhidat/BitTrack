#include "../include/branch.hpp"

std::string getCurrentBranch() {
  // Read the HEAD file to get the current branch name
  std::string line = ErrorHandler::safeReadFirstLine(".bittrack/HEAD");

  // Trim whitespace
  line.erase(line.find_last_not_of(" \t\r\n") + 1);
  return line;
}

std::vector<std::string> getBranchesList() {
  std::vector<std::string> branches;
  std::vector<std::filesystem::path> branchesPaths =
      ErrorHandler::safeListDirectoryFiles(".bittrack/refs/heads");

  for (const auto &branchPath : branchesPaths) {
    branches.push_back(branchPath.string());
  }

  return branches;
}

void copyCurrentCommitObjectToNewBranch(std::string new_branch_name) {
  // Get the current branch and its last commit hash
  std::string current_branch = getCurrentBranch();
  std::string last_commit_hash = getBranchLastCommitHash(current_branch);

  if (current_branch != "" && last_commit_hash != "") {
    // Copy the commit object to the new branch's directory
    insertCommitRecordToHistory(last_commit_hash, new_branch_name);
  }
}

bool attemptAutomaticMerge(const std::filesystem::path &first_file,
                           const std::filesystem::path &second_file,
                           const std::string &target_path) {
  try {
    std::string first_content = ErrorHandler::safeReadFile(first_file);
    std::string second_content = ErrorHandler::safeReadFile(second_file);

    // If the first file is empty, use the second file's content
    if (first_content.empty() && !second_content.empty()) {
      return ErrorHandler::safeWriteFile(target_path, second_content);
    }

    // If the second file is empty, use the first file's content
    else if (!first_content.empty() && second_content.empty()) {
      ErrorHandler::safeWriteFile(target_path, first_content);
      return true;
    }

    // If both files are non-empty, compare line by line after trimming
    // whitespace
    else if (!first_content.empty() && !second_content.empty()) {
      // Split contents into lines
      std::istringstream first_lines(first_content);
      std::istringstream second_lines(second_content);

      // Vectors to store lines
      std::vector<std::string> first_lines_vec;
      std::vector<std::string> second_lines_vec;

      // Read lines into vectors
      std::string line;
      while (std::getline(first_lines, line)) {
        first_lines_vec.push_back(line);
      }

      while (std::getline(second_lines, line)) {
        second_lines_vec.push_back(line);
      }

      // Compare line counts
      if (first_lines_vec.size() == second_lines_vec.size()) {
        // Compare lines after trimming whitespace
        bool identical = true;
        for (size_t i = 0; i < first_lines_vec.size(); ++i) {
          std::string trimmed1 = first_lines_vec[i];
          std::string trimmed2 = second_lines_vec[i];

          trimmed1.erase(0, trimmed1.find_first_not_of(" \t\r\n"));
          trimmed1.erase(trimmed1.find_last_not_of(" \t\r\n") + 1);
          trimmed2.erase(0, trimmed2.find_first_not_of(" \t\r\n"));
          trimmed2.erase(trimmed2.find_last_not_of(" \t\r\n") + 1);

          if (trimmed1 != trimmed2) {
            identical = false;
            break;
          }
        }

        // If all lines are identical after trimming, write to target
        if (identical) {
          return ErrorHandler::safeWriteFile(target_path, first_content);
        }
      }
    }

    return false;
  } catch (const std::exception &) {
    return false;
  }
}

bool hasUncommittedChanges() {
  // Check for staged or unstaged changes
  std::vector<std::string> staged_files = getStagedFiles();
  if (!staged_files.empty()) {
    return true;
  }

  // Check for unstaged changes
  std::vector<std::string> unstaged_files = getUnstagedFiles();
  if (!unstaged_files.empty()) {
    return true;
  }

  return false;
}

void backupUntrackedFiles() {
  // Create backup directory
  ErrorHandler::safeCreateDirectories(".bittrack/untracked_backup");
  std::string current_branch = getCurrentBranch();

  // Iterate through working directory to find untracked files
  for (const auto &entry : ErrorHandler::safeListDirectoryFiles(".")) {

    std::string file_path = entry.string();
    if (file_path.find(".bittrack") != std::string::npos ||
        file_path.find("build/") != std::string::npos ||
        file_path.find(".git/") != std::string::npos ||
        file_path.find(".DS_Store") != std::string::npos ||
        file_path.find(".github/") != std::string::npos) {
      continue;
    }

    // Check if the file is tracked in the current commit
    std::string current_commit = getBranchLastCommitHash(current_branch);
    if (current_commit.empty()) {
      continue;
    }

    // Construct the path to the tracked file in the commit
    std::string tracked_file_path =
        ".bittrack/objects/" + current_commit + "/" + file_path;
    if (std::filesystem::exists(tracked_file_path)) {
      continue;
    }

    // Backup the untracked file
    std::filesystem::path relative_path =
        std::filesystem::relative(file_path, ".");
    std::filesystem::path backup_path =
        ".bittrack/untracked_backup" / relative_path;

    // Create necessary directories in backup path
    if (!backup_path.parent_path().empty()) {
      ErrorHandler::safeCreateDirectories(backup_path.parent_path());
    }

    // Copy the untracked file to backup location
    ErrorHandler::safeCopyFile(file_path, backup_path);
  }
}

void restoreUntrackedFilesFromBackup() {
  if (std::filesystem::exists(".bittrack/untracked_backup")) {
    std::vector<std::filesystem::path> files =
        ErrorHandler::safeListDirectoryFiles(".bittrack/untracked_backup");

    // Iterate through backup directory to restore untracked files
    for (const auto &entry : files) {
      // restore the untracked file to its original location
      std::filesystem::path restore_path = std::filesystem::relative(
          entry.string(), ".bittrack/untracked_backup");

      // Create necessary directories in restore path
      if (!restore_path.parent_path().empty()) {
        ErrorHandler::safeCreateDirectories(restore_path.parent_path());
      }
      ErrorHandler::safeCopyFile(entry, restore_path);
    }
    // remove the backup directory after restoration
    ErrorHandler::safeRemoveFolder(".bittrack/untracked_backup");
  }
}

void restoreFilesFromCommit(const std::string &commit_path) {
  if (!std::filesystem::exists(commit_path)) {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND,
                             "Commit snapshot not found: " + commit_path,
                             ErrorSeverity::ERROR, "restore_files_from_commit");
    return;
  }

  // gather all tracked files from all commits
  std::unordered_set<std::string> all_tracked_files;
  for (const auto &entry :
       std::filesystem::directory_iterator(".bittrack/objects")) {
    // only consider directories (commits)
    if (entry.is_directory()) {
      // Iterate through files in the commit directory
      std::string commit_dir = entry.path().string();
      std::vector<std::filesystem::path> files =
          ErrorHandler::safeListDirectoryFiles(commit_dir);

      // Skip if it's the target commit
      for (const auto &file_entry : files) {
        // Get the relative path of the file within the commit
        std::filesystem::path relative_path =
            std::filesystem::relative(file_entry.string(), commit_dir);
        all_tracked_files.insert(relative_path.string());
      }
    }
  }

  // delete tracked files from working directory
  for (const auto &file_path : all_tracked_files) {
    ErrorHandler::safeRemoveFile(file_path);
  }

  // restore files from the commit snapshot
  std::vector<std::filesystem::path> files =
      ErrorHandler::safeListDirectoryFiles(commit_path);
  for (const auto &entry : files) {
    // Get the relative path of the file within the commit
    std::filesystem::path relative_path =
        std::filesystem::relative(entry.string(), commit_path);
    std::filesystem::path working_file = relative_path;

    if (!working_file.parent_path().empty()) {
      ErrorHandler::safeCreateDirectories(working_file.parent_path());
    }
    // Copy the file to the working directory
    ErrorHandler::safeCopyFile(entry, working_file);
  }
}

void updateWorkingDirectory(const std::string &target_branch) {
  // Get the last commit hash of the target branch
  std::string target_commit = getBranchLastCommitHash(target_branch);
  if (target_commit.empty()) {
    ErrorHandler::printError(ErrorCode::NO_COMMITS_FOUND,
                             "No commits found in target branch: " +
                                 target_branch,
                             ErrorSeverity::ERROR, "update_working_directory");
    return;
  }

  std::string commit_path = ".bittrack/objects/" + target_commit;
  bool has_untracked = false;

  // Check for untracked files in the working directory
  std::vector<std::filesystem::path> files =
      ErrorHandler::safeListDirectoryFiles(".");
  for (const auto &entry : files) {
    if (entry.string().find(".bittrack") == std::string::npos &&
        entry.string().find("build/") == std::string::npos &&
        entry.string().find(".git/") == std::string::npos &&
        entry.string().find(".DS_Store") == std::string::npos &&
        entry.string().find(".github/") == std::string::npos) {
      has_untracked = true;
      break;
    }
  }

  // Backup untracked files if any
  if (has_untracked) {
    backupUntrackedFiles();
  }

  // restore files from the target commit
  restoreFilesFromCommit(commit_path);

  // restore untracked files back to working directory
  if (has_untracked) {
    restoreUntrackedFilesFromBackup();
  }
}

void renameBranch(const std::string &old_name, const std::string &new_name) {
  // Get the list of branches
  std::vector<std::string> branches = getBranchesList();

  // Check if old branch exists and new branch does not exist
  if (std::find(branches.begin(), branches.end(), old_name) == branches.end()) {
    ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND,
                             "Branch '" + old_name + "' does not exist",
                             ErrorSeverity::ERROR, "rename_branch");
    return;
  }

  // Check if new branch name already exists
  if (std::find(branches.begin(), branches.end(), new_name) != branches.end()) {
    ErrorHandler::printError(ErrorCode::BRANCH_ALREADY_EXISTS,
                             "Branch '" + new_name + "' already exists",
                             ErrorSeverity::ERROR, "rename_branch");
    return;
  }

  // rename the branch file
  std::string old_file = ".bittrack/refs/heads/" + old_name;
  std::string new_file = ".bittrack/refs/heads/" + new_name;

  // perform the rename operation
  if (std::rename(old_file.c_str(), new_file.c_str()) != 0) {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR,
                             "Failed to rename branch file",
                             ErrorSeverity::ERROR, "rename_branch");
    return;
  }

  // rename the branch object directory if it exists
  std::string old_dir = ".bittrack/objects/" + old_name;
  std::string new_dir = ".bittrack/objects/" + new_name;

  // Check if old directory exists before renaming
  if (std::filesystem::exists(old_dir)) {
    // perform the directory rename operation
    ErrorHandler::safeRename(old_dir, new_dir);
  }

  // update HEAD if the current branch was renamed
  if (getCurrentBranch() == old_name) {
    ErrorHandler::safeWriteFile(".bittrack/HEAD", new_name + "\n");
  }

  std::cout << "Renamed branch '" << old_name << "' to '" << new_name << "'"
            << std::endl;
}

void showBranchInfo(const std::string &branch_name) {
  // Get the list of branches
  std::vector<std::string> branches = getBranchesList();

  // Check if the branch exists
  if (std::find(branches.begin(), branches.end(), branch_name) ==
      branches.end()) {
    ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND,
                             "Branch '" + branch_name + "' does not exist",
                             ErrorSeverity::ERROR, "show_branch_info");
    return;
  }

  // gather branch information
  std::string current_branch = getCurrentBranch();
  std::string last_commit = getBranchLastCommitHash(branch_name);

  std::cout << "Branch: " << branch_name << std::endl;
  std::cout << "  Current: " << (branch_name == current_branch ? "Yes" : "No")
            << std::endl;
  std::cout << "  Last commit: " << (last_commit.empty() ? "None" : last_commit)
            << std::endl;

  // Read full history file safely
  std::string history_content =
      ErrorHandler::safeReadFile(".bittrack/commits/history");

  // If file missing or failed to open, safeReadFile() already reported error
  if (history_content.empty()) {
    std::cout << "  Commits: 0" << std::endl;
    return;
  }

  // Parse history lines
  int commit_count = 0;
  std::istringstream history_stream(history_content);
  std::string line;

  while (std::getline(history_stream, line)) {
    std::istringstream iss(line);
    std::string commit_hash, branch;

    if (iss >> commit_hash >> branch && branch == branch_name) {
      commit_count++;
    }
  }

  std::cout << "  Commits: " << commit_count << std::endl;
}

void addBranch(const std::string &branch_name) {
  std::vector<std::string> branches = getBranchesList();

  // Check if branch already exists
  if (std::find(branches.begin(), branches.end(), branch_name) ==
      branches.end()) {
    // Create new branch file with current commit hash
    std::string current_branch = getCurrentBranch();
    std::string current_commit = getBranchLastCommitHash(current_branch);

    // Create the branch file
    ErrorHandler::safeWriteFile(".bittrack/refs/heads/" + branch_name,
                                current_commit);

    // add the current commit to the new branch's history
    if (!current_commit.empty()) {
      insertCommitRecordToHistory(current_commit, branch_name);
    }
  } else {
    ErrorHandler::printError(ErrorCode::BRANCH_ALREADY_EXISTS,
                             branch_name + " is already there",
                             ErrorSeverity::ERROR, "add_branch");
  }
}

void removeBranch(const std::string &branch_name) {
  // Get the list of branches
  std::vector<std::string> branches = getBranchesList();

  // Check if the branch exists
  if (std::find(branches.begin(), branches.end(), branch_name) ==
      branches.end()) {
    ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND,
                             branch_name + " is not found",
                             ErrorSeverity::ERROR, "remove_branch");
    return;
  }

  // prevent deletion of the current branch
  std::string current_branch = getCurrentBranch();

  // Check if trying to delete the current branch
  if (current_branch == branch_name) {
    ErrorHandler::printError(ErrorCode::CANNOT_DELETE_CURRENT_BRANCH,
                             "Cannot delete the current branch. Please switch "
                             "to another branch first",
                             ErrorSeverity::ERROR, "remove_branch");
    return;
  }

  // cleanup commits unique to the branch
  cleanupBranchCommits(branch_name);

  // delete the branch file
  std::string branch_file = ".bittrack/refs/heads/" + branch_name;
  if (!ErrorHandler::safeRemoveFile(branch_file)) {
    return;
  }

  std::cout << "Branch '" << branch_name << "' has been removed." << std::endl;
}

void switchBranch(const std::string &branch_name) {
  // Get the list of branches
  std::vector<std::string> branches = getBranchesList();

  // Check if the branch exists
  if (std::find(branches.begin(), branches.end(), branch_name) ==
      branches.end()) {
    ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND,
                             "you must create the branch first",
                             ErrorSeverity::ERROR, "switch_branch");
    return;
  }
  // Check if already in the target branch
  else if (getCurrentBranch() == branch_name) {
    std::cout << "you are already in " << branch_name << std::endl;
    return;
  }

  // Check for uncommitted changes
  if (!ErrorHandler::validateNoUncommittedChanges()) {
    // warn the user about uncommitted changes
    std::cout << "Warning: You have uncommitted changes. Switching branches "
                 "may overwrite your changes."
              << std::endl;
    std::cout << "Staged files: ";
    std::vector<std::string> staged = getStagedFiles();
    for (const auto &file : staged) {
      std::cout << file << " ";
    }
    std::cout << std::endl;

    std::cout << "Unstaged files: ";
    std::vector<std::string> unstaged = getUnstagedFiles();
    for (const auto &file : unstaged) {
      std::cout << file << " ";
    }
    std::cout << std::endl;

    // If the user does not confirm, cancel the branch switch
    // else proceed with the branch switch and use temprorary backup for
    // uncommitted changes
    std::cout << "Do you want to continue? (y/N): ";
    std::string response;
    std::getline(std::cin, response);
    if (response != "y" && response != "Y") {
      std::cout << "Branch switch cancelled." << std::endl;
      return;
    }
  }

  // use backup and restore mechanism to switch branches safely
  updateWorkingDirectory(branch_name);

  ErrorHandler::safeWriteFile(".bittrack/HEAD", branch_name + "\n");

  std::cout << "Switched to branch: " << branch_name << std::endl;
}

bool branchExists(const std::string &branch_name) {
  std::vector<std::string> branches = getBranchesList();
  return std::find(branches.begin(), branches.end(), branch_name) !=
         branches.end();
}

std::string getBranchLastCommitHash(const std::string &branch_name) {
  // Read the branch file to get the last commit hash
  std::string branch_path = ".bittrack/refs/heads/" + branch_name;
  return ErrorHandler::safeReadFirstLine(branch_path);
}

void rebaseBranch(const std::string &source_branch,
                  const std::string &target_branch) {
  try {
    // validate branches and working directory state
    if (!branchExists(source_branch)) {
      ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND,
                               "Source branch '" + source_branch +
                                   "' does not exist",
                               ErrorSeverity::ERROR, "rebase_branch");
      return;
    }

    if (!branchExists(target_branch)) {
      ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND,
                               "Target branch '" + target_branch +
                                   "' does not exist",
                               ErrorSeverity::ERROR, "rebase_branch");
      return;
    }

    // Check for uncommitted changes
    if (!ErrorHandler::validateNoUncommittedChanges()) {
      ErrorHandler::printError(ErrorCode::UNCOMMITTED_CHANGES,
                               "You have uncommitted changes. Please commit or "
                               "stash them before rebasing",
                               ErrorSeverity::ERROR, "rebase_branch");
      return;
    }

    // Check if currently on source branch
    std::string current_branch = getCurrentBranch();
    if (current_branch != source_branch) {
      ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND,
                               "You must be on the source branch '" +
                                   source_branch + "' to rebase",
                               ErrorSeverity::ERROR, "rebase_branch");
      return;
    }

    // find common ancestor between source and target branches
    std::string common_ancestor =
        findCommonAncestor(source_branch, target_branch);
    if (common_ancestor.empty()) {
      ErrorHandler::printError(
          ErrorCode::INTERNAL_ERROR,
          "Could not find common ancestor between branches",
          ErrorSeverity::ERROR, "rebase_branch");
      return;
    }

    // Get commits to rebase from source branch
    std::vector<std::string> commits_to_rebase =
        getCommitChain(common_ancestor, source_branch);
    if (commits_to_rebase.empty()) {
      std::cout << "No commits to rebase. Source branch is already up to date."
                << std::endl;
      return;
    }

    std::cout << "Rebasing " << commits_to_rebase.size() << " commits from '"
              << source_branch << "' onto '" << target_branch << "'..."
              << std::endl;

    // Get last commit of target branch
    std::string target_commit = getBranchLastCommitHash(target_branch);
    if (target_commit.empty()) {
      ErrorHandler::printError(ErrorCode::NO_COMMITS_FOUND,
                               "Target branch has no commits",
                               ErrorSeverity::ERROR, "rebase_branch");
      return;
    }

    std::string backup_commit = getCurrentCommit();

    try {
      // perform the rebase operation
      switchBranch(target_branch);

      // apply each commit from source branch onto target branch
      for (const auto &commit_hash : commits_to_rebase) {
        // apply the commit
        if (!applyCommitDuringRebase(commit_hash)) {
          ErrorHandler::printError(ErrorCode::COMMIT_FAILED,
                                   "Failed to apply commit " + commit_hash +
                                       " during rebase",
                                   ErrorSeverity::ERROR, "rebase_branch");
          ErrorHandler::printError(ErrorCode::COMMIT_FAILED,
                                   "Rebase aborted. Use 'bittrack --branch "
                                   "-rebase-abort' to rollback",
                                   ErrorSeverity::ERROR, "rebase_branch");
          return;
        }
      }

      // after successful rebase, switch back to source branch
      switchBranch(source_branch);

      // update source branch to point to new HEAD
      std::string new_head = getCurrentCommit();
      ErrorHandler::safeWriteFile(".bittrack/refs/heads/" + source_branch,
                                  new_head + "\n");

      std::cout << "Rebase completed successfully!" << std::endl;
    } catch (const std::exception &e) {
      ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                               "Error during rebase: " + std::string(e.what()),
                               ErrorSeverity::ERROR, "rebase_branch");
      ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                               "Rolling back to previous state...",
                               ErrorSeverity::ERROR, "rebase_branch");

      // rollback to backup commit on source branch
      ErrorHandler::safeWriteFile(".bittrack/refs/heads/" + source_branch,
                                  backup_commit + "\n");

      // restore working directory to source branch state
      updateWorkingDirectory(source_branch);

      ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                               "Rollback completed", ErrorSeverity::ERROR,
                               "rebase_branch");
      throw;
    }
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error during rebase: " + std::string(e.what()),
                             ErrorSeverity::ERROR, "rebase_branch");
  }
}

void showBranchHistory(const std::string &branch_name) {
  // Check if branch exists
  if (!branchExists(branch_name)) {
    ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND,
                             "Branch '" + branch_name + "' does not exist",
                             ErrorSeverity::ERROR, "show_branch_history");
    return;
  }

  std::cout << "History for branch '" << branch_name << "':" << std::endl;

  // Read the commit history file
  std::string history_content =
      ErrorHandler::safeReadFile(".bittrack/commits/history");
  if (history_content.empty()) {
    std::cout << "No commit history found." << std::endl;
    return;
  }

  std::istringstream history_stream(history_content);
  std::string line;
  bool found_branch = false;

  // Display commits for the specified branch
  while (std::getline(history_stream, line)) {
    if (line.length() >= branch_name.length() &&
        line.substr(line.length() - branch_name.length()) == branch_name) {
      found_branch = true;
      std::cout << line << std::endl;
    }
  }

  if (!found_branch) {
    std::cout << "No commits found for branch '" << branch_name << "'"
              << std::endl;
  }
}

std::vector<std::string> getBranchCommits(const std::string &branch_name) {
  std::vector<std::string> commits;

  // retrieve all commit hashes associated with the specified branch
  std::string history_content =
      ErrorHandler::safeReadFile(".bittrack/commits/history");
  if (history_content.empty()) {
    return commits; // no history found
  }

  std::istringstream history_stream(history_content);
  std::string line;

  // Read through the history file line by line
  while (std::getline(history_stream, line)) {
    if (line.empty()) {
      continue;
    }

    // parse the line to extract commit hash and branch name
    std::istringstream iss(line);
    std::string commit_hash, branch;
    if (iss >> commit_hash >> branch && branch == branch_name) {
      commits.push_back(commit_hash);
    }
  }

  return commits;
}

void cleanupBranchCommits(const std::string &branch_name) {
  try {
    std::cout << "Cleaning up commits for branch '" << branch_name << "'..."
              << std::endl;

    // Get all commits for the specified branch
    std::vector<std::string> branch_commits = getBranchCommits(branch_name);

    // Check if there are any commits to clean up
    if (branch_commits.empty()) {
      std::cout << "No commits found for branch '" << branch_name << "'"
                << std::endl;
      return;
    }

    // identify unique commits not present in other branches
    std::vector<std::string> unique_commits;
    std::vector<std::string> all_branches = getBranchesList();

    // Check each commit in the branch
    for (const auto &commit_hash : branch_commits) {
      bool is_unique = true;

      // Check if the commit exists in other branches
      for (const auto &other_branch : all_branches) {
        if (other_branch == branch_name) {
          continue;
        }

        // Get commits for the other branch
        std::vector<std::string> other_commits = getBranchCommits(other_branch);

        // Check if the commit is found in the other branch
        if (std::find(other_commits.begin(), other_commits.end(),
                      commit_hash) != other_commits.end()) {
          is_unique = false;
          break;
        }
      }

      // If the commit is unique to the branch, add it to the list
      if (is_unique) {
        unique_commits.push_back(commit_hash);
      }
    }

    std::cout << "Found " << unique_commits.size()
              << " unique commits to clean up" << std::endl;

    for (const auto &commit_hash : unique_commits) {
      // remove commit object directory
      std::string commit_dir = ".bittrack/objects/" + commit_hash;
      if (std::filesystem::exists(commit_dir)) {
        ErrorHandler::safeRemoveFolder(commit_dir);
        std::cout << "  Removed commit object: " << commit_hash << std::endl;
      }

      // remove commit log file
      std::string commit_log = ".bittrack/commits/" + commit_hash;
      if (std::filesystem::exists(commit_log)) {
        ErrorHandler::safeRemoveFile(commit_log);
        std::cout << "  Removed commit log: " << commit_hash << std::endl;
      }
    }

    // update the commit history file to remove entries for the branch
    std::string history_file_content =
        ErrorHandler::safeReadFile(".bittrack/commits/history");
    std::istringstream history_file(history_file_content);
    std::vector<std::string> remaining_lines;

    // Read through the history file line by line
    std::string line;
    while (std::getline(history_file, line)) {
      if (line.empty()) {
        continue;
      }

      // parse the line to extract commit hash and branch name
      std::istringstream iss(line);
      std::string commit_hash, branch;
      if (iss >> commit_hash >> branch) {
        if (branch != branch_name) {
          remaining_lines.push_back(line);
        }
      }
    }

    // write the updated history back to the file
    std::ofstream out_history_file(".bittrack/commits/history");
    for (const auto &remaining_line : remaining_lines) {
      out_history_file << remaining_line << std::endl;
    }
    out_history_file.close();

    std::cout << "  Removed " << branch_commits.size()
              << " commit records from history" << std::endl;

    std::cout << "Commit cleanup completed for branch '" << branch_name << "'"
              << std::endl;
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error during commit cleanup: " +
                                 std::string(e.what()),
                             ErrorSeverity::ERROR, "cleanup_branch_commits");
  }
}

std::string findCommonAncestor(const std::string &branch1,
                               const std::string &branch2) {
  try {
    // Get commits for both branches
    std::vector<std::string> commits1 = getBranchCommits(branch1);
    std::vector<std::string> commits2 = getBranchCommits(branch2);

    if (commits1.empty() || commits2.empty()) {
      return "";
    }

    // find the first common commit in both branches
    for (const auto &commit1 : commits1) {
      // Check if commit1 exists in commits2
      if (std::find(commits2.begin(), commits2.end(), commit1) !=
          commits2.end()) {
        return commit1;
      }
    }

    return "";
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error finding common ancestor: " +
                                 std::string(e.what()),
                             ErrorSeverity::ERROR, "find_common_ancestor");
    return "";
  }
}

std::vector<std::string> getCommitChain(const std::string &from_commit,
                                        const std::string &to_commit) {
  std::vector<std::string> commit_chain;
  try {
    // Get all commits from the to_commit branch
    std::vector<std::string> all_commits = getBranchCommits(to_commit);

    // find the position of from_commit in the list
    if (all_commits.empty()) {
      return commit_chain;
    }

    // locate from_commit in all_commits
    auto from_it =
        std::find(all_commits.begin(), all_commits.end(), from_commit);
    if (from_it == all_commits.end()) {
      return all_commits;
    }

    // collect commits after from_commit
    for (auto it = from_it + 1; it != all_commits.end(); ++it) {
      commit_chain.push_back(*it);
    }

    return commit_chain;
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error getting commit chain: " +
                                 std::string(e.what()),
                             ErrorSeverity::ERROR, "get_commit_chain");
    return commit_chain;
  }
}

bool applyCommitDuringRebase(const std::string &commit_hash) {
  try {
    // Read the commit details from the commit file
    std::string file_content =
        ErrorHandler::safeReadFile(".bittrack/commits/" + commit_hash);
    std::istringstream file_stream(file_content);

    std::string line;
    std::string author, message, timestamp;
    std::vector<std::string> files;
    std::vector<std::string> deleted_files;

    // Read through the commit file line by line
    while (std::getline(file_stream, line)) {
      if (line.find("Author: ") == 0) {
        author = line.substr(8);
      } else if (line.find("Message: ") == 0) {
        message = line.substr(9);
      } else if (line.find("Timestamp: ") == 0) {
        timestamp = line.substr(11);
      } else if (line.find("Files: ") == 0) {
        std::string files_str = line.substr(7);
        std::istringstream iss(files_str);
        std::string file;
        while (iss >> file) {
          files.push_back(file);
        }
      } else if (line.find("Deleted: ") == 0) {
        std::string deleted_str = line.substr(9);
        std::istringstream iss(deleted_str);
        std::string file;
        while (iss >> file) {
          deleted_files.push_back(file);
        }
      }
    }

    // apply the commit changes to the working directory
    for (const auto &file : files) {
      // Copy the file from the commit object to the working directory
      std::string source_file = ".bittrack/objects/" + commit_hash + "/" + file;
      std::filesystem::path file_path(file);
      ErrorHandler::safeCreateDirectories(file_path.parent_path());
      ErrorHandler::safeCopyFile(source_file, file);
      stage(file);
    }

    // handle deleted files
    for (const auto &file : deleted_files) {
      if (ErrorHandler::safeRemoveFile(file)) {
        stage(file + " (deleted)");
      }
    }

    // Create a new commit with the same author and message
    commitChanges(author, message);

    // verify the new commit was created
    std::string new_commit_hash = getCurrentCommit();
    if (new_commit_hash.empty()) {
      ErrorHandler::printError(
          ErrorCode::COMMIT_FAILED, "Failed to create new commit during rebase",
          ErrorSeverity::ERROR, "apply_commit_during_rebase");
      return false;
    }

    return true;
  } catch (const std::exception &e) {
    ErrorHandler::printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Error applying commit during rebase: " + std::string(e.what()),
        ErrorSeverity::ERROR, "apply_commit_during_rebase");
    return false;
  }
}
