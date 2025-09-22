#include "../include/commit.hpp"

void insert_commit_record_to_history(const std::string &last_commit_hash, const std::string &new_branch_name)
{
  const std::string history_path = ".bittrack/commits/history";

  // read existing content
  std::ifstream in_file(history_path);
  std::stringstream buffer;
  buffer << in_file.rdbuf();
  in_file.close();

  // open file for writing (overwrite)
  std::ofstream out_file(history_path);
  out_file << last_commit_hash << " " << new_branch_name << "\n" << buffer.str();
  out_file.close();
}

void store_snapshot(const std::string &file_path, const std::string &commit_hash)
{
  // base path for the snapshot
  std::string newDirPath = ".bittrack/objects/" + commit_hash;

  // determine the relative path of the file
  std::filesystem::path relativePath = std::filesystem::relative(file_path, ".");
  std::filesystem::path snapshot_path = std::filesystem::path(newDirPath) / relativePath;

  // create all necessary directories for the relative path
  std::filesystem::create_directories(snapshot_path.parent_path());

  // read the content of the file into a buffer
  std::ifstream inputFile(file_path, std::ios::binary);
  if (!inputFile)
  {
    std::cerr << "Error: Unable to open file: " << file_path << std::endl;
  }
  std::stringstream buffer;
  buffer << inputFile.rdbuf();
  inputFile.close();

  // write the buffer content to the snapshot file
  std::ofstream snapshotFile(snapshot_path, std::ios::binary);
  if (!snapshotFile)
  {
    std::cerr << "Error: Unable to create snapshot file: " << snapshot_path << std::endl;
  }
  snapshotFile << buffer.str();
  snapshotFile.close();
}

void create_commit_log(const std::string &author, const std::string &message, const std::unordered_map<std::string, std::string> &file_hashes, const std::string &commit_hash)
{
  std::string log_file = ".bittrack/commits/" + commit_hash;

  // get & store the current time
  std::time_t currentTime = std::time(nullptr);
  char formatedTimestamp[80];
  std::strftime(formatedTimestamp, sizeof(formatedTimestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));

  // write the commit information in the log file
  std::ofstream commitFile(log_file);
  commitFile << "Author: " << author << std::endl;
  commitFile << "Branch: " << get_current_branch() << std::endl;
  commitFile << "Timestamp: " << formatedTimestamp << std::endl;
  commitFile << "Message: " << message << std::endl;
  commitFile << "Files: " << std::endl;

  // write the commit files in the log information
  for (const auto &[filePath, fileHash] : file_hashes)
  {
    commitFile << filePath << " " << fileHash << std::endl;
  }
  commitFile.close();

  // store the commit hash in the history
  insert_commit_record_to_history(commit_hash, get_current_branch());

  // write the commit hash in branch file as a lastest commit
  std::ofstream head_file(".bittrack/refs/heads/" + get_current_branch(), std::ios::trunc);
  head_file << commit_hash << std::endl;
  head_file.close();
}

std::string get_last_commit(const std::string &branch)
{
  std::ifstream history_file(".bittrack/commits/history");
  std::string line;

  while (std::getline(history_file, line))
  {
    std::istringstream iss(line);
    std::string commit_file_hash;
    std::string commit_branch;

    if (!(iss >> commit_file_hash >> commit_branch))
    {
      continue;
    }

    if (commit_branch == branch)
    {
      return commit_file_hash;
    }
  }
  return "";
}

void commit_changes(const std::string &author, const std::string &message)
{
  std::ifstream staging_file(".bittrack/index");
  if (!staging_file)
  {
    std::cerr << "no files staged for commit!" << std::endl;
    return;
  }

  std::unordered_map<std::string, std::string> file_hashes;
  std::string timestamp = std::to_string(std::time(nullptr));
  std::string commit_hash = generate_commit_hash(author, message, timestamp);
  std::string line;
  
  // Create commit directory
  std::string commit_dir = ".bittrack/objects/" + commit_hash;
  std::filesystem::create_directories(commit_dir);

  // First, collect all files that will be deleted (from staging area)
  std::set<std::string> deleted_files;
  std::ifstream staging_file_for_deletions(".bittrack/index");
  if (staging_file_for_deletions.is_open())
  {
    std::string line;
    while (std::getline(staging_file_for_deletions, line))
    {
      if (line.empty()) continue;
      
      // Parse using the same logic as get_staged_files
      size_t last_space_pos = line.find_last_of(' ');
      if (last_space_pos != std::string::npos)
      {
        std::string filePath = line.substr(0, last_space_pos);
        std::string fileHash = line.substr(last_space_pos + 1);
        
        // Trim whitespace from hash
        fileHash.erase(0, fileHash.find_first_not_of(" \t"));
        fileHash.erase(fileHash.find_last_not_of(" \t") + 1);
        
        if (fileHash.empty())
        {
          // Remove the (deleted) suffix to get the original filename
          std::string originalFilePath = filePath;
          if (filePath.length() >= 10 && filePath.substr(filePath.length() - 10) == " (deleted)")
          {
            originalFilePath = filePath.substr(0, filePath.length() - 10);
          }
          deleted_files.insert(originalFilePath);
        }
      }
    }
    staging_file_for_deletions.close();
  }

  // Then, include all files from the previous commit (unchanged files)
  std::string previous_commit = get_current_commit();
  if (!previous_commit.empty())
  {
    std::string previous_commit_path = ".bittrack/objects/" + previous_commit;
    if (std::filesystem::exists(previous_commit_path))
    {
      // Copy all files from previous commit to new commit, except deleted ones
      for (const auto& entry : std::filesystem::recursive_directory_iterator(previous_commit_path))
      {
        if (entry.is_regular_file())
        {
          std::string file_path = entry.path().string();
          std::string relative_path = std::filesystem::relative(file_path, previous_commit_path).string();
          
          // Skip files that are being deleted
          if (deleted_files.find(relative_path) != deleted_files.end())
          {
            continue;
          }
          
          // Copy file to new commit directory
          std::string new_file_path = commit_dir + "/" + relative_path;
          std::filesystem::create_directories(std::filesystem::path(new_file_path).parent_path());
          std::filesystem::copy_file(file_path, new_file_path);
          
          // Calculate hash for the file
          std::string file_hash = hash_file(new_file_path);
          file_hashes[relative_path] = file_hash;
        }
      }
    }
  }

  // Then, process staged files (new/modified/deleted files)
  bool has_staged_files = false;
  while (std::getline(staging_file, line))
  {
    if (line.empty()) continue;
    
    has_staged_files = true; // We have at least one staged file
    
    // Parse filepath and hash from staging file using the same logic as get_staged_files
    size_t last_space_pos = line.find_last_of(' ');
    if (last_space_pos != std::string::npos)
    {
      std::string filePath = line.substr(0, last_space_pos);
      std::string fileHash = line.substr(last_space_pos + 1);
      
      // Trim whitespace from hash
      fileHash.erase(0, fileHash.find_first_not_of(" \t"));
      fileHash.erase(fileHash.find_last_not_of(" \t") + 1);
      
      // Check if this is a deleted file (empty hash means deleted)
      if (fileHash.empty())
      {
        // For deleted files, remove the (deleted) suffix to get the original filename
        std::string originalFilePath = filePath;
        if (filePath.length() >= 10 && filePath.substr(filePath.length() - 10) == " (deleted)")
        {
          originalFilePath = filePath.substr(0, filePath.length() - 10);
        }
        
        // Remove from file_hashes and don't store snapshot
        file_hashes.erase(originalFilePath);
        
        // Also remove the file from the commit directory if it exists
        std::string deleted_file_path = commit_dir + "/" + originalFilePath;
        if (std::filesystem::exists(deleted_file_path))
        {
          std::filesystem::remove(deleted_file_path);
        }
      }
      else
      {
        // For regular files, store a copy of the modified file
        store_snapshot(filePath, commit_hash);
        file_hashes[filePath] = fileHash;
      }
    }
  }
  staging_file.close();

  // Check if we have any staged files to commit
  if (!has_staged_files)
  {
    std::cerr << "No files to commit!" << std::endl;
    return;
  }

  // store the commit information in history
  create_commit_log(author, message, file_hashes, commit_hash);

  // update branch ref to point to the new commit
  std::ofstream branch_file(".bittrack/refs/heads/" + get_current_branch(), std::ios::trunc);
  if (branch_file.is_open())
  {
    branch_file << commit_hash << std::endl;
    branch_file.close();
  }

  // clear the index file
  std::ofstream clear_staging_file(".bittrack/index", std::ios::trunc);
  clear_staging_file.close();
}


std::string generate_commit_hash(const std::string &author, const std::string &message, const std::string &timestamp)
{
  std::string combined = author + message + timestamp;
  return sha256_hash(combined);
}

std::string get_current_commit()
{
  // read the current commit from the current branch ref file
  std::string current_branch = get_current_branch();
  if (current_branch.empty())
  {
    return "";
  }
  
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
  std::ifstream file(file_path);
  std::string content;
  if (file.is_open())
  {
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
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
  
  return ss.str();
}

std::string get_current_user()
{
  // Try to get user from environment variables
  const char* user = std::getenv("USER");
  if (user != nullptr)
  {
    return std::string(user);
  }
  
  user = std::getenv("USERNAME");
  if (user != nullptr)
  {
    return std::string(user);
  }
  
  // Fallback to system call
  char buffer[256];
  if (getlogin_r(buffer, sizeof(buffer)) == 0)
  {
    return std::string(buffer);
  }
  
  return "unknown";
}

std::vector<std::string> get_commit_files(const std::string& commit_hash)
{
  std::vector<std::string> files;
  std::string commit_path = ".bittrack/objects/" + commit_hash;

  if (!std::filesystem::exists(commit_path))
  {
    return files;
  }

  for (const auto& entry : std::filesystem::directory_iterator(commit_path))
  {
    if (entry.is_regular_file())
    {
      files.push_back(entry.path().filename().string());
    }
  }
  return files;
}

