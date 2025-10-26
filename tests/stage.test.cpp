#include "../include/stage.hpp"
#include <fstream>
#include <filesystem>

// stage a file and verify it's in the staged files list
bool test_staged_files()
{
  std::ofstream file("stage_test.txt");
  file << "content for staging" << std::endl;
  file.close();

  stage("stage_test.txt");

  std::vector<std::string> staged_files = get_staged_files();
  bool is_staged = std::find(staged_files.begin(), staged_files.end(), "stage_test.txt") != staged_files.end();

  unstage("stage_test.txt");
  std::filesystem::remove("stage_test.txt");

  return is_staged;
}

// unstage a file and verify it's no longer in the staged files list
bool test_unstaged_files()
{
  std::ofstream file("unstaged_test.txt");
  file << "content for unstaging" << std::endl;
  file.close();

  stage("unstaged_test.txt");

  unstage("unstaged_test.txt");

  std::vector<std::string> staged_files = get_staged_files();
  bool is_unstaged = std::find(staged_files.begin(), staged_files.end(), "unstaged_test.txt") == staged_files.end();

  std::filesystem::remove("unstaged_test.txt");

  return is_unstaged;
}

// stage a file, then unstage it, and verify the transitions
bool test_unstage_staged()
{
  std::ofstream file("unstage_staged_test.txt");
  file << "content for unstage staged test" << std::endl;
  file.close();

  stage("unstage_staged_test.txt");

  std::vector<std::string> staged_before = get_staged_files();
  bool was_staged = std::find(staged_before.begin(), staged_before.end(), "unstage_staged_test.txt") != staged_before.end();

  unstage("unstage_staged_test.txt");

  std::vector<std::string> staged_after = get_staged_files();
  bool is_unstaged = std::find(staged_after.begin(), staged_after.end(), "unstage_staged_test.txt") == staged_after.end();

  std::filesystem::remove("unstage_staged_test.txt");

  return was_staged && is_unstaged;
}
