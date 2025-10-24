#include "../include/utils.hpp"

std::string get_file_content(const std::string &file_path)
{
  // Read the content of the file at file_path and return it as a string
  if (!std::filesystem::exists(file_path))
  {
    return "";
  }

  // Open the file and read its content
  std::ifstream file(file_path);
  if (!file.is_open())
  {
    return "";
  }

  // Read the entire file content into a string
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  return buffer.str();
}

std::string format_timestamp(std::time_t timestamp)
{
  // Format the timestamp into a human-readable string
  char buffer[100];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&timestamp));
  return std::string(buffer);
}
