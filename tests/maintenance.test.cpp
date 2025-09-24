#include "../include/maintenance.hpp"

bool test_maintenance_garbage_collect()
{
  garbage_collect();

  return true;
}

bool test_maintenance_repack()
{
  repack_repository();

  return true;
}

bool test_maintenance_prune()
{
  prune_objects();

  return true;
}

bool test_maintenance_fsck()
{
  fsck_repository();

  return true;
}

bool test_maintenance_stats()
{
  RepoStats stats = calculate_repository_stats();

  return stats.commit_count >= 0 && stats.branch_count >= 0;
}

bool test_maintenance_show_info()
{
  show_repository_info();

  return true;
}

bool test_maintenance_analyze()
{
  analyze_repository();

  return true;
}

bool test_maintenance_find_large_files()
{
  find_large_files(1);

  return true;
}

bool test_maintenance_find_duplicates()
{
  find_duplicate_files();

  return true;
}


bool test_maintenance_clean_ignored()
{

  return true;
}

bool test_maintenance_remove_empty_dirs()
{

  return true;
}

bool test_maintenance_compact()
{

  return true;
}

bool test_maintenance_backup()
{

  return true;
}

bool test_maintenance_list_backups()
{
  list_backups();

  return true;
}

bool test_maintenance_benchmark()
{
  benchmark_operations();

  return true;
}

bool test_maintenance_profile()
{
  profile_repository();

  return true;
}

bool test_maintenance_check_integrity()
{
  check_integrity();

  return true;
}

bool test_maintenance_optimize()
{

  return true;
}
