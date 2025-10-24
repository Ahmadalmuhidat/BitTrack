#include "../include/merge.hpp"

MergeResult merge_branches(const std::string &source_branch, const std::string &target_branch)
{
  MergeResult result;

  // Get the last commit hashes of both branches
  std::string source_commit = get_branch_last_commit_hash(source_branch);
  std::string target_commit = get_branch_last_commit_hash(target_branch);

  // Check if branches exist
  if (source_commit.empty() || target_commit.empty())
  {
    result.message = "One or both branches do not exist";
    return result;
  }

  // Check if already up to date
  if (source_commit == target_commit)
  {
    result.success = true;
    result.message = "Already up to date";
    return result;
  }

  // Check for fast-forward merge
  if (is_fast_forward(source_branch, target_branch))
  {
    result.success = true;
    result.message = "Fast-forward merge";
    result.merge_commit = source_commit;
    return result;
  }

  // Perform three-way merge
  std::string merge_base = find_merge_base(source_commit, target_commit);
  if (merge_base.empty())
  {
    result.message = "No common ancestor found";
    return result;
  }

  // Execute the three-way merge
  result = three_way_merge(merge_base, target_commit, source_commit);

  // Create merge commit if successful and no conflicts
  if (result.success && !result.has_conflicts)
  {
    std::string merge_message = "Merge branch '" + source_branch + "' into " + target_branch;
    create_merge_commit(merge_message, {target_commit, source_commit});
    result.merge_commit = get_current_commit();
  }

  return result;
}

MergeResult merge_commits(const std::string &commit1, const std::string &commit2)
{
  MergeResult result;

  // Check if commits exist
  std::string merge_base = find_merge_base(commit1, commit2);
  if (merge_base.empty())
  {
    result.message = "No common ancestor found";
    return result;
  }

  // Execute the three-way merge
  result = three_way_merge(merge_base, commit1, commit2);
  return result;
}

void write_conflict(const std::string &path, const std::string &ours,const std::string &theirs)
{
  std::ofstream conflict(path);
  conflict << "<<<<<<< HEAD\n"
           << ours
           << "\n=======\n"
           << theirs
           << "\n>>>>>>> theirs\n";
  conflict.close();
}

MergeResult three_way_merge(const std::string &base, const std::string &ours, const std::string &theirs)
{
  MergeResult result;
  std::set<std::string> all_files;

  // Collect all files from the three commits
  std::vector<std::string> base_files = get_commit_files(base);
  std::vector<std::string> our_files = get_commit_files(ours);
  std::vector<std::string> their_files = get_commit_files(theirs);

  // Combine all file lists
  all_files.insert(base_files.begin(), base_files.end());
  all_files.insert(our_files.begin(), our_files.end());
  all_files.insert(their_files.begin(), their_files.end());

  // Process each file
  for (const auto &file : all_files)
  {
    bool base_exists = std::filesystem::exists(base + "/" + file);    // Check if file exists in base
    bool our_exists = std::filesystem::exists(ours + "/" + file);     // Check if file exists in ours
    bool their_exists = std::filesystem::exists(theirs + "/" + file); // Check if file exists in theirs

    std::string base_content = base_exists ? get_file_content(base + "/" + file) : "";     // Get base content
    std::string our_content = our_exists ? get_file_content(ours + "/" + file) : "";       // Get our content
    std::string their_content = their_exists ? get_file_content(theirs + "/" + file) : ""; // Get their content

    if (!base_exists) // Handle new files
    {
      if (our_exists && their_exists) // Handle new file additions
      {
        if (our_content == their_content) // New file added identically by both
        {
          ErrorHandler::safeWriteFile(file, our_content); // or their_content, they are the same
          std::cout << "New file added by both: " << file << std::endl;
        }
        else // Conflict in new file
        {
          result.has_conflicts = true;
          result.conflicted_files.push_back(file);
          write_conflict(file, our_content, their_content);
          std::cout << "Conflict in new file: " << file << std::endl;
        }
      }
      else if (our_exists && !their_exists) // New file added by us
      {
        ErrorHandler::safeWriteFile(file, our_content);
        std::cout << "New file added by us: " << file << std::endl;
      }
      else if (!our_exists && their_exists) // New file added by them
      {
        ErrorHandler::safeWriteFile(file, their_content);
        std::cout << "New file added by them: " << file << std::endl;
      }
      continue;
    }

    // Handle deletions and modifications
    if (base_exists && !our_exists && !their_exists) // Deleted by both
    {
      std::remove(file.c_str());
      std::cout << "File deleted by both: " << file << std::endl;
      continue;
    }

    if (base_exists && !our_exists && their_exists) // Deleted by us
    {
      if (base_content == their_content) // No changes by them
      {
        std::remove(file.c_str());
        std::cout << "Deleted by us: " << file << std::endl;
      }
      else // Modified by them
      {
        result.has_conflicts = true;
        result.conflicted_files.push_back(file);
        write_conflict(file, "", their_content);
        std::cout << "Conflict: we deleted, they modified " << file << std::endl;
      }
      continue;
    }

    if (base_exists && our_exists && !their_exists) // Deleted by them
    {
      if (base_content == our_content) // No changes by us
      {
        std::remove(file.c_str());
        std::cout << "Deleted by them: " << file << std::endl;
      }
      else // Modified by us
      {
        result.has_conflicts = true;
        result.conflicted_files.push_back(file);
        write_conflict(file, our_content, "");
        std::cout << "Conflict: they deleted, we modified " << file << std::endl;
      }
      continue;
    }

    if (our_content == their_content) // Both sides identical
    {
      ErrorHandler::safeWriteFile(file, our_content);
      std::cout << "Both sides identical: " << file << std::endl;
      continue;
    }

    if (base_content == our_content) // Their changes
    {
      ErrorHandler::safeWriteFile(file, their_content);
      std::cout << "Merged theirs into " << file << std::endl;
      continue;
    }

    if (base_content == their_content) // Our changes
    {
      ErrorHandler::safeWriteFile(file, our_content);
      std::cout << "Kept ours in " << file << std::endl;
      continue;
    }

    //  Conflict
    result.has_conflicts = true;
    result.conflicted_files.push_back(file);
    write_conflict(file, our_content, their_content);
    std::cout << "Conflict in " << file << std::endl;
  }

  // Finalize merge result
  result.success = !result.has_conflicts;
  if (result.has_conflicts) // Save merge state if there are conflicts
  {
    result.message = "Merge conflicts detected";
    save_merge_state(result);
  }
  else // No conflicts
  {
    result.message = "Merge completed successfully";
  }

  return result;
}

bool has_conflicts()
{
  // Check if a merge is in progress and if there are any conflicted files
  return is_merge_in_progress() && !get_conflicted_files().empty();
}

void show_conflicts()
{
  // Retrieve and display conflicted files
  std::vector<std::string> conflicted_files = get_conflicted_files();

  if (conflicted_files.empty())
  {
    std::cout << "No conflicts found" << std::endl;
    return;
  }

  // Display conflicted files
  std::cout << "Conflicted files:" << std::endl;
  for (const auto &file : conflicted_files)
  {
    std::cout << "  " << file << std::endl;
  }
}

std::vector<std::string> get_conflicted_files()
{
  std::vector<std::string> conflicted_files;

  // Check if a merge is in progress
  if (!is_merge_in_progress())
  {
    return conflicted_files;
  }

  // Load merge state and retrieve conflicted files
  MergeResult merge_state = load_merge_state();
  return merge_state.conflicted_files;
}

void abort_merge()
{
  // Check if a merge is in progress
  if (!is_merge_in_progress())
  {
    std::cout << "No merge in progress" << std::endl;
    return;
  }

  // Clear merge state
  clear_merge_state();
  std::cout << "Merge aborted" << std::endl;
}

void continue_merge()
{
  // Check if a merge is in progress
  if (!is_merge_in_progress())
  {
    std::cout << "No merge in progress" << std::endl;
    return;
  }

  // Check for unresolved conflicts
  if (has_conflicts())
  {
    std::cout << "Cannot continue merge with unresolved conflicts" << std::endl;
    return;
  }

  // Load merge state and clear it
  MergeResult merge_state = load_merge_state();
  clear_merge_state();

  std::cout << "Merge continued successfully" << std::endl;
}

std::string find_merge_base(const std::string &commit1, const std::string &commit2)
{
  std::string current = commit1;
  std::set<std::string> visited;

  // Traverse the ancestry of commit1
  while (!current.empty() && visited.find(current) == visited.end())
  {
    // Mark current commit as visited
    visited.insert(current);

    // Check if current commit is an ancestor of commit2
    if (is_ancestor(current, commit2))
    {
      return current;
    }

    // Move to the parent commit
    current = get_last_commit(current);
  }

  return "";
}

bool is_ancestor(const std::string &ancestor, const std::string &descendant)
{
  std::string current = descendant;
  std::set<std::string> visited;

  // Traverse the ancestry of descendant
  while (!current.empty() && visited.find(current) == visited.end())
  {
    // Mark current commit as visited
    visited.insert(current);

    // Check if current commit matches the ancestor
    if (current == ancestor)
    {
      return true;
    }

    // Move to the parent commit
    current = get_last_commit(current);
  }

  return false;
}

bool is_fast_forward(const std::string &source, const std::string &target)
{
  // Check if source is an ancestor of target
  return is_ancestor(target, source);
}

void save_merge_state(const MergeResult &result)
{
  // Save merge state to a file
  std::string merge_file = ".bittrack/MERGE_HEAD";
  std::ofstream file(merge_file);
  file << "conflicts=" << (result.has_conflicts ? "true" : "false") << std::endl;

  // Save conflicted files
  for (const auto &conflicted_file : result.conflicted_files)
  {
    file << "conflict=" << conflicted_file << std::endl;
  }
  file.close();
}

MergeResult load_merge_state()
{
  // Load merge state from a file
  MergeResult result;
  std::string merge_file = ".bittrack/MERGE_HEAD";

  if (!std::filesystem::exists(merge_file))
  {
    return result;
  }

  // Read merge state
  std::ifstream file(merge_file);
  std::string line;

  while (std::getline(file, line))
  {
    if (line.find("conflicts=") == 0) // has conflicts
    {
      result.has_conflicts = (line.substr(10) == "true");
    }
    else if (line.find("conflict=") == 0) // conflicted file
    {
      result.conflicted_files.push_back(line.substr(9));
    }
  }

  file.close();
  return result;
}

void clear_merge_state()
{
  std::string merge_file = ".bittrack/MERGE_HEAD";
  if (std::filesystem::exists(merge_file))
  {
    std::filesystem::remove(merge_file);
  }
}

bool is_merge_in_progress()
{
  return std::filesystem::exists(".bittrack/MERGE_HEAD");
}

void create_merge_commit(const std::string &message, const std::vector<std::string> &parents)
{
  // Create a new commit object for the merge commit
  std::string commit_hash = generate_commit_hash(get_current_user(), message, get_current_timestamp());
  std::string commit_dir = ".bittrack/objects/" + commit_hash;
  std::filesystem::create_directories(commit_dir);

  // Write commit metadata
  std::string commit_file = commit_dir + "/commit.txt";
  std::ofstream file(commit_file);

  if (!file.is_open())
  {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR, "Could not create merge commit file", ErrorSeverity::ERROR, "create_merge_commit");
    return;
  }

  // Write commit details
  file << "commit " << commit_hash << std::endl;
  file << "message " << message << std::endl;
  file << "timestamp " << get_current_timestamp() << std::endl;
  file << "author " << get_current_user() << std::endl;

  // Write parent commits
  for (const auto &parent : parents)
  {
    file << "parent " << parent << std::endl;
  }

  file.close();

  // Update HEAD to point to the new merge commit
  std::ofstream head_file(".bittrack/refs/heads/" + get_current_branch());
  head_file << commit_hash << std::endl;
  head_file.close();

  // Insert commit record into history
  insert_commit_record_to_history(commit_hash, get_current_branch());

  std::cout << "Created merge commit: " << commit_hash << std::endl;
}
