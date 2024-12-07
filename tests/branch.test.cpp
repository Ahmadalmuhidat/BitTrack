#include "../src/branch.cpp"

bool testBranchMaster()
{
  std::string current_branch = getCurrentBranch();
  return (current_branch == "master");
}