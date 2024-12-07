#include <filesystem>
#include <algorithm>
#include <gtest/gtest.h>

#include "branch.test.cpp"
#include "stage.test.cpp"
#include "commit.test.cpp"

TEST(branch_tests, valid_current_branch_test)
{
  EXPECT_TRUE(testBranchMaster());
}

TEST(staging_tests, stage_file_test)
{
  EXPECT_TRUE(testStagedFiles());
}

TEST(ignore_tests, ignored_file_test)
{
  EXPECT_FALSE(testIgnoreFiles());
}

TEST(staging_tests, unstage_stage_test)
{
  EXPECT_TRUE(testUnstageFiles());
}

TEST(commit_tests, commit_staged_file_test)
{
  EXPECT_TRUE(testCommitStagedFiles());
}

int main(int argc, char **argv)
{
  // create new repo
  system("./bittrack init");

  // create test file
  std::ofstream test_file("test_file.txt");
  test_file.close();

  // insert content in file
  std::ofstream test_content("test_file.txt");
  test_content << "test content" << std::endl;

  // start testing
  testing::InitGoogleTest(&argc, argv);
  int res = RUN_ALL_TESTS();

  std::filesystem::remove_all(".bittrack");
  std::cout << "Repository removed." << std::endl;

  return 0;
}
