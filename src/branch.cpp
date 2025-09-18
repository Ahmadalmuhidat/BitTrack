#include "../include/branch.hpp"
#include "../include/utils.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include "../include/remote.hpp"

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

  // copy the content of the current branch to the new branch
  if (current_branch != "" && last_commit_hash != "")
  {
    std::string parent_path = ".bittrack/objects/";
    std::string source = parent_path + current_branch + "/" + last_commit_hash;
    std::string destination = parent_path + new_branch_name + "/" + last_commit_hash;
    std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive);
  }
  // store the commit hash in the history
  insert_commit_record_to_history(last_commit_hash, new_branch_name);
}

void add_branch_implementaion(std::string name)
{
  std::vector<std::string> branches = get_branches_list();

  // check if the branch not exist
  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  {
    // create a file with the new branch name
    std::ofstream newBranchHeadFile(".bittrack/refs/heads/" + name);
    newBranchHeadFile.close();

    // create a directory with the new branch name
    std::filesystem::create_directory(".bittrack/objects/" + name);

    copy_current_commit_object_to_branch(name);

    // insert in the head file the random hash as last commit
    std::ofstream headFile(".bittrack/refs/heads/" + name, std::ios::trunc);
    headFile << get_branch_last_commit_hash(get_current_branch()) << std::endl;
    headFile.close();
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

void merge_two_branches(const std::string &first_branch, const std::string &second_branch)
{
  try
  {
    if (first_branch.empty() || second_branch.empty())
    {
      throw BitTrackError(
        ErrorCode::INVALID_ARGUMENTS,
        "Branch names cannot be empty",
        ErrorSeverity::ERROR,
        "merge_two_branches"
      );
    }

    if (first_branch == second_branch)
    {
      throw BitTrackError(
        ErrorCode::INVALID_ARGUMENTS,
        "Cannot merge branch with itself",
        ErrorSeverity::ERROR,
        "merge_two_branches"
      );
    }

    if (!std::filesystem::exists(".bittrack"))
    {
      throw BitTrackError(
        ErrorCode::NOT_IN_REPOSITORY,
        "Not in a BitTrack repository",
        ErrorSeverity::ERROR,
        "merge_two_branches"
      );
    }

    if (has_uncommitted_changes())
    {
      throw BitTrackError(
        ErrorCode::UNCOMMITTED_CHANGES,
        "Cannot merge with uncommitted changes. Please commit or stash your changes first.",
        ErrorSeverity::ERROR,
        "merge_two_branches"
      );
    }

    std::vector<std::string> branches = get_branches_list();

    if (std::find(branches.begin(), branches.end(), first_branch) == branches.end())
    {
      throw BitTrackError(
        ErrorCode::BRANCH_NOT_FOUND,
        "Branch '" + first_branch + "' not found",
        ErrorSeverity::ERROR,
        "merge_two_branches"
      );
    }

    if (std::find(branches.begin(), branches.end(), second_branch) == branches.end())
    {
      throw BitTrackError(
        ErrorCode::BRANCH_NOT_FOUND,
        "Branch '" + second_branch + "' not found",
        ErrorSeverity::ERROR,
        "merge_two_branches"
      );
    }

    std::string current_branch = get_current_branch();
    if (current_branch != first_branch)
    {
      throw BitTrackError(
        ErrorCode::INVALID_ARGUMENTS,
        "Must be on the target branch (" + first_branch + ") to perform merge",
        ErrorSeverity::ERROR,
        "merge_two_branches"
      );
    }

    std::cout << "Merging branch '" << second_branch << "' into '" << first_branch << "'..." << std::endl;

    std::string first_commit = get_branch_last_commit_hash(first_branch);
    std::string second_commit = get_branch_last_commit_hash(second_branch);

    if (first_commit.empty() || second_commit.empty())
    {
      throw BitTrackError(
        ErrorCode::NO_COMMITS_FOUND,
        "One or both branches have no commits",
        ErrorSeverity::ERROR,
        "merge_two_branches"
      );
    }

    std::filesystem::path first_path = ".bittrack/objects/" + first_branch + "/" + first_commit;
    std::filesystem::path second_path = ".bittrack/objects/" + second_branch + "/" + second_commit;

    if (!std::filesystem::exists(first_path) || !std::filesystem::exists(second_path))
    {
      throw BitTrackError(
        ErrorCode::REPOSITORY_CORRUPTED,
        "Branch commit directories not found",
        ErrorSeverity::ERROR,
        "merge_two_branches"
      );
    }

    std::unordered_set<std::string> all_files;
    std::unordered_map<std::string, std::filesystem::path> first_files;
    std::unordered_map<std::string, std::filesystem::path> second_files;

    for (const auto &entry: std::filesystem::recursive_directory_iterator(first_path))
    {
      if (entry.is_regular_file())
      {
        std::filesystem::path relative = std::filesystem::relative(entry.path(), first_path);
        std::string relative_str = relative.string();
        all_files.insert(relative_str);
        first_files[relative_str] = entry.path();
      }
    }

    for (const auto &entry: std::filesystem::recursive_directory_iterator(second_path))
    {
      if (entry.is_regular_file())
      {
        std::filesystem::path relative = std::filesystem::relative(entry.path(), second_path);
        std::string relative_str = relative.string();
        all_files.insert(relative_str);
        second_files[relative_str] = entry.path();
      }
    }

    std::vector<std::string> new_files;
    std::vector<std::string> modified_files;
    std::vector<std::string> conflicted_files;
    std::vector<std::string> unchanged_files;

    for (const auto &file_path: all_files)
    {
      bool in_first = first_files.find(file_path) != first_files.end();
      bool in_second = second_files.find(file_path) != second_files.end();

      if (in_first && in_second)
      {
        try
        {
          if (compare_files_contents_if_equal(first_files[file_path], second_files[file_path]))
          {
            unchanged_files.push_back(file_path);
          }
          else
          {
            // files differ - attempt automatic merge
            if (attempt_automatic_merge(first_files[file_path], second_files[file_path], file_path))
            {
              modified_files.push_back(file_path);
              std::cout << "[MERGED] " << file_path << " - automatically merged" << std::endl;
            }
            else
            {
              conflicted_files.push_back(file_path);
              std::cout << "[CONFLICT] " << file_path << " - manual resolution required" << std::endl;
            }
          }
        }
        catch (const std::exception &e)
        {
          conflicted_files.push_back(file_path);
          std::cout << "[CONFLICT] " << file_path << " - error during comparison: " << e.what() << std::endl;
        }
      }
      else if (in_first && !in_second)
      {
        unchanged_files.push_back(file_path);
      }
      else if (!in_first && in_second)
      {
        try
        {
          std::filesystem::path target_path = first_path / file_path;
          std::filesystem::create_directories(target_path.parent_path());
          std::filesystem::copy_file(second_files[file_path], target_path);
          new_files.push_back(file_path);
          std::cout << "[ADDED] " << file_path << " from " << second_branch << std::endl;
        }
        catch (const std::exception &e)
        {
          std::cerr << "[ERROR] Failed to add " << file_path << ": " << e.what() << std::endl;
        }
      }
    }

    if (!conflicted_files.empty())
    {
      std::cout << "\nMerge conflicts detected in " << conflicted_files.size() << " file(s):" << std::endl;
      for (const auto &file : conflicted_files)
      {
        std::cout << "  - " << file << std::endl;
      }
      std::cout << "\nPlease resolve conflicts manually and then commit the changes." << std::endl;
      throw BitTrackError(ErrorCode::MERGE_CONFLICT, "Merge conflicts detected in " + std::to_string(conflicted_files.size()) + " file(s)", ErrorSeverity::ERROR, "merge_two_branches");
    }

    std::cout << "\nStaging changes..." << std::endl;
    for (const auto &file : new_files)
    {
      stage(file);
    }
    for (const auto &file : modified_files)
    {
      stage(file);
    }

    std::cout << "Creating merge commit..." << std::endl;
    std::string commit_message = "Merge branch '" + second_branch + "' into " + first_branch;
    commit_changes("BitTrack", commit_message);

    std::cout << "\nMerge completed successfully!" << std::endl;
    std::cout << "  - Added: " << new_files.size() << " file(s)" << std::endl;
    std::cout << "  - Modified: " << modified_files.size() << " file(s)" << std::endl;
    std::cout << "  - Unchanged: " << unchanged_files.size() << " file(s)" << std::endl;
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("merge_two_branches")
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

      std::string tracked_file_path = ".bittrack/objects/" + get_current_branch() + "/" + current_commit + "/" + file_path;
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

  // remove all tracked files from working directory
  std::string current_commit = get_branch_last_commit_hash(get_current_branch());
  if (!current_commit.empty())
  {
    std::string current_commit_path = ".bittrack/objects/" + get_current_branch() + "/" + current_commit;
    if (std::filesystem::exists(current_commit_path))
    {
      for (const auto &entry : std::filesystem::recursive_directory_iterator(current_commit_path))
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

  std::string commit_path = ".bittrack/objects/" + target_branch + "/" + target_commit;

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

void compare_branches(const std::string &branch1, const std::string &branch2)
{
  std::vector<std::string> branches = get_branches_list();

  if (std::find(branches.begin(), branches.end(), branch1) == branches.end())
  {
    std::cerr << "Error: Branch '" << branch1 << "' does not exist." << std::endl;
    return;
  }

  if (std::find(branches.begin(), branches.end(), branch2) == branches.end())
  {
    std::cerr << "Error: Branch '" << branch2 << "' does not exist." << std::endl;
    return;
  }

  std::string commit1 = get_branch_last_commit_hash(branch1);
  std::string commit2 = get_branch_last_commit_hash(branch2);

  if (commit1.empty() || commit2.empty())
  {
    std::cout << "Cannot compare branches - one or both have no commits." << std::endl;
    return;
  }

  std::cout << "Comparing branches:" << std::endl;
  std::cout << "  " << branch1 << " (" << commit1 << ")" << std::endl;
  std::cout << "  " << branch2 << " (" << commit2 << ")" << std::endl;
  std::cout << "=========================================" << std::endl;

  // Get files from both branches
  std::set<std::string> files1 = get_branch_files(branch1);
  std::set<std::string> files2 = get_branch_files(branch2);
  
  // Find common files
  std::set<std::string> common_files;
  std::set_intersection(files1.begin(), files1.end(), 
                       files2.begin(), files2.end(),
                       std::inserter(common_files, common_files.begin()));
  
  // Find files only in branch1
  std::set<std::string> only_in_branch1;
  std::set_difference(files1.begin(), files1.end(),
                     common_files.begin(), common_files.end(),
                     std::inserter(only_in_branch1, only_in_branch1.begin()));
  
  // Find files only in branch2
  std::set<std::string> only_in_branch2;
  std::set_difference(files2.begin(), files2.end(),
                     common_files.begin(), common_files.end(),
                     std::inserter(only_in_branch2, only_in_branch2.begin()));
  
  // Show differences
  if (!only_in_branch1.empty())
  {
    std::cout << "Files only in " << branch1 << ":" << std::endl;
    for (const auto& file : only_in_branch1)
    {
      std::cout << "  + " << file << std::endl;
    }
  }
  
  if (!only_in_branch2.empty())
  {
    std::cout << "Files only in " << branch2 << ":" << std::endl;
    for (const auto& file : only_in_branch2)
    {
      std::cout << "  + " << file << std::endl;
    }
  }
  
  if (!common_files.empty())
  {
    std::cout << "Common files (" << common_files.size() << "):" << std::endl;
    for (const auto& file : common_files)
    {
      std::string content1 = get_file_content_from_branch(branch1, file);
      std::string content2 = get_file_content_from_branch(branch2, file);
      
      if (content1 != content2)
      {
        std::cout << "  ~ " << file << " (modified)" << std::endl;
      }
      else
      {
        std::cout << "  = " << file << " (identical)" << std::endl;
      }
    }
  }
  
  std::cout << std::endl;
  std::cout << "Summary:" << std::endl;
  std::cout << "  " << branch1 << ": " << files1.size() << " files" << std::endl;
  std::cout << "  " << branch2 << ": " << files2.size() << " files" << std::endl;
  std::cout << "  Common: " << common_files.size() << " files" << std::endl;
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
  if (std::remove(branch_file.c_str()) != 0)
  {
    std::perror("Error deleting branch file");
    return;
  }

  std::string branch_dir = ".bittrack/objects/" + branch_name;
  std::filesystem::remove_all(branch_dir);
}

void switch_branch(const std::string &branch_name)
{
  std::vector<std::string> branches = get_branches_list();

  if (std::find(branches.begin(), branches.end(), name) == branches.end())
  { // check if the branch exist
    std::cout << "you must create the branch first" << std::endl;
    return;
  }
  else if (get_current_branch() == name)
  { // check if it is not already the head branch
    std::cout << "you are already in " << name << std::endl;
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
  update_working_directory(name);

  // update HEAD to point to new branch
  std::ofstream HeadFile(".bittrack/HEAD", std::ios::trunc);
  HeadFile << name << std::endl;
  HeadFile.close();

  std::cout << "Switched to branch: " << name << std::endl;
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

void merge_branch(const std::string& source_branch, const std::string& target_branch)
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
      std::cout << "Error: You have uncommitted changes. Please commit or stash them before merging." << std::endl;
      return;
    }
    
    // Switch to target branch
    std::string current_branch = get_current_branch();
    if (current_branch != target_branch)
    {
      switch_branch(target_branch);
    }
    
    // Perform the merge using the existing merge_two_branches function
    merge_two_branches(source_branch, target_branch);
    
    std::cout << "Successfully merged branch '" << source_branch << "' into '" << target_branch << "'" << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cout << "Error during merge: " << e.what() << std::endl;
  }
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

// Helper functions for branch comparison
std::set<std::string> get_branch_files(const std::string& branch_name)
{
  std::set<std::string> files;
  
  std::string commit_hash = get_branch_last_commit_hash(branch_name);
  if (commit_hash.empty())
  {
    return files;
  }
  
  std::string commit_dir = ".bittrack/objects/" + branch_name + "/" + commit_hash;
  if (!std::filesystem::exists(commit_dir))
  {
    return files;
  }
  
  for (const auto& entry : std::filesystem::directory_iterator(commit_dir))
  {
    if (entry.is_regular_file())
    {
      std::string filename = entry.path().filename().string();
      if (filename != "commit.txt") // Exclude commit metadata
      {
        files.insert(filename);
      }
    }
  }
  
  return files;
}

std::string get_file_content_from_branch(const std::string& branch_name, const std::string& filename)
{
  std::string commit_hash = get_branch_last_commit_hash(branch_name);
  if (commit_hash.empty())
  {
    return "";
  }
  
  std::string file_path = ".bittrack/objects/" + branch_name + "/" + commit_hash + "/" + filename;
  if (!std::filesystem::exists(file_path))
  {
    return "";
  }
  
  std::ifstream file(file_path);
  if (!file.is_open())
  {
    return "";
  }
  
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  
  return buffer.str();
}
