#include "../include/maintenance.hpp"

void garbage_collect()
{
  std::cout << "Running garbage collection..." << std::endl;

  // get list of unreachable objects
  std::vector<std::string> unreachable = get_unreachable_objects();

  if (unreachable.empty())
  {
    std::cout << "No unreachable objects found" << std::endl;
    return;
  }

  // remove unreachable objects and calculate freed space
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

  // gather all objects
  std::string objects_dir = ".bittrack/objects";
  if (!std::filesystem::exists(objects_dir))
  {
    std::cout << "No objects to repack" << std::endl;
    return;
  }

  // statistics
  size_t original_count = 0;
  size_t repacked_count = 0;
  size_t original_size = 0;
  size_t repacked_size = 0;

  // calculate original stats
  for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
  {
    if (entry.is_regular_file())
    {
      original_count++;
      original_size += std::filesystem::file_size(entry.path());
    }
  }

  // repack objects into a single directory
  std::string packed_dir = objects_dir + "/packed";
  std::filesystem::create_directories(packed_dir);

  // move objects to packed directory
  for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
  {
    // skip already packed objects
    if (entry.is_regular_file() && entry.path().parent_path().string() != packed_dir)
    {
      // move object
      std::string filename = entry.path().filename().string();
      std::string new_path = packed_dir + "/" + filename;

      // avoid overwriting existing files
      if (!std::filesystem::exists(new_path))
      {
        // move file
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

  // get list of unreachable objects
  std::vector<std::string> unreachable = get_unreachable_objects();

  // remove unreachable objects
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

  // check for missing or corrupted objects
  std::vector<std::string> missing_objects;

  // check all objects
  std::string objects_dir = ".bittrack/objects";
  if (std::filesystem::exists(objects_dir))
  {
    // iterate through all objects
    for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
    {
      // check if file is accessible
      if (entry.is_regular_file())
      {
        // try to open the file
        std::ifstream file(entry.path());
        if (!file.good())
        {
          missing_objects.push_back(entry.path().string());
        }
        file.close();
      }
    }
  }

  // report results
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

void show_repository_info()
{
  std::cout << "Repository Information:" << std::endl;
  std::cout << "  Current branch: " << get_current_branch() << std::endl;
  std::cout << "  Current commit: " << get_current_commit() << std::endl;

  size_t total_size = 0;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(".bittrack"))
  {
    if (entry.is_regular_file())
    {
      total_size += std::filesystem::file_size(entry.path());
    }
  }
  std::cout << "  Repository size: " << format_size(total_size) << std::endl;
  std::cout << "  Last modified: " << std::endl;
}

void analyze_repository()
{
  std::cout << "Analyzing repository..." << std::endl;

  RepoStats stats = calculate_repository_stats();

  std::cout << "Analysis Results:" << std::endl;
  std::cout << "  Repository size: " << format_size(stats.total_size) << std::endl;
  std::cout << "  Average commit size: " << format_size(stats.total_size / std::max(stats.commit_count, size_t(1))) << std::endl;
  std::cout << "  Files per commit: " << stats.total_objects / std::max(stats.commit_count, size_t(1)) << std::endl;

  find_large_files();
  find_duplicate_files();
}

void find_large_files(size_t threshold)
{
  std::cout << "Finding files larger than " << format_size(threshold) << "..." << std::endl;

  // store large files
  std::vector<std::pair<std::string, size_t>> large_files;

  // scan files in repository
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    // skip .bittrack directory
    if (entry.is_regular_file() && entry.path().string().find(".bittrack") != 0)
    {
      // check file size
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

  // sort large files by size descending
  std::sort(large_files.begin(), large_files.end(), [](const auto &a, const auto &b)
            { return a.second > b.second; });
  std::cout << "Large files:" << std::endl;

  // display large files
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

  garbage_collect();
  repack_repository();

  std::cout << "Repository optimization completed" << std::endl;
}

RepoStats calculate_repository_stats()
{
  RepoStats stats;

  // calculate total objects and size
  std::string objects_dir = ".bittrack/objects";
  if (std::filesystem::exists(objects_dir)) // check if objects directory exists
  {
    // iterate through all objects
    for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
    {
      // check if file is regular
      if (entry.is_regular_file())
      {
        // update stats
        stats.total_objects++;
        size_t file_size = std::filesystem::file_size(entry.path());
        stats.total_size += file_size;

        // check for largest file
        if (file_size > stats.largest_file_size)
        {
          stats.largest_file_size = file_size;
          stats.largest_file = entry.path().string();
        }
      }
    }
  }

  // estimate commit count from branches
  stats.commit_count = stats.total_objects;
  std::string refs_dir = ".bittrack/refs/heads";

  // count branches
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
  // store unreachable objects
  std::vector<std::string> unreachable;

  // gather all objects
  std::string objects_dir = ".bittrack/objects";
  if (!std::filesystem::exists(objects_dir))
  {
    return unreachable;
  }

  // determine reachable objects
  std::set<std::string> reachable_objects;

  // add current commit
  std::string current_commit = get_current_commit();
  if (!current_commit.empty()) // check if current commit is valid
  {
    reachable_objects.insert(current_commit);
  }

  // add commits from branches
  std::string refs_dir = ".bittrack/refs/heads";
  if (std::filesystem::exists(refs_dir))
  {
    for (const auto &entry : std::filesystem::directory_iterator(refs_dir))
    {
      if (entry.is_regular_file())
      {
        // read commit hash from branch file
        std::ifstream file(entry.path());
        std::string commit_hash;

        if (std::getline(file, commit_hash)) // check if line was read successfully
        {
          reachable_objects.insert(commit_hash);
        }
        file.close();
      }
    }
  }

  // add commits from tags
  for (const auto &entry : std::filesystem::recursive_directory_iterator(objects_dir))
  {
    if (entry.is_regular_file())
    {
      // get object hash
      std::string object_path = entry.path().string();
      std::string object_hash = entry.path().filename().string();

      // check if object is reachable
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
  // store duplicate files
  std::vector<std::string> duplicates;
  std::map<std::string, std::vector<std::string>> file_hashes;

  // scan files in repository
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file() && entry.path().string().find(".bittrack") != 0)
    {
      try
      {
        // compute file hash
        std::string file_hash = hash_file(entry.path().string());
        file_hashes[file_hash].push_back(entry.path().string());
      }
      catch (...)
      {
        continue;
      }
    }
  }

  // collect duplicates
  for (const auto &pair : file_hashes)
  {
    // if more than one file has the same hash, they are duplicates
    if (pair.second.size() > 1)
    {
      // add all but the first file to duplicates
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
  // define size units
  const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  int unit_index = 0;
  double size = static_cast<double>(bytes);

  // convert size to appropriate unit
  while (size >= 1024.0 && unit_index < 4)
  {
    size /= 1024.0;
    unit_index++;
  }

  // format size string
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
  return oss.str();
}
