#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>
#include <vector>

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

void config_set(const std::string& key, const std::string& value, ConfigScope scope = ConfigScope::REPOSITORY);
std::string config_get(const std::string& key, ConfigScope scope = ConfigScope::REPOSITORY);
void config_unset(const std::string& key, ConfigScope scope = ConfigScope::REPOSITORY);
void config_list(ConfigScope scope = ConfigScope::REPOSITORY);
void config_show_all();
void config_set_user_name(const std::string& name);
void config_set_user_email(const std::string& email);
std::string config_get_user_name();
std::string config_get_user_email();
void config_load();
void config_save();
std::map<std::string, std::string> get_config_map(ConfigScope scope);
void set_config_value(const std::string& key, const std::string& value, ConfigScope scope);
std::string get_config_file_path(ConfigScope scope);
std::string get_global_config_path();
std::string get_repository_config_path();
void create_default_config();

#endif
