#include "../include/stage.hpp"

std::unordered_map<std::string, std::string> loadStagedFiles() {
  // Load staged files from the .bittrack/index file
  std::unordered_map<std::string, std::string> staged_files;

  // If the staging index file does not exist, return an empty map
  if (!std::filesystem::exists(".bittrack/index")) {
    return staged_files;
  }

  // Open the staging index file for reading
  std::string staging_file = ErrorHandler::safeReadFile(".bittrack/index");
  std::istringstream staging_file_content(staging_file);

  // Read each line from the staging index file
  std::string line;
  while (std::getline(staging_file_content, line)) {
    if (line.empty()) {
      continue;
    }

    // Parse the file path and hash from the line
    std::istringstream iss(line);
    std::string staged_file_path, staged_file_hash;
    if (iss >> staged_file_path >> staged_file_hash) {
      staged_files[staged_file_path] = staged_file_hash;
    }
  }

  return staged_files;
}

void saveStagedFiles(
    const std::unordered_map<std::string, std::string> &staged_files) {
  // Save the staged files to the .bittrack/index file
  std::string temp_index_path = ".bittrack/index.tmp";
  std::string content = "";

  // Write each staged file and its hash to the temporary staging file
  for (const auto &pair : staged_files) {
    content += pair.first + " " + pair.second + "\n";
  }

  if (!ErrorHandler::safeWriteFile(temp_index_path, content)) {
    throw BitTrackError(ErrorCode::FILE_WRITE_ERROR,
                        "Cannot create temporary staging file",
                        ErrorSeverity::ERROR, "save_staged_files");
  }

  // Replace the old staging index file with the updated one
  if (std::filesystem::exists(".bittrack/index")) {
    ErrorHandler::safeRemoveFile(".bittrack/index");
  }
  ErrorHandler::safeRename(temp_index_path, ".bittrack/index");
}

bool validateFileForStaging(const std::string &file_path) {
  if (file_path.empty()) {
    return false;
  }

  // Check if the file is marked as deleted
  bool is_deleted_file = isDeleted(file_path);
  std::string actual_path = getActualPath(file_path);

  // If the file is not marked as deleted, perform additional checks
  if (!is_deleted_file) {
    // Check if the file exists
    if (!std::filesystem::exists(actual_path)) {
      ErrorHandler::printError(
          ErrorCode::FILE_NOT_FOUND, "File does not exist: " + actual_path,
          ErrorSeverity::WARNING, "validate_file_for_staging");
      return false;
    }

    // Check if the path is a directory
    if (std::filesystem::is_directory(actual_path)) {
      ErrorHandler::printError(ErrorCode::INVALID_FILE_PATH,
                               "Cannot stage directory: " + actual_path,
                               ErrorSeverity::WARNING,
                               "validate_file_for_staging");
      return false;
    }

    // Check if the file should be ignored
    if (shouldIgnoreFile(actual_path)) {
      return false;
    }
  }
  return true;
}

std::string calculateFileHash(const std::string &file_path) {
  // Check if the file is marked as deleted
  bool is_deleted_file = isDeleted(file_path);
  std::string actual_path = getActualPath(file_path);

  if (is_deleted_file) {
    return "";
  }

  // Generate the hash for the actual file path
  std::string fileHash = hashFile(actual_path);
  if (fileHash.empty()) {
    ErrorHandler::printError(ErrorCode::FILE_READ_ERROR,
                             "Could not generate hash for file: " + actual_path,
                             ErrorSeverity::WARNING, "calculate_file_hash");
  }

  return fileHash;
}

bool isFileUnchangedFromCommit(const std::string &file_path,
                               const std::string &file_hash) {
  // Get the current commit hash
  std::string currentCommit = getCurrentCommit();
  if (currentCommit.empty()) {
    return false;
  }

  // Check if the file exists in the current commit
  std::string commitDir = ".bittrack/objects/" + currentCommit;
  if (!std::filesystem::exists(commitDir)) {
    return false;
  }

  // Check if the file exists in the committed files
  std::string committedFilePath = commitDir + "/" + file_path;
  if (!std::filesystem::exists(committedFilePath)) {
    return false;
  }

  // Compare the file hash with the committed file hash
  std::string committedHash = hashFile(committedFilePath);
  return file_hash == committedHash;
}

std::unordered_set<std::string> getTrackedFiles() {
  // Retrieve the set of tracked files from the current branch's commit history
  std::unordered_set<std::string> trackedFiles;
  std::string currentBranch = getCurrentBranch();

  // Open the commit history file
  std::string history_file =
      ErrorHandler::safeReadFile(".bittrack/commits/history");
  std::istringstream history_file_content(history_file);

  // Read each line from the commit history file
  std::string line;
  while (std::getline(history_file_content, line)) {
    if (line.empty()) {
      continue;
    }

    // Parse the commit hash and branch from the line
    std::istringstream iss(line);
    std::string commitHash, branch;

    // Parse the commit hash and branch from the line
    if (iss >> commitHash >> branch && branch == currentBranch) {
      // Retrieve files from the commit
      std::string commitDir = ".bittrack/objects/" + commitHash;
      std::vector<std::filesystem::path> commitFiles =
          ErrorHandler::safeListDirectoryFiles(commitDir);
      if (std::filesystem::exists(
              commitDir)) // Check if commit directory exists
      {
        for (const auto &entry : commitFiles) {
          // Get the relative path of the file within the commit directory
          std::string filePath = entry.string();
          std::string relativePath =
              std::filesystem::relative(filePath, commitDir).string();
          trackedFiles.insert(relativePath);
        }
      }
    }
  }

  return trackedFiles;
}

void stageSingleFile(
    const std::string &file_path,
    std::unordered_map<std::string, std::string> &staged_files) {
  // Validate the file for staging
  if (!validateFileForStaging(file_path)) {
    return;
  }

  // Determine if the file is marked as deleted and get its actual path
  bool is_deleted_file = isDeleted(file_path);
  std::string actual_path = getActualPath(file_path);
  std::string file_hash = calculateFileHash(file_path);

  if (file_hash.empty() && !is_deleted_file) {
    return;
  }

  // Remove any existing entries for the file in the staging area
  staged_files.erase(actual_path);
  staged_files.erase(actual_path + " (deleted)");

  // Check if the file is already committed and unchanged
  if (!is_deleted_file && isFileUnchangedFromCommit(actual_path, file_hash)) {
    std::cout << "File already committed and unchanged: " << actual_path
              << std::endl;
    return;
  }

  // Add the file to the staging area
  std::string stored_path =
      is_deleted_file ? actual_path + " (deleted)" : actual_path;
  staged_files[stored_path] = file_hash;

  if (is_deleted_file) {
    std::cout << "Staged deletion: " << actual_path << std::endl;
  } else {
    std::cout << "Staged: " << actual_path << std::endl;
  }
}

void stageAllFiles(std::unordered_map<std::string, std::string> &staged_files) {
  // Stage all files in the working directory
  std::vector<std::filesystem::path> repository_files =
      ErrorHandler::safeListDirectoryFiles(".");
  for (const auto &entry : repository_files) {
    // Get the file path relative to the repository root
    std::string filePath = entry.string();

    // Skip files in the .bittrack directory and ignored files
    if (filePath.length() > 2 && filePath.substr(0, 2) == "./") {
      filePath = filePath.substr(2);
    }
    stageSingleFile(filePath, staged_files);
  }

  // Handle deleted files
  for (const auto &stagedFile : staged_files) {
    // If the staged file is not marked as deleted but does not exist in the
    // filesystem, mark it as deleted
    if (!isDeleted(stagedFile.first) &&
        !std::filesystem::exists(stagedFile.first)) {
      std::string deletedPath = stagedFile.first + " (deleted)";
      staged_files[deletedPath] = "";
      staged_files.erase(stagedFile.first);
    }
  }

  // Check for tracked files that have been deleted
  std::unordered_set<std::string> trackedFiles = getTrackedFiles();
  for (const auto &trackedFile : trackedFiles) {
    if (!std::filesystem::exists(trackedFile)) {
      stageSingleFile(trackedFile + " (deleted)", staged_files);
    }
  }
}

void stage(const std::string &file_path) {
  try {
    // Validate the file path
    if (file_path.empty()) {
      throw BitTrackError(ErrorCode::INVALID_FILE_PATH,
                          "File path cannot be empty", ErrorSeverity::ERROR,
                          "stage");
    }

    // Check if we are in a BitTrack repository
    if (!std::filesystem::exists(".bittrack")) {
      throw BitTrackError(ErrorCode::NOT_IN_REPOSITORY,
                          "Not in a BitTrack repository", ErrorSeverity::ERROR,
                          "stage");
    }

    // Load the current staged files
    std::unordered_map<std::string, std::string> staged_files =
        loadStagedFiles();

    // Stage the specified file or all files
    if (file_path == ".") {
      stageAllFiles(staged_files);
    } else {
      stageSingleFile(file_path, staged_files);
    }

    saveStagedFiles(staged_files);
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("stage")
}

void unstage(const std::string &filePath) {
  try {
    // Validate the file path
    if (filePath.empty()) {
      throw BitTrackError(ErrorCode::INVALID_FILE_PATH,
                          "File path cannot be empty", ErrorSeverity::ERROR,
                          "unstage");
    }

    // Check if we are in a BitTrack repository
    if (!std::filesystem::exists(".bittrack")) {
      throw BitTrackError(ErrorCode::NOT_IN_REPOSITORY,
                          "Not in a BitTrack repository", ErrorSeverity::ERROR,
                          "unstage");
    }

    // Check if there are any staged files
    if (!std::filesystem::exists(".bittrack/index")) {
      std::cout << "No staged files to unstage" << std::endl;
      return;
    }

    // Determine if the file is marked as deleted and get its actual path
    bool is_deleted_file = isDeleted(filePath);
    std::string actualFilePath = getActualPath(filePath);

    // Check if the file is staged
    std::vector<std::string> stagedFiles = getStagedFiles();
    bool found = false;

    // Look for the file in the staged files
    for (const auto &stagedFile : stagedFiles) {
      if (stagedFile == filePath) {
        found = true;
        break;
      }
    }

    // If the file is not found in the staged files, print a message and return
    if (!found) {
      std::cout << "File is not staged: " << filePath << std::endl;
      return;
    }

    // Open the staging index file for reading
    std::ifstream stagingFile(".bittrack/index");
    if (!stagingFile.is_open()) {
      throw BitTrackError(ErrorCode::FILE_READ_ERROR,
                          "Cannot open staging index file",
                          ErrorSeverity::ERROR, "unstage");
    }

    // Create a temporary file to store the updated staging index
    std::string temp_index_path = ".bittrack/index.tmp";
    std::ofstream tempFile(temp_index_path);

    if (!tempFile.is_open()) {
      stagingFile.close();
      throw BitTrackError(ErrorCode::FILE_WRITE_ERROR,
                          "Cannot create temporary staging file",
                          ErrorSeverity::ERROR, "unstage");
    }

    // Read each line from the staging index file and write to the temporary
    // file, excluding the specified file
    std::string line;
    bool file_removed = false;

    // Process each line in the staging index file
    while (std::getline(stagingFile, line)) {
      if (line.empty()) {
        continue;
      }

      // Parse the file path and hash from the line
      std::istringstream iss(line);
      std::string staged_file_path, staged_file_hash;

      // Check if the line corresponds to the file to be unstaged
      if (iss >> staged_file_path >> staged_file_hash) {
        bool should_remove = false;
        if (staged_file_path == actualFilePath) {
          should_remove = true;
        }

        if (!should_remove) {
          tempFile << line << std::endl;
        } else {
          file_removed = true;
        }
      }
    }

    // Close the files
    stagingFile.close();
    tempFile.close();

    // If the file was not removed, delete the temporary file and print a
    // message
    if (!file_removed) {
      std::filesystem::remove(temp_index_path);
      std::cout << "File was not found in staging area: " << filePath
                << std::endl;
      return;
    }

    // Replace the old staging index file with the updated one
    ErrorHandler::safeRemoveFile(".bittrack/index");
    ErrorHandler::safeRename(temp_index_path, ".bittrack/index");
  } catch (const BitTrackError &e) {
    ErrorHandler::printError(e);
    throw;
  }
  HANDLE_EXCEPTION("unstage")
}

std::vector<std::string> getStagedFiles() {
  std::vector<std::string> files;

  try {
    // If the staging index file does not exist, return an empty list
    if (!std::filesystem::exists(".bittrack/index")) {
      return files;
    }

    // Read each line from the staging index file
    std::string content = ErrorHandler::safeReadFile(".bittrack/index");
    if (content.empty()) {
      return files;
    }
    std::istringstream index_file(content);
    std::string line;
    while (std::getline(index_file, line)) {
      if (line.empty()) {
        continue;
      }

      // Parse the file name and hash from the line
      size_t last_space_pos = line.find_last_of(' ');
      if (last_space_pos != std::string::npos) {
        // Extract file name and hash
        std::string fileName = line.substr(0, last_space_pos);
        std::string fileHash = line.substr(last_space_pos + 1);

        // Trim whitespace from file hash
        fileHash.erase(0, fileHash.find_first_not_of(" \t"));
        fileHash.erase(fileHash.find_last_not_of(" \t") + 1);

        if (fileHash.empty()) {
          if (!isDeleted(fileName)) {
            fileName += " (deleted)";
          }
        }
        files.push_back(fileName);
      }
    }
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error reading staged files: " +
                                 std::string(e.what()),
                             ErrorSeverity::ERROR, "get_staged_files");
  }

  return files;
}

std::vector<std::string> getUnstagedFiles() {

  std::unordered_set<std::string> unstagedFiles;

  try {
    // Get the current commit and staged files
    std::string currentCommit = getCurrentCommit();
    std::vector<std::string> stagedFiles = getStagedFiles();
    std::set<std::string> stagedSet(stagedFiles.begin(), stagedFiles.end());

    // Get the list of committed files in the current commit
    std::unordered_set<std::string> committedFiles;
    if (!currentCommit.empty()) {
      // Retrieve files from the current commit
      std::string commitDir = ".bittrack/objects/" + currentCommit;
      if (std::filesystem::exists(commitDir)) {
        // Iterate through the committed files and add them to the set
        for (const auto &entry :
             ErrorHandler::safeListDirectoryFiles(commitDir)) {
          // Get the relative path of the file within the commit directory
          std::string relativePath =
              std::filesystem::relative(entry, commitDir).string();
          committedFiles.insert(relativePath);
        }
      }
    }

    // Check for modified or untracked files in the working directory
    for (const auto &entry : ErrorHandler::safeListDirectoryFiles(".")) {
      // Get the file path relative to the repository root
      std::string filePath = entry.string();

      // Skip files in the .bittrack directory and ignored files
      if (filePath.length() > 2 && filePath.substr(0, 2) == "./") {
        filePath = filePath.substr(2);
      }

      // Skip .bittrack files and ignored files
      if (filePath.find(".bittrack") != std::string::npos ||
          shouldIgnoreFile(filePath)) {
        continue;
      }

      // Skip files that are already staged
      if (stagedSet.find(filePath) != stagedSet.end()) {
        continue;
      }

      // Check if the file is modified compared to the committed version
      bool isModified = false;
      if (committedFiles.find(filePath) != committedFiles.end()) {
        // Compare the file hash with the committed file hash
        std::string commitDir = ".bittrack/objects/" + currentCommit;
        std::string committedFilePath = commitDir + "/" + filePath;

        if (std::filesystem::exists(committedFilePath)) {
          // Calculate hashes and compare
          std::string workingHash = hashFile(filePath);
          std::string committedHash = hashFile(committedFilePath);
          isModified = (workingHash != committedHash);
        }
      } else {
        isModified = true;
      }

      if (isModified) {
        unstagedFiles.insert(filePath);
      }
    }

    for (const auto &committedFile : committedFiles) {
      // If the committed file does not exist in the working directory and is
      // not staged, mark it as deleted
      if (!std::filesystem::exists(committedFile) &&
          stagedSet.find(committedFile) == stagedSet.end()) {
        unstagedFiles.insert(committedFile + " (deleted)");
      }
    }
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error getting unstaged files: " +
                                 std::string(e.what()),
                             ErrorSeverity::ERROR, "get_unstaged_files");
  }

  return std::vector<std::string>(unstagedFiles.begin(), unstagedFiles.end());
}

bool isStaged(const std::string &file_path) {
  try {
    if (file_path.empty()) {
      return false;
    }

    // Get the list of staged files
    std::vector<std::string> stagedFiles = getStagedFiles();
    return std::find(stagedFiles.begin(), stagedFiles.end(), file_path) !=
           stagedFiles.end(); // Check if the file is in the staged files list
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error checking if file is staged: " +
                                 std::string(e.what()),
                             ErrorSeverity::ERROR, "is_staged");
    return false;
  }
}

std::string getFileHash(const std::string &file_path) {
  try {
    if (file_path.empty() || !std::filesystem::exists(file_path)) {
      return "";
    }

    return hashFile(file_path);
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error getting file hash: " +
                                 std::string(e.what()),
                             ErrorSeverity::ERROR, "get_file_hash");
    return "";
  }
}

bool isDeleted(const std::string &file_path) {
  try {
    if (file_path.empty()) {
      return false;
    }

    const std::string deleted_suffix = " (deleted)";
    return file_path.length() > deleted_suffix.length() &&
           file_path.substr(file_path.length() - deleted_suffix.length()) ==
               deleted_suffix; // Check if the file path ends with " (deleted)"
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error checking if file is deleted: " +
                                 std::string(e.what()),
                             ErrorSeverity::ERROR, "is_deleted");
    return false;
  }
}

std::string getActualPath(const std::string &file_path) {
  try {
    if (file_path.empty()) {
      return file_path;
    }

    // If the file is marked as deleted, remove the " (deleted)" suffix
    if (isDeleted(file_path)) {
      const std::string deleted_suffix = " (deleted)";
      return file_path.substr(
          0,
          file_path.length() -
              deleted_suffix
                  .length()); // Return the file path without the deleted suffix
    }

    return file_path;
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error getting actual path: " +
                                 std::string(e.what()),
                             ErrorSeverity::ERROR, "get_actual_path");
    return file_path;
  }
}
