#include "ignore.hpp"

std::string getFileExtension(const std::string &filePath)
{
  return std::filesystem::path(filePath).extension().string();
}

std::vector<std::string> ReadBitignore(const std::string &filePath)
{
  std::vector<std::string> patterns;
  std::ifstream bitignoreFile(filePath);
  std::string line;

  while (std::getline(bitignoreFile, line))
  {
    if (!line.empty() && line[0] != '#')
    {
      patterns.push_back(line);
    }
  }
  return patterns;
}

bool isIgnored(
  const std::string &filePath,
  const std::vector<std::string> &patterns
)
{
  for (const auto &pattern : patterns)
  {
    if (filePath.find(pattern) != std::string::npos)
    {
      return true;
    }
    else if (pattern[0] == '*' && pattern[1] == '.')
    {
      std::string fileExtension = getFileExtension(filePath);

      if (fileExtension == pattern.substr(1))
      {
        return true;
      }
    }
  }
  return false;
}