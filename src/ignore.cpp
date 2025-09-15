#include "../include/ignore.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

// convert Git pattern to regex
std::string IgnorePattern::convert_git_pattern_to_regex(const std::string& git_pattern) {
  std::string regex_pattern;
  
  for (size_t i = 0; i < git_pattern.length(); ++i) {
  char c = git_pattern[i];
  
  switch (c) {
  case '*':
  if (i + 1 < git_pattern.length() && git_pattern[i + 1] == '*') {
  // ** matches any number of directories
  regex_pattern += ".*";
  ++i; // Skip the second *
  } else {
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
L                // Escape regex special characters
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
  if (git_pattern[0] != '/') {
  regex_pattern = ".*" + regex_pattern;
  } else {
  regex_pattern = "^" + regex_pattern;
  }
  
  // if pattern doesn't end with /, it can match files or directories
  if (git_pattern.back() != '/') {
  regex_pattern += "(/.*)?$";
  } else {
  regex_pattern += ".*$";
  }
  
  return regex_pattern;
}

std::string get_file_extension(const std::string& filePath) {
  return std::filesystem::path(filePath).extension().string();
}

std::string normalize_path(const std::string& path) {
  std::string normalized = path;
  
  // convert backslashes to forward slashes
  std::replace(normalized.begin(), normalized.end(), '\\', '/');
  
  // remove leading ./ if present
  if (normalized.length() >= 2 && normalized.substr(0, 2) == "./") {
  normalized = normalized.substr(2);
  }
  
  // remove duplicate slashes
  std::string result;
  bool last_was_slash = false;
  for (char c : normalized) {
  if (c == '/') {
  if (!last_was_slash) {
  result += c;
  }
  last_was_slash = true;
  } else {
  result += c;
  last_was_slash = false;
  }
  }
  
  return result;
}

bool matches_pattern(const std::string& filePath, const IgnorePattern& pattern) {
  std::string normalized_path = normalize_path(filePath);
  
  // for directory patterns, check if the path is within the directory
  if (pattern.is_directory) {
  if (normalized_path.find(pattern.pattern + "/") == 0 || 
  normalized_path == pattern.pattern) {
  return true;
  }
  }
  
  // use regex matching
  try {
  return std::regex_match(normalized_path, pattern.regex_pattern);
  } catch (const std::regex_error&) {
  // fallback to simple string matching
  return normalized_path.find(pattern.pattern) != std::string::npos;
  }
}

std::vector<std::string> read_bitignore(const std::string& filePath) {
  std::vector<std::string> patterns;
  
  if (!std::filesystem::exists(filePath)) {
  return patterns;
  }
  
  std::ifstream bitignoreFile(filePath);
  std::string line;
  
  while (std::getline(bitignoreFile, line)) {
  // remove trailing whitespace
  line.erase(line.find_last_not_of(" \t\r\n") + 1);
  
  // skip empty lines and comments
  if (line.empty() || line[0] == '#') {
  continue;
  }
  
  patterns.push_back(line);
  }
  
  return patterns;
}

std::vector<IgnorePattern> parse_ignore_patterns(const std::vector<std::string>& raw_patterns) {
  std::vector<IgnorePattern> patterns;
  
  for (const auto& raw_pattern : raw_patterns) {
  if (!raw_pattern.empty()) {
  patterns.emplace_back(raw_pattern);
  }
  }
  
  return patterns;
}

bool is_file_ignored(const std::string& filePath, const std::vector<IgnorePattern>& patterns) {
  if (filePath.empty()) {
  return false;
  }
  
  std::string normalized_path = normalize_path(filePath);
  bool ignored = false;
  
  // process patterns in order (later patterns can override earlier ones)
  for (const auto& pattern : patterns) {
  if (matches_pattern(normalized_path, pattern)) {
  if (pattern.is_negation) {
  // negation pattern - unignore this file
  ignored = false;
  } else {
  // regular pattern - ignore this file
  ignored = true;
  }
  }
  }
  
  return ignored;
}

bool should_ignore_file(const std::string& file_path) {
  // check for .bitignore file in current directory and parent directories
  std::string current_dir = std::filesystem::current_path().string();
  std::string bitignore_path = current_dir + "/.bitignore";
  
  // look for .bitignore in current directory and parent directories
  while (current_dir != std::filesystem::path(current_dir).parent_path().string()) {
  bitignore_path = current_dir + "/.bitignore";
  if (std::filesystem::exists(bitignore_path)) {
  break;
  }
  current_dir = std::filesystem::path(current_dir).parent_path().string();
  }
  
  if (!std::filesystem::exists(bitignore_path)) {
  return false;
  }
  
  std::vector<std::string> raw_patterns = read_bitignore(bitignore_path);
  std::vector<IgnorePattern> patterns = parse_ignore_patterns(raw_patterns);
  
  return is_file_ignored(file_path, patterns);
}

  // legacy function for backward compatibility
std::string get_file_extenion(const std::string& filePath) {
  return get_file_extension(filePath);
}