#include "../include/ignore.hpp"

std::string getFileExtenion(const std::string &filePath)
{
  return std::filesystem::path(filePath).extension().string();
}

std::vector<std::string> ReadBitignore(const std::string &filePath)
{
  std::vector<std::string> patterns;
  std::ifstream bitignoreFile(filePath);
  std::string line;

  // read bitignore file patterns and store them in array
  while (std::getline(bitignoreFile, line))
  {
    if (!line.empty() && line[0] != '#')
    {
      patterns.push_back(line);
    }
  }

  return patterns;
}

bool isIgnored(const std::string &filePath, const std::vector<std::string> &patterns)
{
  for (const auto &pattern: patterns)
  {
    // check the file path if exist
    if (std::filesystem::exists(filePath) == false)
    {
      return false;
    }
    else
    {
      if (filePath.find(pattern) != std::string::npos) // fix
      {
        return true;
      }
      // check the extention pattern
      if (pattern[0] == '*' && pattern[1] == '.')
      {
        // get the pattern file extenion
        std::string fileExtension = getFileExtenion(filePath);

        // check if the file extenion match the targeted extenion
        if (fileExtension == pattern.substr(1))
        {
          return true;
        }
      }
    }
  }
  return false;
}