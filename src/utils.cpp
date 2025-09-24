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
