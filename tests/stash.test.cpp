#include "../include/stash.hpp"
#include "../include/stage.hpp"
#include <fstream>
#include <filesystem>

// create a stash and verify it exists
bool test_stash_creation()
{
  std::ofstream file("stash_test.txt");
  file << "content to stash" << std::endl;
  file.close();

  stage("stash_test.txt");

  stash_changes("Test stash message");

  std::vector<StashEntry> stashes = get_stash_entries();

  std::filesystem::remove("stash_test.txt");

  return !stashes.empty();
}

// list stashes and verify at least two exist
bool test_stash_listing()
{
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

  std::vector<StashEntry> stashes = get_stash_entries();

  std::filesystem::remove("stash_test1.txt");
  std::filesystem::remove("stash_test2.txt");

  return stashes.size() >= 2;
}

// apply a stash and verify the working directory is restored
bool test_stash_apply()
{
  std::ofstream file("stash_apply_test.txt");
  file << "original content" << std::endl;
  file.close();

  stage("stash_apply_test.txt");
  stash_changes("Test stash for apply");

  std::ofstream modified_file("stash_apply_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();

  stash_apply("0");

  std::ifstream restored_file("stash_apply_test.txt");
  std::string content;
  std::getline(restored_file, content);
  restored_file.close();

  std::filesystem::remove("stash_apply_test.txt");

  return content == "original content";
}

// pop a stash and verify it is removed from the stash list
bool test_stash_pop()
{
  std::ofstream file("stash_pop_test.txt");
  file << "content to pop" << std::endl;
  file.close();

  stage("stash_pop_test.txt");
  stash_changes("Test stash for pop");

  std::vector<StashEntry> initial_stashes = get_stash_entries();
  size_t initial_count = initial_stashes.size();

  stash_pop("0");

  std::vector<StashEntry> final_stashes = get_stash_entries();
  size_t final_count = final_stashes.size();

  std::filesystem::remove("stash_pop_test.txt");

  return final_count < initial_count;
}

// drop a stash and verify it is removed from the stash list
bool test_stash_drop()
{
  std::ofstream file("stash_drop_test.txt");
  file << "content to drop" << std::endl;
  file.close();

  stage("stash_drop_test.txt");
  stash_changes("Test stash for drop");

  std::vector<StashEntry> initial_stashes = get_stash_entries();
  size_t initial_count = initial_stashes.size();

  stash_drop("0");

  std::vector<StashEntry> final_stashes = get_stash_entries();
  size_t final_count = final_stashes.size();

  std::filesystem::remove("stash_drop_test.txt");

  return final_count < initial_count;
}

// clear all stashes and verify the stash list is empty
bool test_stash_clear()
{
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

  stash_clear();

  std::vector<StashEntry> stashes = get_stash_entries();

  std::filesystem::remove("stash_clear_test1.txt");
  std::filesystem::remove("stash_clear_test2.txt");

  return stashes.empty();
}

// check if stashes exist and verify the result
bool test_stash_has_stashes()
{
  bool initially_empty = !stash_has_stashes();

  std::ofstream file("stash_has_test.txt");
  file << "content" << std::endl;
  file.close();
  stage("stash_has_test.txt");
  stash_changes("Test stash");

  bool has_stashes = stash_has_stashes();

  std::filesystem::remove("stash_has_test.txt");
  stash_clear();

  return initially_empty && has_stashes;
}
