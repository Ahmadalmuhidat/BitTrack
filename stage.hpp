#ifndef STAGE_HPP
#define STAGE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

#include "ignore.cpp"
#include "hash.cpp"

void stage(std::string FilePath);
void unstage(const std::string &filePath);
void DisplayStagedFiles();
void DisplayUnstagedFiles();
bool compareWithCurrentVersion(const std::string &CurrentFile);
std::string normalizePath(const std::string &path);
std::string getCurrentCommit();

#endif