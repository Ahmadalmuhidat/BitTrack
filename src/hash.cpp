#include "../include/hash.hpp"

std::string to_hex_string(unsigned char *hash, std::size_t length)
{
  std::ostringstream hexStream;

  for (std::size_t i = 0; i < length; ++i)
  {
    hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return hexStream.str();
}

std::string generate_commit_hash_with_files(const std::string &author, const std::string &commitMessage, const std::unordered_map<std::string, std::string> &fileHashes)
{
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::ostringstream timestampStream;

  timestampStream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");

  std::ostringstream commitDataStream;
  commitDataStream << "Author: " << author << "\n";
  commitDataStream << "Timestamp: " << timestampStream.str() << "\n";
  commitDataStream << "Message: " << commitMessage << "\n";
  commitDataStream << "File Hashes: \n";

  for (const auto &[file, fileHash] : fileHashes)
  {
    commitDataStream << file << " " << fileHash << "\n";
  }
  std::string commitData = commitDataStream.str();
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)commitData.c_str(), commitData.size(), hash);
  return to_hex_string(hash, SHA256_DIGEST_LENGTH);
}

std::string hash_file(const std::string &FilePath)
{
  std::ifstream file(FilePath, std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();

  std::string FileContent = ss.str();
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)FileContent.c_str(), FileContent.size(), hash);

  return to_hex_string(hash, SHA256_DIGEST_LENGTH);
}

std::string sha256_hash(const std::string &input)
{
  std::hash<std::string> hasher;
  size_t hash_value = hasher(input);

  std::stringstream ss;
  ss << std::hex << hash_value;
  return ss.str();
}
