#ifndef COMMIT_HPP
#define COMMIT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <sstream>

#include "hash.hpp"
#include "branch.hpp"

void storeSnapshot(const std::string &filePath, const std::string &CommitHash);
void createCommitLog(
  const std::string &author,
  const std::string &message,
  const std::unordered_map<std::string,
  std::string> &fileHashes,
  const std::string &commitHash
);
void commitChanges(const std::string &author, const std::string &message);
void commitHistory();

#endif