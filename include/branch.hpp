#ifndef BRANCH_HPP
#define BRANCH_HPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>

#include "stage.hpp"
#include "hash.hpp"

std::string getCurrentBranch();
std::vector<std::string> getBranchesList();
void printBranshesList();
void addBranch(std::string name);
void switchBranch(std::string name);

#endif