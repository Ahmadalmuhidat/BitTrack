#include "../include/tag.hpp"

void tagCreate(const std::string &name, const std::string &commit_hash, bool annotated)
{
  // Check if tag already exists
  if (tagExists(name))
  {
    ErrorHandler::printError(ErrorCode::TAG_ERROR, "Tag '" + name + "' already exists", ErrorSeverity::ERROR, "tag_create");
    return;
  }

  // Determine the commit to tag
  std::string target_commit = commit_hash.empty() ? get_current_commit() : commit_hash;
  if (target_commit.empty())
  {
    ErrorHandler::printError(ErrorCode::NO_COMMITS_FOUND, "No commit to tag", ErrorSeverity::ERROR, "tag_create");
    return;
  }

  // Create the tag
  Tag tag;
  tag.name = name;
  tag.commit_hash = target_commit;
  tag.type = annotated ? TagType::ANNOTATED : TagType::LIGHTWEIGHT;
  tag.timestamp = std::time(nullptr);

  // Set additional fields for annotated tags
  if (annotated)
  {
    tag.message = "Tagged commit " + target_commit;
    tag.author = "BitTrack User";
  }

  // Save the tag
  tagSave(tag);

  std::cout << "Created " << (annotated ? "annotated" : "lightweight") << " tag: " << name << " -> " << target_commit << std::endl;
}

void tagList()
{
  // Retrieve all tags
  std::vector<Tag> tags = getAllTags();

  if (tags.empty())
  {
    std::cout << "No tags found" << std::endl;
    return;
  }

  // Display the tags
  std::cout << "Tags:" << std::endl;
  for (const auto &tag : tags)
  {
    std::cout << "  " << tag.name;
    if (tag.type == TagType::ANNOTATED)
    {
      std::cout << " (annotated)";
    }
    else
    {
      std::cout << " (lightweight)";
    }
    std::cout << " -> " << tag.commit_hash << std::endl;
  }
}

void tagDelete(const std::string &name)
{
  // Check if tag exists
  if (!tagExists(name))
  {
    ErrorHandler::printError(ErrorCode::TAG_ERROR, "Tag '" + name + "' does not exist", ErrorSeverity::ERROR, "tag_delete");
    return;
  }

  // Delete the tag
  deleteTagFile(name);
  std::cout << "Deleted tag: " << name << std::endl;
}

void tagDetails(const std::string &name)
{
  // Retrieve the tag
  Tag tag = getTag(name);
  if (tag.name.empty())
  {
    ErrorHandler::printError(ErrorCode::TAG_ERROR, "Tag '" + name + "' not found", ErrorSeverity::ERROR, "tag_details");
    return;
  }

  // Display tag details
  std::cout << "Tag: " << tag.name << std::endl;
  std::cout << "Commit: " << tag.commit_hash << std::endl;
  std::cout << "Type: " << (tag.type == TagType::ANNOTATED ? "annotated" : "lightweight") << std::endl;

  // Display additional details for annotated tags
  if (tag.type == TagType::ANNOTATED)
  {
    std::cout << "Author: " << tag.author << std::endl;
    std::cout << "Date: " << formatTimestamp(tag.timestamp) << std::endl;
    std::cout << "Message: " << tag.message << std::endl;
  }
}

std::vector<Tag> getAllTags()
{
  // Retrieve all tags from the tags directory
  std::vector<Tag> tags;
  std::string tags_dir = getTagsDir();

  // Check if tags directory exists
  if (!std::filesystem::exists(tags_dir))
  {
    return tags;
  }

  // Iterate through tag files and retrieve tag information
  for (const auto &entry : ErrorHandler::safeListDirectoryFiles(tags_dir))
  {
    // Get tag name from file name
    std::string tag_name = entry.filename().string();
    Tag tag = getTag(tag_name);
    if (!tag.name.empty())
    {
      tags.push_back(tag);
    }
  }

  return tags;
}

Tag getTag(const std::string &name)
{
  // Retrieve tag information from the tag file
  Tag tag;
  std::string tag_file = getTagFilePath(name);

  // Check if tag file exists
  if (!std::filesystem::exists(tag_file))
  {
    return tag;
  }

  // Parse the tag file
  std::string file = ErrorHandler::safeReadFile(tag_file);
  std::istringstream file_content(file);
  std::string line;

  // Read tag file lines
  if (std::getline(file_content, line))
  {
    if (line.find("object ") == 0)
    {
      tag.commit_hash = line.substr(7);
    }
  }

  // Read type line
  if (std::getline(file_content, line))
  {
    if (line.find("type ") == 0)
    {
      std::string type = line.substr(5);
      tag.type = (type == "commit") ? TagType::ANNOTATED : TagType::LIGHTWEIGHT;
    }
  }

  // Read tag name line
  if (std::getline(file_content, line))
  {
    if (line.find("tag ") == 0)
    {
      tag.name = line.substr(4);
    }
  }

  // Read tagger line
  if (std::getline(file_content, line))
  {
    if (line.find("tagger ") == 0)
    {
      tag.author = line.substr(7);
    }
  }

  // Read timestamp line
  std::stringstream message_stream;
  bool in_message = false;

  // Read the rest of the file for the message
  while (std::getline(file_content, line))
  {
    if (in_message)
    {
      message_stream << line << std::endl;
    }
    else if (line.empty())
    {
      in_message = true;
    }
  }
  tag.message = message_stream.str();

  // Handle lightweight tags
  if (tag.type == TagType::LIGHTWEIGHT)
  {
    std::ifstream light_file(tag_file);
    std::getline(light_file, tag.commit_hash);
    tag.name = name;
    tag.type = TagType::LIGHTWEIGHT;
  }

  return tag;
}

void tagSave(const Tag &tag)
{
  // Save tag information to the tag file
  std::string tag_file = getTagFilePath(tag.name);
  ErrorHandler::safeCreateDirectories(std::filesystem::path(tag_file).parent_path().string());

  // Write annotated tag format
  if (tag.type == TagType::ANNOTATED)
  {
    ErrorHandler::safeWriteFile(tag_file,
                                "object " + tag.commit_hash + "\n" +
                                    "type commit\n" +
                                    "tag " + tag.name + "\n" +
                                    "tagger " + tag.author + " " + std::to_string(tag.timestamp) + "\n\n" +
                                    tag.message);
  }
  else
  {
    ErrorHandler::safeWriteFile(tag_file, tag.commit_hash + "\n");
  }
}

void deleteTagFile(const std::string &name)
{
  std::string tag_file = getTagFilePath(name);
  ErrorHandler::safeRemoveFile(tag_file);
}

std::string getTagFilePath(const std::string &name)
{
  return getTagsDir() + "/" + name;
}

std::string getTagsDir()
{
  return ".bittrack/refs/tags";
}

bool tagExists(const std::string &name)
{
  // Check if the tag file exists
  return std::filesystem::exists(getTagFilePath(name));
}
