#include "../include/stash.hpp"

void stashChanges(const std::string &message)
{
  // Get staged files
  std::vector<std::string> staged_files = getStagedFiles();
  if (staged_files.empty())
  {
    std::cout << "No staged changes to stash" << std::endl;
    return;
  }

  // Create stash entry
  StashEntry entry;
  entry.id = generateStashId();
  entry.message = message.empty() ? "WIP on " + getCurrentBranchName() : message;
  entry.branch = getCurrentBranchName();
  entry.commit_hash = getCurrentCommit();
  entry.timestamp = std::time(nullptr);
  entry.files = staged_files;

  backupStagedFiles(entry.id);             // Backup staged files
  removeStagedFilesFromWorkingDirectory(); // Remove staged files from working directory

  if (!ErrorHandler::safeWriteFile(".bittrack/index", ""))
  { // Clear staging area
    ErrorHandler::printError(
        ErrorCode::FILE_WRITE_ERROR,
        "Could not clear staging area during stash",
        ErrorSeverity::ERROR,
        "stashChanges");
  }

  // Save stash entry
  saveStashEntry(entry);

  std::cout << "Stashed " << staged_files.size() << " staged files: " << entry.message << std::endl;
}

void stashList()
{
  // Get stash entries
  std::vector<StashEntry> entries = getStashEntries();

  if (entries.empty())
  {
    std::cout << "No stashes found" << std::endl;
    return;
  }

  // Display stash entries
  std::cout << "Stash list:" << std::endl;
  for (const auto &entry : entries)
  {
    std::cout << "  " << entry.id << ": " << entry.message << " (" << formatTimestamp(entry.timestamp) << ")" << std::endl;
  }
}

void stashShow(const std::string &stash_id)
{
  // If no stash ID provided, show the latest stash
  if (stash_id.empty())
  {
    // Get stash entries
    std::vector<StashEntry> entries = getStashEntries();
    if (entries.empty())
    {
      std::cout << "No stashes found" << std::endl;
      return;
    }

    // Show latest stash
    stashShow(entries[0].id);
    return;
  }

  // Get stash entry
  StashEntry entry = getStashEntry(stash_id);
  if (entry.id.empty())
  {
    std::cout << "Stash not found: " << stash_id << std::endl;
    return;
  }

  // Display stash details
  std::cout << "Stash: " << entry.id << std::endl;
  std::cout << "Message: " << entry.message << std::endl;
  std::cout << "Branch: " << entry.branch << std::endl;
  std::cout << "Commit: " << entry.commit_hash << std::endl;
  std::cout << "Timestamp: " << formatTimestamp(entry.timestamp) << std::endl;
  std::cout << "Files: " << entry.files.size() << std::endl;
}

void stashApply(const std::string &stash_id)
{
  // If no stash ID provided, apply the latest stash
  if (stash_id.empty())
  {
    // Get stash entries
    std::vector<StashEntry> entries = getStashEntries();
    if (entries.empty())
    {
      std::cout << "No stashes found" << std::endl;
      return;
    }

    // Apply latest stash
    stashApply(entries[0].id);
    return;
  }

  // Get stash entry
  StashEntry entry = getStashEntry(stash_id);
  if (entry.id.empty())
  {
    std::cout << "Stash not found: " << stash_id << std::endl;
    return;
  }

  // Restore working directory from stash
  restoreWorkingDirectory(entry.id);

  // Stage the restored files
  for (const auto &file : entry.files)
  {
    if (std::filesystem::exists(file))
    {
      stage(file);
    }
  }

  std::cout << "Applied stash: " << entry.message << " (" << entry.files.size() << " files staged)" << std::endl;
}

void stashPop(const std::string &stash_id)
{
  // Apply the stash
  stashApply(stash_id);
  if (!stash_id.empty())
  {
    // Drop the specified stash
    stashDrop(stash_id);
  }
  else
  {
    // Get stash entries
    std::vector<StashEntry> entries = getStashEntries();
    if (!entries.empty())
    {
      // Drop the latest stash
      stashDrop(entries[0].id);
    }
  }
}

void stashDrop(const std::string &stash_id)
{
  if (stash_id.empty())
  {
    std::cout << "Stash ID required for drop operation" << std::endl;
    return;
  }

  // Get stash entry
  StashEntry entry = getStashEntry(stash_id);
  if (entry.id.empty())
  {
    std::cout << "Stash not found: " << stash_id << std::endl;
    return;
  }

  // Remove stash files
  std::string stash_dir = getStashFilePath(stash_id);
  ErrorHandler::safeRemoveFile(stash_dir); // Delete stash directory

  // Delete stash entry from index
  deleteStashEntry(stash_id);

  std::cout << "Dropped stash: " << stash_id << std::endl;
}

void stashClear()
{
  // Get all stash entries
  std::vector<StashEntry> entries = getStashEntries();

  for (const auto &entry : entries)
  {
    std::string stash_dir = getStashFilePath(entry.id); // Remove stash files
    ErrorHandler::safeRemoveFolder(stash_dir);          // Delete stash directory
  }

  // Clear stash index
  std::string stash_index = getStashDir() + "/index";
  if (std::filesystem::exists(stash_index))
  {
    ErrorHandler::safeRemoveFile(stash_index); // Delete index file
  }

  std::cout << "Cleared all stashes" << std::endl;
}

bool stashHasStashes()
{
  // Check if there are any stash entries
  return !getStashEntries().empty();
}

std::vector<StashEntry> getStashEntries()
{
  // Read stash entries from index file
  std::vector<StashEntry> entries;
  std::string stash_index = getStashDir() + "/index";

  if (!std::filesystem::exists(stash_index))
  {
    return entries;
  }

  // Parse stash index file
  std::string file = ErrorHandler::safeReadFile(stash_index);
  std::istringstream file_content(file);
  std::string line;

  while (std::getline(file_content, line))
  {
    if (line.empty())
    {
      continue;
    }

    // Format: id|message|branch|commit_hash|timestamp
    StashEntry entry;
    std::istringstream iss(line);
    std::string token;

    if (std::getline(iss, token, '|'))
    {
      entry.id = token;
    }
    if (std::getline(iss, token, '|'))
    {
      entry.message = token;
    }
    if (std::getline(iss, token, '|'))
    {
      entry.branch = token;
    }
    if (std::getline(iss, token, '|'))
    {
      entry.commit_hash = token;
    }
    if (std::getline(iss, token, '|'))
    {
      entry.timestamp = std::stoll(token);
    }

    entries.push_back(entry);
  }

  return entries;
}

StashEntry getStashEntry(const std::string &stash_id)
{
  // Find stash entry by ID
  std::vector<StashEntry> entries = getStashEntries();

  for (const auto &entry : entries)
  {
    if (entry.id == stash_id)
    {
      // Load associated files
      StashEntry result = entry;

      // List files in the stash directory
      std::string stash_dir = getStashFilePath(stash_id);

      if (std::filesystem::exists(stash_dir))
      {
        // Recursively list files
        for (const auto &file_entry :
             ErrorHandler::safeListDirectoryFiles(stash_dir))
        {
          // Get relative path
          std::string rel_path = std::filesystem::relative(file_entry, stash_dir).string();
          result.files.push_back(rel_path);
        }
      }
      return result;
    }
  }

  return StashEntry();
}

void saveStashEntry(const StashEntry &entry)
{
  // Append stash entry to index file
  std::string stash_dir = getStashDir();
  std::string stash_index = stash_dir + "/index";
  ErrorHandler::safeCreateDirectories(stash_dir); // Ensure stash directory exists

  // Format: id|message|branch|commit_hash|timestamp
  std::string entry_line = entry.id + "|" + entry.message + "|" + entry.branch + "|" + entry.commit_hash + "|" + std::to_string(entry.timestamp) + "\n";

  if (!ErrorHandler::safeAppendFile(stash_index, entry_line))
  {
    ErrorHandler::printError(
        ErrorCode::FILE_WRITE_ERROR,
        "Could not save stash entry",
        ErrorSeverity::ERROR,
        "saveStashEntry");
  }
}

void deleteStashEntry(const std::string &stash_id)
{
  // Remove stash entry from index file
  std::vector<StashEntry> entries = getStashEntries();
  std::string stash_index = getStashDir() + "/index";

  // Rewrite index file without the specified stash entry
  std::string new_content = "";
  for (const auto &entry : entries)
  {
    // Skip the entry to be deleted
    if (entry.id != stash_id)
    {
      new_content += entry.id + "|" + entry.message + "|" + entry.branch + "|" + entry.commit_hash + "|" + std::to_string(entry.timestamp) + "\n";
    }
  }

  if (!ErrorHandler::safeWriteFile(stash_index, new_content))
  {
    ErrorHandler::printError(
        ErrorCode::FILE_WRITE_ERROR,
        "Could not update stash index after deletion",
        ErrorSeverity::ERROR,
        "deleteStashEntry");
  }
}

std::string generateStashId()
{
  return "stash_" + std::to_string(std::time(nullptr));
}

void backupStagedFiles(const std::string &stash_id)
{
  // Create stash directory
  std::string stash_dir = getStashFilePath(stash_id);
  ErrorHandler::safeCreateDirectories(stash_dir); // Ensure stash directory exists

  // Get staged files
  std::vector<std::string> staged_files = getStagedFiles();

  // Copy staged files to stash directory
  for (const auto &file : staged_files)
  {
    // Check if file exists
    if (std::filesystem::exists(file))
    {
      // Create destination path
      std::string dest_path = stash_dir + "/" + file;
      ErrorHandler::safeCreateDirectories(std::filesystem::path(dest_path).parent_path());
      ErrorHandler::safeCopyFile(file, dest_path);
    }
  }
}

void restoreWorkingDirectory(const std::string &stash_id)
{
  // Get stash directory
  std::string stash_dir = getStashFilePath(stash_id);

  if (!std::filesystem::exists(stash_dir))
  {
    std::cout << "Stash directory not found: " << stash_dir << std::endl;
    return;
  }

  // Restore files from stash directory to working directory
  for (const auto &entry : ErrorHandler::safeListDirectoryFiles(stash_dir))
  {
    // Get relative path
    std::string rel_path = std::filesystem::relative(entry, stash_dir).string();
    std::filesystem::path parent_path = std::filesystem::path(rel_path).parent_path();

    // Create parent directories if they don't exist
    if (!parent_path.empty())
    {
      ErrorHandler::safeCreateDirectories(parent_path);
    }
    // Copy file back to working directory
    ErrorHandler::safeCopyFile(entry, rel_path);
  }
}

std::vector<std::string> getAllTrackedFiles()
{
  // Get all tracked files (staged and unstaged)
  std::vector<std::string> files;
  std::vector<std::string> staged = getStagedFiles();
  std::vector<std::string> unstaged = getUnstagedFiles();

  // Combine and deduplicate
  files.insert(files.end(), staged.begin(), staged.end());
  files.insert(files.end(), unstaged.begin(), unstaged.end());

  // Remove duplicates
  std::sort(files.begin(), files.end());
  files.erase(std::unique(files.begin(), files.end()), files.end());

  return files;
}

void removeStagedFilesFromWorkingDirectory()
{
  // Get staged files
  std::vector<std::string> staged_files = getStagedFiles();

  // Remove staged files from working directory
  for (const auto &file : staged_files)
  {
    if (std::filesystem::exists(file))
    {
      ErrorHandler::safeRemoveFile(file);
    }
  }
}

std::string getStashDir()
{
  return ".bittrack/stash";
}

std::string getStashFilePath(const std::string &stash_id)
{
  return getStashDir() + "/" + stash_id;
}
