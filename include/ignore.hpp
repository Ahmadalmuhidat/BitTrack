#ifndef IGNORE_HPP
#define IGNORE_HPP

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

std::string get_file_extenion(const std::string &filePath);
std::vector<std::string> read_bitignore(const std::string &filePath);
bool is_file_ignored(const std::string &filePath, const std::vector<std::string> &patterns);

#endif