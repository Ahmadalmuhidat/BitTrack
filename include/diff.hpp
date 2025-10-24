#ifndef DIFF_HPP
#define DIFF_HPP

#include <algorithm>
#include <sstream>
#include <fstream>
#include <set>
#include <iomanip>
#include <vector>
#include <string>
#include <iostream>

#include "stage.hpp"
#include "commit.hpp"
#include "branch.hpp"

// Types of lines in a diff
enum class DiffLineType
{
  CONTEXT, // unchanged line
  ADDITION, // added line
  DELETION // deleted line
};

// Represents a single line in a diff
struct DiffLine
{
  DiffLineType type; // type of diff line
  int line_number; // line number in the file
  std::string content; // content of the line
  
  DiffLine(DiffLineType diff_type, int line_number, const std::string& text): type(diff_type), line_number(line_number), content(text) {}
};

// Represents a hunk of differences between two files
struct DiffHunk
{
  int old_start; // starting line number in old file
  int old_count; // number of lines in old file
  int new_start; // starting line number in new file
  int new_count; // number of lines in new file
  std::vector<DiffLine> lines;
  std::string header;
  
  DiffHunk(int old_string, int old_count, int new_start, int new_count, const std::string& header): old_start(old_string), old_count(old_count), new_start(new_start), new_count(new_count), header(header) {}
};

// Represents the result of a diff operation between two files
struct DiffResult
{
  std::string file1; // first file path
  std::string file2; // second file path
  std::vector<DiffHunk> hunks; // list of diff hunks
  bool is_binary; // indicates if files are binary
  
  DiffResult(const std::string& file1, const std::string& file2): file1(file1), file2(file2), is_binary(false) {}
};

DiffResult compare_files(const std::string& file1, const std::string& file2);
DiffResult compare_file_to_content(const std::string& file, const std::string& content);
DiffResult diff_staged();
DiffResult diff_unstaged();
DiffResult diff_working_directory();
void show_diff(const DiffResult& result);
bool is_binary_file(const std::string& file_path);
std::vector<std::string> read_file_lines(const std::string& file_path);
std::vector<DiffHunk> compute_hunks(const std::vector<std::string>& old_lines, const std::vector<std::string>& new_lines);
std::vector<DiffLine> compute_diff_lines(const std::vector<std::string>& old_lines, const std::vector<std::string>& new_lines);
void print_diff_line(const DiffLine& line, const std::string& prefix = "");
std::string get_diff_line_prefix(DiffLineType type);

#endif
