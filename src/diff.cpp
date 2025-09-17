#include "../include/diff.hpp"

DiffResult compare_files(const std::string &file1, const std::string &file2)
{
  DiffResult result(file1, file2);

  if (!std::filesystem::exists(file1) || !std::filesystem::exists(file2))
  {
    return result;
  }

  result.is_binary = is_binary_file(file1) || is_binary_file(file2);
  if (result.is_binary)
  {
    return result;
  }

  std::vector<std::string> lines1 = read_file_lines(file1);
  std::vector<std::string> lines2 = read_file_lines(file2);

  result.hunks = compute_hunks(lines1, lines2);
  return result;
}

DiffResult compare_file_to_content(const std::string &file, const std::string &content)
{
  DiffResult result(file, "<content>");

  if (!std::filesystem::exists(file))
  {
    return result;
  }

  result.is_binary = is_binary_file(file);
  if (result.is_binary)
  {
    return result;
  }

  std::vector<std::string> file_lines = read_file_lines(file);

  // split content into lines
  std::vector<std::string> content_lines;
  std::istringstream iss(content);
  std::string line;
  while (std::getline(iss, line))
  {
    content_lines.push_back(line);
  }

  result.hunks = compute_hunks(file_lines, content_lines);
  return result;
}

DiffResult diff_staged()
{
  DiffResult result("staged", "working");

  std::vector<std::string> staged_files = get_staged_files();
  if (staged_files.empty())
  {
    return result;
  }

  for (const auto &file : staged_files)
  {
    if (is_binary_file(file))
    {
      continue;
    }

    std::string staged_content = get_staged_file_content(file);
    DiffResult file_diff = compare_file_to_content(file, staged_content);

    // merge hunks from this file
    for (const auto &hunk : file_diff.hunks)
    {
      DiffHunk file_hunk = hunk;
      file_hunk.header = file + ": " + hunk.header;
      result.hunks.push_back(file_hunk);
    }
  }

  return result;
}

DiffResult diff_unstaged()
{
  DiffResult result("working", "staged");

  std::vector<std::string> unstaged_files = get_unstaged_files();
  if (unstaged_files.empty())
  {
    return result;
  }

  for (const auto &file : unstaged_files)
  {
    if (is_binary_file(file))
    {
      continue;
    }

    std::string staged_content = get_staged_file_content(file);
    DiffResult file_diff = compare_file_to_content(file, staged_content);

    // merge hunks from this file
    for (const auto &hunk : file_diff.hunks)
    {
      DiffHunk file_hunk = hunk;
      file_hunk.header = file + ": " + hunk.header;
      result.hunks.push_back(file_hunk);
    }
  }

  return result;
}

DiffResult diff_working_directory()
{
  DiffResult result("working", "last commit");

  std::string current_commit = get_current_commit();
  if (current_commit.empty())
  {
    return result;
  }

  // get all tracked files
  std::vector<std::string> all_files;
  for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
  {
    if (entry.is_regular_file() && entry.path().string().find(".bittrack") != 0)
    {
      all_files.push_back(entry.path().string());
    }
  }

  for (const auto &file : all_files)
  {
    if (is_binary_file(file))
    {
      continue;
    }

    // compare with last commit version
    std::string commit_file = ".bittrack/objects/" + get_current_branch() + "/" + current_commit + "/" + file;
    if (std::filesystem::exists(commit_file))
    {
      DiffResult file_diff = compare_files(file, commit_file);

      // lMerge hunks from this file
      for (const auto &hunk : file_diff.hunks)
      {
        DiffHunk file_hunk = hunk;
        file_hunk.header = file + ": " + hunk.header;
        result.hunks.push_back(file_hunk);
      }
    }
  }

  return result;
}

DiffResult diff_commits(const std::string &commit1, const std::string &commit2)
{
  DiffResult result(commit1, commit2);

  std::string commit1_path = ".bittrack/objects/" + get_current_branch() + "/" + commit1;
  std::string commit2_path = ".bittrack/objects/" + get_current_branch() + "/" + commit2;

  if (!std::filesystem::exists(commit1_path) || !std::filesystem::exists(commit2_path))
  {
    return result;
  }

  std::set<std::string> all_files;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(commit1_path))
  {
    if (entry.is_regular_file())
    {
      std::string rel_path = std::filesystem::relative(entry.path(), commit1_path).string();
      all_files.insert(rel_path);
    }
  }
  for (const auto &entry : std::filesystem::recursive_directory_iterator(commit2_path))
  {
    if (entry.is_regular_file())
    {
      std::string rel_path = std::filesystem::relative(entry.path(), commit2_path).string();
      all_files.insert(rel_path);
    }
  }

  for (const auto &file : all_files)
  {
    std::string file1 = commit1_path + "/" + file;
    std::string file2 = commit2_path + "/" + file;

    if (is_binary_file(file1) || is_binary_file(file2))
    {
      continue;
    }

    DiffResult file_diff = compare_files(file1, file2);

    for (const auto &hunk : file_diff.hunks)
    {
      DiffHunk file_hunk = hunk;
      file_hunk.header = file + ": " + hunk.header;
      result.hunks.push_back(file_hunk);
    }
  }

  return result;
}

DiffResult diff_commit_to_working(const std::string &commit)
{
  DiffResult result(commit, "working");

  std::string commit_path = ".bittrack/objects/" + get_current_branch() + "/" + commit;
  if (!std::filesystem::exists(commit_path))
  {
    return result;
  }

  std::set<std::string> commit_files;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(commit_path))
  {
    if (entry.is_regular_file())
    {
      std::string rel_path = std::filesystem::relative(entry.path(), commit_path).string();
      commit_files.insert(rel_path);
    }
  }

  for (const auto &file : commit_files)
  {
    std::string commit_file = commit_path + "/" + file;

    if (is_binary_file(commit_file) || is_binary_file(file))
    {
      continue;
    }

    DiffResult file_diff = compare_files(commit_file, file);

    for (const auto &hunk : file_diff.hunks)
    {
      DiffHunk file_hunk = hunk;
      file_hunk.header = file + ": " + hunk.header;
      result.hunks.push_back(file_hunk);
    }
  }

  return result;
}

DiffResult diff_branches(const std::string &branch1, const std::string &branch2)
{
  DiffResult result(branch1, branch2);

  std::string commit1_path = ".bittrack/refs/heads/" + branch1;
  std::string commit2_path = ".bittrack/refs/heads/" + branch2;

  if (!std::filesystem::exists(commit1_path) || !std::filesystem::exists(commit2_path))
  {
    return result;
  }

  std::ifstream file1(commit1_path);
  std::ifstream file2(commit2_path);
  std::string commit1, commit2;
  std::getline(file1, commit1);
  std::getline(file2, commit2);
  file1.close();
  file2.close();

  return diff_commits(commit1, commit2);
}

void show_diff(const DiffResult &result)
{
  if (result.is_binary)
  {
    std::cout << "Binary files differ" << std::endl;
    return;
  }

  if (result.hunks.empty())
  {
    std::cout << "No differences found" << std::endl;
    return;
  }

  std::cout << "--- " << result.file1 << std::endl;
  std::cout << "+++ " << result.file2 << std::endl;

  for (const auto &hunk : result.hunks)
  {
    std::cout << hunk.header << std::endl;
    for (const auto &line : hunk.lines)
    {
      print_diff_line(line);
    }
  }
}

void show_unified_diff(const DiffResult &result)
{
  show_diff(result);
}

void show_side_by_side_diff(const DiffResult &result)
{
  if (result.is_binary)
  {
    std::cout << "Binary files differ" << std::endl;
    return;
  }

  if (result.hunks.empty())
  {
    std::cout << "No differences found" << std::endl;
    return;
  }

  std::cout << std::setw(50) << std::left << result.file1 << " | " << result.file2 << std::endl;
  std::cout << std::string(100, '-') << std::endl;

  for (const auto &hunk : result.hunks)
  {
    std::cout << hunk.header << std::endl;

    int old_line = hunk.old_start;
    int new_line = hunk.new_start;

    for (const auto &line : hunk.lines)
    {
      std::string old_content = (line.type == DiffLineType::ADDITION) ? "" : line.content;
      std::string new_content = (line.type == DiffLineType::DELETION) ? "" : line.content;
      std::cout << std::setw(3) << old_line << " " << std::setw(45) << std::left << old_content << " | " << std::setw(3) << new_line << " " << new_content << std::endl;

      if (line.type != DiffLineType::ADDITION)
        old_line++;
      if (line.type != DiffLineType::DELETION)
        new_line++;
    }
  }
}

void show_compact_diff(const DiffResult &result)
{
  if (result.is_binary)
  {
    std::cout << "Binary files differ" << std::endl;
    return;
  }

  if (result.hunks.empty())
  {
    std::cout << "No differences found" << std::endl;
    return;
  }

  int total_changes = 0;
  for (const auto &hunk : result.hunks)
  {
    for (const auto &line : hunk.lines)
    {
      if (line.type == DiffLineType::ADDITION || line.type == DiffLineType::DELETION)
      {
        total_changes++;
      }
    }
  }

  std::cout << result.file1 << " -> " << result.file2 << " (" << total_changes << " changes)" << std::endl;
}

bool is_binary_file(const std::string &file_path)
{
  if (!std::filesystem::exists(file_path))
  {
    return false;
  }

  std::ifstream file(file_path, std::ios::binary);
  char buffer[1024];
  file.read(buffer, sizeof(buffer));
  size_t bytes_read = file.gcount();

  for (size_t i = 0; i < bytes_read; i++)
  {
    if (buffer[i] == '\0')
    {
      return true;
    }
  }

  return false;
}

std::vector<std::string> read_file_lines(const std::string &file_path)
{
  std::vector<std::string> lines;
  std::ifstream file(file_path);
  std::string line;

  while (std::getline(file, line))
  {
    lines.push_back(line);
  }

  return lines;
}

std::vector<DiffHunk> compute_hunks(const std::vector<std::string> &old_lines, const std::vector<std::string> &new_lines)
{
  std::vector<DiffHunk> hunks;
  std::vector<DiffLine> diff_lines = compute_diff_lines(old_lines, new_lines);

  if (diff_lines.empty())
  {
    return hunks;
  }

  DiffHunk current_hunk(0, 0, 0, 0, "");
  int old_line = 1;
  int new_line = 1;

  for (const auto &line : diff_lines)
  {
    if (line.type == DiffLineType::ADDITION || line.type == DiffLineType::DELETION)
    {
      if (current_hunk.lines.empty())
      {
        current_hunk.old_start = old_line;
        current_hunk.new_start = new_line;
        current_hunk.header = "@@ -" + std::to_string(old_line) + ",0 +" + std::to_string(new_line) + ",0 @@";
      }
      current_hunk.lines.push_back(line);
    }
    else
    {
      if (!current_hunk.lines.empty())
      {
        current_hunk.old_count = current_hunk.lines.size();
        current_hunk.new_count = current_hunk.lines.size();
        hunks.push_back(current_hunk);
        current_hunk = DiffHunk(0, 0, 0, 0, "");
      }
    }

    if (line.type != DiffLineType::ADDITION)
      old_line++;
    if (line.type != DiffLineType::DELETION)
      new_line++;
  }

  if (!current_hunk.lines.empty())
  {
    current_hunk.old_count = current_hunk.lines.size();
    current_hunk.new_count = current_hunk.lines.size();
    hunks.push_back(current_hunk);
  }

  return hunks;
}

std::vector<DiffLine> compute_diff_lines(const std::vector<std::string> &old_lines, const std::vector<std::string> &new_lines)
{
  std::vector<DiffLine> diff_lines;
  size_t max_lines = std::max(old_lines.size(), new_lines.size());

  for (size_t i = 0; i < max_lines; i++)
  {
    bool old_exists = (i < old_lines.size());
    bool new_exists = (i < new_lines.size());

    if (old_exists && new_exists)
    {
      if (old_lines[i] == new_lines[i])
      {
        diff_lines.push_back(DiffLine(DiffLineType::CONTEXT, i + 1, old_lines[i]));
      }
      else
      {
        diff_lines.push_back(DiffLine(DiffLineType::DELETION, i + 1, old_lines[i]));
        diff_lines.push_back(DiffLine(DiffLineType::ADDITION, i + 1, new_lines[i]));
      }
    }
    else if (old_exists)
    {
      diff_lines.push_back(DiffLine(DiffLineType::DELETION, i + 1, old_lines[i]));
    }
    else
    {
      diff_lines.push_back(DiffLine(DiffLineType::ADDITION, i + 1, new_lines[i]));
    }
  }

  return diff_lines;
}

void print_diff_line(const DiffLine &line, const std::string &prefix)
{
  std::cout << prefix << get_diff_line_prefix(line.type) << line.content << std::endl;
}

std::string get_diff_line_prefix(DiffLineType type)
{
  switch (type)
  {
  case DiffLineType::ADDITION:
    return "+ ";
  case DiffLineType::DELETION:
    return "- ";
  case DiffLineType::CONTEXT:
    return "  ";
  default:
    return "  ";
  }
}

void show_file_history(const std::string &file_path)
{
  std::cout << "File history for: " << file_path << std::endl;
  std::cout << "Not implemented yet" << std::endl;
}

void show_file_blame(const std::string &file_path)
{
  std::cout << "File blame for: " << file_path << std::endl;
  std::cout << "Not implemented yet" << std::endl;
}

void show_file_log(const std::string &file_path, int max_entries)
{
  std::cout << "File log for: " << file_path << " (max " << max_entries << " entries)" << std::endl;
  std::cout << "Not implemented yet" << std::endl;
}
