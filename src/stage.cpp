#include "../include/stage.hpp"

void stage(std::string file_path)
{
  if (!std::filesystem::exists(file_path))
  {
    std::cerr << "Error: File does not exist!" << std::endl;
    return;
  }

  std::string fileHash = hash_file(file_path);
  std::ifstream staging_file(".bittrack/index");
  std::string line;
  std::unordered_map<std::string, std::string> staged_files;

  while (std::getline(staging_file, line))
  {
    std::istringstream iss(line);
    std::string staged_file_path;
    std::string staged_file_hash;

    if (!(iss >> staged_file_path >> staged_file_hash))
    {
      continue;
    }

    staged_files[staged_file_path] = staged_file_hash;

    if (staged_file_path == file_path)
    {
      if (staged_file_hash == fileHash)
      {
        std::cerr << "File is already staged and unchanged." << std::endl;
        return;
      }
    }
  }
  staging_file.close();

  staged_files[file_path] = fileHash;
  std::ofstream updatedStagingFile(".bittrack/index", std::ios::trunc);

  for (const auto &[path, hash] : staged_files)
  {
    updatedStagingFile << path << " " << hash << std::endl;
  }
  updatedStagingFile.close();
}

void unstage(const std::string &filePath)
{
  std::ifstream stagingFile(".bittrack/index");
  std::ofstream tempFile(".bittrack/index_temp");
  std::vector<std::string> stagedFiles = get_staged_files();

  if (!std::filesystem::exists(filePath))
  {
    std::cerr << "Error: File does not exist!" << std::endl;
    return;
  }

  if (std::find(stagedFiles.begin(), stagedFiles.end(), filePath) == stagedFiles.end())
  {
    std::cerr << "the file is not staged already" << std::endl;
  }

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
  std::ifstream stagingFile(".bittrack/index");
  std::string line;
  std::vector<std::string> files;

  while (std::getline(stagingFile, line))
  {
    std::istringstream iss(line);
    std::string fileName;
    std::string fileHash;

    if (!(iss >> fileName >> fileHash))
    {
      continue;
    }
    files.push_back(fileName);
  }
  stagingFile.close();
  return files;
}

std::vector<std::string> get_unstaged_files()
{
  std::unordered_set<std::string> UnstagedFiles;
  std::unordered_map<std::string, std::string> UnstagedFileHashes;
  std::ifstream IndexFile(".bittrack/index");
  std::string CurrentBranch = ".bittrack/objects/" + get_current_branch() + "/" + get_current_commit();
  std::string line;
  std::vector<std::string> files;

  while (std::getline(IndexFile, line))
  {
    std::istringstream iss(line);
    std::string fileName;
    std::string fileHash;

    if (!(iss >> fileName >> fileHash))
    {
      continue;
    }
    std::string FilesMap = normalize_file_path(fileName);

    UnstagedFiles.insert(FilesMap);
    UnstagedFileHashes[FilesMap] = fileHash;
  }
  IndexFile.close();

  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string filePath = entry.path().string();
      std::string normalizedFilePath = normalize_file_path(filePath);
      std::vector<std::string> ignorePatterns = read_bitignore(".bitignore");

      if (filePath.find(".bittrack") != std::string::npos)
      {
        continue;
      }
      if (is_file_ignored(filePath, ignorePatterns))
      {
        continue;
      }
      if (!compare_with_current_version(filePath, CurrentBranch))
      {
        if (UnstagedFiles.find(normalizedFilePath) == UnstagedFiles.end())
        {
          files.push_back(normalizedFilePath);
        }
        else
        {
          std::string currentHash = hash_file(filePath);
          if (currentHash != UnstagedFileHashes[normalizedFilePath])
          {
            files.push_back(normalizedFilePath);
          }
        }
      }
    }
  }
  return files;
}

std::string normalize_file_path(const std::string &path)
{
  if (path.substr(0, 2) == "./")
  {
    return path.substr(2);
  }
  return path;
}

bool compare_with_current_version(const std::string &CurrentFile, const std::string &CurrentBranch)
{
  for (const auto &entry: std::filesystem::recursive_directory_iterator(CurrentBranch))
  {
    if (entry.is_regular_file())
    {
      std::string CurrentVersionFile = entry.path().string();
      std::string CurrentFileHash = hash_file(CurrentFile);
      std::string CurrentVersionFileHash = hash_file(CurrentVersionFile);

      if (CurrentFileHash == CurrentVersionFileHash)
      {
        return true;
      }
    }
  }
  return false;
}

std::string get_current_commit()
{
  std::ifstream stagingFile(".bittrack/refs/heads/" + get_current_branch());
  std::string line;

  if (std::getline(stagingFile, line))
  {
    stagingFile.close();
    return line;
  }
  stagingFile.close();
  return "";
}