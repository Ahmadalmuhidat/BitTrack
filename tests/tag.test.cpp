#include "../include/tag.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include <fstream>
#include <filesystem>

bool test_tag_creation()
{
  std::ofstream file("tag_test.txt");
  file << "content for tagging" << std::endl;
  file.close();
  
  stage("tag_test.txt");
  commit_changes("test_user", "commit for tagging");
  
  tag_create("v1.0", "", true);
  
  bool exists = tag_exists("v1.0");
  
  std::filesystem::remove("tag_test.txt");
  tag_delete("v1.0");
  
  return exists;
}

bool test_lightweight_tag_creation()
{
  std::ofstream file("lightweight_tag_test.txt");
  file << "content for lightweight tagging" << std::endl;
  file.close();
  
  stage("lightweight_tag_test.txt");
  commit_changes("test_user", "commit for lightweight tagging");
  
  tag_create("v1.1", "", false);
  
  bool exists = tag_exists("v1.1");
  
  std::filesystem::remove("lightweight_tag_test.txt");
  tag_delete("v1.1");
  
  return exists;
}

bool test_tag_listing()
{
  std::ofstream file("tag_list_test.txt");
  file << "content for tag listing" << std::endl;
  file.close();
  
  stage("tag_list_test.txt");
  commit_changes("test_user", "commit for tag listing");
  
  tag_create("v1.0", "", true);
  tag_create("v1.1", "", false);
  tag_create("v2.0", "", true);
  
  std::vector<Tag> tags = get_all_tags();
  
  std::filesystem::remove("tag_list_test.txt");
  tag_delete("v1.0");
  tag_delete("v1.1");
  tag_delete("v2.0");
  
  return tags.size() >= 3;
}

bool test_tag_deletion()
{
  std::ofstream file("tag_delete_test.txt");
  file << "content for tag deletion" << std::endl;
  file.close();
  
  stage("tag_delete_test.txt");
  commit_changes("test_user", "commit for tag deletion");
  
  tag_create("v1.0", "", true);
  
  bool exists_before = tag_exists("v1.0");
  
  tag_delete("v1.0");
  
  bool exists_after = tag_exists("v1.0");
  
  std::filesystem::remove("tag_delete_test.txt");
  
  return exists_before && !exists_after;
}

bool test_tag_show()
{
  std::ofstream file("tag_show_test.txt");
  file << "content for tag show" << std::endl;
  file.close();
  
  stage("tag_show_test.txt");
  commit_changes("test_user", "commit for tag show");
  
  tag_create("v1.0", "", true);
  
  tag_details("v1.0");
  
  std::filesystem::remove("tag_show_test.txt");
  tag_delete("v1.0");
  
  return true; 
}

bool test_tag_checkout()
{
  std::ofstream file("tag_checkout_test.txt");
  file << "content for tag checkout" << std::endl;
  file.close();
  
  stage("tag_checkout_test.txt");
  commit_changes("test_user", "commit for tag checkout");
  
  tag_create("v1.0", "", true);
  
  tag_checkout("v1.0");
  
  std::filesystem::remove("tag_checkout_test.txt");
  tag_delete("v1.0");
  
  return true; 
}

bool test_tag_get_commit()
{
  std::ofstream file("tag_commit_test.txt");
  file << "content for tag commit test" << std::endl;
  file.close();
  
  stage("tag_commit_test.txt");
  commit_changes("test_user", "commit for tag commit test");
  
  std::string current_commit = get_current_commit();
  
  tag_create("v1.0", "", true);
  
  std::string tag_commit = get_commit_hash("v1.0");
  
  std::filesystem::remove("tag_commit_test.txt");
  tag_delete("v1.0");
  
  return tag_commit == current_commit;
}
