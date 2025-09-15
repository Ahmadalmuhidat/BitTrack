#ifndef BRANCH_HPP
#define BRANCH_HPP

#include <string>
#include <vector>

std::string get_current_branch();
std::vector<std::string> get_branches_list();
std::vector<std::string> list_branches();
void add_branch(const std::string& branch_name);
void remove_branch(const std::string& branch_name);
void switch_branch(const std::string& branch_name);
void create_branch(const std::string& branch_name);
void delete_branch(const std::string& branch_name);
void rename_branch(const std::string& old_name, const std::string& new_name);
void show_branch_info(const std::string& branch_name);
bool branch_exists(const std::string& branch_name);
std::string get_branch_commit(const std::string& branch_name);
std::string get_current_commit();
std::vector<std::string> get_staged_files();
std::vector<std::string> get_unstaged_files();
void insert_commit_to_history(const std::string& commit_hash, const std::string& branch_name);
void merge_branch(const std::string& source_branch, const std::string& target_branch);
void rebase_branch(const std::string& source_branch, const std::string& target_branch);
void show_branch_history(const std::string& branch_name);
void compare_branches(const std::string& branch1, const std::string& branch2);

#endif
