#include "../include/stage.hpp"

void stage(const std::string &file_path)
{
  try
  {
    if (file_path.empty())
    {
      throw BitTrackError(ErrorCode::INVALID_FILE_PATH, "File path cannot be empty", ErrorSeverity::ERROR, "stage");
    }

    if (!std::filesystem::exists(".bittrack"))
    {
      throw BitTrackError(ErrorCode::NOT_IN_REPOSITORY, "Not in a BitTrack repository", ErrorSeverity::ERROR, "stage");
    }

    std::unordered_map<std::string, std::string> staged_files;

    if (std::filesystem::exists(".bittrack/index"))
    {
      std::ifstream staging_file(".bittrack/index");
      if (!staging_file.is_open())
      {
        throw BitTrackError(ErrorCode::FILE_READ_ERROR, "Cannot open staging index file", ErrorSeverity::ERROR, "stage");
      }

      std::string line;
      while (std::getline(staging_file, line))
      {
        if (line.empty())
        {
          continue;
        }

        std::istringstream iss(line);
        std::string staged_file_path, staged_file_hash;
        if (iss >> staged_file_path >> staged_file_hash)
        {
          staged_files[staged_file_path] = staged_file_hash;
        }
      }
      staging_file.close();
    }

    auto stage_single_file = [&](const std::string &path)
    {
      try
      {
        // Check if this is a deleted file (has "(deleted)" suffix)
        bool is_deleted_file = false;
        std::string actual_path = path;
        if (path.length() > 10 && path.substr(path.length() - 10) == " (deleted)")
        {
          is_deleted_file = true;
          actual_path = path.substr(0, path.length() - 10);
        }

        if (!is_deleted_file && !std::filesystem::exists(actual_path))
        {
          std::cerr << "Warning: File does not exist: " << actual_path << std::endl;
          return;
        }

        if (!is_deleted_file && std::filesystem::is_directory(actual_path))
        {
          std::cerr << "Warning: Cannot stage directory: " << actual_path << std::endl;
          return;
        }

        if (!is_deleted_file && should_ignore_file(actual_path))
        {
          return;
        }

        std::string fileHash = "";
        if (!is_deleted_file)
        {
          fileHash = hash_file(actual_path);
          if (fileHash.empty())
          {
            std::cerr << "Warning: Could not generate hash for file: " << actual_path << std::endl;
            return;
          }
        }

        // Remove any existing staging entry for this file (both original and deleted versions)
        staged_files.erase(actual_path);
        staged_files.erase(actual_path + " (deleted)");
        
        // Check if file is already committed and unchanged (skip for deleted files)
        if (!is_deleted_file)
        {
          std::string currentCommit = get_current_commit();
          if (!currentCommit.empty())
          {
            std::string commitDir = ".bittrack/objects/" + currentCommit;
            if (std::filesystem::exists(commitDir))
            {
              // Check if the exact same file path exists in the commit
              std::string committedFilePath = commitDir + "/" + actual_path;
              if (std::filesystem::exists(committedFilePath))
              {
                std::string committedHash = hash_file(committedFilePath);
                if (fileHash == committedHash)
                {
                  std::cout << "File already committed and unchanged: " << actual_path << std::endl;
                  return;
                }
              }
            }
          }
        }

        // Store the file with (deleted) suffix if it's a deleted file
        std::string stored_path = is_deleted_file ? actual_path + " (deleted)" : actual_path;
        staged_files[stored_path] = fileHash;
        if (is_deleted_file)
        {
          std::cout << "Staged deletion: " << actual_path << std::endl;
        }
        else
        {
          std::cout << "Staged: " << actual_path << std::endl;
        }
      }
      catch (const std::exception &e)
      {
        std::cerr << "Error staging file " << path << ": " << e.what() << std::endl;
      }
    };

    if (file_path == ".")
    {
      // First, stage all existing files
      for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
      {
        if (entry.is_regular_file())
        {
          std::string filePath = entry.path().string();
          
          // Normalize path by removing leading "./" if present
          if (filePath.length() > 2 && filePath.substr(0, 2) == "./")
          {
            filePath = filePath.substr(2);
          }
          
          stage_single_file(filePath);
        }
      }
      
      // Check for staged files that no longer exist and mark them as deleted
      for (const auto& stagedFile : staged_files)
      {
        // Check if this is a regular file (not already marked as deleted)
        if (stagedFile.first.length() < 10 || stagedFile.first.substr(stagedFile.first.length() - 10) != " (deleted)")
        {
          if (!std::filesystem::exists(stagedFile.first))
          {
            // File was staged but no longer exists, mark it as deleted
            std::string deletedPath = stagedFile.first + " (deleted)";
            staged_files[deletedPath] = ""; // Empty hash for deleted files
            staged_files.erase(stagedFile.first); // Remove the original entry
          }
        }
      }
      
      // Then, check for deleted files that were previously tracked
      std::unordered_set<std::string> trackedFiles;
      std::string currentBranch = get_current_branch();
      
      // Read commit history to get all tracked files
      std::ifstream history_file(".bittrack/commits/history");
      if (history_file.is_open())
      {
        std::string line;
        while (std::getline(history_file, line))
        {
          if (line.empty()) continue;
          
          std::istringstream iss(line);
          std::string commitHash, branch;
          if (iss >> commitHash >> branch && branch == currentBranch)
          {
            // Get files from this commit
            std::string commitDir = ".bittrack/objects/" + commitHash;
            if (std::filesystem::exists(commitDir))
            {
              for (const auto &entry : std::filesystem::recursive_directory_iterator(commitDir))
              {
                if (entry.is_regular_file())
                {
                  std::string filePath = entry.path().string();
                  // Get relative path from commit directory
                  std::string relativePath = std::filesystem::relative(filePath, commitDir).string();
                  trackedFiles.insert(relativePath);
                }
              }
            }
          }
        }
        history_file.close();
      }
      
      // Check for deleted files (tracked files that no longer exist)
      for (const auto& trackedFile : trackedFiles)
      {
        if (!std::filesystem::exists(trackedFile))
        {
          // This is a deleted file, stage it
          stage_single_file(trackedFile + " (deleted)");
        }
      }
    }
    else
    {
      stage_single_file(file_path);
    }

    std::string temp_index_path = ".bittrack/index.tmp";
    std::ofstream updatedStagingFile(temp_index_path, std::ios::trunc);
    if (!updatedStagingFile.is_open())
    {
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR, "Cannot create temporary staging file", ErrorSeverity::ERROR, "stage");
    }

    for (const auto &pair : staged_files)
    {
      updatedStagingFile << pair.first << " " << pair.second << std::endl;
    }
    updatedStagingFile.close();

    if (std::filesystem::exists(".bittrack/index"))
    {
      std::filesystem::remove(".bittrack/index");
    }
    std::filesystem::rename(temp_index_path, ".bittrack/index");
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("stage")
}

void unstage(const std::string &filePath)
{
  try
  {
    if (filePath.empty())
    {
      throw BitTrackError(ErrorCode::INVALID_FILE_PATH, "File path cannot be empty", ErrorSeverity::ERROR, "unstage");
    }

    if (!std::filesystem::exists(".bittrack"))
    {
      throw BitTrackError(ErrorCode::NOT_IN_REPOSITORY, "Not in a BitTrack repository", ErrorSeverity::ERROR, "unstage");
    }

    if (!std::filesystem::exists(".bittrack/index"))
    {
      std::cout << "No staged files to unstage" << std::endl;
      return;
    }

    // Handle deleted files - check both with and without (deleted) suffix
    std::string actualFilePath = filePath;
    bool is_deleted_file = false;
    if (filePath.length() > 10 && filePath.substr(filePath.length() - 10) == " (deleted)")
    {
      is_deleted_file = true;
      actualFilePath = filePath.substr(0, filePath.length() - 10);
    }
    
    std::vector<std::string> stagedFiles = get_staged_files();
    bool found = false;
    for (const auto& stagedFile : stagedFiles)
    {
      if (stagedFile == filePath)
      {
        found = true;
        break;
      }
    }
    
    if (!found)
    {
      std::cout << "File is not staged: " << filePath << std::endl;
      return;
    }

    std::ifstream stagingFile(".bittrack/index");
    if (!stagingFile.is_open())
    {
      throw BitTrackError(ErrorCode::FILE_READ_ERROR, "Cannot open staging index file", ErrorSeverity::ERROR, "unstage");
    }

    std::string temp_index_path = ".bittrack/index.tmp";
    std::ofstream tempFile(temp_index_path);
    if (!tempFile.is_open())
    {
      stagingFile.close();
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR, "Cannot create temporary staging file", ErrorSeverity::ERROR, "unstage");
    }

    std::string line;
    bool file_removed = false;
    while (std::getline(stagingFile, line))
    {
      if (line.empty())
        continue;

      std::istringstream iss(line);
      std::string staged_file_path, staged_file_hash;
      if (iss >> staged_file_path >> staged_file_hash)
      {
        // Check if this is the file we want to unstage
        bool should_remove = false;
        if (staged_file_path == actualFilePath)
        {
          should_remove = true;
        }
        
        if (!should_remove)
        {
          tempFile << line << std::endl;
        }
        else
        {
          file_removed = true;
        }
      }
    }

    stagingFile.close();
    tempFile.close();

    if (!file_removed)
    {
      std::filesystem::remove(temp_index_path);
      std::cout << "File was not found in staging area: " << filePath << std::endl;
      return;
    }

    // Atomic rename
    std::filesystem::remove(".bittrack/index");
    std::filesystem::rename(temp_index_path, ".bittrack/index");
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("unstage")
}

std::vector<std::string> get_staged_files()
{
  std::vector<std::string> files;
  
  try
  {
    if (!std::filesystem::exists(".bittrack/index"))
    {
      return files;
    }

    std::ifstream index_file(".bittrack/index");
    if (!index_file.is_open())
    {
      std::cerr << "Warning: Cannot open staging index file" << std::endl;
      return files;
    }

    std::string line;
    while (std::getline(index_file, line))
    {
      if (line.empty())
        continue;

      // Parse the line manually to handle deleted files correctly
      // Find the last space to separate filename from hash
      size_t last_space_pos = line.find_last_of(' ');
      if (last_space_pos != std::string::npos)
      {
        std::string fileName = line.substr(0, last_space_pos);
        std::string fileHash = line.substr(last_space_pos + 1);
        
        // Trim whitespace from hash
        fileHash.erase(0, fileHash.find_first_not_of(" \t"));
        fileHash.erase(fileHash.find_last_not_of(" \t") + 1);
        
        // If the hash is empty or just whitespace, it means this is a deleted file
        if (fileHash.empty())
        {
          // The filename should already have (deleted) suffix, but let's make sure
          if (fileName.length() < 10 || fileName.substr(fileName.length() - 10) != " (deleted)")
          {
            fileName += " (deleted)";
          }
        }
        
        files.push_back(fileName);
      }
    }
    index_file.close();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error reading staged files: " << e.what() << std::endl;
  }

  return files;
}

std::vector<std::string> get_unstaged_files()
{
  std::unordered_set<std::string> unstagedFiles;

  try
  {
    // First, get all files that are tracked in the current branch's history
    std::unordered_set<std::string> trackedFiles;
    std::string currentBranch = get_current_branch();
    
    // Read commit history to get all tracked files
    std::ifstream history_file(".bittrack/commits/history");
    if (history_file.is_open())
    {
      std::string line;
      while (std::getline(history_file, line))
      {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string commitHash, branch;
        if (iss >> commitHash >> branch && branch == currentBranch)
        {
          // Get files from this commit
          std::string commitDir = ".bittrack/objects/" + commitHash;
          if (std::filesystem::exists(commitDir))
          {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(commitDir))
            {
              if (entry.is_regular_file())
              {
                std::string filePath = entry.path().string();
                // Get relative path from commit directory
                std::string relativePath = std::filesystem::relative(filePath, commitDir).string();
                trackedFiles.insert(relativePath);
              }
            }
          }
        }
      }
      history_file.close();
    }

    // Get currently staged files to exclude them from unstaged
    std::vector<std::string> stagedFiles = get_staged_files();
    std::set<std::string> stagedSet(stagedFiles.begin(), stagedFiles.end());

    // Check for deleted files (tracked files that no longer exist in working directory)
    // Get files from the current commit to see what files are currently tracked
    std::unordered_set<std::string> currentCommitFiles;
    std::string currentCommitHash = get_current_commit();
    if (!currentCommitHash.empty())
    {
      std::string commitDir = ".bittrack/objects/" + currentCommitHash;
      if (std::filesystem::exists(commitDir))
      {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(commitDir))
        {
          if (entry.is_regular_file())
          {
            std::string filePath = entry.path().string();
            std::string relativePath = std::filesystem::relative(filePath, commitDir).string();
            currentCommitFiles.insert(relativePath);
          }
        }
      }
    }
    
    for (const auto& trackedFile : trackedFiles)
    {
      if (!std::filesystem::exists(trackedFile))
      {
        // File was tracked but no longer exists - it's been deleted
        // Only add to unstaged if it's not already staged and it's in the current commit
        // (meaning it was deleted after the current commit)
        std::string deletedFileName = trackedFile + " (deleted)";
        if (stagedSet.find(trackedFile) == stagedSet.end() && 
            stagedSet.find(deletedFileName) == stagedSet.end() &&
            currentCommitFiles.find(trackedFile) != currentCommitFiles.end())
        {
          unstagedFiles.insert(deletedFileName);
        }
      }
    }

    // Check all files in working directory
    for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
    {
      if (entry.is_regular_file())
      {
        std::string filePath = entry.path().string();
        
        // Normalize path by removing leading "./" if present
        if (filePath.length() > 2 && filePath.substr(0, 2) == "./")
        {
          filePath = filePath.substr(2);
        }

        // Skip system files
        if (filePath.find(".bittrack") == std::string::npos && !should_ignore_file(filePath))
        {
          // Skip files that are already staged
          if (stagedSet.find(filePath) != stagedSet.end())
          {
            continue;
          }

          // If no tracked files yet, consider all files as untracked
          // If we have tracked files, consider files that are not tracked as unstaged
          if (trackedFiles.empty() || trackedFiles.find(filePath) == trackedFiles.end())
          {
            unstagedFiles.insert(filePath);
          }
          else
          {
            // File is tracked, check if it has been modified
            std::string currentCommit = get_current_commit();
            if (!currentCommit.empty())
            {
              std::string commitDir = ".bittrack/objects/" + currentCommit;
              if (std::filesystem::exists(commitDir))
              {
                std::string committedFilePath = commitDir + "/" + std::filesystem::path(filePath).filename().string();
                if (std::filesystem::exists(committedFilePath))
                {
                  // Compare file hashes to detect changes
                  std::string workingHash = hash_file(filePath);
                  std::string committedHash = hash_file(committedFilePath);
                  
                  if (workingHash != committedHash)
                  {
                    unstagedFiles.insert(filePath);
                  }
                }
              }
            }
          }
        }
      }
    }

    // Get staged files and their hashes
    std::unordered_map<std::string, std::string> stagedFilesWithHashes;
    if (std::filesystem::exists(".bittrack/index"))
    {
      std::ifstream index_file(".bittrack/index");
      if (index_file.is_open())
      {
        std::string line;
        while (std::getline(index_file, line))
        {
          if (line.empty())
            continue;

          std::istringstream iss(line);
          std::string fileName, fileHash;
          if (iss >> fileName >> fileHash)
          {
            stagedFilesWithHashes[fileName] = fileHash;
          }
        }
        index_file.close();
      }
    }

    // Remove staged files from unstaged list, but only if they haven't been modified
    for (const auto &stagedFile : stagedFilesWithHashes)
    {
      const std::string &fileName = stagedFile.first;
      const std::string &stagedHash = stagedFile.second;
      
      // Check if file exists and get its current hash
      if (std::filesystem::exists(fileName))
      {
        std::string currentHash = hash_file(fileName);
        // Only remove from unstaged if the hashes match (file unchanged)
        if (currentHash == stagedHash)
        {
          unstagedFiles.erase(fileName);
        }
        // If hashes don't match, keep the file in unstaged (it's been modified)
      }
      else
      {
        // File doesn't exist anymore, remove from unstaged
        unstagedFiles.erase(fileName);
      }
    }

    // Remove files that match the current commit (unchanged files)
    std::string currentCommit = get_current_commit();
    if (!currentCommit.empty())
    {
      std::string commitDir = ".bittrack/objects/" + currentCommit;
      
      if (std::filesystem::exists(commitDir))
      {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(commitDir))
        {
          if (entry.is_regular_file())
          {
            std::string filePath = entry.path().string();
            // Get relative path from commit directory
            std::string relativePath = std::filesystem::relative(filePath, commitDir).string();
            
            // Check if file exists in working directory
            if (std::filesystem::exists(relativePath))
            {
              std::string currentHash = hash_file(relativePath);
              std::string committedHash = hash_file(entry.path().string());
              
              // Only remove from unstaged if the hashes match (file unchanged since commit)
              if (currentHash == committedHash)
              {
                unstagedFiles.erase(relativePath);
              }
              // If hashes don't match, keep the file in unstaged (it's been modified since commit)
            }
          }
        }
      }
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error getting unstaged files: " << e.what() << std::endl;
  }

  return std::vector<std::string>(unstagedFiles.begin(), unstagedFiles.end());
}

bool is_staged(const std::string &file_path)
{
  try
  {
    if (file_path.empty())
    {
      return false;
    }

    std::vector<std::string> stagedFiles = get_staged_files();
    return std::find(stagedFiles.begin(), stagedFiles.end(), file_path) != stagedFiles.end();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error checking if file is staged: " << e.what() << std::endl;
    return false;
  }
}

std::string get_file_hash(const std::string &file_path)
{
  try
  {
    if (file_path.empty() || !std::filesystem::exists(file_path))
    {
      return "";
    }

    return hash_file(file_path);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error getting file hash: " << e.what() << std::endl;
    return "";
  }
}

bool is_file_ignored_by_patterns(const std::string &file_path, const std::vector<std::string> &patterns)
{
  try
  {
    if (file_path.empty())
    {
      return false;
    }

    std::vector<IgnorePattern> ignorePatterns;
    for (const auto &pattern : patterns)
    {
      if (!pattern.empty())
      {
        ignorePatterns.emplace_back(pattern);
      }
    }

    return ::is_file_ignored_by_ignore_patterns(file_path, ignorePatterns);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error checking if file is ignored: " << e.what() << std::endl;
    return false;
  }
}

