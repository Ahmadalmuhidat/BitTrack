#include "stage.hpp"

void AddToStage(std::string FilePath)
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
        std::cout << "File is already staged and unchanged." << std::endl;
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

  std::cout << "Added/updated in staging: " << FilePath << std::endl;
}

void RemoveFromStage(const std::string &filePath)
{
  std::ifstream stagingFile(".bittrack/index");
  std::ofstream tempFile(".bittrack/index_temp");

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

  std::cout << "Unstaged: " << filePath << std::endl;
}

void DisplayStagedFiles()
{
  std::ifstream stagingFile(".bittrack/index");
  std::string line;

  std::cout << "Staged files:" << std::endl;
  while (std::getline(stagingFile, line))
  {
    std::istringstream iss(line);
    std::string fileName;
    std::string fileHash;

    if (!(iss >> fileName >> fileHash))
    {
      continue;
    }
    std::cout << "\033[32m" << fileName << "\033[0m" << std::endl;
  }
  stagingFile.close();
}

void DisplayUnstagedFiles()
{
  std::unordered_set<std::string> UnstagedFiles;
  std::unordered_map<std::string, std::string> UnstagedFileHashes;

  std::ifstream IndexFile(".bittrack/index");
  std::string line;

  while (std::getline(IndexFile, line))
  {
    std::istringstream iss(line);
    std::string fileName;
    std::string fileHash;

    if (!(iss >> fileName >> fileHash))
    {
      continue;
    }
    std::string FilesMap = NormalizePath(fileName);

    UnstagedFiles.insert(FilesMap);
    UnstagedFileHashes[FilesMap] = fileHash;
  }
  IndexFile.close();

  std::cout << "Unstaged files:" << std::endl;

  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string filePath = entry.path().string();
      std::string normalizedFilePath = NormalizePath(filePath);
      std::vector<std::string> ignorePatterns = ReadBitignore(".bitignore");

      if (filePath.find(".bittrack") != std::string::npos)
      {
        continue;
      }
      if (isIgnored(filePath, ignorePatterns))
      {
        continue;
      }
      if (!CompareWithCurrentVersion(filePath))
      {
        if (UnstagedFiles.find(normalizedFilePath) == UnstagedFiles.end())
        {
          std::cout << "\033[31m" << normalizedFilePath << "\033[0m" << std::endl;
        }
        else
        {
          std::string currentHash = HashFile(filePath);
          if (currentHash != UnstagedFileHashes[normalizedFilePath])
          {
            std::cout << "\033[31m" << normalizedFilePath << "\033[0m" << std::endl;
          }
        }
      }
    }
  }
}

std::string NormalizePath(const std::string &path)
{
  if (path.substr(0, 2) == "./")
  {
    return path.substr(2);
  }
  return path;
}

bool CompareWithCurrentVersion(const std::string &CurrentFile)
{
  for (const auto &entry: std::filesystem::recursive_directory_iterator(".bittrack/objects/" + GetCurrentCommit()))
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

std::string GetCurrentCommit()
{
  std::ifstream stagingFile(".bittrack/refs/heads/master");
  std::string line;

  while (std::getline(stagingFile, line))
  {
    std::istringstream iss(line);
  }
  return line;
}