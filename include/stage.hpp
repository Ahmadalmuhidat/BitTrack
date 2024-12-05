#ifndef STAGE_HPP
#define STAGE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

#include "../src/ignore.cpp"
#include "../src/hash.cpp"
#include "branch.hpp"

void stage(std::string FilePath);
void unstage(const std::string &filePath);
std::vector<std::string> getStagedFiles();
std::vector<std::string> getUnstagedFiles();
bool compareWithCurrentVersion(const std::string &CurrentFile, const std::string &CurrentBranch);
std::string normalizePath(const std::string &path);
std::string getCurrentCommit();

#endif