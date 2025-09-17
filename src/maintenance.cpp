#include "../include/maintenance.hpp"

void garbage_collect()
{
  std::cout << "Running garbage collection..." << std::endl;

  std::vector<std::string> unreachable = get_unreachable_objects();

  if (unreachable.empty())
  {
    std::cout << "No unreachable objects found" << std::endl;
    return;
  }

  size_t freed_space = 0;
  for (const auto &obj : unreachable)
  {
    if (std::filesystem::exists(obj))
    {
      freed_space += std::filesystem::file_size(obj);
      std::filesystem::remove(obj);
    }
  }

  std::cout << "Removed " << unreachable.size() << " unreachable objects" << std::endl;
  std::cout << "Freed " << format_size(freed_space) << " of space" << std::endl;
}

void repack_repository()
{
  std::cout << "Repacking repository..." << std::endl;

  std::string objects_dir = ".bittrack/objects/" + get_current_branch();
  if (!std::filesystem::exists(objects_dir))
  {
    std::cout << "No objects to repack" << std::endl;
    return;
  }

  size_t original_count = 0;
  size_t repacked_count = 0;
  size_t original_size = 0;
  size_t repacked_size = 0;

  // lCount original objects
  for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
  {
    if (entry.is_regular_file())
    {
      original_count++;
      original_size += std::filesystem::file_size(entry.path());
    }
  }

  // lCreate packed objects directory
  std::string packed_dir = objects_dir + "/packed";
  std::filesystem::create_directories(packed_dir);

  // lSimple repacking: move all objects to packed directory
  for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
  {
    if (entry.is_regular_file() && entry.path().parent_path().string() != packed_dir)
    {
      std::string filename = entry.path().filename().string();
      std::string new_path = packed_dir + "/" + filename;

      if (!std::filesystem::exists(new_path))
      {
        std::filesystem::rename(entry.path(), new_path);
        repacked_count++;
        repacked_size += std::filesystem::file_size(new_path);
      }
    }
  }

  std::cout << "Repacked " << repacked_count << " objects" << std::endl;
  std::cout << "Original size: " << format_size(original_size) << std::endl;
  std::cout << "Repacked size: " << format_size(repacked_size) << std::endl;
  std::cout << "Repository repacked successfully" << std::endl;
}

void prune_objects()
{
  std::cout << "Pruning objects..." << std::endl;

  std::vector<std::string> unreachable = get_unreachable_objects();

  for (const auto &obj : unreachable)
  {
    if (std::filesystem::exists(obj))
    {
      std::filesystem::remove(obj);
    }
  }

  std::cout << "Pruned " << unreachable.size() << " objects" << std::endl;
}

void fsck_repository()
{
  std::cout << "Checking repository integrity..." << std::endl;

  // check for missing objects
  std::vector<std::string> missing_objects;

  // check all commits
  std::string objects_dir = ".bittrack/objects/" + get_current_branch();
  if (std::filesystem::exists(objects_dir))
  {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
    {
      if (entry.is_regular_file())
      {
        // check if file is readable
        std::ifstream file(entry.path());
        if (!file.good())
        {
          missing_objects.push_back(entry.path().string());
        }
        file.close();
      }
    }
  }

  if (missing_objects.empty())
  {
    std::cout << "Repository integrity check passed" << std::endl;
  }
  else
  {
    std::cout << "Found " << missing_objects.size() << " corrupted objects" << std::endl;
    for (const auto &obj : missing_objects)
    {
      std::cout << "  " << obj << std::endl;
    }
  }
}

void show_repository_stats()
{
  RepoStats stats = calculate_repository_stats();

  std::cout << "Repository Statistics:" << std::endl;
  std::cout << "  Total objects: " << stats.total_objects << std::endl;
  std::cout << "  Total size: " << format_size(stats.total_size) << std::endl;
  std::cout << "  Commits: " << stats.commit_count << std::endl;
  std::cout << "  Branches: " << stats.branch_count << std::endl;
  std::cout << "  Tags: " << stats.tag_count << std::endl;

  if (!stats.largest_file.empty())
  {
    std::cout << "  Largest file: " << stats.largest_file << " (" << format_size(stats.largest_file_size) << ")" << std::endl;
  }
}

void show_repository_info()
{
  std::cout << "Repository Information:" << std::endl;
  std::cout << "  Current branch: " << get_current_branch() << std::endl;
  std::cout << "  Current commit: " << get_current_commit() << std::endl;
  std::cout << "  Repository size: " << get_repository_size() << std::endl;
  std::cout << "  Last modified: " << std::endl; // would get from filesystem
}

void analyze_repository()
{
  std::cout << "Analyzing repository..." << std::endl;

  RepoStats stats = calculate_repository_stats();

  std::cout << "Analysis Results:" << std::endl;
  std::cout << "  Repository size: " << format_size(stats.total_size) << std::endl;
  std::cout << "  Average commit size: " << format_size(stats.total_size / std::max(stats.commit_count, size_t(1))) << std::endl;
  std::cout << "  Files per commit: " << stats.total_objects / std::max(stats.commit_count, size_t(1)) << std::endl;

  // find large files
  find_large_files();

  // find duplicates
  find_duplicate_files();
}

void find_large_files(size_t threshold)
{
  std::cout << "Finding files larger than " << format_size(threshold) << "..." << std::endl;

  std::vector<std::pair<std::string, size_t>> large_files;

  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file() && entry.path().string().find(".bittrack") != 0)
    {
      size_t file_size = std::filesystem::file_size(entry.path());
      if (file_size > threshold)
      {
        large_files.push_back({entry.path().string(), file_size});
      }
    }
  }

  if (large_files.empty())
  {
    std::cout << "No large files found" << std::endl;
    return;
  }

  // sort by size
  std::sort(large_files.begin(), large_files.end(), [](const auto &a, const auto &b)
            { return a.second > b.second; });
  std::cout << "Large files:" << std::endl;

  for (const auto &file : large_files)
  {
    std::cout << "  " << file.first << " (" << format_size(file.second) << ")" << std::endl;
  }
}

void find_duplicate_files()
{
  std::cout << "Finding duplicate files..." << std::endl;

  std::vector<std::string> duplicates = get_duplicate_files();

  if (duplicates.empty())
  {
    std::cout << "No duplicate files found" << std::endl;
    return;
  }

  std::cout << "Duplicate files:" << std::endl;
  for (const auto &file : duplicates)
  {
    std::cout << "  " << file << std::endl;
  }
}

void optimize_repository()
{
  std::cout << "Optimizing repository..." << std::endl;

  // run garbage collection
  garbage_collect();

  // repack repository
  repack_repository();

  // remove empty directories
  remove_empty_directories();

  std::cout << "Repository optimization completed" << std::endl;
}

void clean_untracked_files()
{
  std::cout << "Cleaning untracked files..." << std::endl;

  std::vector<std::string> tracked_files = get_staged_files();
  std::set<std::string> tracked_set(tracked_files.begin(), tracked_files.end());

  size_t removed_count = 0;
  size_t freed_space = 0;

  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string file_path = entry.path().string();

      // skip .bittrack directory and tracked files
      if (file_path.find(".bittrack") == 0 || tracked_set.find(file_path) != tracked_set.end())
      {
        continue;
      }

      // check if file is ignored using Git-like ignore system
      if (should_ignore_file(file_path))
      {
        continue;
      }

      size_t file_size = std::filesystem::file_size(entry.path());
      std::filesystem::remove(entry.path());
      removed_count++;
      freed_space += file_size;
      std::cout << "  Removed: " << file_path << std::endl;
    }
  }

  std::cout << "Removed " << removed_count << " untracked files" << std::endl;
  std::cout << "Freed " << format_size(freed_space) << " of space" << std::endl;
}

void clean_ignored_files()
{
  std::cout << "Cleaning ignored files..." << std::endl;

  std::vector<std::string> tracked_files = get_staged_files();
  std::set<std::string> tracked_set(tracked_files.begin(), tracked_files.end());

  size_t removed_count = 0;
  size_t freed_space = 0;

  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file())
    {
      std::string file_path = entry.path().string();

      // skip .bittrack directory and tracked files
      if (file_path.find(".bittrack") == 0 || tracked_set.find(file_path) != tracked_set.end())
      {
        continue;
      }

      // only remove ignored files using Git-like ignore system
      if (should_ignore_file(file_path))
      {
        size_t file_size = std::filesystem::file_size(entry.path());
        std::filesystem::remove(entry.path());
        removed_count++;
        freed_space += file_size;
        std::cout << "  Removed ignored file: " << file_path << std::endl;
      }
    }
  }

  std::cout << "Removed " << removed_count << " ignored files" << std::endl;
  std::cout << "Freed " << format_size(freed_space) << " of space" << std::endl;
}

void remove_empty_directories()
{
  std::cout << "Removing empty directories..." << std::endl;

  size_t removed_count = 0;

  // remove empty directories (excluding .bittrack)
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_directory() && entry.path().string().find(".bittrack") != 0)
    {
      if (std::filesystem::is_empty(entry.path()))
      {
        std::filesystem::remove(entry.path());
        removed_count++;
      }
    }
  }

  std::cout << "Removed " << removed_count << " empty directories" << std::endl;
}

void compact_repository()
{
  std::cout << "Compacting repository..." << std::endl;

  // run optimization
  optimize_repository();

  std::cout << "Repository compaction completed" << std::endl;
}

void backup_repository(const std::string &backup_path)
{
  std::string target_path = backup_path.empty() ? "backup_" + std::to_string(std::time(nullptr)) : backup_path;
  std::cout << "Creating backup to: " << target_path << std::endl;

  // copy entire repository
  std::filesystem::create_directories(target_path);
  std::filesystem::copy(".", target_path, std::filesystem::copy_options::recursive);

  std::cout << "Backup created successfully" << std::endl;
}

void list_backups()
{
  std::cout << "Available backups:" << std::endl;

  // look for backup directories
  for (const auto &entry : std::filesystem::directory_iterator("."))
  {
    if (entry.is_directory() && entry.path().filename().string().find("backup_") == 0)
    {
      std::cout << "  " << entry.path().filename().string() << std::endl;
    }
  }
}

void restore_from_backup(const std::string &backup_path)
{
  if (!std::filesystem::exists(backup_path))
  {
    std::cout << "Backup not found: " << backup_path << std::endl;
    return;
  }

  std::cout << "Restoring from backup: " << backup_path << std::endl;

  // create a temporary directory for the current state
  std::string temp_dir = ".bittrack_temp_" + std::to_string(std::time(nullptr));
  std::filesystem::create_directories(temp_dir);

  try
  {
    // move current .bittrack to temp directory
    if (std::filesystem::exists(".bittrack"))
    {
      std::filesystem::rename(".bittrack", temp_dir + "/.bittrack");
    }

    // copy backup .bittrack to current directory
    std::string backup_bittrack = backup_path + "/.bittrack";
    if (std::filesystem::exists(backup_bittrack))
    {
      std::filesystem::copy(backup_bittrack, ".bittrack", std::filesystem::copy_options::recursive);
    }

    // copy backup files to current directory
    for (const auto &entry : std::filesystem::directory_iterator(backup_path))
    {
      if (entry.is_regular_file() && entry.path().filename().string() != ".bittrack")
      {
        std::filesystem::copy(entry.path(), entry.path().filename(), std::filesystem::copy_options::overwrite_existing);
      }
    }

    std::cout << "Restore completed successfully" << std::endl;
    std::cout << "Previous state backed up to: " << temp_dir << std::endl;
  }
  catch (const std::exception &e)
  {
    std::cout << "Restore failed: " << e.what() << std::endl;

    // restore previous state
    if (std::filesystem::exists(temp_dir + "/.bittrack"))
    {
      std::filesystem::remove_all(".bittrack");
      std::filesystem::rename(temp_dir + "/.bittrack", ".bittrack");
    }
  }
}

void benchmark_operations()
{
  std::cout << "Benchmarking operations..." << std::endl;

  auto start = std::chrono::high_resolution_clock::now();

  // benchmark various operations
  show_repository_stats();

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << "Benchmark completed in " << duration.count() << "ms" << std::endl;
}

void profile_repository()
{
  std::cout << "Profiling repository..." << std::endl;

  // this would implement profiling logic
  std::cout << "Repository profiling completed" << std::endl;
}

void check_integrity()
{
  std::cout << "Checking repository integrity..." << std::endl;

  fsck_repository();

  std::cout << "Integrity check completed" << std::endl;
}

RepoStats calculate_repository_stats()
{
  RepoStats stats;

  // count objects
  std::string objects_dir = ".bittrack/objects/" + get_current_branch();
  if (std::filesystem::exists(objects_dir))
  {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
    {
      if (entry.is_regular_file())
      {
        stats.total_objects++;
        size_t file_size = std::filesystem::file_size(entry.path());
        stats.total_size += file_size;

        if (file_size > stats.largest_file_size)
        {
          stats.largest_file_size = file_size;
          stats.largest_file = entry.path().string();
        }
      }
    }
  }

  // count commits (simplified)
  stats.commit_count = stats.total_objects; // this is a rough estimate

  // lCount branches
  std::string refs_dir = ".bittrack/refs/heads";
  if (std::filesystem::exists(refs_dir))
  {
    for (const auto &entry : std::filesystem::directory_iterator(refs_dir))
    {
      if (entry.is_regular_file())
      {
        stats.branch_count++;
      }
    }
  }

  // count tags
  std::string tags_dir = ".bittrack/refs/tags";
  if (std::filesystem::exists(tags_dir))
  {
    for (const auto &entry : std::filesystem::directory_iterator(tags_dir))
    {
      if (entry.is_regular_file())
      {
        stats.tag_count++;
      }
    }
  }

  return stats;
}

std::vector<std::string> get_unreachable_objects()
{
  std::vector<std::string> unreachable;

  std::string objects_dir = ".bittrack/objects/" + get_current_branch();
  if (!std::filesystem::exists(objects_dir))
  {
    return unreachable;
  }

  std::set<std::string> reachable_objects;

  // get all commit hashes from refs
  // lFor now, we'll get commits from the current branch
  std::string current_commit = get_current_commit();
  if (!current_commit.empty())
  {
    reachable_objects.insert(current_commit);
  }

  // get all branch refs
  std::string refs_dir = ".bittrack/refs/heads";
  if (std::filesystem::exists(refs_dir))
  {
    for (const auto &entry : std::filesystem::directory_iterator(refs_dir))
    {
      if (entry.is_regular_file())
      {
        std::ifstream file(entry.path());
        std::string commit_hash;
        if (std::getline(file, commit_hash))
        {
          reachable_objects.insert(commit_hash);
        }
        file.close();
      }
    }
  }

  // find unreachable objects
  for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
  {
    if (entry.is_regular_file())
    {
      std::string object_path = entry.path().string();
      std::string object_hash = entry.path().filename().string();

      if (reachable_objects.find(object_hash) == reachable_objects.end())
      {
        unreachable.push_back(object_path);
      }
    }
  }

  return unreachable;
}

std::vector<std::string> get_duplicate_files()
{
  std::vector<std::string> duplicates;
  std::map<std::string, std::vector<std::string>> file_hashes;

  // scan all files in the repository
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file() && entry.path().string().find(".bittrack") != 0)
    {
      try
      {
        std::string file_hash = hash_file(entry.path().string());
        file_hashes[file_hash].push_back(entry.path().string());
      }
      catch (...)
      {
        // skip files that can't be hashed
        continue;
      }
    }
  }

  // find files with duplicate hashes
  for (const auto &pair : file_hashes)
  {
    if (pair.second.size() > 1)
    {
      // add all but the first file as duplicates
      for (size_t i = 1; i < pair.second.size(); ++i)
      {
        duplicates.push_back(pair.second[i]);
      }
    }
  }

  return duplicates;
}

std::string format_size(size_t bytes)
{
  const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  int unit = 0;
  double size = bytes;

  while (size >= 1024 && unit < 4)
  {
    size /= 1024;
    unit++;
  }

  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
  return oss.str();
}

std::string get_repository_size()
{
  size_t total_size = 0;

  for (const auto &entry : std::filesystem::recursive_directory_iterator(".bittrack"))
  {
    if (entry.is_regular_file())
    {
      total_size += std::filesystem::file_size(entry.path());
    }
  }

  return format_size(total_size);
}

