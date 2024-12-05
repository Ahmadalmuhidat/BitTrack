#include "../include/commit.hpp"

void storeSnapshot(const std::string &filePath, const std::string &CommitHash)
{
  std::string newDir = ".bittrack/objects/" + getCurrentBranch() + "/" + CommitHash;

  std::ifstream inputFile(filePath, std::ios::binary);
  std::filesystem::create_directory(newDir);

  std::stringstream buffer;
  buffer << inputFile.rdbuf();
  std::string fileContent = buffer.str();

  std::string snapshotPath = newDir + "/" + filePath;

  std::cout << snapshotPath << std::endl;

  std::ofstream snapshotFile(snapshotPath, std::ios::binary);
  snapshotFile << fileContent;
  snapshotFile.close();
}

void createCommitLog(
  const std::string &author,
  const std::string &message,
  const std::unordered_map<std::string, std::string> &fileHashes,
  const std::string &commitHash
)
{
  std::string logFile = ".bittrack/commits/" + commitHash;
  std::string currentBranch = getCurrentBranch();
  std::time_t currentTime = std::time(nullptr);
  char formatedTimestamp[80];
  std::strftime(
    formatedTimestamp,
    sizeof(formatedTimestamp),
    "%Y-%m-%d %H:%M:%S",
    std::localtime(&currentTime)
  );

  std::ofstream commitFile(logFile);
  commitFile << "Author: " << author << std::endl;
  commitFile << "Branch: " << currentBranch << std::endl;
  commitFile << "Timestamp: " << formatedTimestamp << std::endl;
  commitFile << "Message: " << message << std::endl;
  commitFile << "Files: " << std::endl;

  for (const auto &[filePath, fileHash] : fileHashes)
  {
    commitFile << filePath << " " << fileHash << std::endl;
  }
  commitFile.close();

  std::ofstream historyFile(".bittrack/commits/history", std::ios::app);
  historyFile << commitHash << std::endl;
  historyFile.close();

  std::ofstream headFile(".bittrack/refs/heads/" + getCurrentBranch(), std::ios::trunc);
  headFile << commitHash << std::endl;
  headFile.close();
}

void commitChanges(const std::string &author, const std::string &message)
{
  std::ifstream stagingFile(".bittrack/index");
  if (!stagingFile)
  {
    std::cerr << "No files staged for commit!" << std::endl;
    return;
  }

  std::unordered_map<std::string, std::string> FileHashes;
  std::string line;

  std::string CommitHash = GenerateCommitHash(author, message, FileHashes);

  while (std::getline(stagingFile, line))
  {
    std::string filePath = line.substr(0, line.find(" "));
    std::string fileHash = HashFile(filePath);

    storeSnapshot(filePath, CommitHash);
    FileHashes[filePath] = fileHash;
  }
  stagingFile.close();

  createCommitLog(author, message, FileHashes, CommitHash);

  std::ofstream clearStagingFile(".bittrack/index", std::ios::trunc);
  clearStagingFile.close();
}

void commitHistory()
{
  std::ifstream filePath(".bittrack/commits/history");
  std::string FileName;

  while (std::getline(filePath, FileName))
  {
    std::ifstream CommitLog (".bittrack/commits/" + FileName);
    std::string line;
    while (std::getline(CommitLog, line))
    {
      std::cout << line << std::endl;
    }
    std::cout << "\n" << std::endl;
  }

  return;
}