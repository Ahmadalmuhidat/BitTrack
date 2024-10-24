#ifndef IGNORE_HPP
#define IGNORE_HPP

std::string getFileExtension(const std::string &filePath);
std::vector<std::string> ReadBitignore(const std::string &filePath);
bool isIgnored(const std::string &filePath, const std::vector<std::string> &patterns);

#endif