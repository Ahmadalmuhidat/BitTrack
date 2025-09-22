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
bool is_deleted(const std::string& file_path);
std::string get_actual_path(const std::string& file_path);

// Helper functions for stage operations
std::unordered_map<std::string, std::string> load_staged_files();
void save_staged_files(const std::unordered_map<std::string, std::string>& staged_files);
bool validate_file_for_staging(const std::string& file_path);
std::string calculate_file_hash(const std::string& file_path);
bool is_file_unchanged_from_commit(const std::string& file_path, const std::string& file_hash);
void stage_single_file_impl(const std::string& file_path, std::unordered_map<std::string, std::string>& staged_files);
void stage_all_files(std::unordered_map<std::string, std::string>& staged_files);

#endif
