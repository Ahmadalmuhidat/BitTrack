#ifndef BRANCH_HPP
#define BRANCH_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <set>

#include "error.hpp"
#include "utils.hpp"
#include "stage.hpp"
#include "commit.hpp"
#include "remote.hpp"

std::string get_current_branch();
std::vector<std::string> get_branches_list();
void add_branch(const std::string& branch_name);
void remove_branch(const std::string& branch_name);
void switch_branch(const std::string& branch_name);
void create_branch(const std::string& branch_name);
void rename_branch(const std::string& old_name, const std::string& new_name);
void show_branch_info(const std::string& branch_name);
bool branch_exists(const std::string& branch_name);
std::string get_branch_last_commit_hash(const std::string& branch_name);
void insert_commit_record_to_history(const std::string& commit_hash, const std::string& branch_name);
void rebase_branch(const std::string& source_branch, const std::string& target_branch);
void show_branch_history(const std::string& branch_name);
bool has_uncommitted_changes();
std::vector<std::string> get_branch_commits(const std::string& branch_name);
void cleanup_branch_commits(const std::string& branch_name);
std::string find_common_ancestor(const std::string& branch1, const std::string& branch2);
std::vector<std::string> get_commit_chain(const std::string& from_commit, const std::string& to_commit);
bool apply_commit_during_rebase(const std::string& commit_hash);

#endif
