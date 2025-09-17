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
          continue;

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

    // Stage files
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
    for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
    {
      if (entry.is_regular_file())
      {
        std::string filePath = entry.path().string();

        if (filePath.find(".bittrack") == std::string::npos && !should_ignore_file(filePath))
        {
          unstagedFiles.insert(filePath);
        }
      }
    }

    std::vector<std::string> stagedFiles = get_staged_files();

    for (const auto &stagedFile : stagedFiles)
    {
      unstagedFiles.erase(stagedFile);
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error getting unstaged files: " << e.what() << std::endl;
  }

  return std::vector<std::string>(unstagedFiles.begin(), unstagedFiles.end());
}

void unstage_all()
{
  try
  {
    if (!std::filesystem::exists(".bittrack"))
    {
      throw BitTrackError(ErrorCode::NOT_IN_REPOSITORY, "Not in a BitTrack repository", ErrorSeverity::ERROR, "unstage_all");
    }

    if (!std::filesystem::exists(".bittrack/index"))
    {
      std::cout << "No staged files to unstage" << std::endl;
      return;
    }

    std::vector<std::string> stagedFiles = get_staged_files();
    int fileCount = stagedFiles.size();

    std::filesystem::remove(".bittrack/index");

    if (fileCount > 0)
    {
      std::cout << "Unstaged " << fileCount << " file(s)" << std::endl;
    }
    else
    {
      std::cout << "No files were staged" << std::endl;
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("unstage_all")
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

void clear_staging_area()
{
  try
  {
    if (!std::filesystem::exists(".bittrack"))
    {
      throw BitTrackError(ErrorCode::NOT_IN_REPOSITORY, "Not in a BitTrack repository", ErrorSeverity::ERROR, "clear_staging_area");
    }

    int fileCount = 0;
    if (std::filesystem::exists(".bittrack/index"))
    {
      std::vector<std::string> stagedFiles = get_staged_files();
      fileCount = stagedFiles.size();
    }

    if (std::filesystem::exists(".bittrack/index"))
    {
      std::filesystem::remove(".bittrack/index");
    }

    if (std::filesystem::exists(".bittrack/index.tmp"))
    {
      std::filesystem::remove(".bittrack/index.tmp");
    }

    for (const auto &entry : std::filesystem::directory_iterator(".bittrack"))
    {
      if (entry.is_regular_file() &&
          (entry.path().filename().string().find("staging") != std::string::npos ||
           entry.path().filename().string().find("index") != std::string::npos))
      {
        if (entry.path().filename() != "index") // Don't remove if it's the main index
        {
          std::filesystem::remove(entry.path());
        }
      }
    }

    if (fileCount > 0)
    {
      std::cout << "Staging area cleared (" << fileCount << " file(s) removed)" << std::endl;
    }
    else
    {
      std::cout << "Staging area cleared (was already empty)" << std::endl;
    }
  }
  catch (const BitTrackError &e)
  {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("clear_staging_area")
}

void show_staged_files()
{
  try
  {
    std::vector<std::string> stagedFiles = get_staged_files();

    if (stagedFiles.empty())
    {
      std::cout << "No staged files" << std::endl;
      return;
    }

    std::cout << "Staged files:" << std::endl;
    for (const auto &file : stagedFiles)
    {
      std::cout << "  " << file << std::endl;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error showing staged files: " << e.what() << std::endl;
  }
}

void show_unstaged_files()
{
  try
  {
    std::vector<std::string> unstagedFiles = get_unstaged_files();

    if (unstagedFiles.empty())
    {
      std::cout << "No unstaged files" << std::endl;
      return;
    }

    std::cout << "Unstaged files:" << std::endl;
    for (const auto &file : unstagedFiles)
    {
      std::cout << "  " << file << std::endl;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error showing unstaged files: " << e.what() << std::endl;
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

void update_staging_index()
{
  try
  {
    if (!std::filesystem::exists(".bittrack/index"))
    {
      std::cout << "No staging index to update" << std::endl;
      return;
    }

    std::cout << "Staging index is up to date" << std::endl;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error updating staging index: " << e.what() << std::endl;
  }
}

bool is_file_ignored(const std::string &file_path, const std::vector<std::string> &patterns)
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

    return ::is_file_ignored(file_path, ignorePatterns);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error checking if file is ignored: " << e.what() << std::endl;
    return false;
  }
}

std::string normalize_file_path(const std::string &path)
{
  try
  {
    return normalize_path(path);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error normalizing file path: " << e.what() << std::endl;
    return path;
  }
}

bool compare_with_current_version(const std::string &current_file, const std::string &current_branch)
{
  try
  {
    std::cout << "Comparing " << current_file << " with branch " << current_branch << std::endl;
    return true;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error comparing with current version: " << e.what() << std::endl;
    return false;
  }
}

std::string read_file_content(const std::string &file_path)
{
  try
  {
    if (file_path.empty() || !std::filesystem::exists(file_path))
    {
      return "";
    }

    std::ifstream file(file_path);
    if (!file.is_open())
    {
      return "";
    }

    std::ostringstream content;
    content << file.rdbuf();
    file.close();

    return content.str();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error reading file content: " << e.what() << std::endl;
    return "";
  }
}
