#ifndef HASH_HPP
#define HASH_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <openssl/sha.h>
#include <sstream>

#include "ignore.hpp"

std::string HashFile(const std::string& FilePath);
std::string toHexString(unsigned char *hash, std::size_t length);
std::string GenerateCommitHash(
  const std::string &author,
  const std::string &commitMessage,
  const std::unordered_map<std::string, std::string> &fileHashes
);
void DisplayFilesHashes();

#endif