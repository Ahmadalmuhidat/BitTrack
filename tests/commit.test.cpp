#include "../include/commit.hpp"
#include "../include/stage.hpp"
#include <fstream>
#include <filesystem>

// commit a staged file and verify it's no longer staged
bool test_commit_staged_files()
{
  std::ofstream file("commit_test.txt");
  file << "content for commit test" << std::endl;
  file.close();

  stage("commit_test.txt");

  commitChanges("test_user", "test commit message");

  std::vector<std::string> staged_files = getStagedFiles();
  bool is_committed = std::find(staged_files.begin(), staged_files.end(), "commit_test.txt") == staged_files.end();

  std::filesystem::remove("commit_test.txt");

  return is_committed;
}
