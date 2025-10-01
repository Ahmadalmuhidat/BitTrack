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
  DiffResult result("last commit", "staged");

  std::vector<std::string> staged_files = get_staged_files();
  if (staged_files.empty())
  {
    return result;
  }

  std::string current_commit = get_current_commit();
  if (current_commit.empty())
  {
    for (const auto &file : staged_files)
    {
      if (is_binary_file(file))
      {
        continue;
      }

      std::string staged_content = get_staged_file_content(file);

      std::vector<std::string> content_lines;
      std::istringstream iss(staged_content);
      std::string line;
      while (std::getline(iss, line))
      {
        content_lines.push_back(line);
      }

      if (!content_lines.empty())
      {
        DiffHunk hunk(0, 0, 1, content_lines.size(), file + ": @@ -0,0 +1," + std::to_string(content_lines.size()) + " @@");

        for (const auto &content_line : content_lines)
        {
          DiffLine diff_line(DiffLineType::ADDITION, 0, content_line);
          hunk.lines.push_back(diff_line);
        }

        result.hunks.push_back(hunk);
      }
    }
    return result;
  }

  for (const auto &file : staged_files)
  {
    if (is_binary_file(file))
    {
      continue;
    }

    std::string staged_content = get_staged_file_content(file);
    std::string commit_file = ".bittrack/objects/" + current_commit + "/" + file;

    if (std::filesystem::exists(commit_file))
    {
      DiffResult file_diff = compare_files(commit_file, file);

      for (const auto &hunk : file_diff.hunks)
      {
        DiffHunk file_hunk = hunk;
        file_hunk.header = file + ": " + hunk.header;
        result.hunks.push_back(file_hunk);
      }
    }
    else
    {
      DiffResult file_diff("/dev/null", file);

      std::vector<std::string> content_lines;
      std::istringstream iss(staged_content);
      std::string line;
      while (std::getline(iss, line))
      {
        content_lines.push_back(line);
      }

      if (!content_lines.empty())
      {
        DiffHunk hunk(0, 0, 1, content_lines.size(), "@@ -0,0 +1," + std::to_string(content_lines.size()) + " @@");

        for (const auto &content_line : content_lines)
        {
          DiffLine diff_line(DiffLineType::ADDITION, 0, content_line);
          hunk.lines.push_back(diff_line);
        }

        file_diff.hunks.push_back(hunk);
      }

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

  std::vector<std::string> staged_files = get_staged_files();
  std::vector<std::string> unstaged_files = get_unstaged_files();

  std::set<std::string> all_files;
  for (const auto &file : staged_files)
  {
    all_files.insert(file);
  }
  for (const auto &file : unstaged_files)
  {
    all_files.insert(file);
  }

  std::string commit_dir = ".bittrack/objects/" + current_commit;
  if (std::filesystem::exists(commit_dir))
  {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(commit_dir))
    {
      if (entry.is_regular_file())
      {
        std::string rel_path = std::filesystem::relative(entry.path(), commit_dir).string();
        all_files.insert(rel_path);
      }
    }
  }

  for (const auto &file : all_files)
  {
    if (is_binary_file(file))
    {
      continue;
    }

    std::string commit_file = ".bittrack/objects/" + current_commit + "/" + file;
    if (std::filesystem::exists(commit_file))
    {
      DiffResult file_diff = compare_files(file, commit_file);

      for (const auto &hunk : file_diff.hunks)
      {
        DiffHunk file_hunk = hunk;
        file_hunk.header = file + ": " + hunk.header;
        result.hunks.push_back(file_hunk);
      }
    }
    else
    {
      std::vector<std::string> file_lines = read_file_lines(file);

      if (!file_lines.empty())
      {
        DiffHunk hunk(0, 0, 1, file_lines.size(), file + ": @@ -0,0 +1," + std::to_string(file_lines.size()) + " @@");

        for (const auto &file_line : file_lines)
        {
          DiffLine diff_line(DiffLineType::ADDITION, 0, file_line);
          hunk.lines.push_back(diff_line);
        }

        result.hunks.push_back(hunk);
      }
    }
  }

  return result;
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

bool is_binary_file(const std::string &file_path)
{
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
    {
      old_line++;
    }
    if (line.type != DiffLineType::DELETION)
    {
      new_line++;
    }
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
