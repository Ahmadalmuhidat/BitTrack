#include "branch.hpp"

std::string CurrentBranch()
{
  if (!std::filesystem::exists(".bittrack"))
  {
    std::cout << "Not inside a BitTrack repository!" << std::endl;
    return "";
  }

  std::ifstream headFile(".bittrack/HEAD");
  if (!headFile.is_open())
  {
    std::cerr << "Error: Could not open HEAD file." << std::endl;
    return "";
  }

  std::string line;
  std::getline(headFile, line);
  headFile.close();

  const std::string prefix = "ref: refs/heads/";
  if (line.find(prefix) == 0)
  {
    return line.substr(prefix.length());
  }

  std::cerr << "Error: Invalid HEAD format." << std::endl;
  return "";
}

void listBranches()
{
  for (const auto &entry : std::filesystem::recursive_directory_iterator(".bittrack/refs/heads"))
  {
    if (entry.is_regular_file())
    {
      std::string filePath = entry.path().string();
      const std::string prefix = ".bittrack/refs/heads/";

      if (filePath.find(prefix) == 0)
      {
        std::string head = CurrentBranch();
        std::string branch = filePath.substr(prefix.length());

        if (branch == head)
        {
          std::cout << "\033[32m" << filePath.substr(prefix.length()) << "\033[0m" << std::endl;
        }
        else
        {
          std::cout << filePath.substr(prefix.length()) << std::endl;
        }
      }
    }
  }
}