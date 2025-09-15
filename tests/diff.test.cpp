#include "../include/diff.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include <fstream>
#include <filesystem>

bool test_diff_file_comparison()
{
  // Create two test files with different content
  std::ofstream file1("diff_test1.txt");
  file1 << "line 1\nline 2\nline 3" << std::endl;
  file1.close();
  
  std::ofstream file2("diff_test2.txt");
  file2 << "line 1\nmodified line 2\nline 3" << std::endl;
  file2.close();
  
  // Compare files
  DiffResult result = compare_files("diff_test1.txt", "diff_test2.txt");
  
  // Clean up
  std::filesystem::remove("diff_test1.txt");
  std::filesystem::remove("diff_test2.txt");
  
  return !result.hunks.empty() && !result.is_binary;
}

bool test_diff_identical_files()
{
  // Create two identical files
  std::ofstream file1("diff_identical1.txt");
  file1 << "identical content" << std::endl;
  file1.close();
  
  std::ofstream file2("diff_identical2.txt");
  file2 << "identical content" << std::endl;
  file2.close();
  
  // Compare files
  DiffResult result = compare_files("diff_identical1.txt", "diff_identical2.txt");
  
  // Clean up
  std::filesystem::remove("diff_identical1.txt");
  std::filesystem::remove("diff_identical2.txt");
  
  return result.hunks.empty() && !result.is_binary;
}

bool test_diff_binary_file_detection()
{
  // Create a binary-like file with actual null characters
  std::ofstream file("binary_test.bin", std::ios::binary);
  file << "binary content with null";
  file.put('\0');  // Add actual null character
  file << "characters" << std::endl;
  file.close();
  
  // Test binary detection
  bool is_binary = is_binary_file("binary_test.bin");
  
  // Clean up
  std::filesystem::remove("binary_test.bin");
  
  return is_binary;
}

bool test_diff_staged_changes()
{
  // Create a test file
  std::ofstream file("diff_staged_test.txt");
  file << "original content" << std::endl;
  file.close();
  
  // Stage the file
  stage("diff_staged_test.txt");
  
  // Modify the file
  std::ofstream modified_file("diff_staged_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();
  
  // Test diff staged
  DiffResult result = diff_staged();
  
  // Clean up
  unstage("diff_staged_test.txt");
  std::filesystem::remove("diff_staged_test.txt");
  
  return !result.hunks.empty();
}

bool test_diff_unstaged_changes()
{
  // Create a test file
  std::ofstream file("diff_unstaged_test.txt");
  file << "original content" << std::endl;
  file.close();
  
  // Stage the file
  stage("diff_unstaged_test.txt");
  
  // Modify the file
  std::ofstream modified_file("diff_unstaged_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();
  
  // Test diff unstaged
  DiffResult result = diff_unstaged();
  
  // Clean up
  unstage("diff_unstaged_test.txt");
  std::filesystem::remove("diff_unstaged_test.txt");
  
  return !result.hunks.empty();
}

bool test_file_history()
{
  // Create a test file
  std::ofstream file("history_test.txt");
  file << "test content" << std::endl;
  file.close();
  
  // Stage and commit
  stage("history_test.txt");
  commit_changes("test_user", "test commit for history");
  
  // Test file history
  show_file_history("history_test.txt");
  
  // Clean up
  std::filesystem::remove("history_test.txt");
  
  return true; // If no exception thrown, test passes
}
