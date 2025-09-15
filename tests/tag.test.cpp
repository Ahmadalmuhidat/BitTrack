#include "../include/tag.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include <fstream>
#include <filesystem>

bool test_tag_creation()
{
  // Create a test file and commit it first
  std::ofstream file("tag_test.txt");
  file << "content for tagging" << std::endl;
  file.close();
  
  stage("tag_test.txt");
  commit_changes("test_user", "commit for tagging");
  
  // Create a tag
  tag_create("v1.0", "", true);
  
  // Check if tag was created
  bool exists = tag_exists("v1.0");
  
  // Clean up
  std::filesystem::remove("tag_test.txt");
  tag_delete("v1.0");
  
  return exists;
}

bool test_lightweight_tag_creation()
{
  // Create a test file and commit it first
  std::ofstream file("lightweight_tag_test.txt");
  file << "content for lightweight tagging" << std::endl;
  file.close();
  
  stage("lightweight_tag_test.txt");
  commit_changes("test_user", "commit for lightweight tagging");
  
  // Create a lightweight tag
  tag_create("v1.1", "", false);
  
  // Check if tag was created
  bool exists = tag_exists("v1.1");
  
  // Clean up
  std::filesystem::remove("lightweight_tag_test.txt");
  tag_delete("v1.1");
  
  return exists;
}

bool test_tag_listing()
{
  // Create a test file and commit it first
  std::ofstream file("tag_list_test.txt");
  file << "content for tag listing" << std::endl;
  file.close();
  
  stage("tag_list_test.txt");
  commit_changes("test_user", "commit for tag listing");
  
  // Create multiple tags
  tag_create("v1.0", "", true);
  tag_create("v1.1", "", false);
  tag_create("v2.0", "", true);
  
  // List tags
  std::vector<Tag> tags = get_all_tags();
  
  // Clean up
  std::filesystem::remove("tag_list_test.txt");
  tag_delete("v1.0");
  tag_delete("v1.1");
  tag_delete("v2.0");
  
  return tags.size() >= 3;
}

bool test_tag_deletion()
{
  // Create a test file and commit it first
  std::ofstream file("tag_delete_test.txt");
  file << "content for tag deletion" << std::endl;
  file.close();
  
  stage("tag_delete_test.txt");
  commit_changes("test_user", "commit for tag deletion");
  
  // Create a tag
  tag_create("v1.0", "", true);
  
  // Verify tag exists
  bool exists_before = tag_exists("v1.0");
  
  // Delete tag
  tag_delete("v1.0");
  
  // Verify tag is deleted
  bool exists_after = tag_exists("v1.0");
  
  // Clean up
  std::filesystem::remove("tag_delete_test.txt");
  
  return exists_before && !exists_after;
}

bool test_tag_show()
{
  // Create a test file and commit it first
  std::ofstream file("tag_show_test.txt");
  file << "content for tag show" << std::endl;
  file.close();
  
  stage("tag_show_test.txt");
  commit_changes("test_user", "commit for tag show");
  
  // Create a tag
  tag_create("v1.0", "", true);
  
  // Show tag (this should not throw an exception)
  tag_show("v1.0");
  
  // Clean up
  std::filesystem::remove("tag_show_test.txt");
  tag_delete("v1.0");
  
  return true; // If no exception thrown, test passes
}

bool test_tag_checkout()
{
  // Create a test file and commit it first
  std::ofstream file("tag_checkout_test.txt");
  file << "content for tag checkout" << std::endl;
  file.close();
  
  stage("tag_checkout_test.txt");
  commit_changes("test_user", "commit for tag checkout");
  
  // Create a tag
  tag_create("v1.0", "", true);
  
  // Checkout tag (this should not throw an exception)
  tag_checkout("v1.0");
  
  // Clean up
  std::filesystem::remove("tag_checkout_test.txt");
  tag_delete("v1.0");
  
  return true; // If no exception thrown, test passes
}

bool test_tag_get_commit()
{
  // Create a test file and commit it first
  std::ofstream file("tag_commit_test.txt");
  file << "content for tag commit test" << std::endl;
  file.close();
  
  stage("tag_commit_test.txt");
  commit_changes("test_user", "commit for tag commit test");
  
  std::string current_commit = get_current_commit();
  
  // Create a tag
  tag_create("v1.0", "", true);
  
  // Get tag commit
  std::string tag_commit = get_commit_hash("v1.0");
  
  // Clean up
  std::filesystem::remove("tag_commit_test.txt");
  tag_delete("v1.0");
  
  return tag_commit == current_commit;
}
