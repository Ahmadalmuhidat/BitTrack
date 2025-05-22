#include "../src/commit.cpp"

bool test_commit_staged_files()
{
  stage("test_file.txt");
  commit_changes("almuhidat", "test commit");

  std::string commit_path = ".bittrack/objects/"  + get_current_branch() + "/" + get_current_commit() + "/test_file.txt";
  bool file_exists = std::filesystem::exists(commit_path);
  return file_exists; 
}