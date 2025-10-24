#ifndef MERGE_HPP
#define MERGE_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <set>

#include "commit.hpp"
#include "branch.hpp"
#include "stage.hpp"
#include "utils.hpp"
#include "error.hpp"

// Result of a merge operation
struct MergeResult
{
  bool success; // true if merge was successful
  bool has_conflicts; // true if there were conflicts
  std::string merge_commit; // hash of the merge commit
  std::vector<std::string> conflicted_files; // list of files with conflicts
  std::string message; // merge message
  
  MergeResult(): success(false), has_conflicts(false) {}
};

// Details of a merge conflict
struct MergeConflict
{
  std::string file_path; // path of the file with conflict
  std::string conflict_type; // type of conflict (e.g., content, binary)
  std::vector<std::string> conflict_markers; // conflict markers in the file
  
  MergeConflict(const std::string& path, const std::string& type): file_path(path), conflict_type(type) {}
};

MergeResult merge_branches(const std::string& source_branch, const std::string& target_branch);
MergeResult merge_commits(const std::string& commit1, const std::string& commit2);
MergeResult three_way_merge(const std::string& base, const std::string& ours, const std::string& theirs);
bool has_conflicts();
void show_conflicts();
std::vector<std::string> get_conflicted_files();
void abort_merge();
void continue_merge();
void write_conflict(const std::string &path, const std::string &ours,const std::string &theirs);
std::string find_merge_base(const std::string& commit1, const std::string& commit2);
bool is_ancestor(const std::string& ancestor, const std::string& descendant);
bool is_fast_forward(const std::string& source, const std::string& target);
void save_merge_state(const MergeResult& result);
MergeResult load_merge_state();
void clear_merge_state();
bool is_merge_in_progress();
std::vector<std::string> get_commit_files(const std::string& commit_hash);
void create_merge_commit(const std::string& message, const std::vector<std::string>& parents);

#endif
