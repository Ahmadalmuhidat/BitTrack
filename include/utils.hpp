#ifndef UTILS_HPP
#define UTILS_HPP

#include <ctime>
#include <string>

#include "error.hpp"

std::string formatTimestamp(std::time_t timestamp);
std::string base64Encode(const std::string &input);
std::string base64Decode(const std::string &encoded);
size_t writeCallback(
    void *contents,
    size_t size,
    size_t nmemb, void *userp);

#endif
