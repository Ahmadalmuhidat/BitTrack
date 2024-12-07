#include <filesystem>
#include <algorithm>
#include <gtest/gtest.h>

#include "branch.test.cpp"
#include "stage.test.cpp"
#include "commit.test.cpp"

void prepareTestEnviroment()
{
  // create new repo
  std::cout << "preparing a new repo for testing..." << std::endl;
  system("build/bittrack init");

  // create a test file
  std::cout << "create a test file..." << std::endl;

  std::ofstream test_file("test_file.txt");
  test_file.close();

  // insert content in the test file
  std::ofstream test_content("test_file.txt");
  test_content << "test content" << std::endl;
}

void cleanTestEnviroment()
{
  std::cout << "cleaning testing enviroment" << std::endl;
  std::filesystem::remove_all(".bittrack");
}

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
  prepareTestEnviroment();

  testing::InitGoogleTest(&argc, argv);
  int res = RUN_ALL_TESTS();

  cleanTestEnviroment();

  return 0;
}