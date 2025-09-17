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

enum class TagType
{
  LIGHTWEIGHT,
  ANNOTATED
};

struct Tag
{
  std::string name;
  std::string commit_hash;
  TagType type;
  std::string message;
  std::string author;
  std::time_t timestamp;
  
  Tag(): type(TagType::LIGHTWEIGHT), timestamp(0) {}
};

void tag_create(const std::string& name, const std::string& commit_hash = "", bool annotated = false);
void tag_list();
void tag_delete(const std::string& name);
void tag_show(const std::string& name);
void tag_checkout(const std::string& name);
std::vector<Tag> get_all_tags();
Tag get_tag(const std::string& name);
void save_tag(const Tag& tag);
void delete_tag_file(const std::string& name);
std::string get_tag_file_path(const std::string& name);
std::string get_commit_hash(const std::string& name);
std::string get_tags_dir();
std::string format_timestamp(std::time_t timestamp);
bool tag_exists(const std::string& name);
std::string get_current_commit();

#endif
