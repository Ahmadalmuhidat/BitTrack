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
#include <unistd.h>
#include <set>

#include "hash.hpp"
#include "branch.hpp"
#include "stage.hpp"
#include "remote.hpp"
#include "error.hpp"

void commit_changes(const std::string& author, const std::string& message);
void store_snapshot(const std::string& file_path, const std::string& commit_hash);
std::string get_current_commit();
std::string get_last_commit(const std::string& branch_name = "");
void show_commit_history();
std::string generate_commit_hash(const std::string& author, const std::string& message, const std::string& timestamp);
std::string get_current_timestamp();
std::string get_current_user();
std::vector<std::string> get_commit_files(const std::string& commit_hash);

#endif
