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
  // read the first line in HEAD file and return it
  std::string line;
  std::getline(headFile, line);
  headFile.close();
  return line;
}

std::vector<std::string> get_branches_list()
{
  std::vector<std::string> branchesList;
  const std::string prefix = ".bittrack/refs/heads/";
  // get the names of the files inside the heads folder and store them in the branchesList array
  for (const auto &entry: std::filesystem::recursive_directory_iterator(".bittrack/refs/heads"))
  {
    if (entry.is_regular_file())
    {
      std::string branch = entry.path().string(); // get the file path
      branchesList.push_back(branch.substr(prefix.length())); // sustract the file name from the path
    }
  }
  return branchesList;
}

void print_branshes_list()
{
  std::vector<std::string> branchesList = get_branches_list();
  std::string head = get_current_branch();

  for (int i = 0; i < branchesList.size(); i++)
  {
    std::string branch = branchesList[i];

    if (branch == head)
    {
      // if the branch is the current head print it in green color
      std::cout << "\033[32m" << branch << "\033[0m" << std::endl;
    }
    else
    {
      std::cout << branch << std::endl;
    }
  }
}

void copy_current_commit_to_branch(std::string new_branch_name)
{
  std::string current_branch = get_current_branch();
  std::string last_commit_hash = get_last_commit(current_branch);

  // copy the content of the current branch to the new branch
  if (current_branch != "" && last_commit_hash != "")
  {
    std::string parent_path = ".bittrack/objects/";
    std::string source = parent_path + current_branch  + "/" + last_commit_hash;
    std::string destination = parent_path + new_branch_name  + "/" + last_commit_hash;
    std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive);
  }

  // store the commit hash in the history
  insert_commit_to_history(last_commit_hash, new_branch_name);
}

void add_branch(std::string name)
{
  std::vector<std::string> branches = get_branches_list();

  // check if the branch not exist
  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  {
    // create a file with the new branch name
    std::ofstream newBranch(".bittrack/refs/heads/" + name);
    newBranch.close();

    // create a directory with the new branch name
    std::filesystem::create_directory(".bittrack/objects/" + name);

    copy_current_commit_to_branch(name);

    // insert in the head file the random hash as last commit
    std::ofstream headFile(".bittrack/refs/heads/" + name, std::ios::trunc);
    headFile << get_last_commit(get_current_branch()) << std::endl;
    headFile.close();
  }
  else
  {
    std::cout << name << " is already there" << std::endl;
  }
}

void remove_branch(std::string branch_name)
{
  std::vector<std::string> branches = get_branches_list();

  // check if the branch exists
  if (std::find(branches.begin(), branches.end(), branch_name) == branches.end())
  {
    std::cout << branch_name << " is not found there" << std::endl;
    return;
  }

  // check if it is the current branch
  std::string current_branch = get_current_branch();

  if (current_branch != branch_name)
  {
    std::cout << "You must be in the " << branch_name << " branch first" << std::endl;
    return;
  }

  // check if there are staged files
  std::vector<std::string> staged_files = get_staged_files();

  if (!staged_files.empty())
  {
    std::cout << "Please unstage all files before removing the branch." << std::endl;
    return;
  }

  std::string branch_file = ".bittrack/refs/heads/" + branch_name;
  if (std::remove(branch_file.c_str()) != 0) // replace with filesystem
  {
    std::perror("Error deleting branch file");
    return;
  }

  std::string branch_dir = ".bittrack/objects/" + branch_name;
  std::filesystem::remove_all(branch_dir);
}

// Utility to compare file contents
bool compare_files_contents(const std::filesystem::path& first_file, const std::filesystem::path& second_file) {
  std::ifstream first_file_stream(first_file, std::ios::binary);
  std::ifstream second_file_stream(second_file, std::ios::binary);

  if (!first_file_stream || !second_file_stream)
  {
    return false;
  }

  std::istreambuf_iterator<char> begin1(first_file_stream), end1;
  std::istreambuf_iterator<char> begin2(second_file_stream), end2;
  return std::equal(begin1, end1, begin2, end2);
}

void merge_two_branches(const std::string& first_branch, const std::string& second_branch) {
  std::vector<std::string> branches = get_branches_list();

  // Validate branches
  if (std::find(branches.begin(), branches.end(), first_branch) == branches.end() ||
      std::find(branches.begin(), branches.end(), second_branch) == branches.end()) {
    std::cout << "[ERROR] One of the branches was not found.\n";
    return;
  }

  // Construct full paths to both branch's last commit directories
  std::filesystem::path first_path = ".bittrack/objects/" + first_branch + "/" + get_last_commit(first_branch);
  std::filesystem::path second_path = ".bittrack/objects/" + second_branch + "/" + get_last_commit(second_branch);

  // Compare directory contents
  for (const auto& entry : std::filesystem::recursive_directory_iterator(first_path))
  {
    if (!std::filesystem::is_regular_file(entry))
    {
      continue;
    }

    std::filesystem::path relative = std::filesystem::relative(entry.path(), first_path);
    std::filesystem::path second_file = second_path / relative;

    if (!std::filesystem::exists(second_file))
    {
      std::filesystem::create_directories(second_file.parent_path());
      std::filesystem::copy_file(entry.path(), second_file);
      std::cout << "[ADDED] " << relative << " to " << second_branch << "\n";
    } else
    {
      if (!compare_files_contents(entry.path(), second_file))
      {
        std::cout << "[CONFLICT] " << relative << " differs between branches.\n"; // fix: show the user where is the conflict and resolve it, also add force flage
      } else
      {
        std::cout << "[UNCHANGED] " << relative << "\n";
      }
    }
  }
}

bool has_uncommitted_changes()
{
  // Check for staged files
  std::vector<std::string> staged_files = get_staged_files();
  if (!staged_files.empty())
  {
    return true;
  }

  // Check for unstaged files
  std::vector<std::string> unstaged_files = get_unstaged_files();
  if (!unstaged_files.empty())
  {
    return true;
  }

  return false;
}

void backup_untracked_files()
{
  // Create a temporary directory for untracked files
  std::filesystem::create_directories(".bittrack/untracked_backup");
  
  // Get all files in working directory
  for (const auto& entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string file_path = entry.path().string();
      
      // Skip .bittrack directory, build directory, and other system files
      if (file_path.find(".bittrack") != std::string::npos ||
          file_path.find("build/") != std::string::npos ||
          file_path.find(".git/") != std::string::npos ||
          file_path.find(".DS_Store") != std::string::npos ||
          file_path.find(".github/") != std::string::npos)
        continue;
        
      // Check if file is tracked in current branch
      std::string current_commit = get_current_commit();
      if (current_commit.empty())
        continue;
        
      std::string tracked_file_path = ".bittrack/objects/" + get_current_branch() + "/" + current_commit + "/" + file_path;
      if (std::filesystem::exists(tracked_file_path))
        continue;
        
      // This is an untracked file, backup it
      try {
        std::filesystem::path relative_path = std::filesystem::relative(file_path, ".");
        std::filesystem::path backup_path = ".bittrack/untracked_backup" / relative_path;
        
        // Only create directories if the parent path is not empty
        if (!backup_path.parent_path().empty()) {
          std::filesystem::create_directories(backup_path.parent_path());
        }
        std::filesystem::copy_file(file_path, backup_path, std::filesystem::copy_options::overwrite_existing);
      } catch (const std::filesystem::filesystem_error& e) {
        // Skip files that can't be copied (permissions, etc.)
        continue;
      }
    }
  }
}

void restore_untracked_files()
{
  // Restore untracked files from backup
  if (std::filesystem::exists(".bittrack/untracked_backup"))
  {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(".bittrack/untracked_backup"))
    {
      if (entry.is_regular_file())
      {
        std::filesystem::path relative_path = std::filesystem::relative(entry.path(), ".bittrack/untracked_backup");
        std::filesystem::path restore_path = relative_path;
        
        // Only create directories if the parent path is not empty
        if (!restore_path.parent_path().empty()) {
          std::filesystem::create_directories(restore_path.parent_path());
        }
        std::filesystem::copy_file(entry.path(), restore_path, std::filesystem::copy_options::overwrite_existing);
      }
    }
    // Clean up backup
    std::filesystem::remove_all(".bittrack/untracked_backup");
  }
}

void restore_files_from_commit(const std::string& commit_path)
{
  if (!std::filesystem::exists(commit_path))
  {
    std::cerr << "Error: Commit snapshot not found: " << commit_path << std::endl;
    return;
  }

  // First, remove all tracked files from working directory
  std::string current_commit = get_current_commit();
  if (!current_commit.empty())
  {
    std::string current_commit_path = ".bittrack/objects/" + get_current_branch() + "/" + current_commit;
    if (std::filesystem::exists(current_commit_path))
    {
      for (const auto& entry : std::filesystem::recursive_directory_iterator(current_commit_path))
      {
        if (entry.is_regular_file())
        {
          std::filesystem::path relative_path = std::filesystem::relative(entry.path(), current_commit_path);
          std::filesystem::path working_file = relative_path;
          
          if (std::filesystem::exists(working_file))
          {
            std::filesystem::remove(working_file);
          }
        }
      }
    }
  }

  // Copy files from target commit to working directory
  for (const auto& entry : std::filesystem::recursive_directory_iterator(commit_path))
  {
    if (entry.is_regular_file())
    {
      std::filesystem::path relative_path = std::filesystem::relative(entry.path(), commit_path);
      std::filesystem::path working_file = relative_path;
      
      // Only create directories if the parent path is not empty
      if (!working_file.parent_path().empty()) {
        std::filesystem::create_directories(working_file.parent_path());
      }
      std::filesystem::copy_file(entry.path(), working_file, std::filesystem::copy_options::overwrite_existing);
    }
  }
}

void update_working_directory(const std::string& target_branch)
{
  std::string target_commit = get_last_commit(target_branch);
  if (target_commit.empty())
  {
    std::cerr << "Error: No commits found in target branch: " << target_branch << std::endl;
    return;
  }

  std::string commit_path = ".bittrack/objects/" + target_branch + "/" + target_commit;
  
  // Only backup untracked files if there are any
  bool has_untracked = false;
  for (const auto& entry : std::filesystem::recursive_directory_iterator("."))
  {
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
  
  if (has_untracked) {
    backup_untracked_files();
  }
  
  // Restore files from target branch commit
  restore_files_from_commit(commit_path);
  
  // Restore untracked files if we backed them up
  if (has_untracked) {
    restore_untracked_files();
  }
}

void switch_branch(std::string name, bool check_uncommitted)
{
  std::vector<std::string> branches = get_branches_list();

  if (std::find(branches.begin(), branches.end(), name) == branches.end()) // check if the branch exist
  {
    std::cout << "you must create the branch first" << std::endl;
    return;
  }
  else if (get_current_branch() == name) // check if it is not already the head branch
  {
    std::cout << "you are already in " << name << std::endl;
    return;
  }

  // Check for uncommitted changes
  if (has_uncommitted_changes())
  {
    std::cout << "Warning: You have uncommitted changes. Switching branches may overwrite your changes." << std::endl;
    std::cout << "Staged files: ";
    std::vector<std::string> staged = get_staged_files();
    for (const auto& file : staged)
    {
      std::cout << file << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Unstaged files: ";
    std::vector<std::string> unstaged = get_unstaged_files();
    for (const auto& file : unstaged)
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

  // Update working directory to match target branch
  update_working_directory(name);

  // Update HEAD to point to new branch
  std::ofstream HeadFile(".bittrack/HEAD", std::ios::trunc);
  HeadFile << name << std::endl;
  HeadFile.close();
  
  std::cout << "Switched to branch: " << name << std::endl;
}