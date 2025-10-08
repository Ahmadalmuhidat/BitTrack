#ifndef IGNORE_HPP
#define IGNORE_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <regex>

#include "error.hpp"

struct IgnorePattern {
  std::string pattern;
  bool is_negation;  // true if pattern starts with !
  bool is_directory; // true if pattern ends with /
  std::regex regex_pattern;
  
  IgnorePattern(const std::string& p) : pattern(p), is_negation(false), is_directory(false) {
  if (pattern.empty())
  {
  return;
  }
  
  is_negation = (pattern[0] == '!');
  if (is_negation)
  {
  pattern = pattern.substr(1); // remove the !
  }
  
  is_directory = (pattern.back() == '/');
  if (is_directory)
  {
  pattern = pattern.substr(0, pattern.length() - 1); // remove trailing /
  }
  
  // convert Git pattern to regex
  std::string regex_str = convert_git_pattern_to_regex(pattern);
  try
  {
  regex_pattern = std::regex(regex_str, std::regex_constants::icase);
  }
  catch (const std::regex_error&)
  {
  // if regex compilation fails, use simple string matching
  regex_str = ".*" + std::regex_replace(pattern, std::regex("\\*"), ".*") + ".*";
  regex_pattern = std::regex(regex_str, std::regex_constants::icase);
  }
  }
  
private:
  std::string convert_git_pattern_to_regex(const std::string& git_pattern);
};

std::vector<std::string> read_bitignore(const std::string& filePath);
std::vector<IgnorePattern> parse_ignore_patterns(const std::vector<std::string>& raw_patterns);
bool is_file_ignored_by_ignore_patterns(const std::string& filePath, const std::vector<IgnorePattern>& patterns);
bool should_ignore_file(const std::string& file_path);
std::string normalize_path(const std::string& path);
bool matches_pattern(const std::string& filePath, const IgnorePattern& pattern);
void create_default_bitignore();
void add_ignore_pattern(const std::string& pattern);
void remove_ignore_pattern(const std::string& pattern);
void list_ignore_patterns();
bool is_file_ignored_by_patterns(const std::string& file_path, const std::vector<std::string>& patterns);

#endif
