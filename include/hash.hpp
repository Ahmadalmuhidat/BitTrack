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

std::string hash_file(const std::string& FilePath);
std::string to_hex_string(unsigned char *hash, std::size_t length);
std::string generate_commit_hash(const std::string &author, const std::string &commitMessage, const std::unordered_map<std::string, std::string> &fileHashes);
void display_files_hashes();

#endif