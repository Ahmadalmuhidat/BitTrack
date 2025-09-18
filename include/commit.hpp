#ifndef COMMIT_HPP
#define COMMIT_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>

#include "hash.hpp"

void commit_changes(const std::string& author, const std::string& message);
void insert_commit_record_to_history(const std::string& last_commit_hash, const std::string& new_branch_name);
void store_snapshot(const std::string& file_path, const std::string& commit_hash);
std::string get_current_commit();
std::string get_staged_file_content(const std::string& file_path);
std::string get_last_commit(const std::string& branch_name = "");
void show_commit_history();
void show_commit_details(const std::string& commit_hash);
std::vector<std::string> get_commit_files(const std::string& commit_hash);
std::string generate_commit_hash(const std::string& author, const std::string& message, const std::string& timestamp);
std::string get_commit_message(const std::string& commit_hash);
std::string get_commit_author(const std::string& commit_hash);
std::string get_commit_timestamp(const std::string& commit_hash);
std::string get_current_branch();
void create_commit_log(const std::string& author, const std::string& message, const std::unordered_map<std::string, std::string>& file_hashes, const std::string& commit_hash);
std::string get_current_timestamp();
std::string get_current_user();
std::string get_commit_info(const std::string& commit_hash);

#endif
