#include "commit.hpp"

void StoreSnapshot(const std::string &filePath, const std::string &CommitHash)
{
  std::ifstream inputFile(filePath, std::ios::binary);

  std::filesystem::create_directory(".bittrack/objects/" + CommitHash);

  std::stringstream buffer;
  buffer << inputFile.rdbuf();
  std::string fileContent = buffer.str();

  std::string objectPath = ".bittrack/objects/" + CommitHash + "/" + filePath;
  std::ofstream objectFile(objectPath, std::ios::binary);
  objectFile << fileContent;
  objectFile.close();

  std::cout << "Stored snapshot: " << filePath << " -> " << objectPath << std::endl;
}

void CreateCommitLog(
  const std::string &author,
  const std::string &message,
  const std::unordered_map<std::string, std::string> &fileHashes,
  const std::string &commitHash
)
{
  std::time_t currentTime = std::time(nullptr);
  char formatedTimestamp[80];
  std::strftime(
    formatedTimestamp,
    sizeof(formatedTimestamp),
    "%Y-%m-%d %H:%M:%S",
    std::localtime(&currentTime)
  );

  std::ofstream commitFile(".bittrack/commits/" + commitHash);
  commitFile << "Author: " << author << std::endl;
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

  std::ofstream headFile(".bittrack/refs/heads/master", std::ios::trunc);
  headFile << commitHash;
  headFile.close();
}

void CommitChanges(const std::string &author, const std::string &message)
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

    StoreSnapshot(filePath, CommitHash);
    FileHashes[filePath] = fileHash;
  }
  stagingFile.close();

  CreateCommitLog(author, message, FileHashes, CommitHash);

  std::ofstream clearStagingFile(".bittrack/index", std::ios::trunc);
  clearStagingFile.close();

  std::cout << "Commit created with hash: " << CommitHash << std::endl;
}

void CommitHistory()
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