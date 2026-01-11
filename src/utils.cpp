#include "../include/utils.hpp"

std::string formatTimestamp(std::time_t timestamp)
{
  // Format the timestamp into a human-readable string
  char buffer[100];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&timestamp));
  return std::string(buffer);
}

std::string base64Encode(const std::string &input)
{
  // Base64 encoding implementation
  const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string result;
  int val = 0, valb = -6;

  // Encode input string to base64
  for (unsigned char c : input)
  {
    val = (val << 8) + c; // Shift left and add character
    valb += 8;            // Increase bit count
    while (valb >= 0)     // While there are enough bits
    {
      // Extract 6 bits and map to base64 character
      result.push_back(chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) // Handle remaining bits
  {
    result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  while (result.size() % 4) // Pad with '=' characters
  {
    result.push_back('=');
  }
  return result;
}

std::string base64Decode(const std::string &encoded)
{
  const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string decoded;
  int val = 0, valb = -8;
  int padding = 0;

  for (char c : encoded)
  {
    if (c == '=')
    {
      padding++;
      continue;
    }
    if (c == '\n' || c == '\r' || c == ' ' || c == '\t')
    {
      continue; // Skip whitespace and newlines
    }
    size_t pos = chars.find(c);
    if (pos == std::string::npos)
    {
      // Invalid character found, return empty string to indicate error
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "Invalid character in base64 encoded data",
          ErrorSeverity::ERROR,
          "base64Decode");
      return "";
    }
    val = (val << 6) + pos;
    valb += 6;
    if (valb >= 0)
    {
      decoded.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }

  return decoded;
}

size_t writeCallback(
    void *contents,
    size_t size,
    size_t nmemb,
    void *userp)
{
  ((std::string *)userp)->append((char *)contents, size * nmemb); // Append received data to string
  return size * nmemb;                                            // Return number of bytes processed
}