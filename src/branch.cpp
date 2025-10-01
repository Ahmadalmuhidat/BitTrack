#include "../include/branch.hpp"

std::string get_current_branch()
{
  std::ifstream headFile(".bittrack/HEAD");

  if (!headFile.is_open())
  {
    std::cerr << "Error: Could not open HEAD file." << std::endl;
    return "";
  }

  std::string line;
  std::getline(headFile, line);
  headFile.close();

  line.erase(line.find_last_not_of(" \t\r\n") + 1);
  return line;
}

std::vector<std::string> get_branches_list()
{
  std::vector<std::string> branchesList;
  const std::string prefix = ".bittrack/refs/heads/";

  if (!std::filesystem::exists(".bittrack/refs/heads"))
  {
    return branchesList;
  }

  for (const auto &entry : std::filesystem::recursive_directory_iterator(".bittrack/refs/heads"))
  {
    if (entry.is_regular_file())
    {
      std::string file_path = entry.path().string();
      branchesList.push_back(file_path.substr(prefix.length()));
    }
  }
  return branchesList;
}

void copy_current_commit_object_to_branch(std::string new_branch_name)
{
  std::string current_branch = get_current_branch();
  std::string last_commit_hash = get_branch_last_commit_hash(current_branch);

  if (current_branch != "" && last_commit_hash != "")
  {
    insert_commit_record_to_history(last_commit_hash, new_branch_name);
  }
}

void add_branch_implementaion(std::string name)
{
  std::vector<std::string> branches = get_branches_list();

  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  {
    std::string current_commit = get_branch_last_commit_hash(get_current_branch());

    std::ofstream newBranchHeadFile(".bittrack/refs/heads/" + name);
    if (newBranchHeadFile.is_open())
    {
      newBranchHeadFile << current_commit << std::endl;
      newBranchHeadFile.close();
    }

    if (!current_commit.empty())
    {
      insert_commit_record_to_history(current_commit, name);
    }
  }
  else
  {
    std::cout << name << " is already there" << std::endl;
  }
}

bool attempt_automatic_merge(const std::filesystem::path &first_file, const std::filesystem::path &second_file, const std::string &target_path)
{
  try
  {
    std::ifstream first_stream(first_file);
    std::ifstream second_stream(second_file);

    if (!first_stream.is_open() || !second_stream.is_open())
    {
      return false;
    }

    std::string first_content((std::istreambuf_iterator<char>(first_stream)), std::istreambuf_iterator<char>());
    std::string second_content((std::istreambuf_iterator<char>(second_stream)), std::istreambuf_iterator<char>());

    first_stream.close();
    second_stream.close();

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
    else if (!first_content.empty() && !second_content.empty())
    {
      std::istringstream first_lines(first_content);
      std::istringstream second_lines(second_content);

      std::vector<std::string> first_lines_vec;
      std::vector<std::string> second_lines_vec;

      std::string line;
      while (std::getline(first_lines, line))
      {
        first_lines_vec.push_back(line);
      }
      while (std::getline(second_lines, line))
      {
        second_lines_vec.push_back(line);
      }

      if (first_lines_vec.size() == second_lines_vec.size())
      {
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
  std::vector<std::string> staged_files = get_staged_files();
  if (!staged_files.empty())
  {
    return true;
  }

  std::vector<std::string> unstaged_files = get_unstaged_files();
  if (!unstaged_files.empty())
  {
    return true;
  }
  return false;
}

void backup_untracked_files()
{
  std::filesystem::create_directories(".bittrack/untracked_backup");

  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string file_path = entry.path().string();

      if (file_path.find(".bittrack") != std::string::npos || file_path.find("build/") != std::string::npos || file_path.find(".git/") != std::string::npos || file_path.find(".DS_Store") != std::string::npos || file_path.find(".github/") != std::string::npos)
      {
        continue;
      }

      std::string current_commit = get_branch_last_commit_hash(get_current_branch());
      if (current_commit.empty())
      {
        continue;
      }

      std::string tracked_file_path = ".bittrack/objects/" + current_commit + "/" + file_path;
      if (std::filesystem::exists(tracked_file_path))
      {
        continue;
      }

      try
      {
        std::filesystem::path relative_path = std::filesystem::relative(file_path, ".");
        std::filesystem::path backup_path = ".bittrack/untracked_backup" / relative_path;

        if (!backup_path.parent_path().empty())
        {
          std::filesystem::create_directories(backup_path.parent_path());
        }
        std::filesystem::copy_file(file_path, backup_path, std::filesystem::copy_options::overwrite_existing);
      }
      catch (const std::filesystem::filesystem_error &e)
      {
        continue;
      }
    }
  }
}

void restore_untracked_files()
{
  if (std::filesystem::exists(".bittrack/untracked_backup"))
  {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(".bittrack/untracked_backup"))
    {
      if (entry.is_regular_file())
      {
        std::filesystem::path restore_path = std::filesystem::relative(entry.path(), ".bittrack/untracked_backup");

        if (!restore_path.parent_path().empty())
        {
          std::filesystem::create_directories(restore_path.parent_path());
        }
        std::filesystem::copy_file(entry.path(), restore_path, std::filesystem::copy_options::overwrite_existing);
      }
    }
    std::filesystem::remove_all(".bittrack/untracked_backup");
  }
}

void restore_files_from_commit(const std::string &commit_path)
{
  if (!std::filesystem::exists(commit_path))
  {
    std::cerr << "Error: Commit snapshot not found: " << commit_path << std::endl;
    return;
  }

  std::unordered_set<std::string> all_tracked_files;
  for (const auto &entry : std::filesystem::directory_iterator(".bittrack/objects"))
  {
    if (entry.is_directory())
    {
      std::string commit_dir = entry.path().string();
      for (const auto &file_entry : std::filesystem::recursive_directory_iterator(commit_dir))
      {
        if (file_entry.is_regular_file())
        {
          std::filesystem::path relative_path = std::filesystem::relative(file_entry.path(), commit_dir);
          all_tracked_files.insert(relative_path.string());
        }
      }
    }
  }

  for (const auto &file_path : all_tracked_files)
  {
    if (std::filesystem::exists(file_path))
    {
      std::filesystem::remove(file_path);
    }
  }

  for (const auto &entry : std::filesystem::recursive_directory_iterator(commit_path))
  {
    if (entry.is_regular_file())
    {
      std::filesystem::path relative_path = std::filesystem::relative(entry.path(), commit_path);
      std::filesystem::path working_file = relative_path;

      if (!working_file.parent_path().empty())
      {
        std::filesystem::create_directories(working_file.parent_path());
      }
      std::filesystem::copy_file(entry.path(), working_file, std::filesystem::copy_options::overwrite_existing);
    }
  }
}

void update_working_directory(const std::string &target_branch)
{
  std::string target_commit = get_branch_last_commit_hash(target_branch);
  if (target_commit.empty())
  {
    std::cerr << "Error: No commits found in target branch: " << target_branch << std::endl;
    return;
  }

  std::string commit_path = ".bittrack/objects/" + target_commit;

  bool has_untracked = false;
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string file_path = entry.path().string();
      if (file_path.find(".bittrack") == std::string::npos && file_path.find("build/") == std::string::npos && file_path.find(".git/") == std::string::npos && file_path.find(".DS_Store") == std::string::npos && file_path.find(".github/") == std::string::npos)
      {
        has_untracked = true;
        break;
      }
    }
  }

  if (has_untracked)
  {
    backup_untracked_files();
  }

  restore_files_from_commit(commit_path);

  if (has_untracked)
  {
    restore_untracked_files();
  }
}

void rename_branch(const std::string &old_name, const std::string &new_name)
{
  std::vector<std::string> branches = get_branches_list();

  if (std::find(branches.begin(), branches.end(), old_name) == branches.end())
  {
    std::cerr << "Error: Branch '" << old_name << "' does not exist." << std::endl;
    return;
  }

  if (std::find(branches.begin(), branches.end(), new_name) != branches.end())
  {
    std::cerr << "Error: Branch '" << new_name << "' already exists." << std::endl;
    return;
  }

  std::string old_file = ".bittrack/refs/heads/" + old_name;
  std::string new_file = ".bittrack/refs/heads/" + new_name;

  if (std::rename(old_file.c_str(), new_file.c_str()) != 0)
  {
    std::cerr << "Error: Failed to rename branch file." << std::endl;
    return;
  }

  std::string old_dir = ".bittrack/objects/" + old_name;
  std::string new_dir = ".bittrack/objects/" + new_name;

  if (std::filesystem::exists(old_dir))
  {
    std::filesystem::rename(old_dir, new_dir);
  }

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
  std::vector<std::string> branches = get_branches_list();

  if (std::find(branches.begin(), branches.end(), branch_name) == branches.end())
  {
    std::cerr << "Error: Branch '" << branch_name << "' does not exist." << std::endl;
    return;
  }

  std::string current_branch = get_current_branch();
  std::string last_commit = get_branch_last_commit_hash(branch_name);

  std::cout << "Branch: " << branch_name << std::endl;
  std::cout << "  Current: " << (branch_name == current_branch ? "Yes" : "No") << std::endl;
  std::cout << "  Last commit: " << (last_commit.empty() ? "None" : last_commit) << std::endl;

  int commit_count = 0;
  std::ifstream history_file(".bittrack/commits/history");
  std::string line;
  while (std::getline(history_file, line))
  {
    std::istringstream iss(line);
    std::string commit_hash, branch;
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
  add_branch_implementaion(std::string(branch_name));
}

void create_branch(const std::string &branch_name)
{
  add_branch(branch_name);
}

void remove_branch(const std::string &branch_name)
{
  std::vector<std::string> branches = get_branches_list();

  if (std::find(branches.begin(), branches.end(), branch_name) == branches.end())
  {
    std::cout << branch_name << " is not found there" << std::endl;
    return;
  }

  std::string current_branch = get_current_branch();

  if (current_branch == branch_name)
  {
    std::cout << "Cannot delete the current branch. Please switch to another branch first." << std::endl;
    return;
  }

  cleanup_branch_commits(branch_name);

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
  std::vector<std::string> branches = get_branches_list();

  if (std::find(branches.begin(), branches.end(), branch_name) == branches.end())
  {
    std::cout << "you must create the branch first" << std::endl;
    return;
  }
  else if (get_current_branch() == branch_name)
  {
    std::cout << "you are already in " << branch_name << std::endl;
    return;
  }

  if (!ErrorHandler::validateNoUncommittedChanges())
  {
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

    std::cout << "Do you want to continue? (y/N): ";
    std::string response;
    std::getline(std::cin, response);
    if (response != "y" && response != "Y")
    {
      std::cout << "Branch switch cancelled." << std::endl;
      return;
    }
  }

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
  std::string branch_path = ".bittrack/refs/heads/" + branch_name;
  if (!std::filesystem::exists(branch_path))
  {
    return "";
  }

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

    if (!ErrorHandler::validateNoUncommittedChanges())
    {
      std::cout << "Error: You have uncommitted changes. Please commit or stash them before rebasing." << std::endl;
      return;
    }

    std::string current_branch = get_current_branch();
    if (current_branch != source_branch)
    {
      std::cout << "Error: You must be on the source branch '" << source_branch << "' to rebase." << std::endl;
      return;
    }

    std::string common_ancestor = find_common_ancestor(source_branch, target_branch);
    if (common_ancestor.empty())
    {
      std::cout << "Error: Could not find common ancestor between branches." << std::endl;
      return;
    }

    std::vector<std::string> commits_to_rebase = get_commit_chain(common_ancestor, source_branch);
    if (commits_to_rebase.empty())
    {
      std::cout << "No commits to rebase. Source branch is already up to date." << std::endl;
      return;
    }

    std::cout << "Rebasing " << commits_to_rebase.size() << " commits from '" << source_branch << "' onto '" << target_branch << "'..." << std::endl;

    std::string target_commit = get_branch_last_commit_hash(target_branch);
    if (target_commit.empty())
    {
      std::cout << "Error: Target branch has no commits." << std::endl;
      return;
    }

    std::string backup_commit = get_current_commit();

    try
    {
      switch_branch(target_branch);

      for (const auto &commit_hash : commits_to_rebase)
      {
        if (!apply_commit_during_rebase(commit_hash))
        {
          std::cout << "Error: Failed to apply commit " << commit_hash << " during rebase." << std::endl;
          std::cout << "Rebase aborted. Use 'bittrack --branch -rebase-abort' to rollback." << std::endl;
          return;
        }
      }

      switch_branch(source_branch);

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

      std::ofstream branch_file(".bittrack/refs/heads/" + source_branch);
      if (branch_file.is_open())
      {
        branch_file << backup_commit << std::endl;
        branch_file.close();
      }

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
    if (!branch_exists(branch_name))
    {
      std::cout << "Error: Branch '" << branch_name << "' does not exist." << std::endl;
      return;
    }

    std::cout << "History for branch '" << branch_name << "':" << std::endl;

    std::ifstream history_file(".bittrack/commits/history");
    if (!history_file.is_open())
    {
      std::cout << "No commit history found." << std::endl;
      return;
    }

    std::string line;
    bool found_branch = false;
    while (std::getline(history_file, line))
    {
      if (line.length() > branch_name.length() && line.substr(line.length() - branch_name.length()) == branch_name)
      {
        found_branch = true;
        std::cout << line << std::endl;
      }
    }

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
  std::vector<std::string> commits;

  std::ifstream history_file(".bittrack/commits/history");
  if (!history_file.is_open())
  {
    return commits;
  }

  std::string line;
  while (std::getline(history_file, line))
  {
    if (line.empty())
      continue;

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

    std::vector<std::string> branch_commits = get_branch_commits(branch_name);

    if (branch_commits.empty())
    {
      std::cout << "No commits found for branch '" << branch_name << "'" << std::endl;
      return;
    }

    std::vector<std::string> unique_commits;
    std::vector<std::string> all_branches = get_branches_list();

    for (const auto &commit_hash : branch_commits)
    {
      bool is_unique = true;

      for (const auto &other_branch : all_branches)
      {
        if (other_branch == branch_name)
          continue;

        std::vector<std::string> other_commits = get_branch_commits(other_branch);
        if (std::find(other_commits.begin(), other_commits.end(), commit_hash) != other_commits.end())
        {
          is_unique = false;
          break;
        }
      }

      if (is_unique)
      {
        unique_commits.push_back(commit_hash);
      }
    }

    std::cout << "Found " << unique_commits.size() << " unique commits to clean up" << std::endl;

    for (const auto &commit_hash : unique_commits)
    {
      std::string commit_dir = ".bittrack/objects/" + commit_hash;
      if (std::filesystem::exists(commit_dir))
      {
        std::filesystem::remove_all(commit_dir);
        std::cout << "  Removed commit object: " << commit_hash << std::endl;
      }

      std::string commit_log = ".bittrack/commits/" + commit_hash;
      if (std::filesystem::exists(commit_log))
      {
        std::filesystem::remove(commit_log);
        std::cout << "  Removed commit log: " << commit_hash << std::endl;
      }
    }

    std::ifstream history_file(".bittrack/commits/history");
    std::vector<std::string> remaining_lines;

    if (history_file.is_open())
    {
      std::string line;
      while (std::getline(history_file, line))
      {
        if (line.empty())
          continue;

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
    std::cerr << "Error during commit cleanup: " << e.what() << std::endl;
  }
}

std::string find_common_ancestor(const std::string &branch1, const std::string &branch2)
{
  try
  {
    std::vector<std::string> commits1 = get_branch_commits(branch1);
    std::vector<std::string> commits2 = get_branch_commits(branch2);

    if (commits1.empty() || commits2.empty())
    {
      return "";
    }

    for (const auto &commit1 : commits1)
    {
      if (std::find(commits2.begin(), commits2.end(), commit1) != commits2.end())
      {
        return commit1;
      }
    }

    return "";
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error finding common ancestor: " << e.what() << std::endl;
    return "";
  }
}

std::vector<std::string> get_commit_chain(const std::string &from_commit, const std::string &to_commit)
{
  std::vector<std::string> commit_chain;

  try
  {
    std::vector<std::string> all_commits = get_branch_commits(to_commit);

    if (all_commits.empty())
    {
      return commit_chain;
    }

    auto from_it = std::find(all_commits.begin(), all_commits.end(), from_commit);
    if (from_it == all_commits.end())
    {
      return all_commits;
    }

    for (auto it = from_it + 1; it != all_commits.end(); ++it)
    {
      commit_chain.push_back(*it);
    }

    return commit_chain;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error getting commit chain: " << e.what() << std::endl;
    return commit_chain;
  }
}

bool apply_commit_during_rebase(const std::string &commit_hash)
{
  try
  {
    std::string commit_file = ".bittrack/commits/" + commit_hash;
    if (!std::filesystem::exists(commit_file))
    {
      std::cerr << "Commit file not found: " << commit_hash << std::endl;
      return false;
    }

    std::ifstream file(commit_file);
    if (!file.is_open())
    {
      std::cerr << "Cannot open commit file: " << commit_hash << std::endl;
      return false;
    }

    std::string line;
    std::string author, message, timestamp;
    std::vector<std::string> files;
    std::vector<std::string> deleted_files;

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

    for (const auto &file : files)
    {
      std::string source_file = ".bittrack/objects/" + commit_hash + "/" + file;
      if (std::filesystem::exists(source_file))
      {
        std::filesystem::path file_path(file);
        if (!file_path.parent_path().empty())
        {
          std::filesystem::create_directories(file_path.parent_path());
        }

        std::filesystem::copy_file(source_file, file, std::filesystem::copy_options::overwrite_existing);
        stage(file);
      }
    }

    for (const auto &file : deleted_files)
    {
      if (std::filesystem::exists(file))
      {
        std::filesystem::remove(file);
        stage(file + " (deleted)");
      }
    }

    commit_changes(author, message);

    std::string new_commit_hash = get_current_commit();
    if (new_commit_hash.empty())
    {
      std::cerr << "Failed to create new commit during rebase" << std::endl;
      return false;
    }

    return true;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error applying commit during rebase: " << e.what() << std::endl;
    return false;
  }
}
