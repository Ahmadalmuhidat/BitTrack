#include "../include/ignore.hpp"

std::string IgnorePattern::convert_git_pattern_to_regex(const std::string &git_pattern)
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
    case '/':
      // / is literal in Git patterns
      regex_pattern += '/';
      break;
    default:
      regex_pattern += c;
      break;
    }
  }

  // if pattern doesn't start with /, it can match anywhere in the path
  if (git_pattern[0] != '/')
  {
    regex_pattern = ".*" + regex_pattern;
  }
  else
  {
    regex_pattern = "^" + regex_pattern;
  }

  // if pattern doesn't end with /, it can match files or directories
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


std::string normalize_path(const std::string &path)
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

bool matches_pattern(const std::string &filePath, const IgnorePattern &pattern)
{
  std::string normalized_path = normalize_path(filePath);

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

std::vector<std::string> read_bitignore(const std::string &filePath)
{
  std::vector<std::string> patterns;

  if (!std::filesystem::exists(filePath))
  {
    return patterns;
  }

  std::ifstream bitignoreFile(filePath);
  std::string line;

  while (std::getline(bitignoreFile, line))
  {
    // remove trailing whitespace
    line.erase(line.find_last_not_of(" \t\r\n") + 1);

    // skip empty lines and comments
    if (line.empty() || line[0] == '#')
    {
      continue;
    }

    patterns.push_back(line);
  }

  return patterns;
}

std::vector<IgnorePattern> parse_ignore_patterns(const std::vector<std::string> &raw_patterns)
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

bool is_file_ignored_by_ignore_patterns(const std::string &filePath, const std::vector<IgnorePattern> &patterns)
{
  if (filePath.empty())
  {
    return false;
  }

  std::string normalized_path = normalize_path(filePath);
  bool ignored = false;

  // process patterns in order (later patterns can override earlier ones)
  for (const auto &pattern : patterns)
  {
    if (matches_pattern(normalized_path, pattern))
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

bool is_file_ignored_by_patterns(const std::string &file_path, const std::vector<std::string> &patterns)
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

    return ::is_file_ignored_by_ignore_patterns(file_path, ignorePatterns);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error checking if file is ignored: " << e.what() << std::endl;
    return false;
  }
}

bool should_ignore_file(const std::string &file_path)
{
  // check for .bitignore file in current directory and parent directories
  std::string current_dir = std::filesystem::current_path().string();
  std::string bitignore_path = current_dir + "/.bitignore";

  // look for .bitignore in current directory and parent directories
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

  std::vector<std::string> raw_patterns = read_bitignore(bitignore_path);
  std::vector<IgnorePattern> patterns = parse_ignore_patterns(raw_patterns);

  return is_file_ignored_by_ignore_patterns(file_path, patterns);
}

void create_default_bitignore()
{
  std::ofstream bitignore_file(".bitignore");
  if (bitignore_file.is_open())
  {
    bitignore_file << "# BitTrack ignore file\n";
    bitignore_file << "# Add patterns to ignore files and directories\n\n";
    bitignore_file << "# Compiled files\n";
    bitignore_file << "*.o\n";
    bitignore_file << "*.exe\n";
    bitignore_file << "*.out\n";
    bitignore_file << "*.a\n";
    bitignore_file << "*.so\n";
    bitignore_file << "*.dylib\n\n";
    bitignore_file << "# Build directories\n";
    bitignore_file << "build/\n";
    bitignore_file << "bin/\n";
    bitignore_file << "obj/\n\n";
    bitignore_file << "# IDE files\n";
    bitignore_file << ".vscode/\n";
    bitignore_file << ".idea/\n";
    bitignore_file << "*.swp\n";
    bitignore_file << "*.swo\n\n";
    bitignore_file << "# OS files\n";
    bitignore_file << ".DS_Store\n";
    bitignore_file << "Thumbs.db\n";
    bitignore_file.close();
    std::cout << "Created default .bitignore file." << std::endl;
  }
}

void add_ignore_pattern(const std::string& pattern)
{
  std::ofstream bitignore_file(".bitignore", std::ios::app);
  if (bitignore_file.is_open())
  {
    bitignore_file << pattern << "\n";
    bitignore_file.close();
    std::cout << "Added pattern to .bitignore: " << pattern << std::endl;
  }
  else
  {
    std::cout << "Error: Could not open .bitignore file." << std::endl;
  }
}

void remove_ignore_pattern(const std::string& pattern)
{
  std::vector<std::string> patterns;
  std::ifstream bitignore_file(".bitignore");
  
  if (!bitignore_file.is_open())
  {
    std::cout << "Error: Could not open .bitignore file." << std::endl;
    return;
  }
  
  std::string line;
  while (std::getline(bitignore_file, line))
  {
    if (line != pattern)
    {
      patterns.push_back(line);
    }
  }
  bitignore_file.close();
  
  std::ofstream out_file(".bitignore");
  if (out_file.is_open())
  {
    for (const auto& p : patterns)
    {
      out_file << p << "\n";
    }
    out_file.close();
    std::cout << "Removed pattern from .bitignore: " << pattern << std::endl;
  }
}

void list_ignore_patterns()
{
  std::ifstream bitignore_file(".bitignore");
  if (!bitignore_file.is_open())
  {
    std::cout << "No .bitignore file found." << std::endl;
    return;
  }
  
  std::cout << "Current ignore patterns:" << std::endl;
  std::string line;
  int line_num = 1;
  while (std::getline(bitignore_file, line))
  {
    if (!line.empty() && line[0] != '#')
    {
      std::cout << line_num << ": " << line << std::endl;
    }
    line_num++;
  }
  bitignore_file.close();
}