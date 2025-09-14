#ifndef REMOTE_HPP
#define REMOTE_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <curl/curl.h>

#include "../libs/miniz/miniz.h"

#include "branch.hpp"

void set_remote_origin(const std::string &url);
std::string get_remote_origin();
bool compress_folder(const std::string &folder_path, const std::string &zip_path);
void push();
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
void pull();

#endif