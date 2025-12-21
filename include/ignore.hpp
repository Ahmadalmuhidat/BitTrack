#ifndef IGNORE_HPP
#define IGNORE_HPP

#include <regex>
#include <string>
#include <vector>

#include "error.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

struct IgnorePattern
{
  std::string pattern;
  bool is_negation;  // true if pattern starts with !
  bool is_directory; // true if pattern ends with /
  std::regex regex_pattern;

  IgnorePattern(const std::string &p) : pattern(p), is_negation(false), is_directory(false)
  {
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
    std::string regex_str = convertGitPatternToRegex(pattern);
    try
    {
      regex_pattern = std::regex(regex_str, std::regex_constants::icase);
    }
    catch (const std::regex_error &)
    {
      // If regex compilation fails, use simple string matching
      regex_str = ".*" + std::regex_replace(pattern, std::regex("\\*"), ".*") + ".*";
      regex_pattern = std::regex(regex_str, std::regex_constants::icase);
    }
  }

private:
  std::string convertGitPatternToRegex(const std::string &git_pattern);
};

std::vector<std::string> readBitignore(const std::string &filePath);
std::vector<IgnorePattern>
parseIgnorePatterns(const std::vector<std::string> &raw_patterns);
bool isFileIgnoredByIgnorePatterns(const std::string &filePath, const std::vector<IgnorePattern> &patterns);
bool shouldIgnoreFile(const std::string &file_path);
std::string normalizePath(const std::string &path);
bool matchesPattern(const std::string &filePath, const IgnorePattern &pattern);
void createDefaultBitignore();
void addIgnorePattern(const std::string &pattern);
void removeIgnorePattern(const std::string &pattern);
void listIgnorePatterns();
bool isFileIgnoredByPatterns(const std::string &file_path, const std::vector<std::string> &patterns);

#endif
