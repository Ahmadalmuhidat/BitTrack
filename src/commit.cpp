#include "../include/commit.hpp"

void storeSnapshot(const std::string &filePath, const std::string &CommitHash)
{
  // Base path for the snapshot
  std::string newDirPath = ".bittrack/objects/" + getCurrentBranch() + "/" + CommitHash;

  // Determine the relative path of the file
  std::filesystem::path relativePath = std::filesystem::relative(filePath, ".");
  std::filesystem::path snapshotPath = std::filesystem::path(newDirPath) / relativePath;

  // Create all necessary directories for the relative path
  std::filesystem::create_directories(snapshotPath.parent_path());

  // Read the content of the file into a buffer
  std::ifstream inputFile(filePath, std::ios::binary);
  if (!inputFile) {
    std::cerr << "Error: Unable to open file: " << filePath << std::endl;
  }
  std::stringstream buffer;
  buffer << inputFile.rdbuf();
  inputFile.close();

  // Write the buffer content to the snapshot file
  std::ofstream snapshotFile(snapshotPath, std::ios::binary);
  if (!snapshotFile) {
    std::cerr << "Error: Unable to create snapshot file: " << snapshotPath << std::endl;
  }
  snapshotFile << buffer.str();
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
  std::ofstream commitFile(logFile);
  commitFile << "Author: " << author << std::endl;
  commitFile << "Branch: " << getCurrentBranch() << std::endl;
  commitFile << "Timestamp: " << formatedTimestamp << std::endl;
  commitFile << "Message: " << message << std::endl;
  commitFile << "Files: " << std::endl;

  // write the commit files in the log information
  for (const auto &[filePath, fileHash] : fileHashes)
  {
    commitFile << filePath << " " << fileHash << std::endl;
  }
  commitFile.close();

  // store the commit hash in the history
  std::ofstream historyFile(".bittrack/commits/history", std::ios::app);
  historyFile << commitHash << std::endl;
  historyFile.close();

  // write the commit hash in branch file as a lastest commit
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
  std::string CommitHash = GenerateCommitHash(author, message, FileHashes); // generate hash for the current commit
  std::string line;

  while (std::getline(stagingFile, line))
  {
    // get each file and hash it
    std::string filePath = line.substr(0, line.find(" "));
    std::string fileHash = HashFile(filePath);

    // store a copy of the modified file
    storeSnapshot(filePath, CommitHash);
    FileHashes[filePath] = fileHash;
  }
  stagingFile.close();

  // store the commit information in history
  createCommitLog(author, message, FileHashes, CommitHash);

  // clear the index file
  std::ofstream clearStagingFile(".bittrack/index", std::ios::trunc);
  clearStagingFile.close();
}

void commitHistory()
{
  // read the history file contains commits logs in order
  std::ifstream filePath(".bittrack/commits/history");
  std::string FileName;

  while (std::getline(filePath, FileName))
  {
    std::ifstream CommitLog (".bittrack/commits/" + FileName);
    std::string line;

    // print the commit file content
    while (std::getline(CommitLog, line))
    {
      std::cout << line << std::endl;
    }
    std::cout << "\n" << std::endl;
    CommitLog.close();
  }
  filePath.close();
}