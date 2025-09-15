#include "../include/commit.hpp"
#include "../include/hash.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>

void insert_commit_to_history(const std::string& last_commit_hash, const std::string& new_branch_name)
{
  const std::string history_path = ".bittrack/commits/history";

  // Read existing content
  std::ifstream in_file(history_path);
  std::stringstream buffer;
  buffer << in_file.rdbuf();
  in_file.close();

  // Open file for writing (overwrite)
  std::ofstream out_file(history_path);
  out_file << last_commit_hash << " " << new_branch_name << "\n" << buffer.str();
  out_file.close();
}

void store_snapshot(const std::string &file_path, const std::string &commit_hash)
{
  // Base path for the snapshot
  std::string newDirPath = ".bittrack/objects/" + get_current_branch() + "/" + commit_hash;

  // Determine the relative path of the file
  std::filesystem::path relativePath = std::filesystem::relative(file_path, ".");
  std::filesystem::path snapshot_path = std::filesystem::path(newDirPath) / relativePath;

  // Create all necessary directories for the relative path
  std::filesystem::create_directories(snapshot_path.parent_path());

  // Read the content of the file into a buffer
  std::ifstream inputFile(file_path, std::ios::binary);
  if (!inputFile)
  {
    std::cerr << "Error: Unable to open file: " << file_path << std::endl;
  }
  std::stringstream buffer;
  buffer << inputFile.rdbuf();
  inputFile.close();

  // Write the buffer content to the snapshot file
  std::ofstream snapshotFile(snapshot_path, std::ios::binary);
  if (!snapshotFile) {
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
  std::strftime(
    formatedTimestamp,
    sizeof(formatedTimestamp),
    "%Y-%m-%d %H:%M:%S",
    std::localtime(&currentTime)
  );

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
  insert_commit_to_history(commit_hash, get_current_branch());

  // write the commit hash in branch file as a lastest commit
  std::ofstream head_file(".bittrack/refs/heads/" + get_current_branch(), std::ios::trunc);
  head_file << commit_hash << std::endl;
  head_file.close();
}

std::string get_last_commit(const std::string& branch)
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

void commit_history()
{
  // read the history file contains commits logs in order
  std::ifstream file_path(".bittrack/commits/history");
  std::string line;

  while (std::getline(file_path, line))
  {
    std::istringstream iss(line);
    std::string commit_file_hash;
    std::string commit_branch;

    if (!(iss >> commit_file_hash >> commit_branch))
    {
      continue;
    }

    std::ifstream commit_log (".bittrack/commits/" + commit_file_hash);
    std::string line;

    // print the commit file content
    while (std::getline(commit_log, line))
    {
      std::cout << line << std::endl;
    }
    std::cout << "\n" << std::endl;
    commit_log.close();
  }
  file_path.close();
}

std::string generate_commit_hash(const std::string& author, const std::string& message, const std::string& timestamp)
{
  // Create a simple hash based on author, message, and timestamp
  std::string combined = author + message + timestamp;
  return sha256_hash(combined);
}

std::string get_current_commit()
{
  // Read the current commit from .bittrack/HEAD
  std::ifstream head_file(".bittrack/HEAD");
  std::string commit_hash;
  if (head_file.is_open()) {
    std::getline(head_file, commit_hash);
    head_file.close();
  }
  return commit_hash;
}

std::string get_staged_file_content(const std::string& file_path)
{
  std::ifstream file(file_path);
  std::string content;
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      content += line + "\n";
    }
    file.close();
  }
  return content;
}
