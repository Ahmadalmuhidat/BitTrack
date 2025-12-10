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

std::string to_hex_string(unsigned char *hash, std::size_t length);
std::string generate_commit_hash_with_files(const std::string &author, const std::string &commitMessage, const std::unordered_map<std::string, std::string> &fileHashes);
std::string calculate_file_hash(const std::string &file_path);
std::string generate_file_hash(const std::string &content);
std::string sha256_hash(const std::string &input);
std::string hash_file(const std::string &file_path);

#endif
