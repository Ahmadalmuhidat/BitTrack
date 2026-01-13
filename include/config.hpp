#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <string>

#include "error.hpp"

// Configuration scope enumeration
enum class ConfigScope
{
  GLOBAL,
  REPOSITORY
};

// Represents a configuration entry
struct ConfigEntry
{
  std::string key;
  std::string value;
  ConfigScope scope;

  ConfigEntry(
      const std::string &key,
      const std::string &value,
      ConfigScope scope) : key(key), value(value), scope(scope) {}
};

static std::map<std::string, std::string> global_config;
static std::map<std::string, std::string> repository_config;

void configSet(
    const std::string &key,
    const std::string &value,
    ConfigScope scope = ConfigScope::REPOSITORY);
std::string configGet(
    const std::string &key,
    ConfigScope scope = ConfigScope::REPOSITORY);
void configUnset(
    const std::string &key,
    ConfigScope scope = ConfigScope::REPOSITORY);
void configList(ConfigScope scope = ConfigScope::REPOSITORY);
void configLoad();
void setConfigValue(
    const std::string &key,
    const std::string &value,
    ConfigScope scope);
std::string getGlobalConfigPath();
std::string getRepositoryConfigPath();
void saveRepo();
void saveGlobal();

#endif
