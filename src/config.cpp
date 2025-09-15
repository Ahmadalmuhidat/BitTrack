#include "../include/config.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

// Global configuration storage
static std::map<std::string, std::string> global_config;
static std::map<std::string, std::string> repository_config;

void config_set(const std::string& key, const std::string& value, ConfigScope scope)
{
  set_config_value(key, value, scope);
  config_save();
  std::cout << "Set " << (scope == ConfigScope::GLOBAL ? "global" : "repository") << " config: " << key << " = " << value << std::endl;
}

std::string config_get(const std::string& key, ConfigScope scope)
{
  config_load();
  
  if (scope == ConfigScope::GLOBAL)
  {
    auto it = global_config.find(key);
    return (it != global_config.end()) ? it->second : "";
  }
  else
  {
    auto it = repository_config.find(key);
    if (it != repository_config.end())
    {
      return it->second;
    }
    // Fall back to global config
    it = global_config.find(key);
    return (it != global_config.end()) ? it->second : "";
  }
}

void config_unset(const std::string& key, ConfigScope scope)
{
  config_load();
  
  if (scope == ConfigScope::GLOBAL)
  {
    global_config.erase(key);
  }
  else
  {
    repository_config.erase(key);
  }
  
  config_save();
  std::cout << "Unset " << (scope == ConfigScope::GLOBAL ? "global" : "repository") << " config: " << key << std::endl;
}

void config_list(ConfigScope scope)
{
  config_load();
  
  std::map<std::string, std::string>* config_map = nullptr;
  std::string scope_name;
  
  if (scope == ConfigScope::GLOBAL)
  {
    config_map = &global_config;
    scope_name = "global";
  }
  else
  {
    config_map = &repository_config;
    scope_name = "repository";
  }
  
  std::cout << scope_name << " configuration:" << std::endl;
  
  if (config_map->empty())
  {
    std::cout << "  (no configuration set)" << std::endl;
    return;
  }
  
  for (const auto& entry : *config_map)
  {
    std::cout << "  " << entry.first << " = " << entry.second << std::endl;
  }
}

void config_show_all()
{
  config_load();
  
  std::cout << "All configuration:" << std::endl;
  
  if (!global_config.empty())
  {
    std::cout << "Global:" << std::endl;
    for (const auto& entry : global_config)
    {
      std::cout << "  " << entry.first << " = " << entry.second << std::endl;
    }
  }
  
  if (!repository_config.empty())
  {
    std::cout << "Repository:" << std::endl;
    for (const auto& entry : repository_config)
    {
      std::cout << "  " << entry.first << " = " << entry.second << std::endl;
    }
  }
  
  if (global_config.empty() && repository_config.empty())
  {
    std::cout << "  (no configuration set)" << std::endl;
  }
}

void config_set_user_name(const std::string& name)
{
  config_set("user.name", name, ConfigScope::GLOBAL);
}

void config_set_user_email(const std::string& email)
{
  config_set("user.email", email, ConfigScope::GLOBAL);
}

std::string config_get_user_name()
{
  return config_get("user.name", ConfigScope::GLOBAL);
}

std::string config_get_user_email()
{
  return config_get("user.email", ConfigScope::GLOBAL);
}

void config_load()
{
  // Load global config
  std::string global_path = get_global_config_path();
  if (std::filesystem::exists(global_path))
  {
    std::ifstream file(global_path);
    std::string line;
    
    while (std::getline(file, line))
    {
      if (line.empty() || line[0] == '#') continue;
      
      size_t pos = line.find('=');
      if (pos != std::string::npos)
      {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        global_config[key] = value;
      }
    }
    file.close();
  }
  
  // Load repository config
  std::string repo_path = get_repository_config_path();
  if (std::filesystem::exists(repo_path))
  {
    std::ifstream file(repo_path);
    std::string line;
    
    while (std::getline(file, line))
    {
      if (line.empty() || line[0] == '#')
      {
        continue;
      }
      
      size_t pos = line.find('=');
      if (pos != std::string::npos)
      {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        repository_config[key] = value;
      }
    }
    file.close();
  }
  
  // Create default config if none exists
  if (global_config.empty() && repository_config.empty())
  {
    create_default_config();
  }
}

void config_save()
{
  // Save global config
  std::string global_path = get_global_config_path();
  std::filesystem::create_directories(std::filesystem::path(global_path).parent_path());
  
  std::ofstream global_file(global_path);
  global_file << "# BitTrack Global Configuration" << std::endl;
  for (const auto& entry : global_config)
  {
    global_file << entry.first << "=" << entry.second << std::endl;
  }
  global_file.close();
  
  // Save repository config
  std::string repo_path = get_repository_config_path();
  std::filesystem::create_directories(std::filesystem::path(repo_path).parent_path());
  
  std::ofstream repo_file(repo_path);
  repo_file << "# BitTrack Repository Configuration" << std::endl;
  for (const auto& entry : repository_config)
  {
    repo_file << entry.first << "=" << entry.second << std::endl;
  }
  repo_file.close();
}

std::map<std::string, std::string> get_config_map(ConfigScope scope)
{
  config_load();
  return (scope == ConfigScope::GLOBAL) ? global_config : repository_config;
}

void set_config_value(const std::string& key, const std::string& value, ConfigScope scope)
{
  if (scope == ConfigScope::GLOBAL)
  {
    global_config[key] = value;
  }
  else
  {
    repository_config[key] = value;
  }
}

std::string get_config_file_path(ConfigScope scope) {
  return (scope == ConfigScope::GLOBAL) ? get_global_config_path() : get_repository_config_path();
}

std::string get_global_config_path()
{
  const char* home = std::getenv("HOME");
  if (home)
  {
    return std::string(home) + "/.bittrack/config";
  }
  return ".bittrack/config";
}

std::string get_repository_config_path()
{
  return ".bittrack/config";
}

void create_default_config()
{
  // Set default values
  global_config["core.editor"] = "nano";
  global_config["core.pager"] = "less";
  global_config["init.defaultBranch"] = "main";
  global_config["user.name"] = "";
  global_config["user.email"] = "";
  repository_config["core.repositoryFormatVersion"] = "0";
  repository_config["core.filemode"] = "true";
  repository_config["core.bare"] = "false";
}