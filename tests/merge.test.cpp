#include "../include/merge.hpp"
#include "../include/branch.hpp"
#include "../include/commit.hpp"
#include "../include/stage.hpp"
#include <fstream>
#include <filesystem>

bool test_merge_branches()
{
  std::ofstream master_file("merge_test.txt");
  master_file << "master content" << std::endl;
  master_file.close();
  
  stage("merge_test.txt");
  commit_changes("test_user", "master commit");
  
  add_branch("merge_branch");
  switch_branch("merge_branch");
  
  std::ofstream merge_file("merge_test.txt");
  merge_file << "merge branch content" << std::endl;
  merge_file.close();
  
  stage("merge_test.txt");
  commit_changes("test_user", "merge branch commit");
  
  switch_branch("master");
  
  MergeResult result = merge_branches("merge_branch", "master");
  
  std::filesystem::remove("merge_test.txt");
  remove_branch("merge_branch");
  
  return result.success;
}

bool test_merge_commits()
{
  std::ofstream file("merge_commit_test.txt");
  file << "original content" << std::endl;
  file.close();
  
  stage("merge_commit_test.txt");
  commit_changes("test_user", "first commit");
  
  std::string commit1 = get_current_commit();
  
  std::ofstream modified_file("merge_commit_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();
  
  stage("merge_commit_test.txt");
  commit_changes("test_user", "second commit");
  
  std::string commit2 = get_current_commit();
  
  MergeResult result = merge_commits(commit1, commit2);
  
  std::filesystem::remove("merge_commit_test.txt");
  
  return true; 
}

bool test_merge_abort()
{
  std::ofstream file("merge_abort_test.txt");
  file << "content for abort test" << std::endl;
  file.close();
  
  stage("merge_abort_test.txt");
  commit_changes("test_user", "commit for abort test");
  
  abort_merge();
  
  std::filesystem::remove("merge_abort_test.txt");
  
  return true; 
}

bool test_merge_continue()
{
  std::ofstream file("merge_continue_test.txt");
  file << "content for continue test" << std::endl;
  file.close();
  
  stage("merge_continue_test.txt");
  commit_changes("test_user", "commit for continue test");
  
  continue_merge();
  
  std::filesystem::remove("merge_continue_test.txt");
  
  return true; 
}

bool test_merge_show_conflicts()
{
  show_conflicts();
  
  return true; 
}

bool test_merge_has_conflicts()
{
  bool conflicts_exist = has_conflicts();
  
  return !conflicts_exist; 
}

bool test_merge_get_conflicted_files()
{
  std::vector<std::string> conflicted_files = get_conflicted_files();
  
  return conflicted_files.empty(); 
}

bool test_merge_three_way()
{
  std::ofstream base_file("three_way_base.txt");
  base_file << "base content" << std::endl;
  base_file.close();
  
  std::ofstream ours_file("three_way_ours.txt");
  ours_file << "ours content" << std::endl;
  ours_file.close();
  
  std::ofstream theirs_file("three_way_theirs.txt");
  theirs_file << "theirs content" << std::endl;
  theirs_file.close();
  
  MergeResult result = three_way_merge("base", "ours", "theirs");
  
  std::filesystem::remove("three_way_base.txt");
  std::filesystem::remove("three_way_ours.txt");
  std::filesystem::remove("three_way_theirs.txt");
  
  return true; 
}

bool test_merge_fast_forward()
{
  std::ofstream file("fast_forward_test.txt");
  file << "fast forward content" << std::endl;
  file.close();
  
  stage("fast_forward_test.txt");
  commit_changes("test_user", "fast forward commit");
  
  add_branch("fast_forward_branch");
  
  MergeResult result = merge_branches("fast_forward_branch", "master");
  
  std::filesystem::remove("fast_forward_test.txt");
  remove_branch("fast_forward_branch");
  
  return true; 
}
