#include "../src/branch.cpp"

bool test_branch_master()
{
  std::string current_branch = get_current_branch();
  return (current_branch == "master");
}

bool test_add_new_branch()
{
  add_branch("new_branch");
  bool new_branch_folder_created = std::filesystem::exists(".bittrack/objects/new_branch") && std::filesystem::is_directory(".bittrack/objects/new_branch");
  bool new_branch_file_created = std::filesystem::exists(".bittrack/refs/heads/new_branch");
  return (new_branch_file_created && new_branch_folder_created);
}

bool test_checkout_to_new_branch()
{
  switch_branch("new_branch");
  std::string current_branch = get_current_branch();
  return (current_branch == "new_branch");
}

bool test_remove_new_branch()
{
  remove_branch("new_branch");
  bool new_branch_folder_found = std::filesystem::exists(".bittrack/objects/new_branch");
  bool new_branch_file_found = std::filesystem::exists(".bittrack/refs/heads/new_branch");
  return (new_branch_file_found == false && new_branch_folder_found == false);
}

// Test working directory update on branch switch
bool test_working_directory_update()
{
  // Create a file in master branch
  std::ofstream master_file("test_master.txt");
  master_file << "content in master branch" << std::endl;
  master_file.close();
  
  // Stage and commit the file
  stage("test_master.txt");
  commit_changes("test_user", "commit in master");
  
  // Create a new branch
  add_branch("test_branch");
  
  // Switch to test branch
  switch_branch("test_branch");
  
  // Modify the file in test branch
  std::ofstream test_file("test_master.txt");
  test_file << "content in test branch" << std::endl;
  test_file.close();
  
  // Stage and commit in test branch
  stage("test_master.txt");
  commit_changes("test_user", "commit in test branch");
  
  // Switch back to master
  switch_branch("master");
  
  // Check if working directory has master content
  if (!std::filesystem::exists("test_master.txt")) {
    return false;
  }
  
  std::ifstream file("test_master.txt");
  std::string content;
  std::getline(file, content);
  file.close();
  
  // Clean up
  std::filesystem::remove("test_master.txt");
  
  return content == "content in master branch";
}

// Test untracked file preservation
bool test_untracked_file_preservation()
{
  // Create an untracked file
  std::ofstream untracked_file("untracked.txt");
  untracked_file << "this is untracked" << std::endl;
  untracked_file.close();
  
  // Switch branches
  switch_branch("test_branch");
  switch_branch("master");
  
  // Check if untracked file still exists
  bool file_exists = std::filesystem::exists("untracked.txt");
  
  // Clean up
  if (std::filesystem::exists("untracked.txt")) {
    std::filesystem::remove("untracked.txt");
  }
  
  return file_exists;
}

// Test uncommitted changes detection
bool test_uncommitted_changes_detection()
{
  // Create a file and stage it (uncommitted)
  std::ofstream staged_file("staged_uncommitted.txt");
  staged_file << "staged but not committed" << std::endl;
  staged_file.close();
  
  stage("staged_uncommitted.txt");
  
  // Check if has_uncommitted_changes detects it
  bool has_changes = has_uncommitted_changes();
  
  // Clean up
  unstage("staged_uncommitted.txt");
  std::filesystem::remove("staged_uncommitted.txt");
  
  return has_changes;
}

// Test switching to non-existent branch
bool test_switch_to_nonexistent_branch()
{
  // Try to switch to a non-existent branch
  switch_branch("nonexistent_branch");
  
  // Should still be on current branch (master)
  std::string current_branch = get_current_branch();
  return current_branch == "master";
}

// Test switching to same branch
bool test_switch_to_same_branch()
{
  // Try to switch to current branch
  switch_branch("master");
  
  // Should still be on master
  std::string current_branch = get_current_branch();
  return current_branch == "master";
}

// Test file restoration from commit
bool test_file_restoration_from_commit()
{
  // Create a file in test branch
  std::ofstream test_file("restoration_test.txt");
  test_file << "original content" << std::endl;
  test_file.close();
  
  stage("restoration_test.txt");
  commit_changes("test_user", "test commit");
  
  // Modify the file
  std::ofstream modified_file("restoration_test.txt");
  modified_file << "modified content" << std::endl;
  modified_file.close();
  
  // Switch to another branch and back
  switch_branch("test_branch");
  switch_branch("master");
  
  // Check if file was restored to original content
  std::ifstream file("restoration_test.txt");
  std::string content;
  std::getline(file, content);
  file.close();
  
  // Clean up
  std::filesystem::remove("restoration_test.txt");
  
  return content == "original content";
}

// merge