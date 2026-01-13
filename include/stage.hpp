#ifndef STAGE_HPP
#define STAGE_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

#include "error.hpp"
#include "ignore.hpp"
#include "hash.hpp"
#include "branch.hpp"
#include "commit.hpp"
#include "error.hpp"

void stage(const std::string &file_path);
void unstage(const std::string &file_path);
std::vector<std::string> getStagedFiles();
std::vector<std::string> getUnstagedFiles();
std::string getFileHash(const std::string &file_path);
std::string getStagedFileContent(const std::string &file_path);
bool isDeleted(const std::string &file_path);
std::string getActualPath(const std::string &file_path);
std::unordered_map<std::string, std::string> loadStagedFiles();
void saveStagedFiles(const std::unordered_map<std::string, std::string> &staged_files);
bool validateFileForStaging(const std::string &file_path);
std::string calculateFileHash(const std::string &file_path);
bool isFileUnchangedFromCommit(
    const std::string &file_path,
    const std::string &file_hash);
void stageSingleFile(
    const std::string &file_path,
    std::unordered_map<std::string, std::string> &staged_files);
void stageAllFiles(std::unordered_map<std::string, std::string> &staged_files);

#endif
