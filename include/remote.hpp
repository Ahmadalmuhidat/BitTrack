#ifndef REMOTE_HPP
#define REMOTE_HPP

#include <iostream>
#include <string>
#include <filesystem>
#include <curl/curl.h>

#include "../libs/miniz/miniz.h"

std::string get_remote_repositry();
bool compress_folder(const std::string &folder_path, const std::string &zip_path);
void push(const std::string &url, const std::string &file_path);
void pull();

#endif