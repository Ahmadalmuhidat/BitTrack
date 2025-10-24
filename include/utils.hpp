#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

std::string get_file_content(const std::string& file_path);
std::string format_timestamp(std::time_t timestamp);

#endif
