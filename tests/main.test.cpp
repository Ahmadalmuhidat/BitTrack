#include <filesystem>
#include <algorithm>
#include <gtest/gtest.h>

#include "branch.test.cpp"
#include "stage.test.cpp"
#include "commit.test.cpp"

void prepare_test_enviroment()
{
  // Clean up any existing test files
  system("rm -f test_file.txt test_master.txt untracked.txt staged_uncommitted.txt restoration_test.txt");
  system("rm -rf .bittrack");
  
  // create new repo
  std::cout << "preparing a new repo for testing..." << std::endl;
  system("build/bittrack init");

  // create a test file
  std::cout << "create a test file..." << std::endl;

  std::ofstream test_file("test_file.txt");
  test_file << "test content" << std::endl;
  test_file.close();
}

TEST(t01_branch, valid_current_branch_is_master_test)
{
  EXPECT_TRUE(test_branch_master());
}

TEST(t02_branch, add_new_branch_test)
{
  EXPECT_TRUE(test_add_new_branch());
}

TEST(t03_branch, checkout_to_new_branch_test)
{
  EXPECT_TRUE(test_checkout_to_new_branch());
}

TEST(t04_stage, stage_file_test)
{
  EXPECT_TRUE(test_staged_files());
}

TEST(t05_ignore, ignored_file_test)
{
  EXPECT_FALSE(test_ignored_files());
}

TEST(t06_stage, unstage_stage_test)
{
  EXPECT_TRUE(test_unstaged_files());
}

TEST(commit_tests, commit_staged_file_test)
{
  EXPECT_TRUE(test_commit_staged_files());
}

TEST(t07_branch, remove_new_branch_test)
{
  EXPECT_TRUE(test_remove_new_branch());
}

TEST(t08_branch, working_directory_update_test)
{
  EXPECT_TRUE(test_working_directory_update());
}

TEST(t09_branch, untracked_file_preservation_test)
{
  EXPECT_TRUE(test_untracked_file_preservation());
}

TEST(t10_branch, uncommitted_changes_detection_test)
{
  EXPECT_TRUE(test_uncommitted_changes_detection());
}

TEST(t11_branch, switch_to_nonexistent_branch_test)
{
  EXPECT_TRUE(test_switch_to_nonexistent_branch());
}

TEST(t12_branch, switch_to_same_branch_test)
{
  EXPECT_TRUE(test_switch_to_same_branch());
}

TEST(t13_branch, file_restoration_from_commit_test)
{
  EXPECT_TRUE(test_file_restoration_from_commit());
}

int main(int argc, char **argv)
{
  prepare_test_enviroment();

  testing::InitGoogleTest(&argc, argv);
  int res = RUN_ALL_TESTS();
  return 0;
}