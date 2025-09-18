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
std::string get_current_commit();
std::vector<std::string> get_staged_files();
std::vector<std::string> get_unstaged_files();
void insert_commit_record_to_history(const std::string& commit_hash, const std::string& branch_name);
void merge_branch(const std::string& source_branch, const std::string& target_branch);
void rebase_branch(const std::string& source_branch, const std::string& target_branch);
void show_branch_history(const std::string& branch_name);
void compare_branches(const std::string& branch1, const std::string& branch2);
void merge_two_branches(const std::string& first_branch, const std::string& second_branch);
bool has_uncommitted_changes();

// Helper functions for branch comparison
std::set<std::string> get_branch_files(const std::string& branch_name);
std::string get_file_content_from_branch(const std::string& branch_name, const std::string& filename);

#endif
