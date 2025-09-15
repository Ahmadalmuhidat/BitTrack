#include "../include/stage.hpp"
#include "../include/branch.hpp"
#include "../include/ignore.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

void stage(const std::string& file_path)
{
  // Call the non-const version by creating a copy
  std::string path_copy = file_path;
  stage(path_copy);
}

void stage(std::string file_path)
{
  try {
    std::unordered_map<std::string, std::string> staged_files;

    // read existing staged files
    if (std::filesystem::exists(".bittrack/index"))
    {
      std::ifstream staging_file(".bittrack/index");
      if (staging_file.is_open()) {
        std::string line;
        while (std::getline(staging_file, line))
        {
          std::istringstream iss(line);
          std::string staged_file_path, staged_file_hash;
          if (iss >> staged_file_path >> staged_file_hash)
          {
            staged_files[staged_file_path] = staged_file_hash;
          }
        }
        staging_file.close();
      }
    }

    // helper lambda to stage a file
    auto stage_single_file = [&](const std::string& path) {
      if (!std::filesystem::exists(path) || std::filesystem::is_directory(path))
      {
        return;
      }

      // Simple ignore check
      if (path.find(".bittrack") != std::string::npos) {
        return;
      }

      // Generate dummy hash
      std::string fileHash = "dummy_hash_" + path;

      if (staged_files.find(path) != staged_files.end() && staged_files[path] == fileHash)
      {
        std::cerr << "File already staged and unchanged: " << path << std::endl;
        return;
      }

      staged_files[path] = fileHash;
      std::cout << "staged: " << path << std::endl;
    };

    // if ".", stage all files recursively
    if (file_path == ".")
    {
      for (const auto& entry : std::filesystem::recursive_directory_iterator("."))
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

    // write back updated index
    std::ofstream updatedStagingFile(".bittrack/index", std::ios::trunc);
    if (updatedStagingFile.is_open()) {
      for (const auto& pair : staged_files)
      {
        updatedStagingFile << pair.first << " " << pair.second << std::endl;
      }
      updatedStagingFile.close();
    }
  } catch (const std::exception& e) {
    std::cerr << "Error in stage function: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown error in stage function" << std::endl;
  }
}

void unstage(const std::string &filePath)
{
  if (!std::filesystem::exists(filePath))
  {
    std::cerr << "Error: File does not exist!" << std::endl;
    return;
  }

  // Check if index file exists
  if (!std::filesystem::exists(".bittrack/index"))
  {
    std::cerr << "No staged files to unstage" << std::endl;
    return;
  }

  std::vector<std::string> stagedFiles = get_staged_files();
  if (std::find(stagedFiles.begin(), stagedFiles.end(), filePath) == stagedFiles.end())
  {
    std::cerr << "the file is not staged already" << std::endl;
    return;
  }

  std::ifstream stagingFile(".bittrack/index");
  std::ofstream tempFile(".bittrack/index_temp");

  // add unstage . (all)
  std::string line;
  while (std::getline(stagingFile, line))
  {
    if (line.find(filePath) == std::string::npos)
    {
      tempFile << line << std::endl;
    }
  }
  stagingFile.close();
  tempFile.close();

  std::filesystem::remove(".bittrack/index");
  std::filesystem::rename(".bittrack/index_temp", ".bittrack/index");
}

std::vector<std::string> get_staged_files()
{
  std::vector<std::string> files;
  
  // Check if index file exists
  if (!std::filesystem::exists(".bittrack/index"))
  {
    return files; // Return empty vector if no index file
  }
  
  std::ifstream index_file(".bittrack/index");
  std::string line;
  
  while (std::getline(index_file, line)) {
    std::istringstream iss(line);
    std::string fileName, fileHash;
    if (iss >> fileName >> fileHash) {
      files.push_back(fileName);
    }
  }
  
  return files;
}

std::vector<std::string> get_unstaged_files()
{
  std::unordered_set<std::string> UnstagedFiles;

  // get all files in the current directory
  for (const auto& entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string filePath = entry.path().string();
      if (filePath.find(".bittrack") == std::string::npos)
      {
        UnstagedFiles.insert(filePath);
      }
    }
  }

  // get staged files
  std::vector<std::string> stagedFiles = get_staged_files();

  // remove staged files from unstaged files
  for (const auto& stagedFile : stagedFiles)
  {
    UnstagedFiles.erase(stagedFile);
  }

  return std::vector<std::string>(UnstagedFiles.begin(), UnstagedFiles.end());
}
