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

std::string getCurrentBranchName();
std::vector<std::string> getBranchesList();
void addBranch(const std::string &branch_name);
void removeBranch(const std::string &branch_name);
void checkoutToBranch(const std::string &branch_name);
void renameBranch(
    const std::string &old_name,
    const std::string &new_name);
void printBranchInfo(const std::string &branch_name);
bool isBranchExists(const std::string &branch_name);
std::string getBranchLastCommitHash(const std::string &branch_name);
void insertCommitRecordToHistory(
    const std::string &commit_hash,
    const std::string &branch_name);
void rebaseBranch(
    const std::string &source_branch,
    const std::string &target_branch);
void printBranchHistory(const std::string &branch_name);
bool hasUncommittedChanges();
std::vector<std::string> getBranchCommits(const std::string &branch_name);
void cleanupBranchCommits(const std::string &branch_name);
std::string findCommonAncestor(
    const std::string &branch1,
    const std::string &branch2);
std::vector<std::string> getCommitsChain(
    const std::string &from_commit,
    const std::string &to_commit);
bool applyCommitDuringRebase(const std::string &commit_hash);

#endif