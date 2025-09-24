#include "../include/utils.hpp"

std::string get_file_content(const std::string &file_path)
{
  if (!std::filesystem::exists(file_path))
  {
    return "";
  }

  std::ifstream file(file_path);
  if (!file.is_open())
  {
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  return buffer.str();
}

bool compare_files_contents_if_equal(const std::filesystem::path &first_file, const std::filesystem::path &second_file)
{
  std::ifstream first_file_stream(first_file, std::ios::binary);
  std::ifstream second_file_stream(second_file, std::ios::binary);

  if (!first_file_stream || !second_file_stream)
  {
    return false;
  }

  std::istreambuf_iterator<char> begin1(first_file_stream), end1;
  std::istreambuf_iterator<char> begin2(second_file_stream), end2;
  return std::equal(begin1, end1, begin2, end2);
}
