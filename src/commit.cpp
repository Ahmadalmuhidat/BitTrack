#include "../include/commit.hpp"

void insert_commit_record_to_history(const std::string &last_commit_hash, const std::string &new_branch_name)
{
  // Prepend the new commit record to the history file
  const std::string history_path = ".bittrack/commits/history";

  std::ifstream in_file(history_path);
  std::stringstream buffer;
  buffer << in_file.rdbuf();
  in_file.close();

  // Write the new commit record followed by the existing content
  std::ofstream out_file(history_path);
  out_file << last_commit_hash << " " << new_branch_name << "\n"
           << buffer.str();
  out_file.close();
}

void store_snapshot(const std::string &file_path, const std::string &commit_hash)
{
  // Create the snapshot directory structure
  std::string newDirPath = ".bittrack/objects/" + commit_hash;
  std::filesystem::path relativePath = std::filesystem::relative(file_path, ".");
  std::filesystem::path snapshot_path = std::filesystem::path(newDirPath) / relativePath;

  // Ensure the parent directories exist
  std::filesystem::create_directories(snapshot_path.parent_path());

  // Copy the file to the snapshot location
  std::ifstream inputFile(file_path, std::ios::binary);
  if (!inputFile)
  {
    ErrorHandler::printError(ErrorCode::FILE_READ_ERROR, "Unable to open file: " + file_path, ErrorSeverity::ERROR, "store_snapshot");
  }

  // Read the entire file content into a buffer
  std::stringstream buffer;
  buffer << inputFile.rdbuf();
  inputFile.close();

  // Write the buffer content to the snapshot file
  std::ofstream snapshotFile(snapshot_path, std::ios::binary);
  if (!snapshotFile)
  {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR, "Unable to create snapshot file: " + snapshot_path.string(), ErrorSeverity::ERROR, "store_snapshot");
  }
  snapshotFile << buffer.str();
  snapshotFile.close();
}

void create_commit_log(const std::string &author, const std::string &message, const std::unordered_map<std::string, std::string> &file_hashes, const std::string &commit_hash)
{
  // Create commit log file
  std::string log_file = ".bittrack/commits/" + commit_hash;
  std::string current_branch = get_current_branch();

  // Get current timestamp
  std::time_t currentTime = std::time(nullptr);
  char formatedTimestamp[80];
  std::strftime(formatedTimestamp, sizeof(formatedTimestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));

  std::ofstream commitFile(log_file);
  commitFile << "Author: " << author << std::endl;
  commitFile << "Branch: " << current_branch << std::endl;
  commitFile << "Timestamp: " << formatedTimestamp << std::endl;
  commitFile << "Message: " << message << std::endl;
  commitFile << "Files: " << std::endl;

  // Write file paths and their hashes
  for (const auto &[filePath, fileHash] : file_hashes)
  {
    commitFile << filePath << " " << fileHash << std::endl;
  }
  commitFile.close();

  // Update commit history
  insert_commit_record_to_history(commit_hash, current_branch);

  // Update branch reference
  std::ofstream head_file(".bittrack/refs/heads/" + current_branch, std::ios::trunc);
  head_file << commit_hash << std::endl;
  head_file.close();
}

std::string get_last_commit(const std::string &branch)
{
  // Read the history file to find the last commit for the specified branch
  std::ifstream history_file(".bittrack/commits/history");
  std::string line;

  // Iterate through the history file
  while (std::getline(history_file, line))
  {
    // Parse each line to extract commit hash and branch name
    std::istringstream iss(line);
    std::string commit_file_hash;
    std::string commit_branch;

    // Check if the line has the correct format
    if (!(iss >> commit_file_hash >> commit_branch))
    {
      continue;
    }

    // Check if the branch matches
    if (commit_branch == branch)
    {
      return commit_file_hash;
    }
  }
  return "";
}

void commit_changes(const std::string &author, const std::string &message)
{
  // Check for unpushed commits
  if (has_unpushed_commits())
  {
    ErrorHandler::printError(ErrorCode::UNCOMMITTED_CHANGES, "Cannot create new commit while there are unpushed commits", ErrorSeverity::ERROR, "commit_changes");
    ErrorHandler::printError(ErrorCode::UNCOMMITTED_CHANGES, "Please push your current commit before creating a new one", ErrorSeverity::INFO, "commit_changes");
    ErrorHandler::printError(ErrorCode::UNCOMMITTED_CHANGES, "Use 'bittrack --push' to push your changes", ErrorSeverity::INFO, "commit_changes");
    return;
  }

  // Read staged files from the index
  std::ifstream staging_file(".bittrack/index");
  if (!staging_file)
  {
    ErrorHandler::printError(ErrorCode::STAGING_FAILED, "no files staged for commit!", ErrorSeverity::ERROR, "commit_changes");
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
  std::ifstream staging_file_for_deletions(".bittrack/index");

  // Read staged deletions
  if (staging_file_for_deletions.is_open())
  {
    // Reset file pointer to the beginning
    std::string line;
    while (std::getline(staging_file_for_deletions, line))
    {
      if (line.empty())
      {
        continue;
      }

      // Parse the line to extract file path and hash
      size_t last_space_pos = line.find_last_of(' ');
      if (last_space_pos != std::string::npos)
      {
        // Extract file path and hash
        std::string filePath = line.substr(0, last_space_pos);
        std::string fileHash = line.substr(last_space_pos + 1);

        // Trim whitespace from fileHash
        fileHash.erase(0, fileHash.find_first_not_of(" \t"));
        fileHash.erase(fileHash.find_last_not_of(" \t") + 1);

        // Check for deletions (empty hash)
        if (fileHash.empty())
        {
          // Record deleted file
          std::string originalFilePath = get_actual_path(filePath);
          deleted_files.insert(originalFilePath);
        }
      }
    }
    staging_file_for_deletions.close();
  }

  // Copy unchanged files from previous commit
  std::string previous_commit = get_current_commit();
  if (!previous_commit.empty())
  {
    // Copy files from previous commit, excluding deleted files
    std::string previous_commit_path = ".bittrack/objects/" + previous_commit;
    if (std::filesystem::exists(previous_commit_path))
    {
      // Iterate through files in the previous commit
      for (const auto &entry : std::filesystem::recursive_directory_iterator(previous_commit_path))
      {
        // Only process regular files
        if (entry.is_regular_file())
        {
          // Determine relative path
          std::string file_path = entry.path().string();
          std::string relative_path = std::filesystem::relative(file_path, previous_commit_path).string();

          if (deleted_files.find(relative_path) != deleted_files.end())
          {
            continue;
          }

          // Copy file to new commit directory
          std::string new_file_path = commit_dir + "/" + relative_path;
          std::filesystem::create_directories(std::filesystem::path(new_file_path).parent_path());
          std::filesystem::copy_file(file_path, new_file_path);

          // Compute and store file hash
          std::string file_hash = hash_file(new_file_path);
          file_hashes[relative_path] = file_hash;
        }
      }
    }
  }

  // Process staged files
  bool has_staged_files = false;
  while (std::getline(staging_file, line))
  {
    if (line.empty())
    {
      continue;
    }

    has_staged_files = true;

    // Parse the line to extract file path and hash
    size_t last_space_pos = line.find_last_of(' ');
    if (last_space_pos != std::string::npos)
    {
      // Extract file path and hash
      std::string filePath = line.substr(0, last_space_pos);
      std::string fileHash = line.substr(last_space_pos + 1);

      fileHash.erase(0, fileHash.find_first_not_of(" \t"));
      fileHash.erase(fileHash.find_last_not_of(" \t") + 1);

      // Handle deletions
      if (fileHash.empty())
      {
        // File is marked for deletion
        std::string originalFilePath = get_actual_path(filePath);
        file_hashes[originalFilePath] = "";
        std::string deleted_file_path = commit_dir + "/" + originalFilePath;

        // Remove the file from the commit snapshot if it exists
        if (std::filesystem::exists(deleted_file_path))
        {
          std::filesystem::remove(deleted_file_path);
        }
      }
      else
      {
        // File is added or modified
        store_snapshot(filePath, commit_hash);
        file_hashes[filePath] = fileHash;
      }
    }
  }
  staging_file.close();

  // Check if there were any staged files
  if (!has_staged_files)
  {
    ErrorHandler::printError(ErrorCode::STAGING_FAILED, "No files to commit!", ErrorSeverity::ERROR, "commit_changes");
    return;
  }

  // Create commit log
  create_commit_log(author, message, file_hashes, commit_hash);

  // Update branch reference
  std::ofstream branch_file(".bittrack/refs/heads/" + get_current_branch(), std::ios::trunc);
  if (branch_file.is_open())
  {
    branch_file << commit_hash << std::endl;
    branch_file.close();
  }

  // Clear the staging area
  std::ofstream clear_staging_file(".bittrack/index", std::ios::trunc);
  clear_staging_file.close();
}

std::string generate_commit_hash(const std::string &author, const std::string &message, const std::string &timestamp)
{
  // Combine author, message, and timestamp to create a unique commit hash
  std::string combined = author + message + timestamp;
  return sha256_hash(combined);
}

std::string get_current_commit()
{
  // Get the current branch name
  std::string current_branch = get_current_branch();
  if (current_branch.empty())
  {
    return "";
  }

  // Read the current commit hash from the branch reference file
  std::ifstream branch_file(".bittrack/refs/heads/" + current_branch);
  std::string commit_hash;
  if (branch_file.is_open())
  {
    std::getline(branch_file, commit_hash);
    branch_file.close();
  }
  return commit_hash;
}

std::string get_staged_file_content(const std::string &file_path)
{
  // Read the content of the staged file
  std::ifstream file(file_path);
  std::string content;

  if (file.is_open())
  {
    // Read the file content line by line
    std::string line;
    while (std::getline(file, line))
    {
      content += line + "\n";
    }
    file.close();
  }
  return content;
}

std::string get_current_timestamp()
{
  // Get the current system time with milliseconds precision
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  // Format the timestamp as a string
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

  return ss.str();
}

std::string get_current_user()
{
  // Retrieve the current user's name from environment variables or system calls
  const char *user = std::getenv("USER");
  if (user != nullptr)
  {
    return std::string(user);
  }

  // Try Windows environment variable
  user = std::getenv("USERNAME");
  if (user != nullptr)
  {
    return std::string(user);
  }

  // Fallback to getlogin_r for POSIX systems
  char buffer[256];
  if (getlogin_r(buffer, sizeof(buffer)) == 0)
  {
    return std::string(buffer);
  }

  return "unknown";
}

std::vector<std::string> get_commit_files(const std::string &commit_hash)
{
  // Retrieve the list of files associated with a specific commit
  std::vector<std::string> files;
  std::string commit_path = ".bittrack/objects/" + commit_hash;

  // Check if the commit path exists
  if (!std::filesystem::exists(commit_path))
  {
    return files;
  }

  // Iterate through the files in the commit directory
  for (const auto &entry : std::filesystem::directory_iterator(commit_path))
  {
    if (entry.is_regular_file())
    {
      files.push_back(entry.path().filename().string());
    }
  }
  return files;
}
