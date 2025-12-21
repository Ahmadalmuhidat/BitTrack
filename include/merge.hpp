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
  bool success;                              // true if merge was successful
  bool has_conflicts;                        // true if there were conflicts
  std::string merge_commit;                  // hash of the merge commit
  std::vector<std::string> conflicted_files; // List of files with conflicts
  std::string message;                       // merge message

  MergeResult() : success(false), has_conflicts(false) {}
};

// Details of a merge conflict
struct MergeConflict
{
  std::string file_path;                     // path of the file with conflict
  std::string conflict_type;                 // type of conflict (e.g., content, binary)
  std::vector<std::string> conflict_markers; // conflict markers in the file

  MergeConflict(const std::string &path, const std::string &type) : file_path(path), conflict_type(type) {}
};

MergeResult mergeBranches(const std::string &source_branch, const std::string &target_branch);
MergeResult mergeCommits(const std::string &commit1, const std::string &commit2);
MergeResult threeWayMerge(const std::string &base, const std::string &ours, const std::string &theirs);
bool hasConflicts();
void showConflicts();
std::vector<std::string> getConflictedFiles();
void abortMerge();
void continueMerge();
void writeConflict(const std::string &path, const std::string &ours, const std::string &theirs);
std::string findMergeBase(const std::string &commit1, const std::string &commit2);
bool isAncestor(const std::string &ancestor, const std::string &descendant);
bool isFastForward(const std::string &source, const std::string &target);
void saveMergeState(const MergeResult &result);
MergeResult loadMergeState();
void clearMergeState();
bool isMergeInProgress();
std::vector<std::string> getCommitFiles(const std::string &commit_hash);
void createMergeCommit(const std::string &message, const std::vector<std::string> &parents);

#endif
