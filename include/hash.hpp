#ifndef HASH_HPP
#define HASH_HPP

#include <string>
#include <unordered_map>

std::string to_hex_string(unsigned char* hash, std::size_t length);
std::string generate_commit_hash(const std::string& author, const std::string& commitMessage, const std::unordered_map<std::string, std::string>& fileHashes);
std::string calculate_file_hash(const std::string& file_path);
std::string generate_file_hash(const std::string& content);
std::string sha256_hash(const std::string& input);
std::string hash_file(const std::string& file_path);
void get_index_hashes();
std::vector<std::string> read_bitignore(const std::string& ignore_file_path);
bool is_file_ignored(const std::string& file_path, const std::vector<std::string>& patterns);

#endif
