#include "../include/maintenance.hpp"

// test garbage collection
bool test_maintenance_garbage_collect()
{
  garbage_collect();
  return true;
}

// test repacking repository
bool test_maintenance_repack()
{
  repack_repository();
  return true;
}

// test pruning objects
bool test_maintenance_prune()
{
  prune_objects();
  return true;
}

// test filesystem check
bool test_maintenance_fsck()
{
  fsck_repository();
  return true;
}

// test calculating repository statistics
bool test_maintenance_stats()
{
  RepoStats stats = calculate_repository_stats();
  return stats.commit_count >= 0 && stats.branch_count >= 0;
}

// test showing repository information
bool test_maintenance_show_info()
{
  show_repository_info();
  return true;
}

// test analyzing repository
bool test_maintenance_analyze()
{
  analyze_repository();
  return true;
}

// test finding large files
bool test_maintenance_find_large_files()
{
  find_large_files(1);
  return true;
}

// test finding duplicate files
bool test_maintenance_find_duplicates()
{
  find_duplicate_files();
  return true;
}