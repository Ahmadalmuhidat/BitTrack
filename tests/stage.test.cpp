#include "../src/stage.cpp"

bool test_staged_files()
{
  stage("test_file.txt");

  std::vector<std::string> staged_files = get_staged_files();
  auto it = std::find(
    staged_files.begin(),
    staged_files.end(),
    "test_file.txt"
  );
  return (it != staged_files.end());
}

bool test_unstaged_files()
{
  unstage("test_file.txt");

  std::vector<std::string> staged_files = get_unstaged_files();
  auto it = std::find(
    staged_files.begin(),
    staged_files.end(),
    "test_file.txt"
  );
  return (it != staged_files.end());
}

bool test_ignored_files()
{
  std::vector<std::string> staged_files = get_staged_files();
  auto it = std::find(
    staged_files.begin(),
    staged_files.end(),
    "test_ignore_file.txt"
  );
  return it != staged_files.end();
}