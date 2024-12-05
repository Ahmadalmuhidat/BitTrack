#include "../include/stage.hpp"

void stage(std::string FilePath)
{
  if (!std::filesystem::exists(FilePath))
  {
    std::cerr << "Error: File does not exist!" << std::endl;
    return;
  }

  std::string fileHash = HashFile(FilePath);
  std::ifstream stagingFile(".bittrack/index");
  std::string line;
  std::unordered_map<std::string, std::string> stagedFiles;

  while (std::getline(stagingFile, line))
  {
    std::istringstream iss(line);
    std::string stagedFilePath;
    std::string stagedFileHash;
    if (!(iss >> stagedFilePath >> stagedFileHash))
    {
      continue;
    }

    stagedFiles[stagedFilePath] = stagedFileHash;

    if (stagedFilePath == FilePath)
    {
      if (stagedFileHash == fileHash)
      {
        std::cerr << "File is already staged and unchanged." << std::endl;
        return;
      }
    }
  }
  stagingFile.close();

  stagedFiles[FilePath] = fileHash;
  std::ofstream updatedStagingFile(".bittrack/index", std::ios::trunc);

  for (const auto &[path, hash] : stagedFiles)
  {
    updatedStagingFile << path << " " << hash << std::endl;
  }
  updatedStagingFile.close();
}

void unstage(const std::string &filePath)
{
  std::ifstream stagingFile(".bittrack/index");
  std::ofstream tempFile(".bittrack/index_temp");
  std::vector<std::string> stagedFiles = getStagedFiles();

  if (!std::filesystem::exists(filePath))
  {
    std::cerr << "Error: File does not exist!" << std::endl;
    return;
  }

  if (std::find(stagedFiles.begin(), stagedFiles.end(), filePath) == stagedFiles.end())
  {
    std::cerr << "the file is not staged already" << std::endl;
  }

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

std::vector<std::string> getStagedFiles()
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

std::vector<std::string> getUnstagedFiles()
{
  std::unordered_set<std::string> UnstagedFiles;
  std::unordered_map<std::string, std::string> UnstagedFileHashes;
  std::string CurrentBranch = ".bittrack/objects/" + getCurrentBranch() + "/" + getCurrentCommit();

  std::ifstream IndexFile(".bittrack/index");
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
    std::string FilesMap = normalizePath(fileName);

    UnstagedFiles.insert(FilesMap);
    UnstagedFileHashes[FilesMap] = fileHash;
  }
  IndexFile.close();

  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string filePath = entry.path().string();
      std::string normalizedFilePath = normalizePath(filePath);
      std::vector<std::string> ignorePatterns = ReadBitignore(".bitignore");

      if (filePath.find(".bittrack") != std::string::npos)
      {
        continue;
      }
      if (isIgnored(filePath, ignorePatterns))
      {
        continue;
      }
      if (!compareWithCurrentVersion(filePath, CurrentBranch))
      {
        if (UnstagedFiles.find(normalizedFilePath) == UnstagedFiles.end())
        {
          files.push_back(normalizedFilePath);
        }
        else
        {
          std::string currentHash = HashFile(filePath);
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

std::string normalizePath(const std::string &path)
{
  if (path.substr(0, 2) == "./")
  {
    return path.substr(2);
  }
  return path;
}

bool compareWithCurrentVersion(const std::string &CurrentFile, const std::string &CurrentBranch)
{
  for (const auto &entry: std::filesystem::recursive_directory_iterator(CurrentBranch))
  {
    if (entry.is_regular_file())
    {
      std::string CurrentVersionFile = entry.path().string();
      std::string CurrentFileHash = HashFile(CurrentFile);
      std::string CurrentVersionFileHash = HashFile(CurrentVersionFile);

      if (CurrentFileHash == CurrentVersionFileHash)
      {
        return true;
      }
    }
  }
  return false;
}

std::string getCurrentCommit()
{
  std::ifstream stagingFile(".bittrack/refs/heads/" + getCurrentBranch());
  std::string line;

  if (std::getline(stagingFile, line))
  {
    stagingFile.close();
    return line;
  }
  stagingFile.close();
  return "";
}