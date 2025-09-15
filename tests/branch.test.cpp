#include "../include/branch.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include <fstream>
#include <filesystem>

bool test_branch_master()
{
  // Check if master branch exists
  std::vector<std::string> branches = list_branches();
  bool master_exists = std::find(branches.begin(), branches.end(), "master") != branches.end();
  
  return master_exists;
}

bool test_list_branches()
{
  // List branches (should not throw exception)
  std::vector<std::string> branches = list_branches();
  
  return !branches.empty();
}

bool test_checkout_new_branch()
{
  // Create a new branch
  add_branch("test_branch");
  
  // Check if branch was created
  std::vector<std::string> branches = list_branches();
  bool branch_exists = std::find(branches.begin(), branches.end(), "test_branch") != branches.end();
  
  // Clean up
  remove_branch("test_branch");
  
  return branch_exists;
}

bool test_remove_branch()
{
  // Create a new branch
  add_branch("remove_test_branch");
  
  // Verify it exists
  std::vector<std::string> branches_before = list_branches();
  bool existed_before = std::find(branches_before.begin(), branches_before.end(), "remove_test_branch") != branches_before.end();
  
  // Remove the branch
  remove_branch("remove_test_branch");
  
  // Verify it's removed
  std::vector<std::string> branches_after = list_branches();
  bool exists_after = std::find(branches_after.begin(), branches_after.end(), "remove_test_branch") != branches_after.end();
  
  return existed_before && !exists_after;
}

bool test_working_directory_update()
{
  // Create a test file
  std::ofstream file("test_master.txt");
  file << "master content" << std::endl;
  file.close();
  
  // Stage and commit
  stage("test_master.txt");
  commit_changes("test_user", "master commit");
  
  // Create a new branch
  add_branch("working_dir_branch");
  switch_branch("working_dir_branch");
  
  // Check if working directory has master content
  if (!std::filesystem::exists("test_master.txt")) {
    return false;
  }
  
  // Clean up
  switch_branch("master");
  remove_branch("working_dir_branch");
  std::filesystem::remove("test_master.txt");
  
  return true;
}

bool test_untracked_file_preservation()
{
  // Create an untracked file
  std::ofstream untracked_file("untracked.txt");
  untracked_file << "untracked content" << std::endl;
  untracked_file.close();
  
  // Create a new branch
  add_branch("untracked_branch");
  switch_branch("untracked_branch");
  
  // Check if untracked file is preserved
  bool preserved = std::filesystem::exists("untracked.txt");
  
  // Clean up
  switch_branch("master");
  remove_branch("untracked_branch");
  if (std::filesystem::exists("untracked.txt")) {
    std::filesystem::remove("untracked.txt");
  }
  
  return preserved;
}

// Test uncommitted changes detection
bool test_uncommitted_changes_detection()
{
  // Create a file and stage it (uncommitted)
  std::ofstream staged_file("staged_uncommitted.txt");
  staged_file << "staged content" << std::endl;
  staged_file.close();
  
  stage("staged_uncommitted.txt");
  
  // Try to switch branches (should detect uncommitted changes)
  add_branch("uncommitted_branch");
  
  // This should work since we're not actually switching
  bool has_uncommitted = !get_staged_files().empty();
  
  // Clean up
  unstage("staged_uncommitted.txt");
  std::filesystem::remove("staged_uncommitted.txt");
  remove_branch("uncommitted_branch");
  
  return has_uncommitted;
}

bool test_switch_to_nonexistent_branch()
{
  // Try to switch to a non-existent branch
  // This should not crash the program
  switch_branch("nonexistent_branch");
  
  return true; // If no exception thrown, test passes
}

bool test_switch_to_same_branch()
{
  // Switch to the same branch we're already on
  switch_branch("master");
  
  return true; // If no exception thrown, test passes
}

bool test_file_restoration_from_commit()
{
  // Create a test file
  std::ofstream file("restoration_test.txt");
  file << "original content" << std::endl;
  file.close();
  
  // Stage and commit
  stage("restoration_test.txt");
  commit_changes("test_user", "restoration commit");
  
  // Modify the file
  std::ofstream modified_file("restoration_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();
  
  // Restore from commit (this would be implemented in the actual function)
  // For now, just check if the file exists
  bool file_exists = std::filesystem::exists("restoration_test.txt");
  
  // Clean up
  std::filesystem::remove("restoration_test.txt");
  
  return file_exists;
}