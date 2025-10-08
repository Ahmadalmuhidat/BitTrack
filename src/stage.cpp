#include "../include/stage.hpp"

std::unordered_map<std::string, std::string> load_staged_files()
{
  std::unordered_map<std::string, std::string> staged_files;

  if (!std::filesystem::exists(".bittrack/index"))
  {
    return staged_files;
  }

  std::ifstream staging_file(".bittrack/index");
  if (!staging_file.is_open())
  {
    throw BitTrackError(ErrorCode::FILE_READ_ERROR, "Cannot open staging index file", ErrorSeverity::ERROR, "load_staged_files");
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

  return staged_files;
}

void save_staged_files(const std::unordered_map<std::string, std::string> &staged_files)
{
  std::string temp_index_path = ".bittrack/index.tmp";
  std::ofstream updatedStagingFile(temp_index_path, std::ios::trunc);
  if (!updatedStagingFile.is_open())
  {
    throw BitTrackError(ErrorCode::FILE_WRITE_ERROR, "Cannot create temporary staging file", ErrorSeverity::ERROR, "save_staged_files");
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

bool validate_file_for_staging(const std::string &file_path)
{
  if (file_path.empty())
  {
    return false;
  }

  bool is_deleted_file = is_deleted(file_path);
  std::string actual_path = get_actual_path(file_path);

  if (!is_deleted_file)
  {
    if (!std::filesystem::exists(actual_path))
    {
      ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND, "File does not exist: " + actual_path, ErrorSeverity::WARNING, "validate_file_for_staging");
      return false;
    }

    if (std::filesystem::is_directory(actual_path))
    {
      ErrorHandler::printError(ErrorCode::INVALID_FILE_PATH, "Cannot stage directory: " + actual_path, ErrorSeverity::WARNING, "validate_file_for_staging");
      return false;
    }

    if (should_ignore_file(actual_path))
    {
      return false;
    }
  }
  return true;
}

std::string calculate_file_hash(const std::string &file_path)
{
  bool is_deleted_file = is_deleted(file_path);
  std::string actual_path = get_actual_path(file_path);

  if (is_deleted_file)
  {
    return "";
  }

  std::string fileHash = hash_file(actual_path);
  if (fileHash.empty())
  {
    ErrorHandler::printError(ErrorCode::FILE_READ_ERROR, "Could not generate hash for file: " + actual_path, ErrorSeverity::WARNING, "calculate_file_hash");
  }

  return fileHash;
}

bool is_file_unchanged_from_commit(const std::string &file_path, const std::string &file_hash)
{
  std::string currentCommit = get_current_commit();
  if (currentCommit.empty())
  {
    return false;
  }

  std::string commitDir = ".bittrack/objects/" + currentCommit;
  if (!std::filesystem::exists(commitDir))
  {
    return false;
  }

  std::string committedFilePath = commitDir + "/" + file_path;
  if (!std::filesystem::exists(committedFilePath))
  {
    return false;
  }

  std::string committedHash = hash_file(committedFilePath);
  return file_hash == committedHash;
}

std::unordered_set<std::string> get_tracked_files()
{
  std::unordered_set<std::string> trackedFiles;
  std::string currentBranch = get_current_branch();

  std::ifstream history_file(".bittrack/commits/history");
  if (!history_file.is_open())
  {
    return trackedFiles;
  }

  std::string line;
  while (std::getline(history_file, line))
  {
    if (line.empty())
      continue;

    std::istringstream iss(line);
    std::string commitHash, branch;
    if (iss >> commitHash >> branch && branch == currentBranch)
    {
      std::string commitDir = ".bittrack/objects/" + commitHash;
      if (std::filesystem::exists(commitDir))
      {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(commitDir))
        {
          if (entry.is_regular_file())
          {
            std::string filePath = entry.path().string();
            std::string relativePath = std::filesystem::relative(filePath, commitDir).string();
            trackedFiles.insert(relativePath);
          }
        }
      }
    }
  }
  history_file.close();

  return trackedFiles;
}

void stage_single_file_impl(const std::string &file_path, std::unordered_map<std::string, std::string> &staged_files)
{
  if (!validate_file_for_staging(file_path))
  {
    return;
  }

  bool is_deleted_file = is_deleted(file_path);
  std::string actual_path = get_actual_path(file_path);
  std::string file_hash = calculate_file_hash(file_path);

  if (file_hash.empty() && !is_deleted_file)
  {
    return;
  }

  staged_files.erase(actual_path);
  staged_files.erase(actual_path + " (deleted)");

  if (!is_deleted_file && is_file_unchanged_from_commit(actual_path, file_hash))
  {
    std::cout << "File already committed and unchanged: " << actual_path << std::endl;
    return;
  }

  std::string stored_path = is_deleted_file ? actual_path + " (deleted)" : actual_path;
  staged_files[stored_path] = file_hash;

  if (is_deleted_file)
  {
    std::cout << "Staged deletion: " << actual_path << std::endl;
  }
  else
  {
    std::cout << "Staged: " << actual_path << std::endl;
  }
}

void stage_all_files(std::unordered_map<std::string, std::string> &staged_files)
{
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string filePath = entry.path().string();

      if (filePath.length() > 2 && filePath.substr(0, 2) == "./")
      {
        filePath = filePath.substr(2);
      }
      stage_single_file_impl(filePath, staged_files);
    }
  }

  for (const auto &stagedFile : staged_files)
  {
    if (!is_deleted(stagedFile.first) && !std::filesystem::exists(stagedFile.first))
    {
      std::string deletedPath = stagedFile.first + " (deleted)";
      staged_files[deletedPath] = "";
      staged_files.erase(stagedFile.first);
    }
  }

  std::unordered_set<std::string> trackedFiles = get_tracked_files();
  for (const auto &trackedFile : trackedFiles)
  {
    if (!std::filesystem::exists(trackedFile))
    {
      stage_single_file_impl(trackedFile + " (deleted)", staged_files);
    }
  }
}

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

    std::unordered_map<std::string, std::string> staged_files = load_staged_files();

    if (file_path == ".")
    {
      stage_all_files(staged_files);
    }
    else
    {
      stage_single_file_impl(file_path, staged_files);
    }

    save_staged_files(staged_files);
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

    bool is_deleted_file = is_deleted(filePath);
    std::string actualFilePath = get_actual_path(filePath);

    std::vector<std::string> stagedFiles = get_staged_files();
    bool found = false;
    for (const auto &stagedFile : stagedFiles)
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
      ErrorHandler::printError(ErrorCode::FILE_READ_ERROR, "Cannot open staging index file", ErrorSeverity::ERROR, "get_staged_files");
      return files;
    }

    std::string line;
    while (std::getline(index_file, line))
    {
      if (line.empty())
      {
        continue;
      }

      size_t last_space_pos = line.find_last_of(' ');
      if (last_space_pos != std::string::npos)
      {
        std::string fileName = line.substr(0, last_space_pos);
        std::string fileHash = line.substr(last_space_pos + 1);

        fileHash.erase(0, fileHash.find_first_not_of(" \t"));
        fileHash.erase(fileHash.find_last_not_of(" \t") + 1);

        if (fileHash.empty())
        {
          if (!is_deleted(fileName))
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
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error reading staged files: " + std::string(e.what()), ErrorSeverity::ERROR, "get_staged_files");
  }

  return files;
}

std::vector<std::string> get_unstaged_files()
{
  std::unordered_set<std::string> unstagedFiles;

  try
  {
    std::string currentCommit = get_current_commit();
    std::vector<std::string> stagedFiles = get_staged_files();
    std::set<std::string> stagedSet(stagedFiles.begin(), stagedFiles.end());

    std::unordered_set<std::string> committedFiles;
    if (!currentCommit.empty())
    {
      std::string commitDir = ".bittrack/objects/" + currentCommit;
      if (std::filesystem::exists(commitDir))
      {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(commitDir))
        {
          if (entry.is_regular_file())
          {
            std::string relativePath = std::filesystem::relative(entry.path(), commitDir).string();
            committedFiles.insert(relativePath);
          }
        }
      }
    }

    for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
    {
      if (entry.is_regular_file())
      {
        std::string filePath = entry.path().string();

        if (filePath.length() > 2 && filePath.substr(0, 2) == "./")
        {
          filePath = filePath.substr(2);
        }

        if (filePath.find(".bittrack") != std::string::npos || should_ignore_file(filePath))
        {
          continue;
        }

        if (stagedSet.find(filePath) != stagedSet.end())
        {
          continue;
        }

        bool isModified = false;
        if (committedFiles.find(filePath) != committedFiles.end())
        {
          std::string commitDir = ".bittrack/objects/" + currentCommit;
          std::string committedFilePath = commitDir + "/" + filePath;
          if (std::filesystem::exists(committedFilePath))
          {
            std::string workingHash = hash_file(filePath);
            std::string committedHash = hash_file(committedFilePath);
            isModified = (workingHash != committedHash);
          }
        }
        else
        {
          isModified = true;
        }

        if (isModified)
        {
          unstagedFiles.insert(filePath);
        }
      }
    }

    for (const auto &committedFile : committedFiles)
    {
      if (!std::filesystem::exists(committedFile) && stagedSet.find(committedFile) == stagedSet.end())
      {
        unstagedFiles.insert(committedFile + " (deleted)");
      }
    }
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error getting unstaged files: " + std::string(e.what()), ErrorSeverity::ERROR, "get_unstaged_files");
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
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error checking if file is staged: " + std::string(e.what()), ErrorSeverity::ERROR, "is_staged");
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
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error getting file hash: " + std::string(e.what()), ErrorSeverity::ERROR, "get_file_hash");
    return "";
  }
}

bool is_deleted(const std::string &file_path)
{
  try
  {
    if (file_path.empty())
    {
      return false;
    }

    const std::string deleted_suffix = " (deleted)";
    return file_path.length() > deleted_suffix.length() && file_path.substr(file_path.length() - deleted_suffix.length()) == deleted_suffix;
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error checking if file is deleted: " + std::string(e.what()), ErrorSeverity::ERROR, "is_deleted");
    return false;
  }
}

std::string get_actual_path(const std::string &file_path)
{
  try
  {
    if (file_path.empty())
    {
      return file_path;
    }

    if (is_deleted(file_path))
    {
      const std::string deleted_suffix = " (deleted)";
      return file_path.substr(0, file_path.length() - deleted_suffix.length());
    }

    return file_path;
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION, "Error getting actual path: " + std::string(e.what()), ErrorSeverity::ERROR, "get_actual_path");
    return file_path;
  }
}
