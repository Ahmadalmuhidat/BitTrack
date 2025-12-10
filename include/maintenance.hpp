#ifndef MAINTENANCE_HPP
#define MAINTENANCE_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>
#include <map>
#include <ctime>
#include <vector>
#include <string>

#include "commit.hpp"
#include "branch.hpp"
#include "tag.hpp"
#include "stage.hpp"
#include "hash.hpp"
#include "ignore.hpp"

// Statistics about the repository
struct RepoStats
{
  size_t total_objects;     // total number of objects
  size_t total_size;        // total size of all objects in bytes
  size_t commit_count;      // total number of commits
  size_t branch_count;      // total number of branches
  size_t tag_count;         // total number of tags
  std::string largest_file; // path of the largest file
  size_t largest_file_size; // size of the largest file in bytes

  RepoStats() : total_objects(0), total_size(0), commit_count(0), branch_count(0), tag_count(0), largest_file_size(0) {}
};

void garbage_collect();
void repack_repository();
void prune_objects();
void fsck_repository();
void show_repository_info();
void analyze_repository();
void find_large_files(size_t threshold = 1024 * 1024); // 1MB default
void find_duplicate_files();
void optimize_repository();
RepoStats calculate_repository_stats();
std::vector<std::string> get_unreachable_objects();
std::vector<std::string> get_duplicate_files();
std::string format_size(size_t bytes);

#endif
