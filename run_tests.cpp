#include <filesystem>
#include <algorithm>
#include <gtest/gtest.h>

#include "branch.cpp"
#include "stage.cpp"
#include "commit.cpp"

TEST(branch_tests, valid_current_branch_test)
{
  std::string current_branch = getCurrentBranch();
  EXPECT_TRUE(current_branch == "master");
}

TEST(staging_tests, stage_file_test)
{
  stage("test_file.txt");

  std::vector<std::string> StagedFiles = getStagedFiles();
  auto it = std::find(
    StagedFiles.begin(),
    StagedFiles.end(),
    "test_file.txt"
  );

  EXPECT_TRUE(it != StagedFiles.end());
}

TEST(ignore_tests, ignored_file_test)
{
  std::vector<std::string> StagedFiles = getStagedFiles();
  auto it = std::find(
    StagedFiles.begin(),
    StagedFiles.end(),
    "test_ignore_file.txt"
  );

  EXPECT_FALSE(it != StagedFiles.end());
}

TEST(staging_tests, unstage_stage_test)
{
  unstage("test_file.txt");

  std::vector<std::string> StagedFiles = getUnstagedFiles();
  auto it = std::find(
    StagedFiles.begin(),
    StagedFiles.end(),
    "test_file.txt"
  );

  EXPECT_TRUE(it != StagedFiles.end());
}

TEST(commit_tests, commit_staged_file_test)
{
  stage("test_file.txt");
  commitChanges("almuhidat", "test commit");

  std::string commit_path = ".bittrack/objects/"  + getCurrentBranch() + "/" + getCurrentCommit() + "/test_file.txt";
  bool file_exists = std::filesystem::exists(commit_path);

  EXPECT_TRUE(file_exists);
}


int main(int argc, char **argv)
{
  std::ofstream test_file("test_file.txt");
  test_file.close();

  std::ofstream test_content("test_file.txt");
  test_content << "test content" << std::endl;

  testing::InitGoogleTest(&argc, argv);
  int res = RUN_ALL_TESTS();

  return 0;
}
