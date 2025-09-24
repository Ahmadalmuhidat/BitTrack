#include "../include/diff.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include <fstream>
#include <filesystem>

bool test_diff_file_comparison()
{
  std::ofstream file1("diff_test1.txt");
  file1 << "line 1\nline 2\nline 3" << std::endl;
  file1.close();

  std::ofstream file2("diff_test2.txt");
  file2 << "line 1\nmodified line 2\nline 3" << std::endl;
  file2.close();

  DiffResult result = compare_files("diff_test1.txt", "diff_test2.txt");

  std::filesystem::remove("diff_test1.txt");
  std::filesystem::remove("diff_test2.txt");

  return !result.hunks.empty() && !result.is_binary;
}

bool test_diff_identical_files()
{
  std::ofstream file1("diff_identical1.txt");
  file1 << "identical content" << std::endl;
  file1.close();

  std::ofstream file2("diff_identical2.txt");
  file2 << "identical content" << std::endl;
  file2.close();

  DiffResult result = compare_files("diff_identical1.txt", "diff_identical2.txt");

  std::filesystem::remove("diff_identical1.txt");
  std::filesystem::remove("diff_identical2.txt");

  return result.hunks.empty() && !result.is_binary;
}

bool test_diff_binary_file_detection()
{
  std::ofstream file("binary_test.bin", std::ios::binary);
  file << "binary content with null";
  file.put('\0');
  file << "characters" << std::endl;
  file.close();

  bool is_binary = is_binary_file("binary_test.bin");

  std::filesystem::remove("binary_test.bin");

  return is_binary;
}

bool test_diff_staged_changes()
{
  std::ofstream file("diff_staged_test.txt");
  file << "original content" << std::endl;
  file.close();

  stage("diff_staged_test.txt");

  std::ofstream modified_file("diff_staged_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();

  DiffResult result = diff_staged();

  unstage("diff_staged_test.txt");
  std::filesystem::remove("diff_staged_test.txt");

  return !result.hunks.empty();
}

bool test_diff_unstaged_changes()
{
  std::ofstream file("diff_unstaged_test.txt");
  file << "original content" << std::endl;
  file.close();

  stage("diff_unstaged_test.txt");

  std::ofstream modified_file("diff_unstaged_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();

  DiffResult result = diff_unstaged();

  unstage("diff_unstaged_test.txt");
  std::filesystem::remove("diff_unstaged_test.txt");

  return !result.hunks.empty();
}

bool test_file_history()
{
  std::ofstream file("history_test.txt");
  file << "test content" << std::endl;
  file.close();

  stage("history_test.txt");
  commit_changes("test_user", "test commit for history");

  show_file_history("history_test.txt");

  std::filesystem::remove("history_test.txt");

  return true;
}
