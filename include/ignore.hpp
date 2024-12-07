#ifndef IGNORE_HPP
#define IGNORE_HPP

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

std::string getFileExtenion(const std::string &filePath);
std::vector<std::string> ReadBitignore(const std::string &filePath);
bool isIgnored(
  const std::string &filePath,
  const std::vector<std::string> &patterns
);

#endif