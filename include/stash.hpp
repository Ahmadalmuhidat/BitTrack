#ifndef STASH_HPP
#define STASH_HPP

#include <string>
#include <vector>
#include <ctime>

struct StashEntry
{
  std::string id;
  std::string message;
  std::string branch;
  std::string commit_hash;
  std::time_t timestamp;
  std::vector<std::string> files;
  
  StashEntry() : timestamp(0) {}
};

void stash_changes(const std::string& message = "");
void stash_list();
void stash_show(const std::string& stash_id = "");
void stash_apply(const std::string& stash_id = "");
void stash_pop(const std::string& stash_id = "");
void stash_drop(const std::string& stash_id = "");
void stash_clear();
bool stash_has_stashes();
std::vector<StashEntry> get_stash_entries();
StashEntry get_stash_entry(const std::string& stash_id);
void save_stash_entry(const StashEntry& entry);
void delete_stash_entry(const std::string& stash_id);
std::string generate_stash_id();
void backup_working_directory(const std::string& stash_id);
void restore_working_directory(const std::string& stash_id);
std::vector<std::string> get_tracked_files();
std::string format_timestamp(std::time_t timestamp);
std::string get_stash_dir();
std::string get_stash_file_path(const std::string& stash_id);

#endif
