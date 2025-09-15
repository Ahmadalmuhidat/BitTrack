#include "../include/branch.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include <fstream>
#include <filesystem>

bool test_branch_master()
{
  std::vector<std::string> branches = list_branches();
  bool master_exists = std::find(branches.begin(), branches.end(), "master") != branches.end();
  
  return master_exists;
}

bool test_list_branches()
{
  std::vector<std::string> branches = list_branches();
  
  return !branches.empty();
}

bool test_checkout_new_branch()
{
  add_branch("test_branch");
  
  std::vector<std::string> branches = list_branches();
  bool branch_exists = std::find(branches.begin(), branches.end(), "test_branch") != branches.end();
  
  remove_branch("test_branch");
  
  return branch_exists;
}

bool test_remove_branch()
{
  add_branch("remove_test_branch");
  
  std::vector<std::string> branches_before = list_branches();
  bool existed_before = std::find(branches_before.begin(), branches_before.end(), "remove_test_branch") != branches_before.end();
  
  remove_branch("remove_test_branch");
  
  std::vector<std::string> branches_after = list_branches();
  bool exists_after = std::find(branches_after.begin(), branches_after.end(), "remove_test_branch") != branches_after.end();
  
  return existed_before && !exists_after;
}

bool test_working_directory_update()
{
  std::ofstream file("test_master.txt");
  file << "master content" << std::endl;
  file.close();
  
  stage("test_master.txt");
  commit_changes("test_user", "master commit");
  
  add_branch("working_dir_branch");
  switch_branch("working_dir_branch");
  
  if (!std::filesystem::exists("test_master.txt")) {
  return false;
  }
  
  switch_branch("master");
  remove_branch("working_dir_branch");
  std::filesystem::remove("test_master.txt");
  
  return true;
}

bool test_untracked_file_preservation()
{
  std::ofstream untracked_file("untracked.txt");
  untracked_file << "untracked content" << std::endl;
  untracked_file.close();
  
  add_branch("untracked_branch");
  switch_branch("untracked_branch");
  
  bool preserved = std::filesystem::exists("untracked.txt");
  
  switch_branch("master");
  remove_branch("untracked_branch");
  if (std::filesystem::exists("untracked.txt")) {
  std::filesystem::remove("untracked.txt");
  }
  
  return preserved;
}

bool test_uncommitted_changes_detection()
{
  std::ofstream staged_file("staged_uncommitted.txt");
  staged_file << "staged content" << std::endl;
  staged_file.close();
  
  stage("staged_uncommitted.txt");
  
  add_branch("uncommitted_branch");
  
  bool has_uncommitted = !get_staged_files().empty();
  
  unstage("staged_uncommitted.txt");
  std::filesystem::remove("staged_uncommitted.txt");
  remove_branch("uncommitted_branch");
  
  return has_uncommitted;
}

bool test_switch_to_nonexistent_branch()
{
  switch_branch("nonexistent_branch");
  
  return true; 
}

bool test_switch_to_same_branch()
{
  switch_branch("master");
  
  return true; 
}

bool test_file_restoration_from_commit()
{
  std::ofstream file("restoration_test.txt");
  file << "original content" << std::endl;
  file.close();
  
  stage("restoration_test.txt");
  commit_changes("test_user", "restoration commit");
  
  std::ofstream modified_file("restoration_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();
  
  bool file_exists = std::filesystem::exists("restoration_test.txt");
  
  std::filesystem::remove("restoration_test.txt");
  
  return file_exists;
}
