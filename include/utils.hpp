#ifndef UTILS_HPP
#define UTILS_HPP

#include <ctime>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

std::string get_file_content(const std::string &file_path);
std::string format_timestamp(std::time_t timestamp);

#endif
