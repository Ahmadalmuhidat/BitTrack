#ifndef COMMIT_HPP
#define COMMIT_HPP

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "branch.hpp"
#include "error.hpp"
#include "hash.hpp"
#include "remote.hpp"
#include "stage.hpp"

void commit_changes(const std::string &author, const std::string &message);
void store_snapshot(const std::string &file_path, const std::string &commit_hash);
std::string get_current_commit();
std::string get_last_commit(const std::string &branch_name = "");
std::string get_commit_parent(const std::string &commit_hash);
void show_commit_history();
std::string generate_commit_hash(const std::string &author, const std::string &message, const std::string &timestamp);
std::string get_current_timestamp();
std::string get_current_user();
std::vector<std::string> get_commit_files(const std::string &commit_hash);

#endif
