#ifndef COMMIT_HPP
#define COMMIT_HPP

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "branch.hpp"
#include "error.hpp"
#include "hash.hpp"
#include "remote.hpp"
#include "stage.hpp"

void commitChanges(const std::string &author, const std::string &message);
void storeSnapshot(const std::string &file_path, const std::string &commit_hash);
void createCommitLog( const std::string &author, const std::string &message, const std::unordered_map<std::string, std::string> &file_hashes, const std::string &commit_hash);
std::string getCurrentCommit();
void commitChanges(const std::string &author, const std::string &message);
std::string getLastCommit(const std::string &branch_name = "");
std::string getCommitParent(const std::string &commit_hash);
void showCommitHistory();
std::string generateCommitHash(const std::string &author, const std::string &message, const std::string &timestamp);
std::string getCurrentTimestamp();
std::string getCurrentUser();
std::vector<std::string> getCommitFiles(const std::string &commit_hash);

#endif
