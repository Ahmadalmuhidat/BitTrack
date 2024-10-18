#ifndef HASH_HPP
#define HASH_HPP

std::string HashFile(const std::string& FilePath);
std::string toHexString(unsigned char *hash, std::size_t length);
std::string GenerateCommitHash(
  const std::string& author,
  const std::string& commitMessage,
  const std::unordered_map<std::string,
  std::string>& fileHashes
);
void DisplayFilesHashes();

#endif