#ifndef BRANCH_HPP
#define BRANCH_HPP

std::string getCurrentBranch();
std::vector<std::string> getBranchesList();
void printBranshesList();
void addBranch(std::string name);
void switchBranch(std::string name);

#endif