#ifndef DIFF_HPP
#define DIFF_HPP

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

enum class DiffLineType
{
  CONTEXT,
  ADDITION,
  DELETION
};

struct DiffLine
{
  DiffLineType type;
  int line_number;
  std::string content;
  
  DiffLine(DiffLineType t, int num, const std::string& text): type(t), line_number(num), content(text) {}
};

struct DiffHunk
{
  int old_start;
  int old_count;
  int new_start;
  int new_count;
  std::vector<DiffLine> lines;
  std::string header;
  
  DiffHunk(int old_s, int old_c, int new_s, int new_c, const std::string& h): old_start(old_s), old_count(old_c), new_start(new_s), new_count(new_c), header(h) {}
};

struct DiffResult
{
  std::string file1;
  std::string file2;
  std::vector<DiffHunk> hunks;
  bool is_binary;
  
  DiffResult(const std::string& f1, const std::string& f2): file1(f1), file2(f2), is_binary(false) {}
};

DiffResult compare_files(const std::string& file1, const std::string& file2);
DiffResult compare_file_to_content(const std::string& file, const std::string& content);
DiffResult diff_staged();
DiffResult diff_unstaged();
DiffResult diff_working_directory();
DiffResult diff_commits(const std::string& commit1, const std::string& commit2);
DiffResult diff_commit_to_working(const std::string& commit);
DiffResult diff_branches(const std::string& branch1, const std::string& branch2);
void show_diff(const DiffResult& result);
void show_unified_diff(const DiffResult& result);
void show_side_by_side_diff(const DiffResult& result);
void show_compact_diff(const DiffResult& result);
bool is_binary_file(const std::string& file_path);
std::vector<std::string> read_file_lines(const std::string& file_path);
std::vector<DiffHunk> compute_hunks(const std::vector<std::string>& old_lines, const std::vector<std::string>& new_lines);
std::vector<DiffLine> compute_diff_lines(const std::vector<std::string>& old_lines, const std::vector<std::string>& new_lines);
void print_diff_line(const DiffLine& line, const std::string& prefix = "");
std::string get_diff_line_prefix(DiffLineType type);
void show_file_history(const std::string& file_path);
void show_file_blame(const std::string& file_path);
void show_file_log(const std::string& file_path, int max_entries = 10);
std::string get_staged_file_content(const std::string& file_path);

#endif
