#include "../include/branch.hpp"

std::string get_current_branch()
{
  // read the HEAD file to get the current branch name
  std::ifstream headFile(".bittrack/HEAD");

  // check if the file opened successfully
  if (!headFile.is_open())
  {
    ErrorHandler::printError(ErrorCode::FILE_READ_ERROR, "Could not open HEAD file", ErrorSeverity::ERROR, "get_current_branch");
    return "";
  }

  // read the first line
  std::string line;
  std::getline(headFile, line);
  headFile.close();

  // trim whitespace
  line.erase(line.find_last_not_of(" \t\r\n") + 1);
  return line;
}

std::vector<std::string> get_branches_list()
{
  // list all branch names by reading the .bittrack/refs/heads directory
  std::vector<std::string> branchesList;
  const std::filesystem::path prefix = ".bittrack/refs/heads";

  // check if the directory exists
  if (!std::filesystem::exists(prefix))
  {
    return branchesList;
  }

  // iterate through the directory and collect branch names
  for (const auto &entry : std::filesystem::recursive_directory_iterator(prefix))
  {
    if (entry.is_regular_file())
    {
      // get the relative path from the prefix
      std::filesystem::path relativePath = std::filesystem::relative(entry.path(), prefix);
      branchesList.push_back(relativePath.string());
    }
  }

  return branchesList;
}

void copy_current_commit_object_to_branch(std::string new_branch_name)
{
  // get the current branch and its last commit hash
  std::string current_branch = get_current_branch();
  std::string last_commit_hash = get_branch_last_commit_hash(current_branch);

  if (current_branch != "" && last_commit_hash != "")
  {
    // copy the commit object to the new branch's directory
    insert_commit_record_to_history(last_commit_hash, new_branch_name);
  }
}

bool attempt_automatic_merge(const std::filesystem::path &first_file, const std::filesystem::path &second_file, const std::string &target_path)
{
  try
  {
    // Open both files and read their contents
    std::ifstream first_stream(first_file);
    std::ifstream second_stream(second_file);

    if (!first_stream.is_open() || !second_stream.is_open())
    {
      return false;
    }

    // Read contents into strings
    std::string first_content((std::istreambuf_iterator<char>(first_stream)), std::istreambuf_iterator<char>());
    std::string second_content((std::istreambuf_iterator<char>(second_stream)), std::istreambuf_iterator<char>());

    // Close the file streams
    first_stream.close();
    second_stream.close();

    // If the first file is empty, use the second file's content
    if (first_content.empty() && !second_content.empty())
    {
      std::ofstream target_stream(target_path);
      if (target_stream.is_open())
      {
        target_stream << second_content;
        target_stream.close();
        return true;
      }
    }
    // If the second file is empty, use the first file's content
    else if (!first_content.empty() && second_content.empty())
    {
      std::ofstream target_stream(target_path);
      if (target_stream.is_open())
      {
        target_stream << first_content;
        target_stream.close();
        return true;
      }
    }
    // If both files are non-empty, compare line by line after trimming whitespace
    else if (!first_content.empty() && !second_content.empty())
    {
      // Split contents into lines
      std::istringstream first_lines(first_content);
      std::istringstream second_lines(second_content);

      // Store lines in vectors
      std::vector<std::string> first_lines_vec;
      std::vector<std::string> second_lines_vec;

      // Read lines into vectors
      std::string line;
      while (std::getline(first_lines, line))
      {
        first_lines_vec.push_back(line);
      }
      while (std::getline(second_lines, line))
      {
        second_lines_vec.push_back(line);
      }

      // Compare line counts
      if (first_lines_vec.size() == second_lines_vec.size())
      {
        // Compare lines after trimming whitespace
        bool identical = true;
        for (size_t i = 0; i < first_lines_vec.size(); ++i)
        {
          std::string trimmed1 = first_lines_vec[i];
          std::string trimmed2 = second_lines_vec[i];

          trimmed1.erase(0, trimmed1.find_first_not_of(" \t\r\n"));
          trimmed1.erase(trimmed1.find_last_not_of(" \t\r\n") + 1);
          trimmed2.erase(0, trimmed2.find_first_not_of(" \t\r\n"));
          trimmed2.erase(trimmed2.find_last_not_of(" \t\r\n") + 1);

          if (trimmed1 != trimmed2)
          {
            identical = false;
            break;
          }
        }
        // If all lines are identical after trimming, write to target
        if (identical)
        {
          std::ofstream target_stream(target_path);
          if (target_stream.is_open())
          {
            target_stream << first_content;
            target_stream.close();
            return true;
          }
        }
      }
    }
    return false;
  }
  catch (const std::exception &)
  {
    return false;
  }
}

bool has_uncommitted_changes()
{
  // check for staged or unstaged changes
  std::vector<std::string> staged_files = get_staged_files();
  if (!staged_files.empty())
  {
    return true;
  }

  // check for unstaged changes
  std::vector<std::string> unstaged_files = get_unstaged_files();
  if (!unstaged_files.empty())
  {
    return true;
  }

  return false;
}

void backup_untracked_files()
{
  // create backup directory
  std::filesystem::create_directories(".bittrack/untracked_backup");
  std::string current_branch = get_current_branch();

  // iterate through working directory to find untracked files
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    // skip directories and special files
    if (entry.is_regular_file())
    {
      std::string file_path = entry.path().string();

      if (file_path.find(".bittrack") != std::string::npos ||
          file_path.find("build/") != std::string::npos ||
          file_path.find(".git/") != std::string::npos ||
          file_path.find(".DS_Store") != std::string::npos ||
          file_path.find(".github/") != std::string::npos)
      {
        continue;
      }

      // check if the file is tracked in the current commit
      std::string current_commit = get_branch_last_commit_hash(current_branch);
      if (current_commit.empty())
      {
        continue;
      }

      // construct the path to the tracked file in the commit
      std::string tracked_file_path = ".bittrack/objects/" + current_commit + "/" + file_path;
      if (std::filesystem::exists(tracked_file_path))
      {
        continue;
      }

      try
      {
        // backup the untracked file
        std::filesystem::path relative_path = std::filesystem::relative(file_path, ".");
        std::filesystem::path backup_path = ".bittrack/untracked_backup" / relative_path;

        // create necessary directories in backup path
        if (!backup_path.parent_path().empty())
        {
          std::filesystem::create_directories(backup_path.parent_path());
        }
        // copy the untracked file to backup location
        std::filesystem::copy_file(file_path, backup_path, std::filesystem::copy_options::overwrite_existing);
      }
      catch (const std::filesystem::filesystem_error &e)
      {
        continue;
      }
    }
  }
}

void restore_untracked_files_from_backup()
{
  if (std::filesystem::exists(".bittrack/untracked_backup"))
  {
    // iterate through backup directory to restore untracked files
    for (const auto &entry : std::filesystem::recursive_directory_iterator(".bittrack/untracked_backup"))
    {
      if (entry.is_regular_file())
      {
        // restore the untracked file to its original location
        std::filesystem::path restore_path = std::filesystem::relative(entry.path(), ".bittrack/untracked_backup");

        // create necessary directories in restore path
        if (!restore_path.parent_path().empty())
        {
          std::filesystem::create_directories(restore_path.parent_path());
        }
        std::filesystem::copy_file(entry.path(), restore_path, std::filesystem::copy_options::overwrite_existing);
      }
    }
    // remove the backup directory after restoration
    std::filesystem::remove_all(".bittrack/untracked_backup");
  }
}

void restore_files_from_commit(const std::string &commit_path)
{
  if (!std::filesystem::exists(commit_path))
  {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND, "Commit snapshot not found: " + commit_path, ErrorSeverity::ERROR, "restore_files_from_commit");
    return;
  }

  // gather all tracked files from all commits
  std::unordered_set<std::string> all_tracked_files;
  for (const auto &entry : std::filesystem::directory_iterator(".bittrack/objects"))
  {
    // only consider directories (commits)
    if (entry.is_directory())
    {
      // iterate through files in the commit directory
      std::string commit_dir = entry.path().string();

      // skip if it's the target commit
      for (const auto &file_entry : std::filesystem::recursive_directory_iterator(commit_dir))
      {
        if (file_entry.is_regular_file())
        {
          // get the relative path of the file within the commit
          std::filesystem::path relative_path = std::filesystem::relative(file_entry.path(), commit_dir);
          all_tracked_files.insert(relative_path.string());
        }
      }
    }
  }

  // delete tracked files from working directory
  for (const auto &file_path : all_tracked_files)
  {
    if (std::filesystem::exists(file_path))
    {
      std::filesystem::remove(file_path);
    }
  }

  // restore files from the commit snapshot
  for (const auto &entry : std::filesystem::recursive_directory_iterator(commit_path))
  {
    if (entry.is_regular_file())
    {
      // get the relative path of the file within the commit
      std::filesystem::path relative_path = std::filesystem::relative(entry.path(), commit_path);
      std::filesystem::path working_file = relative_path;

      if (!working_file.parent_path().empty())
      {
        std::filesystem::create_directories(working_file.parent_path());
      }
      // copy the file to the working directory
      std::filesystem::copy_file(entry.path(), working_file, std::filesystem::copy_options::overwrite_existing);
    }
  }
}

void update_working_directory(const std::string &target_branch)
{
  // get the last commit hash of the target branch
  std::string target_commit = get_branch_last_commit_hash(target_branch);
  if (target_commit.empty())
  {
    ErrorHandler::printError(ErrorCode::NO_COMMITS_FOUND, "No commits found in target branch: " + target_branch, ErrorSeverity::ERROR, "update_working_directory");
    return;
  }

  std::string commit_path = ".bittrack/objects/" + target_commit;
  bool has_untracked = false;

  // check for untracked files in the working directory
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    // skip directories and special files
    if (entry.is_regular_file())
    {
      std::string file_path = entry.path().string();
      if (file_path.find(".bittrack") == std::string::npos &&
          file_path.find("build/") == std::string::npos &&
          file_path.find(".git/") == std::string::npos &&
          file_path.find(".DS_Store") == std::string::npos &&
          file_path.find(".github/") == std::string::npos)
      {
        has_untracked = true;
        break;
      }
    }
  }

  // backup untracked files if any
  if (has_untracked)
  {
    backup_untracked_files();
  }

  // restore files from the target commit
  restore_files_from_commit(commit_path);

  // restore untracked files back to working directory
  if (has_untracked)
  {
    restore_untracked_files_from_backup();
  }
}

void rename_branch(const std::string &old_name, const std::string &new_name)
{
  // get the list of branches
  std::vector<std::string> branches = get_branches_list();

  // check if old branch exists and new branch does not exist
  if (std::find(branches.begin(), branches.end(), old_name) == branches.end())
  {
    ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND, "Branch '" + old_name + "' does not exist", ErrorSeverity::ERROR, "rename_branch");
    return;
  }

  // check if new branch name already exists
  if (std::find(branches.begin(), branches.end(), new_name) != branches.end())
  {
    ErrorHandler::printError(ErrorCode::BRANCH_ALREADY_EXISTS, "Branch '" + new_name + "' already exists", ErrorSeverity::ERROR, "rename_branch");
    return;
  }

  // rename the branch file
  std::string old_file = ".bittrack/refs/heads/" + old_name;
  std::string new_file = ".bittrack/refs/heads/" + new_name;

  // perform the rename operation
  if (std::rename(old_file.c_str(), new_file.c_str()) != 0)
  {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR, "Failed to rename branch file", ErrorSeverity::ERROR, "rename_branch");
    return;
  }

  // rename the branch object directory if it exists
  std::string old_dir = ".bittrack/objects/" + old_name;
  std::string new_dir = ".bittrack/objects/" + new_name;

  // check if old directory exists before renaming
  if (std::filesystem::exists(old_dir))
  {
    // perform the directory rename operation
    std::filesystem::rename(old_dir, new_dir);
  }

  // update HEAD if the current branch was renamed
  if (get_current_branch() == old_name)
  {
    std::ofstream headFile(".bittrack/HEAD", std::ios::trunc);
    headFile << new_name << std::endl;
    headFile.close();
  }

  std::cout << "Renamed branch '" << old_name << "' to '" << new_name << "'" << std::endl;
}

void show_branch_info(const std::string &branch_name)
{
  // get the list of branches
  std::vector<std::string> branches = get_branches_list();

  // check if the branch exists
  if (std::find(branches.begin(), branches.end(), branch_name) == branches.end())
  {
    ErrorHandler::printError(ErrorCode::BRANCH_NOT_FOUND, "Branch '" + branch_name + "' does not exist", ErrorSeverity::ERROR, "show_branch_info");
    return;
  }

  // gather branch information
  std::string current_branch = get_current_branch();
  std::string last_commit = get_branch_last_commit_hash(branch_name);

  std::cout << "Branch: " << branch_name << std::endl;
  std::cout << "  Current: " << (branch_name == current_branch ? "Yes" : "No") << std::endl;
  std::cout << "  Last commit: " << (last_commit.empty() ? "None" : last_commit) << std::endl;

  // count commits in the branch
  int commit_count = 0;
  std::ifstream history_file(".bittrack/commits/history");
  std::string line;

  // read through history file to count commits
  while (std::getline(history_file, line))
  {
    // skip empty lines
    std::istringstream iss(line);
    std::string commit_hash, branch;

    // check if the line corresponds to the branch
    if (iss >> commit_hash >> branch && branch == branch_name)
    {
      commit_count++;
    }
  }
  history_file.close();

  std::cout << "  Commits: " << commit_count << std::endl;
}

void add_branch(const std::string &branch_name)
{
  std::vector<std::string> branches = get_branches_list();

  // check if branch already exists
  if (std::find(branches.begin(), branches.end(), branch_name) == branches.end())
  {
    // create new branch file with current commit hash
    std::string current_branch = get_current_branch();
    std::string current_commit = get_branch_last_commit_hash(current_branch);

    // create the branch file
    std::ofstream newBranchHeadFile(".bittrack/refs/heads/" + branch_name);
    if (newBranchHeadFile.is_open())
    {
      // write the current commit hash to the new branch file
      newBranchHeadFile << current_commit << std::endl;
      newBranchHeadFile.close();
    }

    // add the current commit to the new branch's history
    if (!current_commit.empty())
    {
      insert_commit_record_to_history(current_commit, branch_name);
    }
  }
  else
  {
    std::cout << branch_name << " is already there" << std::endl;
  }
}

void remove_branch(const std::string &branch_name)
{
  // get the list of branches
  std::vector<std::string> branches = get_branches_list();

  // check if the branch exists
  if (std::find(branches.begin(), branches.end(), branch_name) == branches.end())
  {
    std::cout << branch_name << " is not found there" << std::endl;
    return;
  }

  // prevent deletion of the current branch
  std::string current_branch = get_current_branch();

  // check if trying to delete the current branch
  if (current_branch == branch_name)
  {
    std::cout << "Cannot delete the current branch. Please switch to another branch first." << std::endl;
    return;
  }

  // cleanup commits unique to the branch
  cleanup_branch_commits(branch_name);

  // delete the branch file
  std::string branch_file = ".bittrack/refs/heads/" + branch_name;
  if (std::remove(branch_file.c_str()) != 0)
  {
    std::perror("Error deleting branch file");
    return;
  }

  std::cout << "Branch '" << branch_name << "' has been removed." << std::endl;
}

void switch_branch(const std::string &branch_name)
{
  // get the list of branches
  std::vector<std::string> branches = get_branches_list();

  // check if the branch exists
  if (std::find(branches.begin(), branches.end(), branch_name) == branches.end())
  {
    std::cout << "you must create the branch first" << std::endl;
    return;
  }
  // check if already in the target branch
  else if (get_current_branch() == branch_name)
  {
    std::cout << "you are already in " << branch_name << std::endl;
    return;
  }

  // check for uncommitted changes
  if (!ErrorHandler::validateNoUncommittedChanges())
  {
    // warn the user about uncommitted changes
    std::cout << "Warning: You have uncommitted changes. Switching branches may overwrite your changes." << std::endl;
    std::cout << "Staged files: ";
    std::vector<std::string> staged = get_staged_files();
    for (const auto &file : staged)
    {
      std::cout << file << " ";
    }
    std::cout << std::endl;

    std::cout << "Unstaged files: ";
    std::vector<std::string> unstaged = get_unstaged_files();
    for (const auto &file : unstaged)
    {
      std::cout << file << " ";
    }
    std::cout << std::endl;

    // if the user does not confirm, cancel the branch switch
    // else proceed with the branch switch and use temprorary backup for uncommitted changes
    std::cout << "Do you want to continue? (y/N): ";
    std::string response;
    std::getline(std::cin, response);
    if (response != "y" && response != "Y")
    {
      std::cout << "Branch switch cancelled." << std::endl;
      return;
    }
  }

  // use backup and restore mechanism to switch branches safely
  update_working_directory(branch_name);

  std::ofstream HeadFile(".bittrack/HEAD", std::ios::trunc);
  HeadFile << branch_name << std::endl;
  HeadFile.close();

  std::cout << "Switched to branch: " << branch_name << std::endl;
}

bool branch_exists(const std::string &branch_name)
{
  std::vector<std::string> branches = get_branches_list();
  return std::find(branches.begin(), branches.end(), branch_name) != branches.end();
}

std::string get_branch_last_commit_hash(const std::string &branch_name)
{
  // read the branch file to get the last commit hash
  std::string branch_path = ".bittrack/refs/heads/" + branch_name;
  if (!std::filesystem::exists(branch_path))
  {
    return "";
  }

  // open the branch file
  std::ifstream file(branch_path);
  std::string commit_hash;

  if (std::getline(file, commit_hash))
  {
    return commit_hash;
  }
  return "";
}

void rebase_branch(const std::string &source_branch, const std::string &target_branch)
{
  try
  {
    // validate branches and working directory state
    if (!branch_exists(source_branch))
    {
      std::cout << "Error: Source branch '" << source_branch << "' does not exist." << std::endl;
      return;
    }

    if (!branch_exists(target_branch))
    {
      std::cout << "Error: Target branch '" << target_branch << "' does not exist." << std::endl;
      return;
    }

    // check for uncommitted changes
    if (!ErrorHandler::validateNoUncommittedChanges())
    {
      std::cout << "Error: You have uncommitted changes. Please commit or stash them before rebasing." << std::endl;
      return;
    }

    // check if currently on source branch
    std::string current_branch = get_current_branch();
    if (current_branch != source_branch)
    {
      std::cout << "Error: You must be on the source branch '" << source_branch << "' to rebase." << std::endl;
      return;
    }

    // find common ancestor between source and target branches
    std::string common_ancestor = find_common_ancestor(source_branch, target_branch);
    if (common_ancestor.empty())
    {
      std::cout << "Error: Could not find common ancestor between branches." << std::endl;
      return;
    }

    // get commits to rebase from source branch
    std::vector<std::string> commits_to_rebase = get_commit_chain(common_ancestor, source_branch);
    if (commits_to_rebase.empty())
    {
      std::cout << "No commits to rebase. Source branch is already up to date." << std::endl;
      return;
    }

    std::cout << "Rebasing " << commits_to_rebase.size() << " commits from '" << source_branch << "' onto '" << target_branch << "'..." << std::endl;

    // get last commit of target branch
    std::string target_commit = get_branch_last_commit_hash(target_branch);
    if (target_commit.empty())
    {
      std::cout << "Error: Target branch has no commits." << std::endl;
      return;
    }

    std::string backup_commit = get_current_commit();

    try
    {
      // perform the rebase operation
      switch_branch(target_branch);

      // apply each commit from source branch onto target branch
      for (const auto &commit_hash : commits_to_rebase)
      {
        // apply the commit
        if (!apply_commit_during_rebase(commit_hash))
        {
          std::cout << "Error: Failed to apply commit " << commit_hash << " during rebase." << std::endl;
          std::cout << "Rebase aborted. Use 'bittrack --branch -rebase-abort' to rollback." << std::endl;
          return;
        }
      }

      // after successful rebase, switch back to source branch
      switch_branch(source_branch);

      // update source branch to point to new HEAD
      std::string new_head = get_current_commit();
      std::ofstream branch_file(".bittrack/refs/heads/" + source_branch);
      if (branch_file.is_open())
      {
        branch_file << new_head << std::endl;
        branch_file.close();
      }

      std::cout << "Rebase completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
      std::cout << "Error during rebase: " << e.what() << std::endl;
      std::cout << "Rolling back to previous state..." << std::endl;

      // rollback to backup commit on source branch
      std::ofstream branch_file(".bittrack/refs/heads/" + source_branch);
      if (branch_file.is_open())
      {
        branch_file << backup_commit << std::endl;
        branch_file.close();
      }

      // restore working directory to source branch state
      update_working_directory(source_branch);

      std::cout << "Rollback completed." << std::endl;
      throw;
    }
  }
  catch (const std::exception &e)
  {
    std::cout << "Error during rebase: " << e.what() << std::endl;
  }
}

void show_branch_history(const std::string &branch_name)
{
  try
  {
    // check if branch exists
    if (!branch_exists(branch_name))
    {
      std::cout << "Error: Branch '" << branch_name << "' does not exist." << std::endl;
      return;
    }

    std::cout << "History for branch '" << branch_name << "':" << std::endl;

    // read the commit history file
    std::ifstream history_file(".bittrack/commits/history");
    if (!history_file.is_open())
    {
      std::cout << "No commit history found." << std::endl;
      return;
    }

    // display commits for the specified branch
    std::string line;
    bool found_branch = false;
    while (std::getline(history_file, line))
    {
      // skip empty lines
      if (line.length() > branch_name.length() && line.substr(line.length() - branch_name.length()) == branch_name)
      {
        found_branch = true;
        std::cout << line << std::endl;
      }
    }

    // if no commits found for the branch
    if (!found_branch)
    {
      std::cout << "No commits found for branch '" << branch_name << "'" << std::endl;
    }
    history_file.close();
  }
  catch (const std::exception &e)
  {
    std::cout << "Error reading branch history: " << e.what() << std::endl;
  }
}

std::vector<std::string> get_branch_commits(const std::string &branch_name)
{
  // retrieve all commit hashes associated with the specified branch
  std::vector<std::string> commits;
  std::ifstream history_file(".bittrack/commits/history");

  // check if the history file opened successfully
  if (!history_file.is_open())
  {
    return commits;
  }

  // read through the history file line by line
  std::string line;
  while (std::getline(history_file, line))
  {
    if (line.empty())
    {
      continue;
    }

    // parse the line to extract commit hash and branch name
    std::istringstream iss(line);
    std::string commit_hash, branch;
    if (iss >> commit_hash >> branch && branch == branch_name)
    {
      commits.push_back(commit_hash);
    }
  }
  history_file.close();

  return commits;
}

void cleanup_branch_commits(const std::string &branch_name)
{
  try
  {
    std::cout << "Cleaning up commits for branch '" << branch_name << "'..." << std::endl;

    // get all commits for the specified branch
    std::vector<std::string> branch_commits = get_branch_commits(branch_name);

    // check if there are any commits to clean up
    if (branch_commits.empty())
    {
      std::cout << "No commits found for branch '" << branch_name << "'" << std::endl;
      return;
    }

    // identify unique commits not present in other branches
    std::vector<std::string> unique_commits;
    std::vector<std::string> all_branches = get_branches_list();

    // check each commit in the branch
    for (const auto &commit_hash : branch_commits)
    {
      bool is_unique = true;

      // check if the commit exists in other branches
      for (const auto &other_branch : all_branches)
      {
        if (other_branch == branch_name)
        {
          continue;
        }

        // get commits for the other branch
        std::vector<std::string> other_commits = get_branch_commits(other_branch);

        // check if the commit is found in the other branch
        if (std::find(other_commits.begin(), other_commits.end(), commit_hash) != other_commits.end())
        {
          is_unique = false;
          break;
        }
      }

      // if the commit is unique to the branch, add it to the list
      if (is_unique)
      {
        unique_commits.push_back(commit_hash);
      }
    }

    std::cout << "Found " << unique_commits.size() << " unique commits to clean up" << std::endl;

    for (const auto &commit_hash : unique_commits)
    {
      // remove commit object directory
      std::string commit_dir = ".bittrack/objects/" + commit_hash;
      if (std::filesystem::exists(commit_dir))
      {
        std::filesystem::remove_all(commit_dir);
        std::cout << "  Removed commit object: " << commit_hash << std::endl;
      }

      // remove commit log file
      std::string commit_log = ".bittrack/commits/" + commit_hash;
      if (std::filesystem::exists(commit_log))
      {
        std::filesystem::remove(commit_log);
        std::cout << "  Removed commit log: " << commit_hash << std::endl;
      }
    }

    // update the commit history file to remove entries for the branch
    std::ifstream history_file(".bittrack/commits/history");
    std::vector<std::string> remaining_lines;

    if (history_file.is_open())
    {
      // read through the history file line by line
      std::string line;
      while (std::getline(history_file, line))
      {
        if (line.empty())
        {
          continue;
        }

        // parse the line to extract commit hash and branch name
        std::istringstream iss(line);
        std::string commit_hash, branch;
        if (iss >> commit_hash >> branch)
        {
          if (branch != branch_name)
          {
            remaining_lines.push_back(line);
          }
        }
      }
      history_file.close();

      // write the updated history back to the file
      std::ofstream out_history_file(".bittrack/commits/history");
      for (const auto &remaining_line : remaining_lines)
      {
        out_history_file << remaining_line << std::endl;
      }
      out_history_file.close();

      std::cout << "  Removed " << branch_commits.size() << " commit records from history" << std::endl;
    }

    std::cout << "Commit cleanup completed for branch '" << branch_name << "'" << std::endl;
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error during commit cleanup: " + std::string(e.what()), ErrorSeverity::ERROR, "cleanup_branch_commits");
  }
}

std::string find_common_ancestor(const std::string &branch1, const std::string &branch2)
{
  try
  {
    // get commits for both branches
    std::vector<std::string> commits1 = get_branch_commits(branch1);
    std::vector<std::string> commits2 = get_branch_commits(branch2);

    if (commits1.empty() || commits2.empty())
    {
      return "";
    }

    // find the first common commit in both branches
    for (const auto &commit1 : commits1)
    {
      // check if commit1 exists in commits2
      if (std::find(commits2.begin(), commits2.end(), commit1) != commits2.end())
      {
        return commit1;
      }
    }

    return "";
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error finding common ancestor: " + std::string(e.what()), ErrorSeverity::ERROR, "find_common_ancestor");
    return "";
  }
}

std::vector<std::string> get_commit_chain(const std::string &from_commit, const std::string &to_commit)
{
  std::vector<std::string> commit_chain;

  try
  {
    // get all commits from the to_commit branch
    std::vector<std::string> all_commits = get_branch_commits(to_commit);

    // find the position of from_commit in the list
    if (all_commits.empty())
    {
      return commit_chain;
    }

    // locate from_commit in all_commits
    auto from_it = std::find(all_commits.begin(), all_commits.end(), from_commit);
    if (from_it == all_commits.end())
    {
      return all_commits;
    }

    // collect commits after from_commit
    for (auto it = from_it + 1; it != all_commits.end(); ++it)
    {
      commit_chain.push_back(*it);
    }

    return commit_chain;
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error getting commit chain: " + std::string(e.what()), ErrorSeverity::ERROR, "get_commit_chain");
    return commit_chain;
  }
}

bool apply_commit_during_rebase(const std::string &commit_hash)
{
  try
  {
    // read the commit details from the commit file
    std::string commit_file = ".bittrack/commits/" + commit_hash;
    if (!std::filesystem::exists(commit_file))
    {
      ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND, "Commit file not found: " + commit_hash, ErrorSeverity::ERROR, "apply_commit_during_rebase");
      return false;
    }

    // open the commit file
    std::ifstream file(commit_file);
    if (!file.is_open())
    {
      ErrorHandler::printError(ErrorCode::FILE_READ_ERROR, "Cannot open commit file: " + commit_hash, ErrorSeverity::ERROR, "apply_commit_during_rebase");
      return false;
    }

    // parse commit details
    std::string line;
    std::string author, message, timestamp;
    std::vector<std::string> files;
    std::vector<std::string> deleted_files;

    // read through the commit file line by line
    while (std::getline(file, line))
    {
      if (line.find("Author: ") == 0)
      {
        author = line.substr(8);
      }
      else if (line.find("Message: ") == 0)
      {
        message = line.substr(9);
      }
      else if (line.find("Timestamp: ") == 0)
      {
        timestamp = line.substr(11);
      }
      else if (line.find("Files: ") == 0)
      {
        std::string files_str = line.substr(7);
        std::istringstream iss(files_str);
        std::string file;
        while (iss >> file)
        {
          files.push_back(file);
        }
      }
      else if (line.find("Deleted: ") == 0)
      {
        std::string deleted_str = line.substr(9);
        std::istringstream iss(deleted_str);
        std::string file;
        while (iss >> file)
        {
          deleted_files.push_back(file);
        }
      }
    }
    file.close();

    // apply the commit changes to the working directory
    for (const auto &file : files)
    {
      // copy the file from the commit object to the working directory
      std::string source_file = ".bittrack/objects/" + commit_hash + "/" + file;
      if (std::filesystem::exists(source_file))
      {
        std::filesystem::path file_path(file);
        if (!file_path.parent_path().empty())
        {
          std::filesystem::create_directories(file_path.parent_path());
        }

        // copy the file to the working directory
        std::filesystem::copy_file(source_file, file, std::filesystem::copy_options::overwrite_existing);
        stage(file);
      }
    }

    // handle deleted files
    for (const auto &file : deleted_files)
    {
      // remove the file from the working directory if it exists
      if (std::filesystem::exists(file))
      {
        std::filesystem::remove(file);
        stage(file + " (deleted)");
      }
    }

    // create a new commit with the same author and message
    commit_changes(author, message);

    // verify the new commit was created
    std::string new_commit_hash = get_current_commit();
    if (new_commit_hash.empty())
    {
      ErrorHandler::printError(ErrorCode::COMMIT_FAILED, "Failed to create new commit during rebase", ErrorSeverity::ERROR, "apply_commit_during_rebase");
      return false;
    }

    return true;
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error applying commit during rebase: " + std::string(e.what()), ErrorSeverity::ERROR, "apply_commit_during_rebase");
    return false;
  }
}
