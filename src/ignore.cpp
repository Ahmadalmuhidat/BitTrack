#include "../include/ignore.hpp"

std::string IgnorePattern::convertGitPatternToRegex(const std::string &git_pattern)
{
  std::string regex_pattern;

  for (size_t i = 0; i < git_pattern.length(); ++i)
  {
    char c = git_pattern[i];

    switch (c)
    {
    case '*':
      if (i + 1 < git_pattern.length() && git_pattern[i + 1] == '*')
      {
        // ** matches any number of directories
        regex_pattern += ".*";
        ++i; // Skip the second *
      }
      else
      {
        // * matches any characters except /
        regex_pattern += "[^/]*";
      }
      break;
    case '?':
      // ? matches any single character except /
      regex_pattern += "[^/]";
      break;
    case '.':
    case '+':
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
    case '^':
    case '$':
    case '|':
    case '\\':
      regex_pattern += '\\';
      regex_pattern += c;
      break;
    case '/': // / is literal in Git patterns
      regex_pattern += '/';
      break;
    default:
      regex_pattern += c;
      break;
    }
  }

  // If pattern doesn't start with /, it can match anywhere in the path
  if (git_pattern[0] != '/')
  {
    regex_pattern = ".*" + regex_pattern;
  }
  else
  {
    regex_pattern = "^" + regex_pattern;
  }

  // If pattern doesn't end with /, it can match files or directories
  if (git_pattern.back() != '/')
  {
    regex_pattern += "(/.*)?$";
  }
  else
  {
    regex_pattern += ".*$";
  }

  return regex_pattern;
}

std::string normalizePath(const std::string &path)
{
  std::string normalized = path;

  // convert backslashes to forward slashes
  std::replace(normalized.begin(), normalized.end(), '\\', '/');

  // remove leading ./ if present
  if (normalized.length() >= 2 && normalized.substr(0, 2) == "./")
  {
    normalized = normalized.substr(2);
  }

  // remove duplicate slashes
  std::string result;
  bool last_was_slash = false;
  for (char c : normalized)
  {
    if (c == '/')
    {
      if (!last_was_slash)
      {
        result += c;
      }
      last_was_slash = true;
    }
    else
    {
      result += c;
      last_was_slash = false;
    }
  }

  return result;
}

bool matchesPattern(const std::string &filePath, const IgnorePattern &pattern)
{
  std::string normalized_path = normalizePath(filePath);

  // for directory patterns, check if the path is within the directory
  if (pattern.is_directory)
  {
    if (normalized_path.find(pattern.pattern + "/") == 0 ||
        normalized_path == pattern.pattern)
    {
      return true;
    }
  }

  // use regex matching
  try
  {
    return std::regex_match(normalized_path, pattern.regex_pattern);
  }
  catch (const std::regex_error &)
  {
    // fallback to simple string matching
    return normalized_path.find(pattern.pattern) != std::string::npos;
  }
}

std::vector<std::string> readBitignore(const std::string &filePath)
{
  std::vector<std::string> patterns;

  // Read entire file using safeReadFile
  std::string file_content = ErrorHandler::safeReadFile(filePath);
  if (file_content.empty() && !std::filesystem::exists(filePath))
  {
    return patterns;
  }

  std::istringstream bitignoreFile(file_content);
  std::string line;

  while (std::getline(bitignoreFile, line))
  {
    // remove trailing whitespace
    size_t last = line.find_last_not_of(" \t\r\n");
    if (last != std::string::npos)
    {
      line = line.substr(0, last + 1);
    }
    else
    {
      line = "";
    }

    // Skip empty lines and comments
    if (line.empty() || line[0] == '#')
    {
      continue;
    }

    patterns.push_back(line);
  }

  return patterns;
}

std::vector<IgnorePattern> parseIgnorePatterns(const std::vector<std::string> &raw_patterns)
{
  std::vector<IgnorePattern> patterns;

  for (const auto &raw_pattern : raw_patterns)
  {
    if (!raw_pattern.empty())
    {
      patterns.emplace_back(raw_pattern);
    }
  }

  return patterns;
}

bool isFileIgnoredByIgnorePatterns(const std::string &filePath, const std::vector<IgnorePattern> &patterns)
{
  if (filePath.empty())
  {
    return false;
  }

  std::string normalized_path = normalizePath(filePath);
  bool ignored = false;

  // process patterns in order (later patterns can override earlier ones)
  for (const auto &pattern : patterns)
  {
    if (matchesPattern(normalized_path, pattern))
    {
      if (pattern.is_negation)
      {
        // negation pattern - unignore this file
        ignored = false;
      }
      else
      {
        // regular pattern - ignore this file
        ignored = true;
      }
    }
  }

  return ignored;
}

bool isFileIgnoredByPatterns(const std::string &file_path, const std::vector<std::string> &patterns)
{
  try
  {
    if (file_path.empty())
    {
      return false;
    }

    std::vector<IgnorePattern> ignorePatterns;
    for (const auto &pattern : patterns)
    {
      if (!pattern.empty())
      {
        ignorePatterns.emplace_back(pattern);
      }
    }

    return ::isFileIgnoredByIgnorePatterns(file_path, ignorePatterns);
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Error checking if file is ignored: " + std::string(e.what()),
        ErrorSeverity::ERROR, "is_file_ignored_by_patterns");
    return false;
  }
}

bool shouldIgnoreFile(const std::string &file_path)
{
  std::string current_dir = std::filesystem::current_path().string();
  std::string bitignore_path = current_dir + "/.bitignore";

  while (current_dir != std::filesystem::path(current_dir).parent_path().string())
  {
    bitignore_path = current_dir + "/.bitignore";
    if (std::filesystem::exists(bitignore_path))
    {
      break;
    }
    current_dir = std::filesystem::path(current_dir).parent_path().string();
  }

  if (!std::filesystem::exists(bitignore_path))
  {
    return false;
  }

  std::vector<std::string> raw_patterns = readBitignore(bitignore_path);
  std::vector<IgnorePattern> patterns = parseIgnorePatterns(raw_patterns);

  return isFileIgnoredByIgnorePatterns(file_path, patterns);
}

void createDefaultBitignore()
{
  std::string content = "# BitTrack ignore file\n"
                        "# Add patterns to ignore files and directories\n\n"
                        "# Compiled files\n"
                        "*.o\n"
                        "*.exe\n"
                        "*.out\n"
                        "*.a\n"
                        "*.so\n"
                        "*.dylib\n\n"
                        "# Build directories\n"
                        "build/\n"
                        "bin/\n"
                        "obj/\n\n"
                        "# IDE files\n"
                        ".vscode/\n"
                        ".idea/\n"
                        "*.swp\n"
                        "*.swo\n\n"
                        "# OS files\n"
                        ".DS_Store\n"
                        "Thumbs.db\n";

  if (ErrorHandler::safeWriteFile(".bitignore", content))
  {
    std::cout << "Created default .bitignore file." << std::endl;
  }
}

void addIgnorePattern(const std::string &pattern)
{
  if (ErrorHandler::safeAppendFile(".bitignore", pattern + "\n"))
  {
    std::cout << "Added pattern to .bitignore: " << pattern << std::endl;
  }
}

void removeIgnorePattern(const std::string &pattern)
{
  std::vector<std::string> patterns = readBitignore(".bitignore");
  std::string new_content = "";
  bool found = false;

  for (const auto &p : patterns)
  {
    if (p != pattern)
    {
      new_content += p + "\n";
    }
    else
    {
      found = true;
    }
  }

  if (found)
  {
    if (ErrorHandler::safeWriteFile(".bitignore", new_content))
    {
      std::cout << "Removed pattern from .bitignore: " << pattern << std::endl;
    }
  }
}

void listIgnorePatterns()
{
  std::string content = ErrorHandler::safeReadFile(".bitignore");
  if (content.empty() && !std::filesystem::exists(".bitignore"))
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "No .bitignore file found",
        ErrorSeverity::ERROR,
        "list_ignore_patterns");
    return;
  }

  std::cout << "Current ignore patterns:" << std::endl;
  std::istringstream bitignoreFile(content);
  std::string line;
  int line_num = 1;
  while (std::getline(bitignoreFile, line))
  {
    if (!line.empty() && line[0] != '#')
    {
      std::cout << line_num << ": " << line << std::endl;
    }
    line_num++;
  }
}