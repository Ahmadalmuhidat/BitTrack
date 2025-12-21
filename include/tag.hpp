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
  ANNOTATED    // Tag with additional metadata
};

struct Tag
{
  std::string name;        // Tag name
  std::string commit_hash; // Commit hash the tag points to
  TagType type;            // Type of tag
  std::string message;     // Tag message (for annotated tags)
  std::string author;      // Author of the tag (for annotated tags)
  std::time_t timestamp;   // Timestamp of the tag (for annotated tags)

  Tag() : type(TagType::LIGHTWEIGHT), timestamp(0) {}
};

void tagCreate(const std::string &name, const std::string &commit_hash = "", bool annotated = false);
void tagList();
void tagDelete(const std::string &name);
void tagDetails(const std::string &name);
std::vector<Tag> getAllTags();
Tag getTag(const std::string &name);
void tagSave(const Tag &tag);
void deleteTagFile(const std::string &name);
bool tagExists(const std::string &name);
std::string getTagFilePath(const std::string &name);
std::string getTagsDir();

#endif
