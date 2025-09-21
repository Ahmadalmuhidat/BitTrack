#include "../include/stage.hpp"
#include "../include/branch.hpp"

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
        if (!std::filesystem::exists(path))
        {
          std::cerr << "Warning: File does not exist: " << path << std::endl;
          return;
        }

        if (std::filesystem::is_directory(path))
        {
          std::cerr << "Warning: Cannot stage directory: " << path << std::endl;
          return;
        }

        if (should_ignore_file(path))
        {
          std::cerr << "File ignored: " << path << std::endl;
          return;
        }

        std::string fileHash = hash_file(path);
        if (fileHash.empty())
        {
          std::cerr << "Warning: Could not generate hash for file: " << path << std::endl;
          return;
        }

        if (staged_files.find(path) != staged_files.end() && staged_files[path] == fileHash)
        {
          std::cout << "File already staged and unchanged: " << path << std::endl;
          return;
        }

        staged_files[path] = fileHash;
        std::cout << "Staged: " << path << std::endl;
      }
      catch (const std::exception &e)
      {
        std::cerr << "Error staging file " << path << ": " << e.what() << std::endl;
      }
    };

    if (file_path == ".")
    {
      for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
      {
        if (entry.is_regular_file())
        {
          stage_single_file(entry.path().string());
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

    std::vector<std::string> stagedFiles = get_staged_files();
    if (std::find(stagedFiles.begin(), stagedFiles.end(), filePath) == stagedFiles.end())
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
        if (staged_file_path != filePath)
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

    std::cout << "Unstaged: " << filePath << std::endl;
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

      std::istringstream iss(line);
      std::string fileName, fileHash;
      if (iss >> fileName >> fileHash)
      {
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
    std::string currentCommit = get_current_commit();
    if (!currentCommit.empty())
    {
      std::string commitDir = ".bittrack/objects/" + currentCommit;
      if (std::filesystem::exists(commitDir))
      {
        for (const auto &entry : std::filesystem::directory_iterator(commitDir))
        {
          if (entry.is_regular_file())
          {
            std::string fileName = entry.path().filename().string();
            trackedFiles.insert(fileName);
          }
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
          // If no commits yet, consider all files as untracked
          // If we have commits, consider files that are not in the current commit as unstaged
          if (currentCommit.empty() || trackedFiles.find(filePath) == trackedFiles.end())
          {
            unstagedFiles.insert(filePath);
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
    if (!currentCommit.empty())
    {
      std::string commitDir = ".bittrack/objects/" + currentCommit;
      
      if (std::filesystem::exists(commitDir))
      {
        for (const auto &entry : std::filesystem::directory_iterator(commitDir))
        {
          if (entry.is_regular_file())
          {
            std::string fileName = entry.path().filename().string();
            
            // Check if file exists in working directory
            if (std::filesystem::exists(fileName))
            {
              std::string currentHash = hash_file(fileName);
              std::string committedHash = hash_file(entry.path().string());
              
              // Only remove from unstaged if the hashes match (file unchanged since commit)
              if (currentHash == committedHash)
              {
                unstagedFiles.erase(fileName);
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

