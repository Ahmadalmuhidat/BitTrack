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

// merge