#include "../include/maintenance.hpp"

// test garbage collection
bool test_maintenance_garbage_collect()
{
  garbageCollect();
  return true;
}

// test repacking repository
bool test_maintenance_repack()
{
  repackRepository();
  return true;
}

// test pruning objects
bool test_maintenance_prune()
{
  pruneObjects();
  return true;
}

// test filesystem check
bool test_maintenance_fsck()
{
  fsckRepository();
  return true;
}

// test calculating repository statistics
bool test_maintenance_stats()
{
  RepoStats stats = calculateRepositoryStats();
  return stats.commit_count >= 0 && stats.branch_count >= 0;
}

// test showing repository information
bool test_maintenance_show_info()
{
  showRepositoryInfo();
  return true;
}

// test analyzing repository
bool test_maintenance_analyze()
{
  analyzeRepository();
  return true;
}

// test finding large files
bool test_maintenance_find_large_files()
{
  findLargeFiles(1);
  return true;
}

// test finding duplicate files
bool test_maintenance_find_duplicates()
{
  findDuplicateFiles();
  return true;
}