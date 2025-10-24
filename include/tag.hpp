#ifndef TAG_HPP
#define TAG_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>

#include "commit.hpp"
#include "branch.hpp"
#include "utils.hpp"

// Types of tags
enum class TagType
{
  LIGHTWEIGHT, // Simple tag pointing to a commit
  ANNOTATED // Tag with additional metadata
};

struct Tag
{
  std::string name; // Tag name
  std::string commit_hash; // Commit hash the tag points to
  TagType type; // Type of tag
  std::string message; // Tag message (for annotated tags)
  std::string author; // Author of the tag (for annotated tags)
  std::time_t timestamp; // Timestamp of the tag (for annotated tags)
  
  Tag(): type(TagType::LIGHTWEIGHT), timestamp(0) {}
};

void tag_create(const std::string& name, const std::string& commit_hash = "", bool annotated = false);
void tag_list();
void tag_delete(const std::string& name);
void tag_details(const std::string& name);
std::vector<Tag> get_all_tags();
Tag get_tag(const std::string& name);
void save_tag(const Tag& tag);
void delete_tag_file(const std::string& name);
bool tag_exists(const std::string& name);
std::string get_tag_file_path(const std::string& name);
std::string get_tags_dir();

#endif
