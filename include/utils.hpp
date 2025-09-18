#ifndef UTILS_HPP
#define UTILS_HPP


#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>

#include "../include/utils.hpp"

// Generic utility functions
std::string get_file_content(const std::string& file_path);
bool compare_files_contents_if_equal(const std::filesystem::path& first_file, const std::filesystem::path& second_file);

#endif
