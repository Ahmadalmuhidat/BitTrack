#include "../include/stash.hpp"

void stash_changes(const std::string &message)
{
  std::vector<std::string> staged_files = get_staged_files();
  if (staged_files.empty())
  {
    std::cout << "No staged changes to stash" << std::endl;
    return;
  }

  StashEntry entry;
  entry.id = generate_stash_id();
  entry.message = message.empty() ? "WIP on " + get_current_branch() : message;
  entry.branch = get_current_branch();
  entry.commit_hash = get_current_commit();
  entry.timestamp = std::time(nullptr);
  entry.files = staged_files;

  backup_staged_files(entry.id);
  remove_staged_files_from_working_directory();

  std::ofstream clear_staging_file(".bittrack/index", std::ios::trunc);
  clear_staging_file.close();

  save_stash_entry(entry);

  std::cout << "Stashed " << staged_files.size() << " staged files: " << entry.message << std::endl;
}

void stash_list()
{
  std::vector<StashEntry> entries = get_stash_entries();

  if (entries.empty())
  {
    std::cout << "No stashes found" << std::endl;
    return;
  }

  std::cout << "Stash list:" << std::endl;
  for (const auto &entry : entries)
  {
    std::cout << "  " << entry.id << ": " << entry.message << " (" << format_timestamp(entry.timestamp) << ")" << std::endl;
  }
}

void stash_show(const std::string &stash_id)
{
  if (stash_id.empty())
  {
    std::vector<StashEntry> entries = get_stash_entries();
    if (entries.empty())
    {
      std::cout << "No stashes found" << std::endl;
      return;
    }
    stash_show(entries[0].id);
    return;
  }

  StashEntry entry = get_stash_entry(stash_id);
  if (entry.id.empty())
  {
    std::cout << "Stash not found: " << stash_id << std::endl;
    return;
  }

  std::cout << "Stash: " << entry.id << std::endl;
  std::cout << "Message: " << entry.message << std::endl;
  std::cout << "Branch: " << entry.branch << std::endl;
  std::cout << "Commit: " << entry.commit_hash << std::endl;
  std::cout << "Timestamp: " << format_timestamp(entry.timestamp) << std::endl;
  std::cout << "Files: " << entry.files.size() << std::endl;
}

void stash_apply(const std::string &stash_id)
{
  if (stash_id.empty())
  {
    std::vector<StashEntry> entries = get_stash_entries();
    if (entries.empty())
    {
      std::cout << "No stashes found" << std::endl;
      return;
    }
    stash_apply(entries[0].id);
    return;
  }

  StashEntry entry = get_stash_entry(stash_id);
  if (entry.id.empty())
  {
    std::cout << "Stash not found: " << stash_id << std::endl;
    return;
  }

  restore_working_directory(entry.id);

  for (const auto &file : entry.files)
  {
    if (std::filesystem::exists(file))
    {
      stage(file);
    }
  }

  std::cout << "Applied stash: " << entry.message << " (" << entry.files.size() << " files staged)" << std::endl;
}

void stash_pop(const std::string &stash_id)
{
  stash_apply(stash_id);
  if (!stash_id.empty())
  {
    stash_drop(stash_id);
  }
  else
  {
    std::vector<StashEntry> entries = get_stash_entries();
    if (!entries.empty())
    {
      stash_drop(entries[0].id);
    }
  }
}

void stash_drop(const std::string &stash_id)
{
  if (stash_id.empty())
  {
    std::cout << "Stash ID required for drop operation" << std::endl;
    return;
  }

  StashEntry entry = get_stash_entry(stash_id);
  if (entry.id.empty())
  {
    std::cout << "Stash not found: " << stash_id << std::endl;
    return;
  }

  std::string stash_dir = get_stash_file_path(stash_id);
  if (std::filesystem::exists(stash_dir))
  {
    std::filesystem::remove_all(stash_dir);
  }

  delete_stash_entry(stash_id);

  std::cout << "Dropped stash: " << stash_id << std::endl;
}

void stash_clear()
{
  std::vector<StashEntry> entries = get_stash_entries();

  for (const auto &entry : entries)
  {
    std::string stash_dir = get_stash_file_path(entry.id);
    if (std::filesystem::exists(stash_dir))
    {
      std::filesystem::remove_all(stash_dir);
    }
  }

  std::string stash_index = get_stash_dir() + "/index";
  if (std::filesystem::exists(stash_index))
  {
    std::filesystem::remove(stash_index);
  }

  std::cout << "Cleared all stashes" << std::endl;
}

bool stash_has_stashes()
{
  return !get_stash_entries().empty();
}

std::vector<StashEntry> get_stash_entries()
{
  std::vector<StashEntry> entries;
  std::string stash_index = get_stash_dir() + "/index";

  if (!std::filesystem::exists(stash_index))
  {
    return entries;
  }

  std::ifstream file(stash_index);
  std::string line;

  while (std::getline(file, line))
  {
    if (line.empty())
      continue;

    StashEntry entry;
    std::istringstream iss(line);
    std::string token;

    if (std::getline(iss, token, '|'))
      entry.id = token;
    if (std::getline(iss, token, '|'))
      entry.message = token;
    if (std::getline(iss, token, '|'))
      entry.branch = token;
    if (std::getline(iss, token, '|'))
      entry.commit_hash = token;
    if (std::getline(iss, token, '|'))
      entry.timestamp = std::stoll(token);

    entries.push_back(entry);
  }

  return entries;
}

StashEntry get_stash_entry(const std::string &stash_id)
{
  std::vector<StashEntry> entries = get_stash_entries();

  for (const auto &entry : entries)
  {
    if (entry.id == stash_id)
    {
      StashEntry result = entry;

      std::string stash_dir = get_stash_file_path(stash_id);
      if (std::filesystem::exists(stash_dir))
      {
        for (const auto &file_entry : std::filesystem::recursive_directory_iterator(stash_dir))
        {
          if (file_entry.is_regular_file())
          {
            std::string rel_path = std::filesystem::relative(file_entry.path(), stash_dir).string();
            result.files.push_back(rel_path);
          }
        }
      }

      return result;
    }
  }

  return StashEntry();
}

void save_stash_entry(const StashEntry &entry)
{
  std::string stash_index = get_stash_dir() + "/index";
  std::filesystem::create_directories(get_stash_dir());

  std::ofstream file(stash_index, std::ios::app);
  file << entry.id << "|" << entry.message << "|" << entry.branch << "|" << entry.commit_hash << "|" << entry.timestamp << std::endl;
  file.close();
}

void delete_stash_entry(const std::string &stash_id)
{
  std::vector<StashEntry> entries = get_stash_entries();
  std::string stash_index = get_stash_dir() + "/index";

  std::ofstream file(stash_index);
  for (const auto &entry : entries)
  {
    if (entry.id != stash_id)
    {
      file << entry.id << "|" << entry.message << "|" << entry.branch << "|" << entry.commit_hash << "|" << entry.timestamp << std::endl;
    }
  }
  file.close();
}

std::string generate_stash_id()
{
  return "stash_" + std::to_string(std::time(nullptr));
}

void backup_staged_files(const std::string &stash_id)
{
  std::string stash_dir = get_stash_file_path(stash_id);
  std::filesystem::create_directories(stash_dir);

  std::vector<std::string> staged_files = get_staged_files();

  for (const auto &file : staged_files)
  {
    if (std::filesystem::exists(file))
    {
      std::string dest_path = stash_dir + "/" + file;
      std::filesystem::create_directories(std::filesystem::path(dest_path).parent_path());
      std::filesystem::copy_file(file, dest_path, std::filesystem::copy_options::overwrite_existing);
    }
  }
}

void restore_working_directory(const std::string &stash_id)
{
  std::string stash_dir = get_stash_file_path(stash_id);

  if (!std::filesystem::exists(stash_dir))
  {
    std::cout << "Stash directory not found: " << stash_dir << std::endl;
    return;
  }

  for (const auto &entry : std::filesystem::recursive_directory_iterator(stash_dir))
  {
    if (entry.is_regular_file())
    {
      std::string rel_path = std::filesystem::relative(entry.path(), stash_dir).string();
      std::filesystem::path parent_path = std::filesystem::path(rel_path).parent_path();

      if (!parent_path.empty())
      {
        std::filesystem::create_directories(parent_path);
      }

      std::filesystem::copy_file(entry.path(), rel_path, std::filesystem::copy_options::overwrite_existing);
    }
  }
}

std::vector<std::string> get_all_tracked_files()
{
  std::vector<std::string> files;
  std::vector<std::string> staged = get_staged_files();
  std::vector<std::string> unstaged = get_unstaged_files();

  files.insert(files.end(), staged.begin(), staged.end());
  files.insert(files.end(), unstaged.begin(), unstaged.end());

  std::sort(files.begin(), files.end());
  files.erase(std::unique(files.begin(), files.end()), files.end());

  return files;
}

void remove_staged_files_from_working_directory()
{
  std::vector<std::string> staged_files = get_staged_files();

  for (const auto &file : staged_files)
  {
    if (std::filesystem::exists(file))
    {
      std::filesystem::remove(file);
    }
  }
}

std::string get_stash_dir()
{
  return ".bittrack/stash";
}

std::string get_stash_file_path(const std::string &stash_id)
{
  return get_stash_dir() + "/" + stash_id;
}
