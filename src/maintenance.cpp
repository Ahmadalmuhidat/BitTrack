#include "../include/maintenance.hpp"

void garbageCollect()
{
  std::cout << "Running garbage collection..." << std::endl;

  // Get list of unreachable objects
  std::vector<std::string> unreachable = getUnreachableObjects();

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
      ErrorHandler::safeRemoveFile(obj);
    }
  }

  std::cout << "Removed " << unreachable.size() << " unreachable objects" << std::endl;
  std::cout << "Freed " << formatSize(freed_space) << " of space" << std::endl;
}

void repackRepository()
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

  std::vector<std::filesystem::path> objects = ErrorHandler::safeListDirectoryFiles(objects_dir);

  // calculate original stats
  for (const auto &entry : objects)
  {
    original_count++;
    original_size += std::filesystem::file_size(entry);
  }

  // repack objects into a single directory
  std::string packed_dir = objects_dir + "/packed";
  std::vector<std::filesystem::path> packed_files = ErrorHandler::safeListDirectoryFiles(packed_dir);

  // move objects to packed directory
  for (const auto &entry : packed_files)
  {
    // Skip already packed objects
    if (entry.parent_path().string() != packed_dir)
    {
      // move object
      std::string filename = entry.filename().string();
      std::string new_path = packed_dir + "/" + filename;

      // avoid overwriting existing files
      if (!std::filesystem::exists(new_path))
      {
        // move file
        std::filesystem::rename(entry, new_path);
        repacked_count++;
        repacked_size += std::filesystem::file_size(new_path);
      }
    }
  }

  std::cout << "Repacked " << repacked_count << " objects" << std::endl;
  std::cout << "Original size: " << formatSize(original_size) << std::endl;
  std::cout << "Repacked size: " << formatSize(repacked_size) << std::endl;
  std::cout << "Repository repacked successfully" << std::endl;
}

void pruneObjects()
{
  std::cout << "Pruning objects..." << std::endl;

  // Get list of unreachable objects
  std::vector<std::string> unreachable = getUnreachableObjects();

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

void fsckRepository()
{
  std::cout << "Checking repository integrity..." << std::endl;

  // Check for missing or corrupted objects
  std::vector<std::string> missing_objects;

  // Check all objects
  std::string objects_dir = ".bittrack/objects";
  if (std::filesystem::exists(objects_dir))
  {
    std::vector<std::filesystem::path> objects = ErrorHandler::safeListDirectoryFiles(objects_dir);
    // Iterate through all objects
    for (const auto &entry : objects)
    {
      // try to open the file
      std::ifstream file(entry);
      if (!file.good())
      {
        missing_objects.push_back(entry);
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

void showRepositoryInfo()
{
  std::cout << "Repository Information:" << std::endl;
  std::cout << "  Current branch: " << getCurrentBranch() << std::endl;
  std::cout << "  Current commit: " << getCurrentCommit() << std::endl;

  size_t total_size = 0;
  std::vector<std::filesystem::path> repository_directories = ErrorHandler::safeListDirectoryFiles(".bittrack");
  for (const auto &entry : repository_directories)
  {
    total_size += std::filesystem::file_size(entry);
  }
  std::cout << "  Repository size: " << formatSize(total_size) << std::endl;
  std::cout << "  Last modified: " << std::endl;
}

void analyzeRepository()
{
  std::cout << "Analyzing repository..." << std::endl;

  RepoStats stats = calculateRepositoryStats();

  std::cout << "Analysis Results:" << std::endl;
  std::cout << "  Repository size: " << formatSize(stats.total_size) << std::endl;
  std::cout << "  Average commit size: " << formatSize(stats.total_size / std::max(stats.commit_count, size_t(1))) << std::endl;
  std::cout << "  Files per commit: " << stats.total_objects / std::max(stats.commit_count, size_t(1)) << std::endl;

  findLargeFiles();
  findDuplicateFiles();
}

void findLargeFiles(size_t threshold)
{
  std::cout << "Finding files larger than " << formatSize(threshold) << "..." << std::endl;

  // store large files
  std::vector<std::pair<std::string, size_t>> large_files;

  // scan files in repository
  std::vector<std::filesystem::path> repository_files = ErrorHandler::safeListDirectoryFiles(".");
  for (const auto &entry : repository_files)
  {
    // Skip .bittrack directory
    if (entry.string().find(".bittrack") != 0)
    {
      // Check file size
      size_t file_size = std::filesystem::file_size(entry);
      if (file_size > threshold)
      {
        large_files.push_back({entry.string(), file_size});
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
  {
    return a.second > b.second;
  });
  std::cout << "Large files:" << std::endl;

  // Display large files
  for (const auto &file : large_files)
  {
    std::cout << "  " << file.first << " (" << formatSize(file.second) << ")" << std::endl;
  }
}

void findDuplicateFiles()
{
  std::cout << "Finding duplicate files..." << std::endl;

  std::vector<std::string> duplicates = getDuplicateFiles();

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

void optimizeRepository()
{
  std::cout << "Optimizing repository..." << std::endl;

  garbageCollect();
  repackRepository();

  std::cout << "Repository optimization completed" << std::endl;
}

RepoStats calculateRepositoryStats()
{
  RepoStats stats;

  // calculate total objects and size
  std::string objects_dir = ".bittrack/objects";
  if (std::filesystem::exists(objects_dir)) // Check if objects directory exists
  {
    // Iterate through all objects
    std::vector<std::filesystem::path> objects = ErrorHandler::safeListDirectoryFiles(objects_dir);
    for (const auto &entry : objects)
    {
      // update stats
      stats.total_objects++;
      size_t file_size = std::filesystem::file_size(entry);
      stats.total_size += file_size;

      // Check for largest file
      if (file_size > stats.largest_file_size)
      {
        stats.largest_file_size = file_size;
        stats.largest_file = entry.string();
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

std::vector<std::string> getUnreachableObjects()
{
  // store unreachable objects
  std::vector<std::string> unreachable;

  // gather all objects
  std::string objects_dir_path = ".bittrack/objects";
  if (!std::filesystem::exists(objects_dir_path))
  {
    return unreachable;
  }

  // determine reachable objects
  std::set<std::string> reachable_objects;

  // add current commit
  std::string current_commit = getCurrentCommit();
  if (!current_commit.empty()) // Check if current commit is valid
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
        // Read commit hash from branch file
        std::ifstream file(entry.path());
        std::string commit_hash;

        if (std::getline(file, commit_hash)) // Check if line was read successfully
        {
          reachable_objects.insert(commit_hash);
        }
        file.close();
      }
    }
  }

  std::vector<std::filesystem::path> objects_dir = ErrorHandler::safeListDirectoryFiles(objects_dir_path);
  for (const auto &entry : objects_dir)
  {
    // Get object hash
    std::string object_path = entry.string();
    std::string object_hash = entry.filename().string();

    // Check if object is reachable
    if (reachable_objects.find(object_hash) == reachable_objects.end())
    {
      unreachable.push_back(object_path);
    }
  }

  return unreachable;
}

std::vector<std::string> getDuplicateFiles()
{
  // store duplicate files
  std::vector<std::string> duplicates;
  std::map<std::string, std::vector<std::string>> file_hashes;

  std::vector<std::filesystem::path> repository_files = ErrorHandler::safeListDirectoryFiles(".");
  for (const auto &entry : repository_files)
  {
    if (entry.string().find(".bittrack") != 0)
    {
      try
      {
        // compute file hash
        std::string file_hash = hashFile(entry.string());
        file_hashes[file_hash].push_back(entry.string());
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
    // If more than one file has the same hash, they are duplicates
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

std::string formatSize(size_t bytes)
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