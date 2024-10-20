#ifndef STAGE_HPP
#define STAGE_HPP

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