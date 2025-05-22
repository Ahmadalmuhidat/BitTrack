#ifndef BRANCH_HPP
#define BRANCH_HPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstdio>

#include "stage.hpp"
#include "hash.hpp"
#include "commit.hpp"

std::string get_current_branch();
std::vector<std::string> get_branches_list();
void print_branshes_list();
void add_branch(std::string name);
void switch_branch(std::string name);
void copy_current_commit_to_branch(std::string new_branch);
void remove_branch(std::string branch_name);
void merge_two_branches(const std::string& first_branch, const std::string& second_branch);
bool compare_files_contents(const std::filesystem::path& first_file, const std::filesystem::path& second_file);

#endif