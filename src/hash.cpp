#include "../include/hash.hpp"

std::string toHexString(unsigned char *hash, std::size_t length)
{
  std::ostringstream hexStream;

  for (std::size_t i = 0; i < length; ++i)
  {
    hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }

  return hexStream.str();
}

std::string GenerateCommitHash(
  const std::string &author,
  const std::string &commitMessage,
  const std::unordered_map<std::string,
  std::string> &fileHashes
)
{
  // get the current time
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::ostringstream timestampStream;

  timestampStream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");

  // get the commit log information to use them in hashing
  std::ostringstream commitDataStream;
  commitDataStream << "Author: " << author << "\n";
  commitDataStream << "Timestamp: " << timestampStream.str() << "\n";
  commitDataStream << "Message: " << commitMessage << "\n";
  commitDataStream << "File Hashes: \n";

  for (const auto &[file, fileHash]: fileHashes)
  {
    commitDataStream << file << " " << fileHash << "\n";
  }

  // hash the commit log information
  std::string commitData = commitDataStream.str();
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)commitData.c_str(), commitData.size(), hash);

  return toHexString(hash, SHA256_DIGEST_LENGTH);
}

std::string HashFile(const std::string &FilePath)
{
  // read the file content
  std::ifstream file(FilePath, std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();

  // hash the file content
  std::string FileContent = ss.str();
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)FileContent.c_str(), FileContent.size(), hash);

  return toHexString(hash, SHA256_DIGEST_LENGTH);
}

void getIndexHashes()
{
  std::unordered_map<std::string, std::string> FileHashes;
  std::vector<std::string> ignorePatterns = ReadBitignore(".bitignore");

  // read all project files
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    // check if the file is part of .bittrack system
    if (entry.is_regular_file() && entry.path().string().find(".bittrack") == std::string::npos)
    {
      // check if the file is ignored
      std::string FilePath = entry.path().string();

      if (isIgnored(FilePath, ignorePatterns))
      {
        continue;
      }

      // hash the file and store it in {key: value} array
      std::string FileHash = HashFile(FilePath);
      FileHashes[FilePath] = FileHash;
    }
  }

  // print the files along with their hashes
  for (const auto &[FilePath, FileHash] : FileHashes)
  {
    std::cout << FilePath << " | " << FileHash << std::endl;
  }
}