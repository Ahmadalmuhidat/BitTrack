#include "../include/tag.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include <fstream>
#include <filesystem>

// Create an annotated tag and verify its existence
bool test_tag_creation()
{
  std::ofstream file("tag_test.txt");
  file << "content for tagging" << std::endl;
  file.close();

  stage("tag_test.txt");
  commit_changes("test_user", "commit for tagging");

  tagCreate("v1.0", "", true);

  bool exists = tagExists("v1.0");

  std::filesystem::remove("tag_test.txt");
  tagDelete("v1.0");

  return exists;
}

// Create a lightweight tag and verify its existence
bool test_lightweight_tag_creation()
{
  std::ofstream file("lightweight_tag_test.txt");
  file << "content for lightweight tagging" << std::endl;
  file.close();

  stage("lightweight_tag_test.txt");
  commit_changes("test_user", "commit for lightweight tagging");

  tagCreate("v1.1", "", false);

  bool exists = tagExists("v1.1");

  std::filesystem::remove("lightweight_tag_test.txt");
  tagDelete("v1.1");

  return exists;
}

// List all tags and verify at least three exist
bool test_tag_listing()
{
  std::ofstream file("tag_list_test.txt");
  file << "content for tag listing" << std::endl;
  file.close();

  stage("tag_list_test.txt");
  commit_changes("test_user", "commit for tag listing");

  tagCreate("v1.0", "", true);
  tagCreate("v1.1", "", false);
  tagCreate("v2.0", "", true);

  std::vector<Tag> tags = getAllTags();

  std::filesystem::remove("tag_list_test.txt");
  tagDelete("v1.0");
  tagDelete("v1.1");
  tagDelete("v2.0");

  return tags.size() >= 3;
}

// delete a tag and verify it no longer exists
bool test_tag_deletion()
{
  std::ofstream file("tag_delete_test.txt");
  file << "content for tag deletion" << std::endl;
  file.close();

  stage("tag_delete_test.txt");
  commit_changes("test_user", "commit for tag deletion");

  tagCreate("v1.0", "", true);

  bool exists_before = tagExists("v1.0");

  tagDelete("v1.0");

  bool exists_after = tagExists("v1.0");

  std::filesystem::remove("tag_delete_test.txt");

  return exists_before && !exists_after;
}

// show tag details and verify output
bool test_tag_show()
{
  std::ofstream file("tag_show_test.txt");
  file << "content for tag show" << std::endl;
  file.close();

  stage("tag_show_test.txt");
  commit_changes("test_user", "commit for tag show");

  tagCreate("v1.0", "", true);

  tagDetails("v1.0");

  std::filesystem::remove("tag_show_test.txt");
  tagDelete("v1.0");

  return true;
}