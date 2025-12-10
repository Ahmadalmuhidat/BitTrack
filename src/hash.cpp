#include "../include/hash.hpp"

std::string to_hex_string(unsigned char *hash, std::size_t length) {
  // Convert the hash bytes to a hexadecimal string
  std::ostringstream hexStream;
  for (std::size_t i = 0; i < length; ++i) {
    // Format each byte as a two-digit hexadecimal number
    hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return hexStream.str();
}

std::string generate_commit_hash_with_files(const std::string &author, const std::string &commitMessage, const std::unordered_map<std::string, std::string> &fileHashes) {
  // Get the current timestamp
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::ostringstream timestampStream;

  // Format the timestamp
  timestampStream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");

  // Prepare the commit data string
  std::ostringstream commitDataStream;
  commitDataStream << "Author: " << author << "\n";
  commitDataStream << "Timestamp: " << timestampStream.str() << "\n";
  commitDataStream << "Message: " << commitMessage << "\n";
  commitDataStream << "File Hashes: \n";

  // Append each file and its hash
  for (const auto &[file, fileHash] : fileHashes) {
    commitDataStream << file << " " << fileHash << "\n";
  }

  // Compute the SHA-256 hash of the commit data
  std::string commitData = commitDataStream.str();
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)commitData.c_str(), commitData.size(), hash);

  return to_hex_string(hash, SHA256_DIGEST_LENGTH);
}

std::string hash_file(const std::string &FilePath) {
  // Read the file content
  std::ifstream file(FilePath, std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();

  // Compute the SHA-256 hash of the file content
  std::string FileContent = ss.str();
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char *)FileContent.c_str(), FileContent.size(), hash);

  return to_hex_string(hash, SHA256_DIGEST_LENGTH);
}

std::string sha256_hash(const std::string &input) {
  // Compute the SHA-256 hash of the input string
  std::hash<std::string> hasher;
  size_t hash_value = hasher(input);

  std::stringstream ss;
  ss << std::hex << hash_value;

  return ss.str();
}
