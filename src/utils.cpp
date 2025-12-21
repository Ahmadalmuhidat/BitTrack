#include "../include/utils.hpp"

std::string formatTimestamp(std::time_t timestamp) {
  // Format the timestamp into a human-readable string
  char buffer[100];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&timestamp));
  return std::string(buffer);
}
