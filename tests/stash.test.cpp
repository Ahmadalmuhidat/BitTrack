#include "../include/stash.hpp"
#include "../include/stage.hpp"
#include <fstream>
#include <filesystem>

bool test_stash_creation()
{
  // Create a test file
  std::ofstream file("stash_test.txt");
  file << "content to stash" << std::endl;
  file.close();
  
  // Stage the file
  stage("stash_test.txt");
  
  // Create stash
  stash_changes("Test stash message");
  
  // Check if stash was created
  std::vector<StashEntry> stashes = get_stash_entries();
  
  // Clean up
  std::filesystem::remove("stash_test.txt");
  
  return !stashes.empty();
}

bool test_stash_listing()
{
  // Create multiple stashes
  std::ofstream file1("stash_test1.txt");
  file1 << "content 1" << std::endl;
  file1.close();
  stage("stash_test1.txt");
  stash_changes("First stash");
  
  std::ofstream file2("stash_test2.txt");
  file2 << "content 2" << std::endl;
  file2.close();
  stage("stash_test2.txt");
  stash_changes("Second stash");
  
  // List stashes
  std::vector<StashEntry> stashes = get_stash_entries();
  
  // Clean up
  std::filesystem::remove("stash_test1.txt");
  std::filesystem::remove("stash_test2.txt");
  
  return stashes.size() >= 2;
}

bool test_stash_apply()
{
  // Create a test file
  std::ofstream file("stash_apply_test.txt");
  file << "original content" << std::endl;
  file.close();
  
  // Stage and stash
  stage("stash_apply_test.txt");
  stash_changes("Test stash for apply");
  
  // Modify the file
  std::ofstream modified_file("stash_apply_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();
  
  // Apply stash
  stash_apply("0");
  
  // Check if stash was applied (file should be restored)
  std::ifstream restored_file("stash_apply_test.txt");
  std::string content;
  std::getline(restored_file, content);
  restored_file.close();
  
  // Clean up
  std::filesystem::remove("stash_apply_test.txt");
  
  return content == "original content";
}

bool test_stash_pop()
{
  // Create a test file
  std::ofstream file("stash_pop_test.txt");
  file << "content to pop" << std::endl;
  file.close();
  
  // Stage and stash
  stage("stash_pop_test.txt");
  stash_changes("Test stash for pop");
  
  // Get initial stash count
  std::vector<StashEntry> initial_stashes = get_stash_entries();
  size_t initial_count = initial_stashes.size();
  
  // Pop stash
  stash_pop("0");
  
  // Get final stash count
  std::vector<StashEntry> final_stashes = get_stash_entries();
  size_t final_count = final_stashes.size();
  
  // Clean up
  std::filesystem::remove("stash_pop_test.txt");
  
  return final_count < initial_count;
}

bool test_stash_drop()
{
  // Create a test file
  std::ofstream file("stash_drop_test.txt");
  file << "content to drop" << std::endl;
  file.close();
  
  // Stage and stash
  stage("stash_drop_test.txt");
  stash_changes("Test stash for drop");
  
  // Get initial stash count
  std::vector<StashEntry> initial_stashes = get_stash_entries();
  size_t initial_count = initial_stashes.size();
  
  // Drop stash
  stash_drop("0");
  
  // Get final stash count
  std::vector<StashEntry> final_stashes = get_stash_entries();
  size_t final_count = final_stashes.size();
  
  // Clean up
  std::filesystem::remove("stash_drop_test.txt");
  
  return final_count < initial_count;
}

bool test_stash_clear()
{
  // Create multiple stashes
  std::ofstream file1("stash_clear_test1.txt");
  file1 << "content 1" << std::endl;
  file1.close();
  stage("stash_clear_test1.txt");
  stash_changes("First stash");
  
  std::ofstream file2("stash_clear_test2.txt");
  file2 << "content 2" << std::endl;
  file2.close();
  stage("stash_clear_test2.txt");
  stash_changes("Second stash");
  
  // Clear all stashes
  stash_clear();
  
  // Check if all stashes were cleared
  std::vector<StashEntry> stashes = get_stash_entries();
  
  // Clean up
  std::filesystem::remove("stash_clear_test1.txt");
  std::filesystem::remove("stash_clear_test2.txt");
  
  return stashes.empty();
}

bool test_stash_has_stashes()
{
  // Initially should have no stashes
  bool initially_empty = !stash_has_stashes();
  
  // Create a stash
  std::ofstream file("stash_has_test.txt");
  file << "content" << std::endl;
  file.close();
  stage("stash_has_test.txt");
  stash_changes("Test stash");
  
  // Should now have stashes
  bool has_stashes = stash_has_stashes();
  
  // Clean up
  std::filesystem::remove("stash_has_test.txt");
  stash_clear();
  
  return initially_empty && has_stashes;
}
