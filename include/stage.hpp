#ifndef STAGE_HPP
#define STAGE_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

#include "error.hpp"
#include "ignore.hpp"
#include "hash.hpp"
#include "branch.hpp"
#include "commit.hpp"

void stage(const std::string& file_path);
void unstage(const std::string& file_path);
std::vector<std::string> get_staged_files();
std::vector<std::string> get_unstaged_files();
bool is_staged(const std::string& file_path);
std::string get_file_hash(const std::string& file_path);
std::string get_staged_file_content(const std::string& file_path);
bool is_file_ignored_by_patterns(const std::string& file_path, const std::vector<std::string>& patterns);

#endif
