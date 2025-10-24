#include "../include/tag.hpp"

void tag_create(const std::string &name, const std::string &commit_hash, bool annotated)
{
  // Check if tag already exists
  if (tag_exists(name))
  {
    std::cout << "Error: Tag '" << name << "' already exists" << std::endl;
    return;
  }

  // Determine the commit to tag
  std::string target_commit = commit_hash.empty() ? get_current_commit() : commit_hash;
  if (target_commit.empty())
  {
    std::cout << "Error: No commit to tag" << std::endl;
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
  save_tag(tag);

  std::cout << "Created " << (annotated ? "annotated" : "lightweight") << " tag: " << name << " -> " << target_commit << std::endl;
}

void tag_list()
{
  // Retrieve all tags
  std::vector<Tag> tags = get_all_tags();

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

void tag_delete(const std::string &name)
{
  // Check if tag exists
  if (!tag_exists(name))
  {
    std::cout << "Error: Tag '" << name << "' does not exist" << std::endl;
    return;
  }

  // Delete the tag
  delete_tag_file(name);
  std::cout << "Deleted tag: " << name << std::endl;
}

void tag_details(const std::string &name)
{
  // Retrieve the tag
  Tag tag = get_tag(name);
  if (tag.name.empty())
  {
    std::cout << "Error: Tag '" << name << "' not found" << std::endl;
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
    std::cout << "Date: " << format_timestamp(tag.timestamp) << std::endl;
    std::cout << "Message: " << tag.message << std::endl;
  }
}

std::vector<Tag> get_all_tags()
{
  // Retrieve all tags from the tags directory
  std::vector<Tag> tags;
  std::string tags_dir = get_tags_dir();

  // Check if tags directory exists
  if (!std::filesystem::exists(tags_dir))
  {
    return tags;
  }

  // Iterate through tag files and retrieve tag information
  for (const auto &entry : std::filesystem::directory_iterator(tags_dir))
  {
    // Only process regular files
    if (entry.is_regular_file())
    {
      // Get tag name from file name
      std::string tag_name = entry.path().filename().string();
      Tag tag = get_tag(tag_name);
      if (!tag.name.empty())
      {
        tags.push_back(tag);
      }
    }
  }
  return tags;
}

Tag get_tag(const std::string &name)
{
  // Retrieve tag information from the tag file
  Tag tag;
  std::string tag_file = get_tag_file_path(name);

  // Check if tag file exists
  if (!std::filesystem::exists(tag_file))
  {
    return tag;
  }

  // Parse the tag file
  std::ifstream file(tag_file);
  std::string line;

  // Read tag file lines
  if (std::getline(file, line))
  {
    if (line.find("object ") == 0)
    {
      tag.commit_hash = line.substr(7);
    }
  }

  // Read type line
  if (std::getline(file, line))
  {
    if (line.find("type ") == 0)
    {
      std::string type = line.substr(5);
      tag.type = (type == "commit") ? TagType::ANNOTATED : TagType::LIGHTWEIGHT;
    }
  }

  // Read tag name line
  if (std::getline(file, line))
  {
    if (line.find("tag ") == 0)
    {
      tag.name = line.substr(4);
    }
  }

  // Read tagger line
  if (std::getline(file, line))
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
  while (std::getline(file, line))
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

void save_tag(const Tag &tag)
{
  // Save tag information to the tag file
  std::string tag_file = get_tag_file_path(tag.name);
  std::filesystem::create_directories(std::filesystem::path(tag_file).parent_path());

  // Write tag data to file
  std::ofstream file(tag_file);

  // Write annotated tag format
  if (tag.type == TagType::ANNOTATED)
  {
    file << "object " << tag.commit_hash << std::endl;
    file << "type commit" << std::endl;
    file << "tag " << tag.name << std::endl;
    file << "tagger " << tag.author << " " << tag.timestamp << std::endl;
    file << std::endl;
    file << tag.message;
  }
  else
  {
    file << tag.commit_hash << std::endl;
  }

  file.close();
}

void delete_tag_file(const std::string &name)
{
  // Delete the tag file
  std::string tag_file = get_tag_file_path(name);
  if (std::filesystem::exists(tag_file))
  {
    std::filesystem::remove(tag_file);
  }
}

std::string get_tag_file_path(const std::string &name)
{
  // Construct the full path to the tag file and return it
  return get_tags_dir() + "/" + name;
}

std::string get_tags_dir()
{
  return ".bittrack/refs/tags";
}

bool tag_exists(const std::string &name)
{
  // Check if the tag file exists
  return std::filesystem::exists(get_tag_file_path(name));
}
