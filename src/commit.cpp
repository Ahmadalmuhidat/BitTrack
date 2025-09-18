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
  std::string newDirPath = ".bittrack/objects/" + get_current_branch() + "/" + commit_hash;

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

  while (std::getline(staging_file, line))
  {
    // get each file and hash it
    std::string filePath = line.substr(0, line.find(" "));
    std::string fileHash = hash_file(filePath);

    // store a copy of the modified file
    store_snapshot(filePath, commit_hash);
    file_hashes[filePath] = fileHash;
  }
  staging_file.close();

  // store the commit information in history
  create_commit_log(author, message, file_hashes, commit_hash);

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
  // read the current commit from .bittrack/HEAD
  std::ifstream head_file(".bittrack/HEAD");
  std::string commit_hash;
  if (head_file.is_open())
  {
    std::getline(head_file, commit_hash);
    head_file.close();
  }
  return commit_hash;
}

void show_commit_details(const std::string& commit_hash)
{
  std::string commit_path = ".bittrack/objects/" + get_current_branch() + "/" + commit_hash;

  if (!std::filesystem::exists(commit_path))
  {
    std::cout << "Commit not found: " << commit_hash << std::endl;
    return;
  }

  std::ifstream commit_file(commit_path);
  if (!commit_file.is_open())
  {
    std::cout << "Error reading commit file." << std::endl;
    return;
  }

  std::string line;
  while (std::getline(commit_file, line))
  {
    std::cout << line << std::endl;
  }
  commit_file.close();
}

std::vector<std::string> get_commit_files(const std::string& commit_hash)
{
  std::vector<std::string> files;
  std::string commit_path = ".bittrack/objects/" + get_current_branch() + "/" + commit_hash;

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

std::string get_commit_message(const std::string& commit_hash)
{
  std::string commit_path = ".bittrack/objects/" + get_current_branch() + "/" + commit_hash;
  
  if (!std::filesystem::exists(commit_path))
  {
    return "";
  }
  
  std::ifstream commit_file(commit_path);
  if (!commit_file.is_open())
  {
    return "";
  }
  
  std::string line;
  while (std::getline(commit_file, line))
  {
    if (line.find("Message: ") == 0)
    {
      return line.substr(9); // Remove "Message: " prefix
    }
  }
  
  return "";
}

std::string get_commit_author(const std::string& commit_hash)
{
  std::string commit_path = ".bittrack/objects/" + get_current_branch() + "/" + commit_hash;
  
  if (!std::filesystem::exists(commit_path))
  {
    return "";
  }
  
  std::ifstream commit_file(commit_path);
  if (!commit_file.is_open())
  {
    return "";
  }
  
  std::string line;
  while (std::getline(commit_file, line))
  {
    if (line.find("Author: ") == 0)
    {
      return line.substr(8); // Remove "Author: " prefix
    }
  }
  
  return "";
}

std::string get_commit_timestamp(const std::string& commit_hash)
{
  std::string commit_path = ".bittrack/objects/" + get_current_branch() + "/" + commit_hash;
  
  if (!std::filesystem::exists(commit_path))
  {
    return "";
  }
  
  std::ifstream commit_file(commit_path);
  if (!commit_file.is_open())
  {
    return "";
  }
  
  std::string line;
  while (std::getline(commit_file, line))
  {
    if (line.find("Timestamp: ") == 0)
    {
      return line.substr(11); // Remove "Timestamp: " prefix
    }
  }
  
  return "";
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

std::string get_commit_info(const std::string& commit_hash)
{
  // Find commit in any branch
  std::string objects_dir = ".bittrack/objects";
  if (!std::filesystem::exists(objects_dir))
  {
    return "";
  }
  
  for (const auto& branch_entry : std::filesystem::directory_iterator(objects_dir))
  {
    if (branch_entry.is_directory())
    {
      std::string commit_dir = branch_entry.path().string() + "/" + commit_hash;
      if (std::filesystem::exists(commit_dir))
      {
        std::string commit_file = commit_dir + "/commit.txt";
        if (std::filesystem::exists(commit_file))
        {
          std::ifstream file(commit_file);
          std::string line;
          std::string message, author, timestamp;
          
          while (std::getline(file, line))
          {
            if (line.find("message ") == 0)
            {
              message = line.substr(8);
            }
            else if (line.find("author ") == 0)
            {
              author = line.substr(7);
            }
            else if (line.find("timestamp ") == 0)
            {
              timestamp = line.substr(10);
            }
          }
          file.close();
          
          return commit_hash.substr(0, 8) + " " + author + " " + timestamp + " " + message;
        }
      }
    }
  }
  
  return "";
}
