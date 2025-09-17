#ifndef MERGE_HPP
#define MERGE_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>

#include "commit.hpp"
#include "branch.hpp"
#include "stage.hpp"

struct MergeResult
{
  bool success;
  bool has_conflicts;
  std::string merge_commit;
  std::vector<std::string> conflicted_files;
  std::string message;
  
  MergeResult(): success(false), has_conflicts(false) {}
};

struct MergeConflict
{
  std::string file_path;
  std::string conflict_type;
  std::vector<std::string> conflict_markers;
  
  MergeConflict(const std::string& path, const std::string& type): file_path(path), conflict_type(type) {}
};

MergeResult merge_branches(const std::string& source_branch, const std::string& target_branch);
MergeResult merge_commits(const std::string& commit1, const std::string& commit2);
MergeResult three_way_merge(const std::string& base, const std::string& ours, const std::string& theirs);
bool has_conflicts();
void show_conflicts();
std::vector<std::string> get_conflicted_files();
void resolve_file_conflict(const std::string& file_path);
void abort_merge();
void continue_merge();
std::string find_merge_base(const std::string& commit1, const std::string& commit2);
bool is_ancestor(const std::string& ancestor, const std::string& descendant);
bool is_fast_forward(const std::string& source, const std::string& target);
void save_merge_state(const MergeResult& result);
MergeResult load_merge_state();
void clear_merge_state();
bool is_merge_in_progress();
std::string get_merge_base_commit(const std::string& branch1, const std::string& branch2);
std::vector<std::string> get_commit_files(const std::string& commit_hash);
std::string get_file_content(const std::string& file_path);
void create_merge_commit(const std::string& message, const std::vector<std::string>& parents);

#endif
