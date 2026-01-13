#ifndef STASH_HPP
#define STASH_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>

#include "stage.hpp"
#include "commit.hpp"
#include "branch.hpp"
#include "utils.hpp"

// Represents a single stash entry
struct StashEntry
{
  std::string id;                 // unique identifier for the stash
  std::string message;            // stash message
  std::string branch;             // branch where the stash was created
  std::string commit_hash;        // commit hash at the time of stashing
  std::time_t timestamp;          // time when the stash was created
  std::vector<std::string> files; // List of files in the stash

  StashEntry() : timestamp(0) {}
};

void stashChanges(const std::string &message = "");
void stashList();
void stashShow(const std::string &stash_id = "");
void stashApply(const std::string &stash_id = "");
void stashPop(const std::string &stash_id = "");
void stashDrop(const std::string &stash_id = "");
void stashClear();
bool stashHasStashes();
std::vector<StashEntry> getStashEntries();
StashEntry getStashEntry(const std::string &stash_id);
void saveStashEntry(const StashEntry &entry);
void deleteStashEntry(const std::string &stash_id);
std::string generateStashId();
void backupStagedFiles(const std::string &stash_id);
void restoreWorkingDirectory(const std::string &stash_id);
void removeStagedFilesFromWorkingDirectory();
std::string getStashDir();
std::string getStashFilePath(const std::string &stash_id);

#endif
