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

void stage(const std::string& file_path);
void unstage(const std::string& file_path);
void unstage_all();
std::vector<std::string> get_staged_files();
std::vector<std::string> get_unstaged_files();
bool is_staged(const std::string& file_path);
void clear_staging_area();
void show_staged_files();
void show_unstaged_files();
std::string get_file_hash(const std::string& file_path);
std::string hash_file(const std::string& file_path);
void update_staging_index();
std::vector<std::string> read_bitignore(const std::string& ignore_file_path);
bool is_file_ignored(const std::string& file_path, const std::vector<std::string>& patterns);
std::string normalize_file_path(const std::string& path);
bool compare_with_current_version(const std::string& current_file, const std::string& current_branch);
std::string get_current_commit();
std::string get_staged_file_content(const std::string& file_path);
std::string read_file_content(const std::string& file_path);

#endif
