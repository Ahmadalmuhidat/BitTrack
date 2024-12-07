#include "../src/stage.cpp"

bool testStagedFiles()
{
  stage("test_file.txt");

  std::vector<std::string> StagedFiles = getStagedFiles();
  auto it = std::find(
    StagedFiles.begin(),
    StagedFiles.end(),
    "test_file.txt"
  );

  return (it != StagedFiles.end());
}

bool testUnstageFiles()
{
  unstage("test_file.txt");

  std::vector<std::string> StagedFiles = getUnstagedFiles();
  auto it = std::find(
    StagedFiles.begin(),
    StagedFiles.end(),
    "test_file.txt"
  );

  return (it != StagedFiles.end());
}

bool testIgnoreFiles()
{
  std::vector<std::string> StagedFiles = getStagedFiles();
  auto it = std::find(
    StagedFiles.begin(),
    StagedFiles.end(),
    "test_ignore_file.txt"
  );

  return it != StagedFiles.end();
}