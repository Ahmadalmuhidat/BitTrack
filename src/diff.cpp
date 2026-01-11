#include "../include/diff.hpp"

DiffResult compareTwoFiles(
    const std::string &file1,
    const std::string &file2)
{
  // Initialize result
  DiffResult result(file1, file2);

  // Check if files exist
  if (!std::filesystem::exists(file1) || !std::filesystem::exists(file2))
  {
    return result;
  }

  // Check if either file is binary
  result.is_binary = isBinaryFile(file1) || isBinaryFile(file2);
  if (result.is_binary)
  {
    return result;
  }

  // Read files lines
  std::vector<std::string> lines1 = readFileLines(file1);
  std::vector<std::string> lines2 = readFileLines(file2);

  // compute hunks
  result.hunks = computeHunks(lines1, lines2);
  return result;
}

DiffResult compareFileWithContent(
    const std::string &file,
    const std::string &content)
{
  // Initialize result
  DiffResult result(file, "<content>");

  // Check if file exists
  if (!std::filesystem::exists(file))
  {
    return result;
  }

  // Check if file is binary
  result.is_binary = isBinaryFile(file);
  if (result.is_binary)
  {
    return result;
  }

  // Read file lines
  std::vector<std::string> file_lines = readFileLines(file);

  std::vector<std::string> content_lines;
  std::istringstream iss(content);
  std::string line;

  while (std::getline(iss, line))
  {
    content_lines.push_back(line);
  }

  // compute hunks
  result.hunks = computeHunks(file_lines, content_lines);
  return result;
}

DiffResult diffStagedFiles()
{
  // Initialize result
  DiffResult result("last commit", "staged");

  // Get staged files
  std::vector<std::string> staged_files = getStagedFiles();
  if (staged_files.empty())
  {
    return result;
  }

  // Get current commit
  std::string current_commit = getCurrentCommit();
  if (current_commit.empty())
  {
    // No commits yet, diff staged against empty
    for (const auto &file : staged_files)
    {
      // Skip binary files
      if (isBinaryFile(file))
      {
        continue;
      }

      // Get staged content
      std::string staged_content = getStagedFileContent(file);

      // Create diff result against /dev/null
      std::vector<std::string> content_lines;
      std::istringstream iss(staged_content);
      std::string line;

      // Read staged content lines
      while (std::getline(iss, line))
      {
        content_lines.push_back(line);
      }

      // Create hunk for added file
      if (!content_lines.empty())
      {
        // Create hunk
        DiffHunk hunk(0, 0, 1, content_lines.size(), file + ": @@ -0,0 +1," + std::to_string(content_lines.size()) + " @@");

        // Add lines to hunk
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

  // Diff each staged file against the last commit
  for (const auto &file : staged_files)
  {
    // Skip binary files
    if (isBinaryFile(file))
    {
      continue;
    }

    // Get staged content
    std::string staged_content = getStagedFileContent(file);
    std::string commit_file = ".bittrack/objects/" + current_commit + "/" + file;

    // Compare staged file to last commit
    if (std::filesystem::exists(commit_file))
    {
      DiffResult file_diff = compareTwoFiles(commit_file, file);

      // Adjust hunk headers to include file name
      for (const auto &hunk : file_diff.hunks)
      {
        DiffHunk file_hunk = hunk;
        file_hunk.header = file + ": " + hunk.header;
        result.hunks.push_back(file_hunk);
      }
    }
    else
    {
      // File is new in staged, diff against /dev/null
      DiffResult file_diff("/dev/null", file);

      // Create hunk for added file
      std::vector<std::string> content_lines;
      std::istringstream iss(staged_content);
      std::string line;

      // Read staged content lines
      while (std::getline(iss, line))
      {
        content_lines.push_back(line);
      }

      // Create hunk
      if (!content_lines.empty())
      {
        // Create hunk
        DiffHunk hunk(0, 0, 1, content_lines.size(), "@@ -0,0 +1," + std::to_string(content_lines.size()) + " @@");

        // Add lines to hunk
        for (const auto &content_line : content_lines)
        {
          // Create addition line
          DiffLine diff_line(DiffLineType::ADDITION, 0, content_line);
          hunk.lines.push_back(diff_line);
        }
        file_diff.hunks.push_back(hunk);
      }

      // Adjust hunk headers to include file name
      for (const auto &hunk : file_diff.hunks)
      {
        // Adjust header
        DiffHunk file_hunk = hunk;
        file_hunk.header = file + ": " + hunk.header;
        result.hunks.push_back(file_hunk);
      }
    }
  }

  return result;
}

DiffResult diffUnstagedFiles()
{
  // Initialize result
  DiffResult result("working", "staged");

  // Get unstaged files
  std::vector<std::string> unstaged_files = getUnstagedFiles();
  if (unstaged_files.empty())
  {
    return result;
  }

  // Diff each unstaged file against the staged content
  for (const auto &file : unstaged_files)
  {
    // Skip binary files
    if (isBinaryFile(file))
    {
      continue;
    }

    // Get staged content
    std::string staged_content = getStagedFileContent(file);
    DiffResult file_diff = compareFileWithContent(file, staged_content);

    // Adjust hunk headers to include file name
    for (const auto &hunk : file_diff.hunks)
    {
      // Adjust header
      DiffHunk file_hunk = hunk;
      file_hunk.header = file + ": " + hunk.header;
      result.hunks.push_back(file_hunk);
    }
  }

  return result;
}

DiffResult diffWorkingDirectory()
{
  // Initialize result
  DiffResult result("working", "last commit");

  // Get current commit
  std::string current_commit = getCurrentCommit();
  if (current_commit.empty())
  {
    return result;
  }

  // Get all files to compare
  std::vector<std::string> staged_files = getStagedFiles();
  std::vector<std::string> unstaged_files = getUnstagedFiles();

  // Combine staged and unstaged files into a set to avoid duplicates
  std::set<std::string> all_files;
  for (const auto &file : staged_files)
  {
    all_files.insert(file);
  }
  for (const auto &file : unstaged_files)
  {
    all_files.insert(file);
  }

  // Also include files from the last commit that may have been deleted in the
  // working directory
  std::string commit_dir = ".bittrack/objects/" + current_commit;
  if (std::filesystem::exists(commit_dir))
  {
    // Recursively iterate through commit directory to find all files
    for (const auto &entry : ErrorHandler::safeListDirectoryFiles(commit_dir))
    {
      std::string rel_path = std::filesystem::relative(entry, commit_dir).string();
      all_files.insert(rel_path);
    }
  }

  // Diff each file against the last commit
  for (const auto &file : all_files)
  {
    // Skip binary files
    if (isBinaryFile(file))
    {
      continue;
    }

    // Get commit file path
    std::string commit_file = ".bittrack/objects/" + current_commit + "/" + file;
    if (std::filesystem::exists(commit_file))
    {
      // Compare working file to last commit
      DiffResult file_diff = compareTwoFiles(file, commit_file);

      // Adjust hunk headers to include file name
      for (const auto &hunk : file_diff.hunks)
      {
        // Adjust header
        DiffHunk file_hunk = hunk;
        file_hunk.header = file + ": " + hunk.header;
        result.hunks.push_back(file_hunk);
      }
    }
    else
    {
      // File is new in working directory, diff against /dev/null
      std::vector<std::string> file_lines = readFileLines(file);

      // Create hunk for added file
      if (!file_lines.empty())
      {
        // Create hunk
        DiffHunk hunk(0, 0, 1, file_lines.size(), file + ": @@ -0,0 +1," + std::to_string(file_lines.size()) + " @@");

        // Add lines to hunk
        for (const auto &file_line : file_lines)
        {
          // Create addition line
          DiffLine diff_line(DiffLineType::ADDITION, 0, file_line);
          hunk.lines.push_back(diff_line);
        }
        result.hunks.push_back(hunk);
      }
    }
  }

  return result;
}

void printDiff(const DiffResult &result)
{
  // Handle binary files
  if (result.is_binary)
  {
    // Indicate binary files differ
    std::cout << "Binary files differ" << std::endl;
    return;
  }

  // Handle no differences
  if (result.hunks.empty())
  {
    std::cout << "No differences found" << std::endl;
    return;
  }

  std::cout << "--- " << result.file1 << std::endl;
  std::cout << "+++ " << result.file2 << std::endl;

  // Print each hunk
  for (const auto &hunk : result.hunks)
  {
    // Print hunk header
    std::cout << hunk.header << std::endl;
    for (const auto &line : hunk.lines)
    {
      printDiffLine(line);
    }
  }
}

bool isBinaryFile(const std::string &file_path)
{
  try
  {
    if (!std::filesystem::exists(file_path))
    {
      return false;
    }

    // Open file in binary mode
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
      return false;
    }

    char buffer[1024];
    file.read(buffer, sizeof(buffer));
    size_t bytes_read = file.gcount();
    file.close();

    // Check for null bytes
    for (size_t i = 0; i < bytes_read; i++)
    {
      // If a null byte is found, it's a binary file
      if (buffer[i] == '\0')
      {
        return true;
      }
    }

    return false;
  }
  catch (...)
  {
    return false;
  }
}

std::vector<std::string> readFileLines(const std::string &file_path)
{
  // Read file lines into vector
  std::vector<std::string> lines;
  std::string file = ErrorHandler::safeReadFile(file_path);
  std::istringstream file_content(file);
  std::string line;

  // Read each line
  while (std::getline(file_content, line))
  {
    lines.push_back(line);
  }
  return lines;
}

std::vector<DiffHunk> computeHunks(
    const std::vector<std::string> &old_lines,
    const std::vector<std::string> &new_lines)
{
  // Compute diff lines
  std::vector<DiffHunk> hunks;
  std::vector<DiffLine> diff_lines = computeDiffLines(old_lines, new_lines);

  // Group diff lines into hunks
  if (diff_lines.empty())
  {
    return hunks;
  }

  // Initialize hunk variables
  DiffHunk current_hunk(0, 0, 0, 0, "");
  int old_line = 1;
  int new_line = 1;

  // Iterate through diff lines
  for (const auto &line : diff_lines)
  {
    // Check if line is addition or deletion
    if (line.type == DiffLineType::ADDITION || line.type == DiffLineType::DELETION)
    {
      // Start new hunk if necessary
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
      // If we hit a context line and have a current hunk, finalize it
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

  // Finalize last hunk if necessary
  if (!current_hunk.lines.empty())
  {
    current_hunk.old_count = current_hunk.lines.size();
    current_hunk.new_count = current_hunk.lines.size();
    hunks.push_back(current_hunk);
  }

  return hunks;
}

std::vector<DiffLine> computeDiffLines(
    const std::vector<std::string> &old_lines,
    const std::vector<std::string> &new_lines)
{
  // Compute diff lines using a simple line-by-line comparison
  std::vector<DiffLine> diff_lines;
  size_t max_lines = std::max(old_lines.size(), new_lines.size());

  // Iterate through lines
  for (size_t i = 0; i < max_lines; i++)
  {
    // Check if line exists in old and new
    bool old_exists = (i < old_lines.size());
    bool new_exists = (i < new_lines.size());

    // Determine line type
    if (old_exists && new_exists)
    {
      // Both lines exist, check for equality
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
    else if (old_exists) // Line exists only in old
    {
      // Line deleted
      diff_lines.push_back(DiffLine(DiffLineType::DELETION, i + 1, old_lines[i]));
    }
    else
    {
      // Line added
      diff_lines.push_back(DiffLine(DiffLineType::ADDITION, i + 1, new_lines[i]));
    }
  }

  return diff_lines;
}

void printDiffLine(
    const DiffLine &line,
    const std::string &prefix)
{
  std::cout << prefix << getDiffLinePrefix(line.type) << line.content << std::endl;
}

std::string getDiffLinePrefix(DiffLineType type)
{
  // Return prefix based on line type
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
