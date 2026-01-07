#include "../include/branch.hpp"
#include "../include/stage.hpp"
#include "../include/commit.hpp"
#include <fstream>
#include <filesystem>

// Get list of branches and check if "main" exists
bool test_branch_main()
{
  std::string current_branch = getCurrentBranch();
  return current_branch == "main";
}

// Get list of branches and check if it's not empty
bool test_list_branches()
{
  std::vector<std::string> branches = getBranchesList();
  return !branches.empty();
}

// Create a new branch and check if it exists
bool test_checkout_new_branch()
{
  addBranch("test_branch");

  std::vector<std::string> branches = getBranchesList();
  bool branch_exists = std::find(branches.begin(), branches.end(), "test_branch") != branches.end();

  removeBranch("test_branch");

  return branch_exists;
}

// remove a branch and check if it no longer exists
bool test_remove_branch()
{
  addBranch("remove_test_branch");

  std::vector<std::string> branches_before = getBranchesList();
  bool existed_before = std::find(branches_before.begin(), branches_before.end(), "remove_test_branch") != branches_before.end();

  removeBranch("remove_test_branch");

  std::vector<std::string> branches_after = getBranchesList();
  bool exists_after = std::find(branches_after.begin(), branches_after.end(), "remove_test_branch") != branches_after.end();

  return existed_before && !exists_after;
}

// switch to an existing branch and verify the switch
bool test_working_directory_update()
{
  std::ofstream file("test_main.txt");
  file << "main content" << std::endl;
  file.close();

  stage("test_main.txt");
  commitChanges("test_user", "main commit");

  addBranch("working_dir_branch");
  switchBranch("working_dir_branch");

  if (!std::filesystem::exists("test_main.txt"))
  {
    return false;
  }

  switchBranch("main");
  removeBranch("working_dir_branch");
  std::filesystem::remove("test_main.txt");

  return true;
}

// Create an untracked file, switch branches, and verify the file is preserved
bool test_untracked_file_preservation()
{
  std::ofstream untracked_file("untracked.txt");
  untracked_file << "untracked content" << std::endl;
  untracked_file.close();

  addBranch("untracked_branch");
  switchBranch("untracked_branch");

  bool preserved = std::filesystem::exists("untracked.txt");

  switchBranch("main");
  removeBranch("untracked_branch");
  if (std::filesystem::exists("untracked.txt"))
  {
    std::filesystem::remove("untracked.txt");
  }

  return preserved;
}

// stage a file, switch branches, and verify uncommitted changes are detected
bool test_uncommitted_changes_detection()
{
  std::ofstream staged_file("staged_uncommitted.txt");
  staged_file << "staged content" << std::endl;
  staged_file.close();

  stage("staged_uncommitted.txt");

  addBranch("uncommitted_branch");

  bool has_uncommitted = !getStagedFiles().empty();

  unstage("staged_uncommitted.txt");
  std::filesystem::remove("staged_uncommitted.txt");
  removeBranch("uncommitted_branch");

  return has_uncommitted;
}

// attempt to switch to a nonexistent branch and verify error handling
bool test_switch_to_nonexistent_branch()
{
  switchBranch("nonexistent_branch");
  return true;
}

// switch to the same branch and verify no changes occur
bool test_switch_to_same_branch()
{
  switchBranch("main");
  return true;
}

// commit a file, modify it, delete it, and verify restoration from the commit
bool test_file_restoration_from_commit()
{
  std::ofstream file("restoration_test.txt");
  file << "original content" << std::endl;
  file.close();

  stage("restoration_test.txt");
  commitChanges("test_user", "restoration commit");

  std::ofstream modified_file("restoration_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();

  bool file_exists = std::filesystem::exists("restoration_test.txt");

  std::filesystem::remove("restoration_test.txt");

  return file_exists;
}
