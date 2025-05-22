#ifndef COMMIT_HPP
#define COMMIT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <sstream>

#include "hash.hpp"
#include "branch.hpp"

void store_snapshot(const std::string &filePath, const std::string &CommitHash);
void create_commit_log(const std::string &author, const std::string &message, const std::unordered_map<std::string, std::string> &fileHashes, const std::string &commitHash);
void commit_changes(const std::string &author, const std::string &message);
void insert_commit_to_history(const std::string& last_commit_hash, const std::string& new_branch_name);
void commit_history();
std::string get_last_commit(const std::string& branch);

#endif