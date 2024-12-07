#include "../src/commit.cpp"

bool testCommitStagedFiles()
{
  stage("test_file.txt");
  commitChanges("almuhidat", "test commit");

  std::string commit_path = ".bittrack/objects/"  + getCurrentBranch() + "/" + getCurrentCommit() + "/test_file.txt";
  bool file_exists = std::filesystem::exists(commit_path);

  return file_exists; 
}