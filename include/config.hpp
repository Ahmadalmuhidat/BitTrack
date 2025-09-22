#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <string>

enum class ConfigScope
{
  GLOBAL,
  REPOSITORY
};

struct ConfigEntry
{
  std::string key;
  std::string value;
  ConfigScope scope;
  
  ConfigEntry(const std::string& k, const std::string& v, ConfigScope s): key(k), value(v), scope(s) {}
};

// global configuration storage
static std::map<std::string, std::string> global_config;
static std::map<std::string, std::string> repository_config;

void config_set(const std::string& key, const std::string& value, ConfigScope scope = ConfigScope::REPOSITORY);
std::string config_get(const std::string& key, ConfigScope scope = ConfigScope::REPOSITORY);
void config_unset(const std::string& key, ConfigScope scope = ConfigScope::REPOSITORY);
void config_list(ConfigScope scope = ConfigScope::REPOSITORY);
void config_load();
void config_save();
void set_config_value(const std::string& key, const std::string& value, ConfigScope scope);
std::string get_global_config_path();
std::string get_repository_config_path();

#endif
