#ifndef STAGE_HPP
#define STAGE_HPP

#include "ignore.cpp"
#include "hash.cpp"

void AddToStage(std::string FilePath);
void RemoveFromStage(const std::string &filePath);
void DisplayStagedFiles();
void DisplayUnstagedFiles();
bool CompareWithCurrentVersion(const std::string &CurrentFile);
std::string NormalizePath(const std::string &path);
std::string GetCurrentCommit();

#endif