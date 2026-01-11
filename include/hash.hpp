#ifndef HASH_HPP
#define HASH_HPP

#include <string>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <openssl/evp.h>
#include <openssl/sha.h>

std::string toHexString(unsigned char *hash, std::size_t length);
std::string generateCommitHash(
    const std::string &author,
    const std::string &commitMessage,
    const std::unordered_map<std::string, std::string> &fileHashes);
std::string calculateFileHash(const std::string &file_path);
std::string sha256Hash(const std::string &input);
std::string hashFile(const std::string &file_path);

#endif
