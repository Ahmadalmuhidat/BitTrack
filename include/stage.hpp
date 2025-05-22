#ifndef STAGE_HPP
#define STAGE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

#include "../src/ignore.cpp"
#include "../src/hash.cpp"
#include "branch.hpp"

void stage(std::string FilePath);
void unstage(const std::string &filePath);
std::vector<std::string> get_staged_files();
std::vector<std::string> get_unstaged_files();
bool compare_with_current_version(const std::string &CurrentFile, const std::string &CurrentBranch);
std::string normalize_file_path(const std::string &path);
std::string get_current_commit();

#endif