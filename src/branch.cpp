#include "../include/branch.hpp"

std::string get_current_branch()
{
  // check if HEAD file is available
  std::ifstream headFile(".bittrack/HEAD");

  if (!headFile.is_open())
  {
    std::cerr << "Error: Could not open HEAD file." << std::endl;
    return "";
  }

  std::string line;
  std::getline(headFile, line);
  headFile.close();
  
  // Remove trailing whitespace and newlines
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
      branchesList.push_back(file_path.substr(prefix.length())); // sustract the file name from the path
    }
  }
  return branchesList;
}


void copy_current_commit_object_to_branch(std::string new_branch_name)
{
  std::string current_branch = get_current_branch();
  std::string last_commit_hash = get_branch_last_commit_hash(current_branch);

  // With the new object structure, commits are stored directly in .bittrack/objects/
  // No need to copy anything - the commit already exists
  if (current_branch != "" && last_commit_hash != "")
  {
    // Just store the commit hash in the history for the new branch
    insert_commit_record_to_history(last_commit_hash, new_branch_name);
  }
}

void add_branch_implementaion(std::string name)
{
  std::vector<std::string> branches = get_branches_list();

  // check if the branch not exist
  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  {
    // Get the current commit hash to set as the new branch's HEAD
    std::string current_commit = get_branch_last_commit_hash(get_current_branch());
    
    // Create the branch ref file with the current commit hash
    std::ofstream newBranchHeadFile(".bittrack/refs/heads/" + name);
    if (newBranchHeadFile.is_open())
    {
      newBranchHeadFile << current_commit << std::endl;
      newBranchHeadFile.close();
    }

    // Store the commit hash in the history for the new branch
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

    // read entire file contents
    std::string first_content(
      (std::istreambuf_iterator<char>(first_stream)),
      std::istreambuf_iterator<char>()
    );
    std::string second_content(
      (std::istreambuf_iterator<char>(second_stream)),
      std::istreambuf_iterator<char>()
    );

    first_stream.close();
    second_stream.close();

    // if the first file has no content and the second has content, write the second file’s content to the target file and report success.
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
    // if the second file is empty but the first isn’t, write the first file’s content.
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
  // check for staged files
  std::vector<std::string> staged_files = get_staged_files();
  if (!staged_files.empty())
  {
    return true;
  }
  // check for unstaged files
  std::vector<std::string> unstaged_files = get_unstaged_files();
  if (!unstaged_files.empty())
  {
    return true;
  }
  return false;
}

void backup_untracked_files()
{
  // create a temporary directory for untracked files
  std::filesystem::create_directories(".bittrack/untracked_backup");

  // get all files in working directory
  for (const auto &entry: std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string file_path = entry.path().string();

      // skip .bittrack directory, build directory, and other system files
      if (file_path.find(".bittrack") != std::string::npos || file_path.find("build/") != std::string::npos || file_path.find(".git/") != std::string::npos || file_path.find(".DS_Store") != std::string::npos || file_path.find(".github/") != std::string::npos)
      {
        continue;
      }

      // check if file is tracked in current branch
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

      // this is an untracked file, backup it
      try
      {
        std::filesystem::path relative_path = std::filesystem::relative(file_path, ".");
        std::filesystem::path backup_path = ".bittrack/untracked_backup" / relative_path;

        // only create directories if the parent path is not empty
        if (!backup_path.parent_path().empty())
        {
          std::filesystem::create_directories(backup_path.parent_path());
        }
        std::filesystem::copy_file(file_path, backup_path, std::filesystem::copy_options::overwrite_existing);
      }
      catch (const std::filesystem::filesystem_error &e)
      {
        // skip files that can't be copied (permissions, etc.)
        continue;
      }
    }
  }
}

void restore_untracked_files()
{
  // restore untracked files from backup
  if (std::filesystem::exists(".bittrack/untracked_backup"))
  {
    for (const auto &entry: std::filesystem::recursive_directory_iterator(".bittrack/untracked_backup"))
    {
      if (entry.is_regular_file())
      {
        std::filesystem::path restore_path = std::filesystem::relative(entry.path(), ".bittrack/untracked_backup");
        // only create directories if the parent path is not empty
        if (!restore_path.parent_path().empty())
        {
          std::filesystem::create_directories(restore_path.parent_path());
        }
        std::filesystem::copy_file(entry.path(), restore_path, std::filesystem::copy_options::overwrite_existing);
      }
    }
    // clean up backup
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

  // Remove all tracked files from working directory
  // Get all files that have ever been tracked in this repository
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

  // Remove all tracked files from working directory
  for (const auto &file_path : all_tracked_files)
  {
    if (std::filesystem::exists(file_path))
    {
      std::filesystem::remove(file_path);
    }
  }

  // copy files from target commit to working directory
  for (const auto &entry : std::filesystem::recursive_directory_iterator(commit_path))
  {
    if (entry.is_regular_file())
    {
      std::filesystem::path relative_path = std::filesystem::relative(entry.path(), commit_path);
      std::filesystem::path working_file = relative_path;

      // only create directories if the parent path is not empty
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

  // only backup untracked files if there are any
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

  // restore files from target branch commit
  restore_files_from_commit(commit_path);

  // restore untracked files if we backed them up
  if (has_untracked)
  {
    restore_untracked_files();
  }
}

void rename_branch(const std::string &old_name, const std::string &new_name)
{
  std::vector<std::string> branches = get_branches_list();

  // check if old branch exists
  if (std::find(branches.begin(), branches.end(), old_name) == branches.end())
  {
    std::cerr << "Error: Branch '" << old_name << "' does not exist." << std::endl;
    return;
  }

  // check if new branch already exists
  if (std::find(branches.begin(), branches.end(), new_name) != branches.end())
  {
    std::cerr << "Error: Branch '" << new_name << "' already exists." << std::endl;
    return;
  }

  // rename branch file
  std::string old_file = ".bittrack/refs/heads/" + old_name;
  std::string new_file = ".bittrack/refs/heads/" + new_name;

  if (std::rename(old_file.c_str(), new_file.c_str()) != 0)
  {
    std::cerr << "Error: Failed to rename branch file." << std::endl;
    return;
  }

  // rename branch directory
  std::string old_dir = ".bittrack/objects/" + old_name;
  std::string new_dir = ".bittrack/objects/" + new_name;

  if (std::filesystem::exists(old_dir))
  {
    std::filesystem::rename(old_dir, new_dir);
  }

  // update head if we're on the renamed branch
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

  // count commits in branch
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
  // call the existing implementation that takes std::string by value
  add_branch_implementaion(std::string(branch_name));
}

void create_branch(const std::string &branch_name)
{
  add_branch(branch_name);
}

void remove_branch(const std::string &branch_name)
{
  std::vector<std::string> branches = get_branches_list();

  // check if the branch exists
  if (std::find(branches.begin(), branches.end(), branch_name) == branches.end())
  {
    std::cout << branch_name << " is not found there" << std::endl;
    return;
  }

  // check if it is the current branch - if so, we need to switch first
  std::string current_branch = get_current_branch();

  if (current_branch == branch_name)
  {
    std::cout << "Cannot delete the current branch. Please switch to another branch first." << std::endl;
    return;
  }

  // remove the branch ref file
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
  { // check if the branch exist
    std::cout << "you must create the branch first" << std::endl;
    return;
  }
  else if (get_current_branch() == branch_name)
  { // check if it is not already the head branch
    std::cout << "you are already in " << branch_name << std::endl;
    return;
  }

  // check for uncommitted changes
  if (has_uncommitted_changes())
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

  // update working directory to match target branch
  update_working_directory(branch_name);

  // update HEAD to point to new branch
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


void rebase_branch(const std::string& source_branch, const std::string& target_branch)
{
  try
  {
    // Check if both branches exist
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
    
    // Check for uncommitted changes
    if (has_uncommitted_changes())
    {
      std::cout << "Error: You have uncommitted changes. Please commit or stash them before rebasing." << std::endl;
      return;
    }
    
    std::cout << "Rebase functionality is not yet fully implemented." << std::endl;
    std::cout << "This would rebase '" << source_branch << "' onto '" << target_branch << "'" << std::endl;
    
    // TODO: Implement actual rebase logic
    // Rebase is a complex operation that involves:
    // 1. Finding the common ancestor
    // 2. Creating new commits for each commit in source_branch
    // 3. Applying changes from target_branch
    // 4. Updating branch references
  }
  catch (const std::exception& e)
  {
    std::cout << "Error during rebase: " << e.what() << std::endl;
  }
}

void show_branch_history(const std::string& branch_name)
{
  try
  {
    // Check if branch exists
    if (!branch_exists(branch_name))
    {
      std::cout << "Error: Branch '" << branch_name << "' does not exist." << std::endl;
      return;
    }
    
    std::cout << "History for branch '" << branch_name << "':" << std::endl;
    
    // Read the commit history file
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
      // Check if line ends with the branch name (format: "commit_hash branch_name")
      if (line.length() > branch_name.length() && 
          line.substr(line.length() - branch_name.length()) == branch_name)
      {
        found_branch = true;
        // Print the commit info
        std::cout << line << std::endl;
      }
    }
    
    if (!found_branch)
    {
      std::cout << "No commits found for branch '" << branch_name << "'" << std::endl;
    }
    
    history_file.close();
  }
  catch (const std::exception& e)
  {
    std::cout << "Error reading branch history: " << e.what() << std::endl;
  }
}

