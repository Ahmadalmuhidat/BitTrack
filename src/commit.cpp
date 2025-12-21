#include "../include/commit.hpp"

void insertCommitRecordToHistory(const std::string &last_commit_hash,
                                 const std::string &new_branch_name) {
  // Prepend the new commit record to the history file
  const std::string history_path = ".bittrack/commits/history";

  // Read existing commit history safely
  std::string old_history = ErrorHandler::safeReadFile(history_path);

  // Build the new full file content:
  std::string new_entry = last_commit_hash + " " + new_branch_name + "\n";
  std::string updated_history = new_entry + old_history;

  // Save using the safe write function
  bool ok = ErrorHandler::safeWriteFile(history_path, updated_history);

  if (!ok) {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR,
                             "Failed to update commit history " + history_path,
                             ErrorSeverity::ERROR,
                             "insertCommitRecordToHistory");
  }
}

void storeSnapshot(const std::string &file_path,
                   const std::string &commit_hash) {
  // Create the snapshot directory structure
  std::string newDirPath = ".bittrack/objects/" + commit_hash;
  std::filesystem::path relativePath =
      std::filesystem::relative(file_path, ".");
  std::filesystem::path snapshot_path =
      std::filesystem::path(newDirPath) / relativePath;

  // Ensure the parent directories exist
  ErrorHandler::safeCreateDirectories(snapshot_path.parent_path());

  // Read the entire file content
  std::string buffer = ErrorHandler::safeReadFile(file_path);
  if (buffer.empty()) {
    ErrorHandler::printError(ErrorCode::FILE_READ_ERROR,
                             "Unable to read file: " + file_path,
                             ErrorSeverity::ERROR, "store_snapshot");
    return;
  }

  // Write the buffer content to the snapshot file
  if (!ErrorHandler::safeWriteFile(snapshot_path, buffer)) {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR,
                             "Unable to create snapshot file: " +
                                 snapshot_path.string(),
                             ErrorSeverity::ERROR, "store_snapshot");
    return;
  }
}

std::string getCommitParent(const std::string &commit_hash) {
  std::string commit_content =
      ErrorHandler::safeReadFile(".bittrack/commits/" + commit_hash);
  if (commit_content.empty()) {
    return "";
  }

  std::istringstream commit_stream(commit_content);
  std::string line;

  while (std::getline(commit_stream, line)) {
    if (line.rfind("Parent: ", 0) == 0) {
      std::string parent = line.substr(8);
      parent.erase(parent.find_last_not_of(" \t\n\r") + 1);
      return parent;
    }
  }

  return "";
}

void create_commit_log(
    const std::string &author, const std::string &message,
    const std::unordered_map<std::string, std::string> &file_hashes,
    const std::string &commit_hash) {
  // Create commit log file
  std::string log_file = ".bittrack/commits/" + commit_hash;
  std::string current_branch = getCurrentBranch();
  std::string parent_commit = get_last_commit(current_branch);

  // Get current timestamp
  std::time_t currentTime = std::time(nullptr);
  char formatedTimestamp[80];
  std::strftime(formatedTimestamp, sizeof(formatedTimestamp),
                "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));

  std::string content = "Author: " + author + "\n";
  content += "Branch: " + current_branch + "\n";
  content += "Timestamp: " + std::string(formatedTimestamp) + "\n";
  if (!parent_commit.empty()) {
    content += "Parent: " + parent_commit + "\n";
  }
  content += "Message: " + message + "\n";
  content += "Files: \n";

  // Write file paths and their hashes
  for (const auto &[filePath, fileHash] : file_hashes) {
    content += filePath + " " + fileHash + "\n";
  }

  if (!ErrorHandler::safeWriteFile(log_file, content)) {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR,
                             "Failed to create commit log: " + log_file,
                             ErrorSeverity::ERROR, "create_commit_log");
  }

  // Update commit history
  insertCommitRecordToHistory(commit_hash, current_branch);

  if (!ErrorHandler::safeWriteFile(".bittrack/refs/heads/" + current_branch,
                                   commit_hash + "\n")) {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR,
                             "Failed to update branch ref",
                             ErrorSeverity::ERROR, "create_commit_log");
  }
}

std::string get_last_commit(const std::string &branch) {
  // Read the history file safely
  std::string history_content =
      ErrorHandler::safeReadFile(".bittrack/commits/history");
  if (history_content.empty()) {
    return "";
  }

  std::istringstream history_stream(history_content);
  std::string line;

  // Iterate through the history file
  while (std::getline(history_stream, line)) {
    // Parse each line to extract commit hash and branch name
    std::istringstream iss(line);
    std::string commit_file_hash, commit_branch;

    // Check if the line has the correct format
    if (!(iss >> commit_file_hash >> commit_branch)) {
      continue;
    }

    // Check if the branch matches
    if (commit_branch == branch) {
      return commit_file_hash;
    }
  }
  return "";
}

void commit_changes(const std::string &author, const std::string &message) {
  // Check for unpushed commits
  if (has_unpushed_commits()) {
    ErrorHandler::printError(
        ErrorCode::UNCOMMITTED_CHANGES,
        "Cannot create new commit while there are unpushed commits",
        ErrorSeverity::ERROR, "commit_changes");
    ErrorHandler::printError(
        ErrorCode::UNCOMMITTED_CHANGES,
        "Please push your current commit before creating a new one",
        ErrorSeverity::INFO, "commit_changes");
    ErrorHandler::printError(ErrorCode::UNCOMMITTED_CHANGES,
                             "Use 'bittrack --push' to push your changes",
                             ErrorSeverity::INFO, "commit_changes");
    return;
  }

  // Prepare to collect file hashes
  std::unordered_map<std::string, std::string> file_hashes;
  std::string timestamp = std::to_string(std::time(nullptr));
  std::string commit_hash = generate_commit_hash(author, message, timestamp);
  std::string line;

  // Create commit directory
  std::string commit_dir = ".bittrack/objects/" + commit_hash;
  std::filesystem::create_directories(commit_dir);

  // Handle deleted files
  std::set<std::string> deleted_files;
  // Read staged deletions
  std::string staging_content = ErrorHandler::safeReadFile(".bittrack/index");
  if (!staging_content.empty()) {
    std::istringstream staging_stream(staging_content);
    std::string line;
    while (std::getline(staging_stream, line)) {
      if (line.empty()) {
        continue;
      }

      // Parse the line to extract file path and hash
      size_t last_space_pos = line.find_last_of(' ');
      if (last_space_pos != std::string::npos) {
        // Extract file path and hash
        std::string filePath = line.substr(0, last_space_pos);
        std::string fileHash = line.substr(last_space_pos + 1);

        // Trim whitespace from fileHash
        fileHash.erase(0, fileHash.find_first_not_of(" \t"));
        fileHash.erase(fileHash.find_last_not_of(" \t") + 1);

        // Check for deletions (empty hash)
        if (fileHash.empty()) {
          // Record deleted file
          std::string originalFilePath = getActualPath(filePath);
          deleted_files.insert(originalFilePath);
        }
      }
    }
  }

  // Copy unchanged files from previous commit
  std::string previous_commit = get_current_commit();
  if (!previous_commit.empty()) {
    // Copy files from previous commit, excluding deleted files
    std::string previous_commit_path = ".bittrack/objects/" + previous_commit;
    if (std::filesystem::exists(previous_commit_path)) {
      // Iterate through files in the previous commit
      for (const auto &entry :
           ErrorHandler::safeListDirectoryFiles(previous_commit_path)) {
        // Determine relative path
        std::string file_path_str = entry.string();
        std::string relative_path =
            std::filesystem::relative(entry, previous_commit_path).string();

        if (deleted_files.find(relative_path) != deleted_files.end()) {
          continue;
        }

        // Copy file to new commit directory
        std::string new_file_path = commit_dir + "/" + relative_path;
        ErrorHandler::safeCreateDirectories(
            std::filesystem::path(new_file_path).parent_path());
        ErrorHandler::safeCopyFile(entry, new_file_path);

        // Compute and store file hash
        std::string file_hash = hashFile(new_file_path);
        file_hashes[relative_path] = file_hash;
      }
    }
  }

  // Process staged files
  std::string staging_content_final =
      ErrorHandler::safeReadFile(".bittrack/index");
  if (staging_content_final.empty()) {
    ErrorHandler::printError(ErrorCode::STAGING_FAILED,
                             "no files staged for commit!",
                             ErrorSeverity::ERROR, "commit_changes");
    return;
  }

  std::istringstream staging_stream_final(staging_content_final);
  bool has_staged_files = false;
  while (std::getline(staging_stream_final, line)) {
    if (line.empty()) {
      continue;
    }

    has_staged_files = true;

    // Parse the line to extract file path and hash
    size_t last_space_pos = line.find_last_of(' ');
    if (last_space_pos != std::string::npos) {
      // Extract file path and hash
      std::string filePath = line.substr(0, last_space_pos);
      std::string fileHash = line.substr(last_space_pos + 1);

      fileHash.erase(0, fileHash.find_first_not_of(" \t"));
      fileHash.erase(fileHash.find_last_not_of(" \t") + 1);

      // Handle deletions
      if (fileHash.empty()) {
        // File is marked for deletion
        std::string originalFilePath = getActualPath(filePath);
        file_hashes[originalFilePath] = "";
        std::string deleted_file_path = commit_dir + "/" + originalFilePath;

        // Remove the file from the commit snapshot if it exists
        if (std::filesystem::exists(deleted_file_path)) {
          ErrorHandler::safeRemoveFile(deleted_file_path);
        }
      } else {
        // File is added or modified
        storeSnapshot(filePath, commit_hash);
        file_hashes[filePath] = fileHash;
      }
    }
  }

  // Check if there were any staged files
  if (!has_staged_files) {
    ErrorHandler::printError(ErrorCode::STAGING_FAILED, "No files to commit!",
                             ErrorSeverity::ERROR, "commit_changes");
    return;
  }

  // Create commit log
  create_commit_log(author, message, file_hashes, commit_hash);

  // Clear the staging area
  ErrorHandler::safeWriteFile(".bittrack/index", "");
}

std::string generate_commit_hash(const std::string &author,
                                 const std::string &message,
                                 const std::string &timestamp) {
  // Combine author, message, and timestamp to create a unique commit hash
  std::string combined = author + message + timestamp;
  return sha256Hash(combined);
}

std::string get_current_commit() {
  // Get the current branch name
  std::string current_branch = getCurrentBranch();
  if (current_branch.empty()) {
    return "";
  }

  // Read the current commit hash from the branch reference file safely
  std::string commit_hash =
      ErrorHandler::safeReadFile(".bittrack/refs/heads/" + current_branch);
  if (!commit_hash.empty() && commit_hash.back() == '\n') {
    commit_hash.pop_back();
  }
  return commit_hash;
}

std::string getStagedFileContent(const std::string &file_path) {
  // Read the content of the staged file
  std::string file = ErrorHandler::safeReadFile(file_path);
  std::istringstream file_content(file);
  std::string content;

  // Read the file content line by line
  std::string line;
  while (std::getline(file_content, line)) {
    content += line + "\n";
  }

  return content;
}

std::string get_current_timestamp() {
  // Get the current system time with milliseconds precision
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  // Format the timestamp as a string
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

  return ss.str();
}

std::string get_current_user() {
  // Retrieve the current user's name from environment variables or system calls
  const char *user = std::getenv("USER");
  if (user != nullptr) {
    return std::string(user);
  }

  // Try Windows environment variable
  user = std::getenv("USERNAME");
  if (user != nullptr) {
    return std::string(user);
  }

  // Fallback to getlogin_r for POSIX systems
  char buffer[256];
  if (getlogin_r(buffer, sizeof(buffer)) == 0) {
    return std::string(buffer);
  }

  return "unknown";
}

std::vector<std::string> getCommitFiles(const std::string &commit_hash) {
  // Retrieve the list of files associated with a specific commit
  std::vector<std::string> files;
  std::string commit_path = ".bittrack/objects/" + commit_hash;

  // Use safe list directory files
  std::vector<std::filesystem::path> commitFiles =
      ErrorHandler::safeListDirectoryFiles(commit_path);
  for (const auto &entry : commitFiles) {
    files.push_back(entry.filename().string());
  }
  return files;
}
