#include "../include/config.hpp"

void config_set(const std::string &key, const std::string &value, ConfigScope scope)
{
  // Load existing configuration
  set_config_value(key, value, scope);
  config_save();
  std::cout << "Set " << (scope == ConfigScope::GLOBAL ? "global" : "repository") << " config: " << key << " = " << value << std::endl;
}

std::string config_get(const std::string &key, ConfigScope scope)
{
  // Load existing configuration
  config_load();

  if (scope == ConfigScope::GLOBAL)
  {
    // Check global config only
    auto it = global_config.find(key);
    return (it != global_config.end()) ? it->second : "";
  }
  else
  {
    // Check repository config first, then global config
    auto it = repository_config.find(key);
    if (it != repository_config.end())
    {
      return it->second;
    }

    // Fallback to global config
    it = global_config.find(key);
    return (it != global_config.end()) ? it->second : "";
  }
}

void config_unset(const std::string &key, ConfigScope scope)
{
  // Load existing configuration
  config_load();

  // Remove the key from the appropriate config map
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
  // Load existing configuration
  config_load();

  // Pointer to the appropriate config map
  std::map<std::string, std::string> *config_map = nullptr;
  std::string scope_name;

  // Determine which config map to use
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

  // Check if the config map is empty
  if (config_map->empty())
  {
    std::cout << "  (no configuration set)" << std::endl;
    return;
  }

  // Print all key-value pairs in the config map
  for (const auto &entry : *config_map)
  {
    std::cout << "  " << entry.first << " = " << entry.second << std::endl;
  }
}

void config_load()
{
  // Clear existing configurations
  std::string global_path = get_global_config_path();

  // Load global configuration
  if (std::filesystem::exists(global_path))
  {
    std::ifstream file(global_path);
    std::string line;

    // Read each line from the config file
    while (std::getline(file, line))
    {
      if (line.empty() || line[0] == '#')
      {
        continue;
      }

      // Parse key-value pairs
      size_t pos = line.find('=');
      if (pos != std::string::npos)
      {
        // Extract key and value
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        global_config[key] = value;
      }
    }
    file.close();
  }

  // Load repository configuration
  std::string repo_path = get_repository_config_path();

  // Load repository configuration
  if (std::filesystem::exists(repo_path))
  {
    // Open the repository config file
    std::ifstream file(repo_path);
    std::string line;

    // Read each line from the config file
    while (std::getline(file, line))
    {
      if (line.empty() || line[0] == '#')
      {
        continue;
      }

      // Parse key-value pairs
      size_t pos = line.find('=');
      if (pos != std::string::npos)
      {
        // Extract key and value
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        repository_config[key] = value;
      }
    }
    file.close();
  }
}

void config_save()
{
  // Save global configuration
  std::string global_path = get_global_config_path();
  std::filesystem::create_directories(std::filesystem::path(global_path).parent_path());

  // Open the global config file for writing and write each key-value pair to the config file
  std::ofstream global_file(global_path);
  global_file << "# BitTrack Global Configuration" << std::endl;
  for (const auto &entry : global_config)
  {
    global_file << entry.first << "=" << entry.second << std::endl;
  }
  global_file.close();

  // Save repository configuration
  std::string repo_path = get_repository_config_path();
  std::filesystem::create_directories(std::filesystem::path(repo_path).parent_path());

  // Open the repository config file for writing and write each key-value pair to the config file
  std::ofstream repo_file(repo_path);
  repo_file << "# BitTrack Repository Configuration" << std::endl;
  for (const auto &entry : repository_config)
  {
    repo_file << entry.first << "=" << entry.second << std::endl;
  }
  repo_file.close();
}

void set_config_value(const std::string &key, const std::string &value, ConfigScope scope)
{
  // Set the key-value pair in the appropriate config map
  if (scope == ConfigScope::GLOBAL)
  {
    global_config[key] = value;
  }
  else
  {
    repository_config[key] = value;
  }
}

std::string get_global_config_path()
{
  // Get the HOME environment variable
  const char *home = std::getenv("HOME");
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
