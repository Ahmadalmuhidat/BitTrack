#include "../include/merge.hpp"

MergeResult mergeBranches(
    const std::string &source_branch,
    const std::string &target_branch)
{
  HookResult hook_result = runHook(HookType::PRE_MERGE);
  MergeResult result;
  if (!hook_result.success)
  {
    std::cout << hook_result.error << std::endl;
    result.success = false;
    return result;
  }

  // Get the last commit hashes of both branches
  std::string source_commit = getBranchLastCommitHash(source_branch);
  std::string target_commit = getBranchLastCommitHash(target_branch);

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
  if (isFastForward(source_branch, target_branch))
  {
    result.success = true;
    result.message = "Fast-forward merge";
    result.merge_commit = source_commit;
    return result;
  }

  // Perform three-way merge
  std::string merge_base = findMergeBase(source_commit, target_commit);
  if (merge_base.empty())
  {
    result.message = "No common ancestor found";
    return result;
  }

  // Execute the three-way merge
  result = threeWayMerge(merge_base, target_commit, source_commit);

  // Create merge commit if successful and no conflicts
  if (result.success)
  {
    std::string merge_message = "Merge branch '" + source_branch + "' into " + target_branch;
    createMergeCommit(merge_message, {target_commit, source_commit});
    result.message = getCurrentCommit();
  }

  return result;
}

MergeResult mergeCommits(
    const std::string &commit1,
    const std::string &commit2)
{
  MergeResult result;

  // Check if commits exist
  std::string merge_base = findMergeBase(commit1, commit2);
  if (merge_base.empty())
  {
    result.message = "No common ancestor found";
    return result;
  }

  // Execute the three-way merge
  result = threeWayMerge(merge_base, commit1, commit2);
  return result;
}

void writeConflict(
    const std::string &path,
    const std::string &ours,
    const std::string &theirs)
{
  std::ofstream conflict(path);
  conflict << "<<<<<<< HEAD\n"
           << ours
           << "\n=======\n"
           << theirs
           << "\n>>>>>>> theirs\n";
  conflict.close();
}

MergeResult threeWayMerge(
    const std::string &base,
    const std::string &ours,
    const std::string &theirs)
{
  MergeResult result;
  std::set<std::string> all_files;

  // Collect all files from the three commits
  std::vector<std::string> base_files = getCommitFiles(base);
  std::vector<std::string> our_files = getCommitFiles(ours);
  std::vector<std::string> their_files = getCommitFiles(theirs);

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

    std::string base_content = base_exists ? ErrorHandler::safeReadFile(base + "/" + file) : "";     // Get base content
    std::string our_content = our_exists ? ErrorHandler::safeReadFile(ours + "/" + file) : "";       // Get our content
    std::string their_content = their_exists ? ErrorHandler::safeReadFile(theirs + "/" + file) : ""; // Get their content

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
          writeConflict(file, our_content, their_content);
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
        writeConflict(file, "", their_content);
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
        writeConflict(file, our_content, "");
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

    // Attempt automatic merge before declaring a conflict
    std::string our_path = ours + "/" + file;
    std::string their_path = theirs + "/" + file;

    if (attemptAutomaticMerge(our_path, their_path, file))
    {
      std::cout << "Successfully auto-merged: " << file << std::endl;
      continue; // Skip the conflict block
    }

    // If auto-merge fails, handle as conflict
    result.has_conflicts = true;
    result.conflicted_files.push_back(file);
    writeConflict(file, our_content, their_content);
    std::cout << "Conflict in " << file << std::endl;
  }

  // Finalize merge result
  result.success = !result.has_conflicts;
  if (result.has_conflicts) // Save merge state if there are conflicts
  {
    result.message = "Merge conflicts detected";
    saveMergeState(result);
  }
  else // No conflicts
  {
    result.message = "Merge completed successfully";
  }

  return result;
}

bool hasConflicts()
{
  // Check if a merge is in progress and if there are any conflicted files
  return isMergeInProgress() && !getConflictedFiles().empty();
}

void showConflicts()
{
  // Retrieve and display conflicted files
  std::vector<std::string> conflicted_files = getConflictedFiles();

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

std::vector<std::string> getConflictedFiles()
{
  std::vector<std::string> conflicted_files;

  // Check if a merge is in progress
  if (!isMergeInProgress())
  {
    return conflicted_files;
  }

  // Load merge state and retrieve conflicted files
  MergeResult merge_state = loadMergeState();
  return merge_state.conflicted_files;
}

void abortMerge()
{
  // Check if a merge is in progress
  if (!isMergeInProgress())
  {
    std::cout << "No merge in progress" << std::endl;
    return;
  }

  // Clear merge state
  clearMergeState();
  std::cout << "Merge aborted" << std::endl;
}

void continueMerge()
{
  // Check if a merge is in progress
  if (!isMergeInProgress())
  {
    std::cout << "No merge in progress" << std::endl;
    return;
  }

  // Check for unresolved conflicts
  if (hasConflicts())
  {
    std::cout << "Cannot continue merge with unresolved conflicts" << std::endl;
    return;
  }

  // Load merge state and clear it
  MergeResult merge_state = loadMergeState();
  clearMergeState();

  std::cout << "Merge continued successfully" << std::endl;
}

std::string findMergeBase(
    const std::string &commit1,
    const std::string &commit2)
{
  std::string current = commit1;
  std::set<std::string> visited;

  // Traverse the ancestry of commit1
  while (!current.empty() && visited.find(current) == visited.end())
  {
    // Mark current commit as visited
    visited.insert(current);

    // Check if current commit is an ancestor of commit2
    if (isAncestor(current, commit2))
    {
      return current;
    }

    // Move to the parent commit
    current = getLastCommit(current);
  }

  return "";
}

bool isAncestor(
    const std::string &ancestor,
    const std::string &descendant)
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
    current = getLastCommit(current);
  }

  return false;
}

bool isFastForward(
    const std::string &source,
    const std::string &target)
{
  // Check if source is an ancestor of target
  return isAncestor(target, source);
}

void saveMergeState(const MergeResult &result)
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

MergeResult loadMergeState()
{
  // Load merge state from a file
  MergeResult result;
  std::string merge_file = ".bittrack/MERGE_HEAD";

  if (!std::filesystem::exists(merge_file))
  {
    return result;
  }

  // Read merge state
  std::string file = ErrorHandler::safeReadFile(merge_file);
  std::istringstream file_content(file);
  std::string line;

  while (std::getline(file_content, line))
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

  return result;
}

void clearMergeState()
{
  ErrorHandler::safeRemoveFile(".bittrack/MERGE_STATE");
}

bool isMergeInProgress()
{
  return std::filesystem::exists(".bittrack/MERGE_HEAD");
}

void createMergeCommit(
    const std::string &message,
    const std::vector<std::string> &parents)
{
  // Create a new commit object for the merge commit
  std::string commit_hash = generateCommitHash(getCurrentUser(), message, getCurrentTimestamp());
  std::string commit_dir = ".bittrack/objects/" + commit_hash;
  ErrorHandler::safeCreateDirectories(commit_dir);

  // Write commit metadata
  std::string commit_file = commit_dir + "/commit.txt";
  ErrorHandler::safeWriteFile(
      commit_file,
      "commit " + commit_hash + "\n" +
          "message " + message + "\n" +
          "timestamp " + getCurrentTimestamp() + "\n" +
          "author " + getCurrentUser() + "\n");

  // Write parent commits
  for (const auto &parent : parents)
  {
    ErrorHandler::safeWriteFile(commit_file, "parent " + parent + "\n");
  }

  // Update HEAD to point to the new merge commit
  std::ofstream head_file(".bittrack/refs/heads/" + getCurrentBranchName());
  head_file << commit_hash << std::endl;
  head_file.close();

  // Insert commit record into history
  insertCommitRecordToHistory(commit_hash, getCurrentBranchName());

  std::cout << "Created merge commit: " << commit_hash << std::endl;
}
